#include "stdafx.h"
#include "EXosipThread.h"
#include "digcalc.h"
#include "ManscdpXmlMessage.h"
#include "GB28181Utils.h"

EXosipThread::EXosipThread(EXosipThreadEvent *exosip_thread_event):
	exosip_thread_event_(exosip_thread_event),
	message_process_thread_(NULL),
	exosip_(NULL)
{
}

EXosipThread::~EXosipThread()
{
}

void EXosipThread::SetUserAgent(std::string agent)
{
	if (exosip_)
		eXosip_set_user_agent(exosip_, agent.c_str());
}

void EXosipThread::Stop()
{
	if (message_process_thread_)
	{
		message_process_thread_->Stop();
		delete message_process_thread_;
		message_process_thread_ = NULL;
	}

	running = false;
	WaitForStop();
}

bool EXosipThread::Start()
{
	return StartThread();
}

void EXosipThread::ThreadProcMain()
{
	if (exosip_ == NULL)
		return ;

	if (message_process_thread_ == NULL)
	{
		message_process_thread_ = new MessageProcessThread(this->exosip_thread_event_);
		if (message_process_thread_)
		{
			if(!message_process_thread_->Start())
				return ;
		}
		else
		{
			SDK_LOG("Create MessageProcessThread fail\n");
			return;
		}
	}

	int ret = -1;
	if (protocol_)
		ret = eXosip_listen_addr(exosip_, IPPROTO_TCP, local_ip_.c_str(), local_port_, AF_INET, 0);
	else
		ret = eXosip_listen_addr(exosip_, IPPROTO_UDP, local_ip_.c_str(), local_port_, AF_INET, 0);
	
	if (ret != 0)
		return ;

	running = true;

	std::string id;
	eXosip_event_t *je = NULL;
	osip_message_t *ack = NULL;
	osip_message_t *answer = NULL;

	while (running)
	{
		je = eXosip_event_wait(exosip_, 0, 50);

		eXosip_lock(exosip_);
		eXosip_default_action(exosip_,je);     //401/407/3xx的默认处理，是往上注册时的需要
		//eXosip_automatic_refresh(exosip_);
		eXosip_unlock(exosip_);

		if (je == NULL)
			continue;

		switch (je->type)
		{
		case EXOSIP_MESSAGE_NEW:
			{
				if (MSG_IS_MESSAGE(je->request))
				{
					osip_from_t *from = osip_message_get_from(je->request);
					if (from == NULL)
					{
						SDK_LOG("[MESSAGE] Request without from header\n");
						break;
					}

					osip_uri_t *uri = osip_from_get_url(from);
					if (uri == NULL)
					{
						SDK_LOG("[MESSAGE] From header without uri\n");
						break;
					}

					std::string plt_id = (uri->username == NULL ? "" : uri->username);

					if (!GB28181Utils::IsPlatformType(plt_id))
					{
						SDK_LOG("[MESSAGE] Username[%s] of uri is not a type of platform\n", plt_id.c_str());
						break;
					}

					if (exosip_thread_event_)
					{
						if(!exosip_thread_event_->OnEXosipThreadEvent_IsUserRegistered(plt_id))
						{
							SDK_LOG("[MESSAGE] Platform[%s] Unregistered\n", plt_id.c_str());
							eXosip_lock(exosip_);
							eXosip_message_build_answer(exosip_, je->tid, 404, &answer);
							eXosip_message_send_answer(exosip_, je->tid, 404, answer);
							eXosip_unlock(exosip_);
							break;
						}
					}
					
					osip_body_t *pBody = NULL;
					char *body = NULL;
					size_t body_len;
					MsgBody msg_body;

					osip_message_get_body(je->request, 0, &pBody);
					if (pBody && osip_body_to_str(pBody, &body, &body_len)==OSIP_SUCCESS && exosip_thread_event_)
					{
						body[body_len] = '\0';
						msg_body.id = plt_id;
						msg_body.data = body;
						message_process_thread_->Push(msg_body);
						osip_free(body);
					}

					eXosip_lock (exosip_);   
					eXosip_message_build_answer(exosip_, je->tid, 200, &answer);   
					eXosip_message_send_answer(exosip_, je->tid, 200, answer);   
					eXosip_unlock (exosip_);
				}
				else if (MSG_IS_REGISTER(je->request))
				{
					ProcessRegister(je);
				}
			}
			break;
		case EXOSIP_CALL_INVITE:
		case EXOSIP_CALL_ACK: 
		case EXOSIP_CALL_CLOSED:    
		case EXOSIP_CALL_MESSAGE_NEW:
			break;
		case EXOSIP_CALL_ANSWERED:
			{
				char *message = NULL;
				size_t msg_len = -1;
				char  *szY, *szCLRF;
				size_t nSSRC;
				char szSSRC[11];
				memset(szSSRC,0,11);

				osip_contact_t *contact = NULL;
				osip_message_get_contact(je->request, 0, &contact);
				if (contact == NULL)
				{
					SDK_LOG("[INVITE_ANSWERED] Response without contact header\n");
					break;
				}

				osip_uri_t *uri = osip_contact_get_url(contact);
				if (uri == NULL)
				{
					SDK_LOG("[INVITE_ANSWERED] Contact header without uri\n");
					break;
				}

				std::string plt_id = (uri->username == NULL ? "" : uri->username);

				if(osip_message_to_str(je->response, &message, &msg_len) != OSIP_SUCCESS) 
					break;

				szY = strstr(message,"y=");
				if (szY == NULL)
				{
					SDK_LOG("[INVITE_ANSWERED] SDP without ssrc\n");
				}
				else
				{
					szCLRF = strstr(szY, "\r\n");
					if(szCLRF == NULL)
					{
						SDK_LOG("[INVITE_ANSWERED] SSRC without CLRF\n");
					}
					else
					{
						memcpy(szSSRC, szY+2, szCLRF-szY-2);
						nSSRC = atoi(szSSRC);
					}
				}
				osip_free(message);

				sdp_message_t *sdp = eXosip_get_sdp_info(je->response);
				char *username = sdp_message_o_username_get(sdp);
				char *media = sdp_message_a_att_value_get(sdp, 0, 0);
				char *dest_ip = sdp_message_o_addr_get(sdp);
				std::string temp_dest_ip = dest_ip;

				if (username == NULL)
				{
					SDK_LOG("[INVITE_ANSWERED] SDP without owner\n");
					sdp_message_free(sdp);
					break ;
				}

				int media_type = 0;
				if (media == NULL)
				{
					media_type = GB_MEDIA_TYPE_PS;
				}
				else
				{
					if (strstr(media, "PS") != NULL)
					{
						media_type = GB_MEDIA_TYPE_PS;
					}
					else if(strstr(media, "MPEG4") != NULL)
					{
						media_type = GB_MEDIA_TYPE_MPEG4;
					}
					else if(strstr(media, "H264") != NULL)
					{
						media_type = GB_MEDIA_TYPE_H264;
					}
					else if(strstr(media, "DAHUA") != NULL)
					{
						media_type = GB_MEDIA_TYPE_DAHUA;
					}
					else if(strstr(media, "HIKVISION") != NULL)
					{
						media_type = GB_MEDIA_TYPE_HIKVISION;
					}
					else if(strstr(media, "PCMA") != NULL)
					{
						media_type = GB_MEDIA_TYPE_PCMA;
					}
					else
					{
						media_type = GB_MEDIA_TYPE_PS;
						sdp_message_free(sdp);
					}
				}

				sdp_message_t *request_sdp = eXosip_get_sdp_info(je->request);
				char *session = sdp_message_s_name_get(request_sdp);
				if (session == NULL)
				{
					SDK_LOG("[INVITE_ANSWERED] SDP without session\n");
					sdp_message_free(sdp);
					sdp_message_free(request_sdp);
					break;
				}

				if (strcmp(session, "Play") == 0)
				{
					if(exosip_thread_event_) 
						exosip_thread_event_->OnEXosipThreadEvent_StartPlaySucceeded(je->cid, je->did, STREAM_TYPE_PLAY_MAIN, media_type, nSSRC);
				}
				else if (strcmp(session, "Playback") == 0)
				{
					if(exosip_thread_event_)
						exosip_thread_event_->OnEXosipThreadEvent_PlaybackSucceeded(je->cid, je->did, STREAM_TYPE_PLAYBACK, media_type, nSSRC);
				}
				else if (strcmp(session, "Download") == 0)
				{
					if(exosip_thread_event_)
						exosip_thread_event_->OnEXosipThreadEvent_PlaybackSucceeded(je->cid, je->did, STREAM_TYPE_DOWNLOAD, media_type, nSSRC);
				}
				else if (strcmp(session,"Talk")==0)
				{
					unsigned short port = GetMediaPort(sdp, "recvonly");

					if(exosip_thread_event_)
					{
						if (port < 0)
							exosip_thread_event_->OnEXosipThreadEvent_StartTalkFailed(je->cid);
						else
							exosip_thread_event_->OnEXosipThreadEvent_StartTalkSucceeded(je->cid, je->did, media_type, temp_dest_ip, port);	
					}
				}
				eXosip_call_build_ack(exosip_,je->did,&ack);
				
				eXosip_lock (exosip_);
				eXosip_call_send_ack(exosip_,je->did,ack);
				eXosip_unlock (exosip_);

				//sdp_message_free(sdp);
				sdp_message_free(request_sdp);
			}
			break;
		case EXOSIP_CALL_REQUESTFAILURE:
			{
				SDK_LOG("[StatusCode] %d\n", je->response->status_code);

				if (403 == je->response->status_code)
				{
					if(exosip_thread_event_)
						exosip_thread_event_->OnEXosipThreadEvent_TcpRequest(je->cid, je->did);
				}
				else
				{
					sdp_message_t *sdp = eXosip_get_sdp_info(je->request);
					if (sdp == NULL)
						break;

					char *session = sdp_message_s_name_get(sdp);
					if (session == NULL)
					{
						sdp_message_free(sdp);
						break;
					}

					if (strcmp(session, "Play") == 0)
					{
						if(exosip_thread_event_) 
							exosip_thread_event_->OnEXosipThreadEvent_StartPlayFailed(je->cid);
					}
					else if (strcmp(session, "Playback") == 0 || strcmp(session, "Download") == 0)
					{
						if(exosip_thread_event_) 
							exosip_thread_event_->OnEXosipThreadEvent_PlaybackFailed(je->cid);
					}
					else if (strcmp(session, "Talk") == 0)
					{
						if(exosip_thread_event_) 
							exosip_thread_event_->OnEXosipThreadEvent_StartTalkFailed(je->cid);
					}

					sdp_message_free(sdp);
				}
			}
			break;
		case EXOSIP_CALL_SERVERFAILURE:
		case EXOSIP_CALL_GLOBALFAILURE:
		case EXOSIP_CALL_RELEASED:
			{
				sdp_message_t *sdp = eXosip_get_sdp_info(je->request);
				if (sdp == NULL)
					break;

				char *session = sdp_message_s_name_get(sdp);
				if (session == NULL)
				{
					sdp_message_free(sdp);
					break;
				}

				if (strcmp(session, "Play") == 0)
				{
					 if(exosip_thread_event_) 
						 exosip_thread_event_->OnEXosipThreadEvent_StartPlayFailed(je->cid);
				}
				else if (strcmp(session, "Playback") == 0 || strcmp(session, "Download") == 0)
				{
					if(exosip_thread_event_) 
						exosip_thread_event_->OnEXosipThreadEvent_PlaybackFailed(je->cid);
				}
				else if (strcmp(session, "Talk") == 0)
				{
					if(exosip_thread_event_) 
						exosip_thread_event_->OnEXosipThreadEvent_StartTalkFailed(je->cid);
				}

				sdp_message_free(sdp);
			}
			break;
		case EXOSIP_CALL_MESSAGE_ANSWERED:
			break;
		case EXOSIP_MESSAGE_ANSWERED:
			break;
		case EXOSIP_MESSAGE_REQUESTFAILURE:
			break;
		case EXOSIP_SUBSCRIPTION_NOTIFY:
			{
				if(MSG_IS_NOTIFY(je->request))
				{		
					osip_from_t *from = osip_message_get_from(je->request);
					if (from == NULL)
					{
						SDK_LOG("[NOTIFY] Request without From header\n");
						break;
					}

					osip_uri_t *uri = osip_from_get_url(from);
					if (uri == NULL)
					{
						SDK_LOG("[NOTIFY] From header without uri\n");
						break;
					}

					std::string plt_id = (uri->username == NULL ? "" : uri->username);

					if (!GB28181Utils::IsPlatformType(plt_id))
					{
						SDK_LOG("[NOTIFY] Username[%s] of uri is not a type of platform\n", plt_id.c_str());
						break;
					}

					if (exosip_thread_event_)
					{
						if(!exosip_thread_event_->OnEXosipThreadEvent_IsUserRegistered(plt_id))
						{
							SDK_LOG("[NOTIFY] Platform[%s] Unregistered\n", plt_id.c_str());
							eXosip_lock(exosip_);
							eXosip_message_build_answer(exosip_, je->tid, 404, &answer);
							eXosip_message_send_answer(exosip_, je->tid, 404, answer);
							eXosip_unlock(exosip_);
							break;
						}
					}

					osip_content_type_t *pContentType = NULL;
					char *szContentType = NULL;
					osip_body_t *pBody = NULL;
					char *body = NULL;
					size_t body_len;
					MsgBody msg_body;

					std::string strProtocol = "application/manscdp+xml";

					pContentType = osip_message_get_content_type(je->request);
					if (pContentType)
					{
						if (osip_content_type_to_str(pContentType, &szContentType) == OSIP_SUCCESS)
						{
							if (!osip_strncasecmp(szContentType, strProtocol.c_str(), strProtocol.length()))
							{
								osip_message_get_body(je->request, 0, &pBody);
								if (pBody && osip_body_to_str(pBody, &body, &body_len) == OSIP_SUCCESS && exosip_thread_event_)
								{
									body[body_len] = '\0';
									msg_body.id = plt_id;
									msg_body.data = body;
									message_process_thread_->Push(msg_body);
									osip_free(body);
								}
								osip_free(szContentType);
							}
						}
					}
				}
			}
			break;
		case EXOSIP_SUBSCRIPTION_ANSWERED:
			{
				osip_to_t *to = osip_message_get_to(je->response);
				if (to == NULL)
				{
					SDK_LOG("[SUBSCRIPTION_ANSWERED] Response without to header\n");
					break;
				}

				osip_uri_t *uri = osip_to_get_url(to);
				if (uri == NULL)
				{
					SDK_LOG("[SUBSCRIPTION_ANSWERED] To header without uri\n");
					break;
				}

				std::string plt_id = (uri->username == NULL ? "" : uri->username);

				if (exosip_thread_event_)
					exosip_thread_event_->OnEXosipThreadEvent_SubscribeAnswered(plt_id, je->did);
			}
			break;
		default:
			printf("The event is not handled(%d)\n", je->type);
			break;
		}
		
		eXosip_event_free(je);
	}
}

int EXosipThread::ProcessRegister(const eXosip_event_t *je)
{ 
	std::string plt_pwd;
	osip_message_t *answer = NULL;
	osip_authorization_t *header;
	char WWW_Authenticate[512]={0};
	static char pszNonce[NONCE_LEN+1]={'0'};
	char *pszCNonce = NULL;
	char *pszUser = "";
	char *pszAlg = "md5";
	char *pszNonceCount = "";
	char *pszMethod = "REGISTER";
	char *pszQop = "";
	char *pszUri = "";
	char *response_h = "";
	//不能用本地的pszNonce，因为它有可能给另一个注册上来时要产生新的nonce时覆盖掉
	char *pszNonceFromHeader="";

	int reg_cseq = 2;
	osip_cseq_t *pCseq = osip_message_get_cseq(je->request);
	if (pCseq != NULL && pCseq->number != NULL)
	{
		reg_cseq = atoi(pCseq->number);
	}

	HASHHEX HA1 = "";
	HASHHEX HA2 = "";
	HASHHEX Response;
	char ch[SIZE_CHAR+1] = {0};
	osip_contact_t *contact=NULL;
	osip_message_get_contact(je->request, 0, &contact);
	if (contact == NULL)
	{
		SDK_LOG("[REGISTER] Request without contact header\n");
		return -1;
	}

	osip_uri_t *uri = osip_contact_get_url(contact);
	if (uri == NULL)
	{
		SDK_LOG("[REGISTER] Contact header without uri\n");
		osip_free(pszCNonce);
		osip_free(pszNonceCount);
		osip_free(pszQop);
		return -1;
	}

	std::string plt_id = CHARPTRTOSTR(uri->username);
	std::string plt_ip = CHARPTRTOSTR(uri->host);
	int plt_port = (uri->port == NULL ? 5060 : atoi(uri->port));
	
	if (exosip_thread_event_ && exosip_thread_event_->OnEXosipThreadEvent_IsUserRegisteredByOtherIP(plt_id, plt_ip, plt_port))
	{
		SDK_LOG("[REGISTER] The user[%s] was registered by other platform\n", plt_id.c_str());
		return -1;
	}
	
	osip_message_get_authorization(je->request, 0, &header);
	if (!header)
	{
		if (!exosip_thread_event_->OnEXosipThreadEvent_IsPasswordReady(plt_id))
		{
			SDK_LOG("[REGISTER] The password of user[%s] is not ready\n", plt_id.c_str());
			return -1;
		}

		memset(pszNonce,0,sizeof(pszNonce)*sizeof(pszNonce[0]));
		memset(WWW_Authenticate,0,sizeof(WWW_Authenticate)*sizeof(WWW_Authenticate[0]));
		
		memcpy(pszNonce,GenerateNonce(),SIZE_CHAR);
		snprintf(WWW_Authenticate,sizeof(WWW_Authenticate),"Digest realm=\"%s\",algorithm=MD5,nonce=\"%s\",stale=false", realm_.c_str(),pszNonce);
		eXosip_lock (exosip_);
		eXosip_message_build_answer (exosip_,je->tid, 401,&answer);
		osip_message_set_header(answer,"WWW-Authenticate",WWW_Authenticate);
		eXosip_message_send_answer (exosip_,je->tid, 401,answer);
		eXosip_unlock (exosip_);
		return 0;
	}
	
	pszMethod = "REGISTER";
	pszAlg = header->algorithm;
	pszUser = header->username;
	pszUri = header->uri;
	pszNonceFromHeader=header->nonce;
	pszNonceCount= header->nonce_count;
	pszQop = header->opaque;
	response_h = header->response;
	
	//去掉首尾双引号
	RemoveDoubleQuote(pszAlg);
	RemoveDoubleQuote(pszUser);
	RemoveDoubleQuote(pszUri);
	RemoveDoubleQuote(pszQop);
	RemoveDoubleQuote(pszNonceCount);
	RemoveDoubleQuote(pszNonceFromHeader);
	RemoveDoubleQuote(response_h);

	if (pszUser == NULL)
	{
		SDK_LOG("[REGISTER] Contact header without user\n");
		return -1;
	}

	if (exosip_thread_event_)
		exosip_thread_event_->OnEXosipThreadEvent_QueryPassword(plt_id, plt_pwd);
	
	memset(HA1,0,sizeof(HA1)*sizeof(HA1[0]));
	memset(HA2,0,sizeof(HA2)*sizeof(HA2[0]));

	if (pszNonceFromHeader==NULL || pszMethod==NULL || pszUri==NULL)
	{
		SDK_LOG("[REGISTER] NonceFromHeader(%d) or Method(%d) or Uri(%d) is NULL\n", pszNonceFromHeader, pszMethod, pszUri);
		return -1;
	}

	DigestCalcHA1(pszAlg, pszUser, realm_.c_str(), plt_pwd.c_str(), pszNonceFromHeader, pszCNonce, HA1);
	DigestCalcResponse(HA1, pszNonceFromHeader,pszNonceCount, pszCNonce, pszQop, 1, pszMethod, pszUri, HA2, Response);
	SDK_LOG("[Response] %s\n", Response);

	
	if (!strcmp(Response,response_h))
	{
		int ret = eXosip_message_build_answer (exosip_,je->tid, 200,&answer);

		osip_header_t *pExpiresHeader = NULL;
		osip_message_get_expires(je->request,0,&pExpiresHeader);
		if (pExpiresHeader)
		{
			osip_message_set_expires(answer,pExpiresHeader->hvalue);
		}

		char *pStrContact=NULL;
		osip_contact_to_str(contact, &pStrContact);
		if (pStrContact)
		{
			osip_message_set_contact(answer,pStrContact);
			osip_free(pStrContact);
		}

		std::string date = GB28181Utils::GetCurrentDate();
		osip_message_set_date(answer, date.c_str());
		eXosip_lock (exosip_);
		eXosip_message_send_answer(exosip_, je->tid, 200, answer);
		eXosip_unlock (exosip_);

		if (exosip_thread_event_ && pExpiresHeader)
		{
			int expires = 3600;
			if (pExpiresHeader->hvalue)
			{
				expires = atoi(pExpiresHeader->hvalue);
				if (expires == 0)
				{
					SDK_LOG("[REGISTER] Platform[%s] cancel registered\n", plt_id.c_str());
					exosip_thread_event_->OnEXosipThreadEvent_UnRegistered(plt_id);
				}
				else
				{
					SDK_LOG("[REGISTER] Platform[%s] registered\n", plt_id.c_str());
					exosip_thread_event_->OnEXosipThreadEvent_Registered(reg_cseq, plt_id, plt_pwd, uri->host, plt_port, expires);
				}
			}
		}
		
	}
	else
	{
		SDK_LOG("[REGISTER] The password of user[%s] is not correct\n", plt_id.c_str());
		eXosip_lock (exosip_);
		eXosip_message_build_answer (exosip_, je->tid, 406, &answer);
		eXosip_message_send_answer (exosip_, je->tid, 406, answer);
		eXosip_unlock (exosip_);
	}
	
	osip_free(pszCNonce);
	osip_free(pszNonceCount);
	osip_free(pszQop);
	return 0; 
}

void EXosipThread::RemoveDoubleQuote(char str[], char remove)
{
	if (!str)
	{
		return;
	}
	int len = (int)strlen(str);
	if (len>256||len<2)
	{
		return;
	}
	if(str[len-1]==remove)
	{
		str[strlen(str)-1]='\0';
		len = (int)strlen(str);
		if (len<1)
		{
			return;
		}
	}
	if (str[0]==remove)
	{
		for (int i=0;i<len-1;i++)
		{
			str[i]=str[i+1];
		}
		str[len-1]='\0';
	}
}

char * EXosipThread::GenerateNonce()
{
	static char nonce[40];
	struct timeval tv;

	osip_gettimeofday(&tv, 0);
	/* yeah, I know... should be a better algorithm */
	/* enclose it in double quotes, as libosip does *not* do it (2.0.6) */
	sprintf(nonce, "%8.8lx%8.8lx%8.8x%8.8x",(long)tv.tv_sec, (long)tv.tv_usec, rand(), rand());
	return nonce;
}

int EXosipThread::GetMediaPort(sdp_message_t *sdp, const char *type)
{
	char *att_name = NULL;
	char *port = NULL;

	if (NULL == type)
		return -1;

	if (strcmp(type, "recvonly") != 0 && strcmp(type, "sendonly") != 0)
		return -1;
	
	for (int media_pos = 0; !sdp_message_endof_media(sdp, media_pos); media_pos++)
	{
		for(int att_pos=0; (att_name = sdp_message_a_att_field_get(sdp, media_pos, att_pos)) != NULL; att_pos++)
		{
			if (0 == strcmp(att_name, type))
			{
				port = sdp_message_m_port_get(sdp, media_pos);
				return port != NULL ? atoi(port) : -1;
			}
		}
	}

	return -1;
}
