#include "StdAfx.h"
#include "DevTalk.h"


void OnSTC_AVCPacketCallback(long hHandle,const char*szKey,int nTranscodeType,int nStreamType,int nPacketType,unsigned char*pData,int nLen,unsigned char*pExtraData,int nExtraSize,void*pUser)
{
	CDevTalk* pThis = (CDevTalk*)pUser;
	if (pThis)
	{
		pThis->OnAVCPacket2MTS(hHandle,nStreamType,pData,nLen);
	}
}

void OnSTC_3RDPacketCallback(long hHandle,const char*szKey,int nTranscodeType,int nStreamType,int nPacketType,unsigned char*pData,int nLen,unsigned char*pExtraData,int nExtraSize,void*pUser)
{
	CDevTalk* pThis = (CDevTalk*)pUser;
	if (pThis)
	{
		pThis->OnMonTo3RDDevice(hHandle,nStreamType,pData,nLen);
	}
}


void OnSTFCallback_SendMessage(long hHandle,const char*szKey,int nMessage,long wParam,long lParam,void*pUser)
{
	CDevTalk* pThis = (CDevTalk*)pUser;
	if (pThis)
	{
		pThis->OnSendTransferMessage(hHandle,szKey,nMessage,wParam,lParam);
	}
}

void OnSTFCallback_RecvMessage(long hHandle,const char*szKey,int nMessage,long wParam,long lParam,void*pUser)
{
	CDevTalk* pThis = (CDevTalk*)pUser;
	if (pThis)
	{
		pThis->OnRecvTransferMessage(hHandle,szKey,nMessage,wParam,lParam);
	}
}

void OnSTFTransferStream(long hHandle,const char*szKey,int nDevType,int nPacketType,unsigned char*pData,int nLen,void*pUser)
{
	CDevTalk* pThis = (CDevTalk*)pUser;
	if (pThis)
	{
		pThis->OnTransferStream(hHandle,szKey,nDevType,nPacketType,pData,nLen);
	}
}

CDevTalk::CDevTalk(IDevTalkCallback* pITalkCallback, int nTalkCodecID)
: m_pDevTalkCallback(pITalkCallback)
, m_hTransferRecv(NULL)
, m_hTransferSend(NULL)
, m_nCodecID (nTalkCodecID)
, m_hAVCto3Party(NULL)
, m_h3PartyToAVC(NULL)
, m_pTalkRespond(NULL)
, m_nDevType(HPD_DVR)
, m_bWantToStop(false)
{
	m_ulTalkAudioID = XGenerateSSRC(); XSleep(1);
	
}

CDevTalk::~CDevTalk(void)
{
}

bool CDevTalk::StartTalk( MPX_TALK_INFO*pTalkData )
{
 	if (m_pTalkRespond == NULL)
 	{
 		m_pTalkRespond = (MPX_TALK_INFO*)malloc(pTalkData->nStructSize+pTalkData->nUserLength+1);
 		memset(m_pTalkRespond,0,sizeof(MPX_TALK_INFO)+pTalkData->nUserLength+1);
 	}
	m_bWantToStop=false;
	m_nDevType = pTalkData->nDevType;
	//m_strUserData = pTalkData->szUserBuffer;

	m_pTalkRespond->nStructSize = sizeof(MPX_TALK_INFO);
	m_pTalkRespond->nCallStatus = MPX_ERR_VOICECALL_FAILED;
	m_pTalkRespond->nAudioCodecID = m_nCodecID;
	m_pTalkRespond->nDevType = pTalkData->nDevType;
	
	SAFE_STRCPY(m_pTalkRespond->szDevID, pTalkData->szDevID);
	m_pTalkRespond->nAudioID = m_ulTalkAudioID;
	SAFE_STRCPY(m_pTalkRespond->szNodeID, NETEC_Node::GetNodeID());
	SAFE_STRCPY(m_pTalkRespond->szLocalAddr, NETEC_Node::GetLocalIP());
	m_pTalkRespond->nLocalPort = NETEC_Node::GetLocalPort();
	SAFE_STRCPY(m_pTalkRespond->szNatAddr, NETEC_Node::GetNATIP());
	m_pTalkRespond->nNatPort = NETEC_Node::GetNATPort();

	SAFE_STRCPY(m_pTalkRespond->szServerID, NETEC_Node::GetMCUID());//自己的MCUID
	SAFE_STRCPY(m_pTalkRespond->szServerAddr, NETEC_Node::GetServerIP());
	m_pTalkRespond->nServerPort = NETEC_Node::GetServerPort();

	SAFE_STRCPY(m_pTalkRespond->szToNodeID, pTalkData->szNodeID);//对方的NODEID

	SAFE_STRCPY(m_pTalkRespond->szUserID, pTalkData->szUserID);
	SAFE_STRCPY(m_pTalkRespond->szUserBuffer, pTalkData->szUserBuffer);
	//memcpy(m_pTalkRespond->szUserBuffer,pTalkData->szUserBuffer, pTalkData->nUserLength);
	m_pTalkRespond->nUserLength = pTalkData->nUserLength;

	if (m_hTransferSend==NULL)
	{
		m_hTransferSend = AVCON_STF_OpenSend(pTalkData->szUserID, m_ulTalkAudioID, 0,OnSTFCallback_SendMessage,this);
	}

	if (m_hTransferRecv==NULL)
	{
		m_hTransferRecv = AVCON_STF_OpenRecv(pTalkData->szUserID,RTSP_SIP,pTalkData->nAudioID,0,pTalkData->szNodeID,
			pTalkData->szNatAddr,pTalkData->nNatPort,pTalkData->szLocalAddr,pTalkData->nLocalPort,pTalkData->szServerID,
			pTalkData->szServerAddr,pTalkData->nServerPort,OnSTFCallback_RecvMessage,this,OnSTFTransferStream,this);
	}

	if (m_pDevTalkCallback)
	{
		return m_pDevTalkCallback->OnDevTalkCallback_StartTalkSDK(1);
	}


	return false;
}

void CDevTalk::StopTalk( const char *szNodeID )
{
	if (NULL != szNodeID && strcmp(szNodeID, m_pTalkRespond->szToNodeID) != 0)
	{
		return ;
	}

	m_bWantToStop=true;
	printf("CDevTalk::StopTalk..\n");

	if (m_pTalkRespond)
	{
		//1:会议对讲还是0:监控对讲
		//停止对讲时，又把audioID还原回去
		if ( strcmp(m_pTalkRespond->szUserBuffer, TALK_TYPE_MEETING) ==0 )
		{
			m_pDevTalkCallback->OnDevTalkCallback_DevChannelChange( m_ulTalkAudioID, false);
		}

		free(m_pTalkRespond);
		m_pTalkRespond = NULL;
	}

	long hTemp = NULL;
	if (m_hTransferRecv)
	{
		hTemp = m_hTransferRecv;
		m_hTransferRecv = NULL;
		AVCON_STF_CloseRecv(hTemp);
	}

	m_pDevTalkCallback->OnDevTalkCallback_StopTalkSDK(1);

	if (m_h3PartyToAVC)
	{
		hTemp = m_h3PartyToAVC;
		m_h3PartyToAVC = NULL;
		AVCON_STC_CloseTranscode(hTemp);
	}

	if (m_hAVCto3Party)
	{
		hTemp = m_hAVCto3Party;
		m_hAVCto3Party = NULL;
		AVCON_STC_CloseTranscode(hTemp);
	}

	if (m_hTransferSend)
	{
		hTemp = m_hTransferSend;
		m_hTransferSend = NULL;
		AVCON_STF_CloseSend(hTemp);
	}
	//m_strUserData = "";

}

void CDevTalk::TalkChanged( unsigned long nAudioID )
{
	if (m_hTransferRecv)
	{
		AVCON_STF_RecvChanged(m_hTransferRecv,nAudioID,0);
	}
}

void CDevTalk::OnOpenDevTalkChn( int nErrCode, void *pBuffer, int nBufLen )
{
	if (nErrCode == 0)
	{
		RespondTalkSuccess();
	}
	else if (nErrCode == 1)
	{
		RespondTalkFailed();
	}
}

void CDevTalk::RespondTalkSuccess()
{
 	if (m_pTalkRespond == NULL)
 	{
 		return;
 	}

	m_pTalkRespond->nCallStatus = MPX_ERR_NO;

	//会议对讲中。设备对讲通道是单独通道必须这么做
	//1:会议对讲还是0:监控对讲
	if ( strcmp(m_pTalkRespond->szUserBuffer, TALK_TYPE_MEETING) ==0 )
	{
		m_pDevTalkCallback->OnDevTalkCallback_DevChannelChange( m_ulTalkAudioID );
	}

	if (m_pDevTalkCallback)
	{
		m_pDevTalkCallback->OnDevTalkCallback_RespondTalk(m_pTalkRespond);
	}
}

void CDevTalk::RespondTalkFailed()
{
 	if (m_pTalkRespond == NULL)
 	{
 		return;
 	}

	m_pTalkRespond->nCallStatus = MPX_ERR_VOICECALL_FAILED;

	if (m_pDevTalkCallback)
	{
		m_pDevTalkCallback->OnDevTalkCallback_RespondTalk( m_pTalkRespond );
	}

	StopTalk(m_pTalkRespond->szToNodeID);
}

void CDevTalk::RespondTalkBusy(MPX_TALK_INFO *pTalkData)
{
	if (pTalkData == NULL)
	{
		return;
	}

	MPX_TALK_INFO* pTalkRespond = (MPX_TALK_INFO*)malloc(pTalkData->nStructSize+pTalkData->nUserLength+1);
	memset(pTalkRespond,0,sizeof(MPX_TALK_INFO)+pTalkData->nUserLength+1);
	pTalkRespond->nStructSize = sizeof(MPX_TALK_INFO);
	pTalkRespond->nCallStatus = MPX_ERR_VOICECALL_BUSY;
	pTalkRespond->nAudioCodecID = m_nCodecID;
	pTalkRespond->nDevType = pTalkData->nDevType;
	SAFE_STRCPY(pTalkRespond->szDevID, pTalkData->szDevID);
	pTalkRespond->nAudioID = m_ulTalkAudioID;
	SAFE_STRCPY(pTalkRespond->szNodeID, NETEC_Node::GetNodeID());
	SAFE_STRCPY(pTalkRespond->szLocalAddr, NETEC_Node::GetLocalIP());
	pTalkRespond->nLocalPort = NETEC_Node::GetLocalPort();
	SAFE_STRCPY(pTalkRespond->szNatAddr, NETEC_Node::GetNATIP());
	pTalkRespond->nNatPort = NETEC_Node::GetNATPort();
	SAFE_STRCPY(pTalkRespond->szServerID, NETEC_Node::GetMCUID());//自己的MCUID
	SAFE_STRCPY(pTalkRespond->szServerAddr, NETEC_Node::GetServerIP());
	pTalkRespond->nServerPort = NETEC_Node::GetServerPort();
	SAFE_STRCPY(pTalkRespond->szToNodeID, pTalkData->szNodeID);//对方的NODEID
	SAFE_STRCPY(pTalkRespond->szUserID, pTalkData->szUserID);
	SAFE_STRCPY(pTalkRespond->szUserBuffer, pTalkData->szUserBuffer);
	pTalkRespond->nUserLength = pTalkData->nUserLength;

	if (m_pDevTalkCallback)
	{
		m_pDevTalkCallback->OnDevTalkCallback_RespondTalk( pTalkRespond );
	}

	free(pTalkRespond);
}

void CDevTalk::OnTransferStream( long hHandle,const char*szKey,int nDevType,int nPacketType,unsigned char*pData,int nLen )
{
	if (m_bWantToStop)
	{
		return;
	}

	if (m_hAVCto3Party==NULL)
	{
		m_hAVCto3Party = AVCON_STC_OpenTranscode(szKey,ATT_AVC,nPacketType, RTSP_SIP, OnSTC_3RDPacketCallback,this);
	}

	if (m_hAVCto3Party)
	{
		AVCON_EXTRA_DATA AvconfFrame;
		AvconfFrame.nStructSize = sizeof(AVCON_EXTRA_DATA);
		AvconfFrame.nCodecType = nPacketType;
		AvconfFrame.nPacketType = APT_AUDIO;
		AvconfFrame.nTimestamp = 0;
		AVCON_STC_InputStandardCodecAVPacket(m_hAVCto3Party,pData,nLen,(unsigned char *)&AvconfFrame,sizeof(AVCON_EXTRA_DATA));
	}
}

void CDevTalk::OnSendTransferMessage( long hHandle,const char*szKey,int nMessage,long wParam,long lParam )
{

}

void CDevTalk::OnRecvTransferMessage( long hHandle,const char*szKey,int nMessage,long wParam,long lParam )
{
	if (m_hTransferRecv!=hHandle || m_bWantToStop)
	{
		return ;
	}

	if (m_pDevTalkCallback)
	{
		m_pDevTalkCallback->OnDevTalkCallback_TalkTimerout();
	}
}

void CDevTalk::OnMonTo3RDDevice( long hHandle,int nStreamType,unsigned char*pData,int nLen )
{
 	if (m_bWantToStop)
 	{
 		return;
 	}

	if (m_hAVCto3Party==hHandle && AST_AUDIO==nStreamType)
	{
		m_pDevTalkCallback->OnDevTalkCallback_SendVoiceToDevice(nStreamType, pData, nLen);
	}
}

void CDevTalk::OnAVCPacket2MTS( long hHandle,int nStreamType,unsigned char*pData,int nLen )
{
 	if (m_bWantToStop)
 	{
 		return;
 	}

	if (m_h3PartyToAVC==hHandle && AST_AUDIO==nStreamType)
	{
		if (m_hTransferSend)
		{
			AVCON_STF_InputAudioPacket(m_hTransferSend,pData,nLen);
		}
	}
}

void CDevTalk::OnVoiceStream( unsigned int dwMediaType, unsigned char *pBuffer, unsigned int dwBufSize, unsigned char*pExtraData,unsigned int nExtraSize )
{
	if (m_bWantToStop)
	{
		return;
	}
	if (dwMediaType == AST_AUDIO )
	{
		if (m_h3PartyToAVC==NULL)
		{
			m_h3PartyToAVC = AVCON_STC_OpenTranscode(UINT2STR(AST_AUDIO).c_str(),ATT_3RD, AST_AUDIO, m_nDevType, OnSTC_AVCPacketCallback,this);
		}

		if (m_h3PartyToAVC)
		{
			AVCON_STC_InputStandardPacket(m_h3PartyToAVC,ASF_RTP,pBuffer,dwBufSize);
		}
	}
}
