#include "stdafx.h"
#include "CGBDeviceSdk.h"
#include "ParserXML.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

HPMON_ISDK * HPMON_ISDK::Create(ISDKCallback *pCallback)
{
	return new CGBSDK(pCallback);
}

HPMON_IDeviceSdk * HPMON_IDeviceSdk::Create(IDeviceSdkCallback *pCallback)
{
	return new CGBDeviceSdk(pCallback);
}



////////////////////////////////////////////////////////////////////////////////////////////////////////
CGBSDK::CGBSDK(ISDKCallback * pCallBack)
:m_pCallBack(pCallBack)
{
	m_pGB28181Main=GB28181Main::GetInstance();
}

CGBSDK::~CGBSDK()
{

}

int CGBSDK::InitSDK()
{
	ConfigData * pConfigData=ConfigData::GetInstance();
	std::string strBinFile=Fun::GetIniPath("");
	unsigned long nPos=strBinFile.find("bin");
	if (nPos!=std::string::npos)
	{
#ifdef WIN32
		char *sz="conf\\sipconfig.xml";
#else
		char *sz="conf/sipconfig.xml";
#endif
		strBinFile.replace(nPos,strlen(sz),sz);
	}
	printf("[GB28181]: Get the sip config path is :%s\n",strBinFile.c_str());
	if(ParserXML::ParserFile(strBinFile,*pConfigData)==false)
	{
		printf("[GB28181]: Parser sip config file failed and use default!\n");
		if(m_pGB28181Main)
		{
			m_pGB28181Main->Init();
			m_pGB28181Main->SetEventHandle(this);
			m_pGB28181Main->SetSIPProtocol(GB_PROTOCOL_UDP);
			m_pGB28181Main->SetSIPPort(5060);
			m_pGB28181Main->SetAudioPort(10002);
			m_pGB28181Main->SetVideoPort(10004);
			m_pGB28181Main->SetSIPServerID("34020000002000000001");
			m_pGB28181Main->Start();
		}
	}
	else
	{
		printf("[GB28181]: Parser sip config file succeed!\n");
		if(m_pGB28181Main)
		{
			m_pGB28181Main->Init(ConfigData::GetInstance()->m_DevicePluginInfo.nOsipTraceLevel,
				ConfigData::GetInstance()->m_DevicePluginInfo.strLocalIP);

			m_pGB28181Main->SetEventHandle(this);
			m_pGB28181Main->SetSIPProtocol(GB_PROTOCOL_UDP);
			m_pGB28181Main->SetSIPPort(ConfigData::GetInstance()->m_DevicePluginInfo.nLocalPort);
			m_pGB28181Main->SetAudioPort(10002);
			m_pGB28181Main->SetVideoPort(10004);
			m_pGB28181Main->SetSIPServerID(ConfigData::GetInstance()->m_DevicePluginInfo.strLocalID);
			m_pGB28181Main->SetMapIP(ConfigData::GetInstance()->m_DevicePluginInfo.strMapIP);
			m_pGB28181Main->SetMapPort(ConfigData::GetInstance()->m_DevicePluginInfo.nMapPort);
			m_pGB28181Main->SetStaticPwd(ConfigData::GetInstance()->m_DevicePluginInfo.bStaticPwd);
			m_pGB28181Main->Start();
		}
	}
	
	return 0;
}

void CGBSDK::UnInitSDK()
{
	if (m_pGB28181Main!=NULL)
	{
		m_pGB28181Main->Stop();
		m_pGB28181Main->UnInit();
	}
}
//--------------------------------------------------

void CGBSDK::OnException(int nErrorCode, void *pBuf,int nBuflen)
{
	if (m_pCallBack!=NULL)
	{
		m_pCallBack->OnSDKCallback_Exception(nErrorCode,pBuf,nBuflen);
	}
}

void CGBSDK::OnGB28181SIPEntityRegistered(const std::string & strID,const std::string & strIP,const int &nPort)
{
	printf("[GB28181]: The [%s] (IP: %s Port :%d) registered\n",strID.c_str(),strIP.c_str(),nPort);
	if (m_pCallBack!=NULL)
	{
		m_pCallBack->OnSDKCallback_RegisterDevice((char*)strIP.c_str(),nPort,(char*)strID.c_str(),"",RTSP_SIP,(char*)"",NULL);
	}
}

void CGBSDK::OnGB28181SIPEntityUnregistered(const std::string &strID)
{
	if (m_pCallBack!=NULL)
	{
		m_pCallBack->OnSDKCallback_UnRegisterDevice((char*)strID.c_str());
	}
}

void CGBSDK::OnGB28181SIPEntityReady(const std::string & strID,const std::string & strIP,const int &nPort)
{
	if (m_pCallBack!=NULL)
	{
		//m_pCallBack->OnSDKCallback_RegisterDevice((char*)strIP.c_str(),nPort,(char*)strID.c_str(),"",RTSP_SIP,(char*)"",NULL);
	}
}

bool CGBSDK::OnGB28181SIPEntityPasswordReady(const std::string & strID)
{
	//有密码就返回true,没密码就向MCU查一下密码，然后返回false
	if (m_pCallBack!=NULL)
	{
		return m_pCallBack->OnSDKCallback_IsPasswordReady((char *)strID.c_str());
		//pb
		//return true;
	}
	return false;
}

std::string CGBSDK::OnGB28181SIPEntityQueryPassword(const std::string & strID)
{
	if (m_pCallBack)
	{
		return m_pCallBack->OnSDKCallback_QueryPassword((char*)strID.c_str());
	}
	return "";
}


////////////////////////////////////////////////////////////////////////////////////////////////////////

CGBDeviceSdk::CGBDeviceSdk(IDeviceSdkCallback * pSdkCallBack)
:m_pSdkCallBack(pSdkCallBack)
, m_nVoiceCodecID(X_AUDIO_CODEC_G711A)
{
	m_pGB28181Main=GB28181Main::GetInstance();
	m_pDevice=NULL;
	m_nVoiceHandle=-1;
	memset(m_VoiceBuffer,0,1024);
	m_nVoiceBufferLen=0;
	m_nSeq=0;
	m_nTimeStamp=XGetTimestamp();
	m_pAlignLeft=NULL;
	m_nCurrentTime=0;
	m_nChannelCount=0;
}

CGBDeviceSdk::~CGBDeviceSdk()
{
	if(m_pAlignLeft!=NULL)
	{
		delete m_pAlignLeft;
	}
}

//-----------------------------------channelsdk-------------------------------//
bool CGBDeviceSdk::CreateChannelSDK(int nHPChannelID,const std::string & strChannelID,CGBDeviceSdk * pGBDeviceSdk)
{
	if (FindChnSDK(nHPChannelID)!=NULL)
	{
		//通道已经创建,防止通道多次上线造成内存泄漏
		return false;
	}
	CGBChannelSdk * pCGBChannelSdk=new CGBChannelSdk(nHPChannelID,strChannelID,pGBDeviceSdk);
	if (pCGBChannelSdk!=NULL)
	{
		pCGBChannelSdk->MON_CHNSDK_Init();
		PushChannelSDK(nHPChannelID, pCGBChannelSdk);
		if (m_pSdkCallBack)
		{
			++m_nChannelCount;
			m_pSdkCallBack->OnDevSdkCallback_RecMessageCb( EM_DEVSDK_GET_DEV_CHANNELNUM, (void*)&m_nChannelCount, sizeof(int));
			m_pSdkCallBack->OnDevSdkCallback_RecChannelInfo(nHPChannelID, ONLINE, pCGBChannelSdk);

		}
		return true;
	}
	return false;
}

void CGBDeviceSdk::PushChannelSDK(int nHPChnID, CGBChannelSdk *pChannelSdk)
{
	KAutoLock l(m_csChannelSdk);
	m_mapChannelSdk[nHPChnID] = pChannelSdk;
}

void CGBDeviceSdk::ClearChannelSDK()
{
	do 
	{
		CGBChannelSdk *pCGBChannelSdk = NULL;
		{
			KAutoLock l(m_csChannelSdk);
			ID_CHANNELSDK_MAP::iterator it = m_mapChannelSdk.begin();
			if (it!=m_mapChannelSdk.end())
			{
				pCGBChannelSdk = it->second;
				m_mapChannelSdk.erase(it);
			}
			else
			{
				break;
			}
		}

		if ( pCGBChannelSdk)
		{
			pCGBChannelSdk->MON_CHNSDK_UnInit();
			delete pCGBChannelSdk;
			pCGBChannelSdk = NULL;
			//发信息给Mondevice清除数据
		}

	} while (1);
}

void CGBDeviceSdk::ReSetChannelSdkSessionID(int hSession)
{
	{
		//KAutoLock l(m_csHikChnSdk);
		CGBChannelSdk *pGBChannelSdk = NULL;
		ID_CHANNELSDK_MAP::iterator it = m_mapChannelSdk.begin();
		while (it!=m_mapChannelSdk.end())
		{
			pGBChannelSdk = it->second;
			if (pGBChannelSdk)
			{
				//pGBChannelSdk->SetSessionID(hSession);
			}
		}
	}
}

CGBChannelSdk * CGBDeviceSdk::FindChnSDK(int nHPChnID)
{
	CGBChannelSdk *pCGBChannelSdk = NULL;
	{
		KAutoLock l(m_csChannelSdk);
		ID_CHANNELSDK_MAP::iterator it = m_mapChannelSdk.find(nHPChnID);
		if (it!=m_mapChannelSdk.end() )
		{
			pCGBChannelSdk =it->second;
		}
	}
	return pCGBChannelSdk;
}


bool CGBDeviceSdk::CreateGBAlarm(int nHPAlarmID,const std::string & strAlarmID)
{
	if (m_pDevice==NULL)
	{
		printf("[GB28181]:The device is empty![CGBDeviceSdk::CreateGBAlarm]\n");
		return false;
	}
	if (FindGBAlarm(nHPAlarmID)!=NULL)
	{
		printf("[GB28181]:The alarm is exist in the device![CGBDeviceSdk::CreateGBAlarm]\n");
		return false;
	}

	GBAlarm * pAlarm=NULL;
	if(m_pGB28181Main->GetAlarm(m_pDevice,strAlarmID,&pAlarm)==false)
	{
		printf("[GB28181]:Get the alarm failed![CGBDeviceSdk::CreateGBAlarm]\n");
		return false;
	}
	PushGBAlarm(nHPAlarmID,pAlarm);
	return true;
}

void CGBDeviceSdk::PushGBAlarm(int nHPAlarmID,GBAlarm * pAlarm)
{
	KAutoLock l(m_csmapGBAlarm);
	m_mapGBAlarm[nHPAlarmID]=pAlarm;
}

GBAlarm * CGBDeviceSdk::FindGBAlarm(int nHPAlarmID)
{
	KAutoLock l(m_csmapGBAlarm);
	HPID_GBALARM_MAP::iterator it=m_mapGBAlarm.find(nHPAlarmID);
	if (it!=m_mapGBAlarm.end())
	{
		return it->second;
	}
	return NULL;
}

void CGBDeviceSdk::ClearGBAlarm()
{
	KAutoLock l(m_csmapGBAlarm);
	HPID_GBALARM_MAP::iterator it=m_mapGBAlarm.begin();
	for(;it!=m_mapGBAlarm.end();it++)
	{
		if (it->second)
		{
			m_pGB28181Main->FreeAlarm(&it->second);
		}
	}
	m_mapGBAlarm.clear();
}


//外部调用
bool CGBDeviceSdk::MON_DEVSDK_Init()
{
	return true;
}

void CGBDeviceSdk::MON_DEVSDK_UnInit()
{
	ClearChannelSDK();
	ClearGBAlarm();

	if (m_pGB28181Main && m_pDevice)
	{
		std::string strGBDeviceID=m_pDevice->m_strID;
		m_pGB28181Main->FreeDevice(&m_pDevice);
		m_pGB28181Main->RemoveRegisterSIPEntity(strGBDeviceID);
	}
}

bool CGBDeviceSdk::MON_DEVSDK_Connect(char *szDevID, char* szIPAddr, char* szDevUser, char* szDevPwd, unsigned short nPort, void *pUserData, int nLen )
{
	m_strGBDeviceID=szDevID;
	if (!m_pDevice)
	{
		m_pGB28181Main->GetDevice(szDevID,&m_pDevice);
		if (m_pDevice)
		{
			m_pDevice->SetDeviceEvent(this);
			if (m_pSdkCallBack)
			{
				m_pSdkCallBack->OnDevSdkCallback_ConnectResult(EM_DEVSDK_CONNECT_SUCCESS, 0, szDevID, NULL, 0 );
			}
		}
	}
	return true;//一直返回true，让每次操作的时候都去Get一下Device这样就好
}

void CGBDeviceSdk::MON_DEVSDK_DisConnect()
{

}

void CGBDeviceSdk::MON_DEVSDK_TimeOutNotOperatorDisConnect()
{

}

void CGBDeviceSdk::DisConnect()
{
	//短连接需要断开吗？
	//if (m_pDevice != NULL)
	//{
	//	m_pGB28181Main->FreeDevice(&m_pDevice);
	//	ReSetChannelSdkSessionID((int)m_pDevice);//free channel?
	//}
}

bool CGBDeviceSdk::MON_DEVSDK_IsConnect()
{
	return false;
}

void CGBDeviceSdk::MON_DEVSDK_GetChannelInfo()
{
	printf("\r\nNodeID:%s\r\n",NETEC_Node::GetNodeID());
	//if (!m_pDevice)
	//{
	//	m_pGB28181Main->GetDevice(m_strGBDeviceID,&m_pDevice);
	//}
	//if (!m_pDevice)
	//{
	//	return ;
	//}
	//m_pDevice->SetDeviceEvent(this);

	//if (m_mapChannelSdk.size()!=0 && m_mapChannelSdk.size()==m_pDevice->GetChannelNum())
	//{
	//	return ;
	//}

	//for (int i=0;i<m_pDevice->GetChannelNum();i++)
	//{
	//	CreateChannelSDK(i,"",this);
	//}

}

void CGBDeviceSdk::MON_DEVSDK_GetDevName()
{
	if (!m_pDevice)
	{
		m_pGB28181Main->GetDevice(m_strGBDeviceID,&m_pDevice);
	}
	if (!m_pDevice)
	{
		return ;
	}
	m_pDevice->SetDeviceEvent(this);

	//设备名称
	if (m_pSdkCallBack)
	{
		m_pSdkCallBack->OnDevSdkCallback_RecMessageCb(EM_DEVSDK_GET_DEV_NAME, (void*)m_pDevice->m_strID.c_str(), m_pDevice->m_strID.length());
	}
}

void CGBDeviceSdk::MON_DEVSDK_SetDevName(const char *szDevName)
{

}

void CGBDeviceSdk::MON_DEVSDK_GetChannelNum()
{
	printf("\r\nNodeID:%s\r\n",NETEC_Node::GetNodeID());
	//SIP不需要上报通道数目，SDK会自己一个通道一个通道回调过来
	//if (!m_pDevice)
	//{
	//	m_pGB28181Main->GetDevice(m_strGBDeviceID,&m_pDevice);
	//}
	//if (!m_pDevice)
	//{
	//	return ;
	//}
	//m_pDevice->SetDeviceEvent(this);

	//int nChannelNum=m_pDevice->GetChannelNum();
	//if (m_pSdkCallBack)
	//{
	//	m_pSdkCallBack->OnDevSdkCallback_RecMessageCb( EM_DEVSDK_GET_DEV_CHANNELNUM, (void*)&nChannelNum, sizeof(int));
	//}
}

void CGBDeviceSdk::MON_DEVSDK_GetDevStatus()
{
	if (m_pSdkCallBack)
	{
		int nStatus = m_pDevice!=NULL?ONLINE:OFFLINE;
		m_pSdkCallBack->OnDevSdkCallback_RecMessageCb( EM_DEVSDK_GET_DEV_STATUS, (void*)&nStatus, sizeof(int) );
	}
}

void CGBDeviceSdk::MON_DEVSDK_GetSerialNo()
{
	if (!m_pDevice)
	{
		m_pGB28181Main->GetDevice(m_strGBDeviceID,&m_pDevice);
	}
	if (!m_pDevice)
	{
		return ;
	}
	m_pDevice->SetDeviceEvent(this);

	if (m_pSdkCallBack)
	{
		m_pSdkCallBack->OnDevSdkCallback_RecMessageCb( EM_DEVSDK_GET_DEV_SERIALNO, (void*)m_pDevice->m_strID.c_str(), sizeof(int) );
	}
}

void CGBDeviceSdk::MON_DEVSDK_GetModelType()
{
	if (!m_pDevice)
	{
		m_pGB28181Main->GetDevice(m_strGBDeviceID,&m_pDevice);
	}
	if (!m_pDevice)
	{
		return ;
	}
	m_pDevice->SetDeviceEvent(this);

	m_nVoiceCodecID = X_AUDIO_CODEC_G711A;

	if (m_pSdkCallBack)
	{
		m_pSdkCallBack->OnDevSdkCallback_RecMessageCb( EM_DEVSDK_GET_DEV_MODELTYPE, (void*)m_pDevice->m_tDeviceInfo.strModel.c_str(), sizeof(int) );
	}
}

void CGBDeviceSdk::MON_DEVSDK_GetTalkCodecID()
{

}

void CGBDeviceSdk::MON_DEVSDK_StartTalk(char *szDevID, int nTalkChnID)
{
	if (!m_pDevice)
	{
		m_pGB28181Main->GetDevice(m_strGBDeviceID,&m_pDevice);
	}
	if (!m_pDevice)
	{
		return ;
	}

	m_pDevice->SetDeviceEvent(this);

	m_nVoiceHandle=m_pGB28181Main->DeviceStartTalk(m_pDevice);
	if (m_nVoiceHandle < 0)
		return ;

	GBAlarm * pAlarm=FindGBAlarm(0);//调试填0
	if (pAlarm)
	{
		m_pGB28181Main->ResetAlarm(pAlarm);
	}
	//if (m_pSdkCallBack)
	//{
	//	m_pSdkCallBack->OnDevSdkCallback_DevTalkResult(0, m_strGBDeviceID, NULL, 0);
	//}
	return ;
}

void CGBDeviceSdk::MON_DEVSDK_StopTalk(char *szDevID, int nTalkChnID)
{
	if (m_nVoiceHandle!=-1)
	{
		m_pGB28181Main->DeviceStopTalk(m_nVoiceHandle);
		m_nVoiceHandle=-1;
	}
}

void CGBDeviceSdk::MON_DEVSDK_SendTalkStream(int nStreamType, unsigned char*pData,int nLen)
{
	if (!m_pDevice)
	{
		m_pGB28181Main->GetDevice(m_strGBDeviceID,&m_pDevice);
	}
	if (!m_pDevice)
	{
		return ;
	}
	m_pDevice->SetDeviceEvent(this);

	if (m_nVoiceHandle!=-1)
	{
		//先往里面放
		//memcpy(m_VoiceBuffer+RTPHEADER_LEN+m_nVoiceBufferLen,pData,nLen);
		//m_nVoiceBufferLen+=nLen;
		//if (m_nVoiceBufferLen+RTPHEADER_LEN>512)
		//{
		//	//printf("RTPHeader size is %d\n",sizeof(RTPHeader));
		//	RTPHeader *pHeader=(RTPHeader *)m_VoiceBuffer;
		//	pHeader->v=2;
		//	pHeader->p=0;
		//	pHeader->x=0;
		//	pHeader->cc=0;//CSRC count 为0
		//	pHeader->m=1;//音频每帧都是完整帧
		//	pHeader->pt=8;//代表PCMA
		//	if (m_nSeq<65536)
		//	{
		//		m_nSeq++;
		//	}
		//	else
		//	{
		//		m_nSeq=1;
		//	}
		//	pHeader->seq=htons(m_nSeq);
		//	pHeader->tm=htonl(m_nTimeStamp+20);
		//	pHeader->tm=htonl();
		//	pHeader->ssrc=htonl(1);

		//	m_pDevice->SendVoiceStream((char *)m_VoiceBuffer,RTPHEADER_LEN+m_nVoiceBufferLen);
		//	printf("The data length is %d\n",RTPHEADER_LEN+m_nVoiceBufferLen);
		//	m_nVoiceBufferLen=0;
		//}

		if (m_pAlignLeft==NULL)
		{
			m_pAlignLeft=new align_left_bytes(*this,160);
		}

		if (m_pAlignLeft)
		{
			m_pAlignLeft->AlignBytes(pData,nLen);
		}

	}
}

void CGBDeviceSdk::MON_DEVSDK_SetDevID(char *szDevID)
{

}

void CGBDeviceSdk::OnDeviceVoicePlaySucceed( int nPlayHandle )
{
	if (m_nVoiceHandle!=nPlayHandle)
	{
		printf("[GBSDK]: Play the audio handle is different!\n");
		m_nVoiceHandle=nPlayHandle;
		m_nSeq=0;
	}

	if (m_pSdkCallBack)
	{
		m_pSdkCallBack->OnDevSdkCallback_DevTalkResult(0, m_strGBDeviceID, NULL, 0);
	}

	m_nTimeStamp=XGetTimestamp();
	
}

void CGBDeviceSdk::OnDeviceVoicePlayFailed( int nPlayHandle )
{
	if (m_nVoiceHandle==nPlayHandle)
	{
		m_nVoiceHandle=-1;
	}

	if (m_pSdkCallBack)
	{
		m_pSdkCallBack->OnDevSdkCallback_DevTalkResult(1, m_strGBDeviceID, NULL, 0);
	}
}

void CGBDeviceSdk::OnDeviceRecvAudioStream(int nPlayHandle,void * pData,int nLen)
{
	if (m_nVoiceHandle != nPlayHandle)
	{
		return;
	}

	if (nLen>0)
	{

		AVCON_EXTRA_DATA extra;
		memset(&extra,0,sizeof(AVCON_EXTRA_DATA));
		extra.nStructSize = sizeof(AVCON_EXTRA_DATA);
		extra.nPacketType = APT_AUDIO;
		extra.nCodecType = m_nVoiceCodecID;
		if (m_pSdkCallBack)
		{
			m_pSdkCallBack->OnDevSdkCallback_DevTalkVoiceStream(AST_AUDIO, (unsigned char*)pData,nLen,(unsigned char*)&extra,extra.nStructSize+extra.nLength);
		}
	}
}

void CGBDeviceSdk::OnDeviceAlarmCallBack(const GBAlarmInfo &tGBAlarmInfo)
{
	if (m_pSdkCallBack)
	{
		m_pSdkCallBack->OnDevSdkCallback_DevAlarmNotify(tGBAlarmInfo.nNum,tGBAlarmInfo.nAlarmMethod,tGBAlarmInfo.nSubAlarmMethod,tGBAlarmInfo.strAlarmTime,0,0);
	}
}

void CGBDeviceSdk::OnDeviceChannelOnline(const std::string & strChannelID,const int & nChannelNum)
{
	if (nChannelNum<1 || nChannelNum>32)
	{
		printf("[GB28181]:The channel num is illegal![CGBDeviceSdk::OnDeviceChannelOnline]\n");
		return ;
	}
	CreateChannelSDK(nChannelNum-1,strChannelID,this);
}

void CGBDeviceSdk::OnDeviceAlarmOnline(const std::string & strAlarmID,const int & nAlarmNum)
{
	if (nAlarmNum<1 || nAlarmNum>32)
	{
		printf("[GB28181]:The alarm num is illegal![CGBDeviceSdk::OnDeviceAlarmOnline]\n");
		return ;
	}
	CreateGBAlarm(nAlarmNum-1,strAlarmID);
}

void CGBDeviceSdk::OnDeviceIPChanged(const char * szIPaddr,const int nPort)
{
	m_pDevice->m_strIP=szIPaddr;
	m_pDevice->m_nPort=nPort;
	{
		KAutoLock l(m_csChannelSdk);
		CGBChannelSdk *pGBChannelSdk = NULL;
		ID_CHANNELSDK_MAP::iterator it = m_mapChannelSdk.begin();
		while (it!=m_mapChannelSdk.end())
		{
			pGBChannelSdk = it->second;
			if (pGBChannelSdk)
			{
				pGBChannelSdk->UpdateChannelIP(szIPaddr,nPort);
			}
			it++;
		}
	}

	MPX_DEVICE_INFO tDeviceInfo;
	SAFE_STRCPY(tDeviceInfo.szDevAddr,szIPaddr);
	tDeviceInfo.nDevPort=nPort;
	if (m_pSdkCallBack)
	{
		m_pSdkCallBack->OnDevSdkCallback_RecMessageCb( EM_DEVSDK_UPDATE_DEV_IP, (void*)&tDeviceInfo, sizeof(MPX_DEVICE_INFO));
	}

}

void CGBDeviceSdk::OnAlignLeftCallback_OneLeftBytes( align_left_bytes* pLeft,unsigned char*pBytes,int nBytes )
{
	if (m_pAlignLeft==pLeft)
	{
		memcpy(m_VoiceBuffer+RTPHEADER_LEN,pBytes,nBytes);
		RTPHeader *pHeader=(RTPHeader *)m_VoiceBuffer;
		pHeader->v=2;
		pHeader->p=0;
		pHeader->x=0;
		pHeader->cc=0;//CSRC count 为0
		pHeader->m=1;//音频每帧都是完整帧
		pHeader->pt=8;//代表PCMA
		if (m_nSeq<65536)
		{
			m_nSeq++;
		}
		else
		{
			m_nSeq=1;
		}
		pHeader->seq=htons(m_nSeq);
		m_nTimeStamp+=160;
		pHeader->tm=htonl(m_nTimeStamp);
		pHeader->ssrc=htonl(1);

		if(m_nCurrentTime==0)
		{
			m_nCurrentTime=XGetTimestamp();
		}
		else
		{
			//TRACE("The time is :%d\n",XGetTimestamp()-m_nCurrentTime);
			unsigned long nInterval=XGetTimestamp()-m_nCurrentTime;
			m_nCurrentTime=XGetTimestamp();
		}
		m_pDevice->SendVoiceStream((char *)m_VoiceBuffer,RTPHEADER_LEN+nBytes);
		//printf("Sending the audio data length is:%d",RTPHEADER_LEN+nBytes);
	}
}