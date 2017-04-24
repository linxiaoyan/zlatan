#include "StdAfx.h"
#include "MonChannel.h"
#include "NETEC/XUtil.h"
#include "GBDefines.h"

#ifdef WIN32
#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#endif

void OnSTC_PacketCallback(long hHandle,const char*szKey,int nTranscodeType,int nStreamType,int nPacketType,
						  unsigned char*pData,int nLen,unsigned char*pExtraData,int nExtraSize,void*pUser)
{
 	CMonChannel* pThis = (CMonChannel*)pUser;
 	if (pThis)
 	{
 		pThis->OnTranscodePacket(hHandle,szKey,nTranscodeType,nStreamType,nPacketType,pData,nLen,pExtraData,nExtraSize);
 	}
}

void TransferMessage(long hHandle,const char*szKey,int nMessage,long wParam,long lParam,void*pUser)
{
 	CMonChannel* pThis = (CMonChannel*)pUser;
 	if (pThis)
 	{	
 		pThis->OnTransferMessage(hHandle,szKey,nMessage,wParam,lParam);
 	}
}

void TransferStream(long hHandle,const char*szKey,int nDevType,int nPacketType,unsigned char*pData,int nLen,void*pUser)
{
 	CMonChannel* pThis = (CMonChannel*)pUser;
 	if (pThis)
 	{	
 		pThis->OnTransferStream(hHandle,szKey,nDevType,nPacketType,pData,nLen);
 	}
}


CMonChannel::CMonChannel(IMonChannelCallback *pCallback, HPMON_IChannelSdk *pChannelSDK)
: m_pChnCallback(pCallback )
, m_pChannelSDK( pChannelSDK )
, m_nChnStatus(OFFLINE)
, m_hVideoTranscode(NULL)
, m_hTransfer(NULL)
, m_hSubTranscode(NULL)
, m_hAudioTranscode(NULL)
, m_bMakeKeyframe(false)
, m_bMakeSubKeyframe(false)
, m_bWantToRePlay(false)
, m_bDvrEventBusy(false)
, m_bWantToClose(false)
{
	

}

CMonChannel::~CMonChannel(void)
{
	//这个已经在CGBDeviceSdk的ClearChannelSDK
	//pCGBChannelSdk->MON_CHNSDK_UnInit();
	//if (m_pChannelSDK!=NULL)
	//{
	//	m_pChannelSDK->MON_CHNSDK_UnInit();
	//}
}

int CMonChannel::Create( int nChnID, const MPX_DEVICE_INFO* pDevInfo )
{
	if (pDevInfo == NULL)
	{
		return FALSE;
	}
	if( m_pChannelSDK )
		m_pChannelSDK->MON_CHNSDK_SetMonChannel(this);

	m_mpxCamInfo.nAudioID = XGenerateSSRC();XSleep(1);
	m_mpxCamInfo.nVideoID = m_mpxCamInfo.nAudioID+1;
	m_mpxCamInfo.nChanID = nChnID;

	m_mpxCamInfo.nDevPort = pDevInfo->nDevPort;
	m_mpxCamInfo.nDevType = pDevInfo->nDevType;

	SAFE_STRCPY(m_mpxCamInfo.szDevID,pDevInfo->szDevID);
	SAFE_STRCPY(m_mpxCamInfo.szDevAddr,pDevInfo->szDevAddr);
	SAFE_STRCPY(m_mpxCamInfo.szDevUser,pDevInfo->szDevUser);
	SAFE_STRCPY(m_mpxCamInfo.szDevPass,pDevInfo->szDevPass);
	SAFE_STRCPY(m_mpxCamInfo.szSerialNo,pDevInfo->szSerialNo);

	SAFE_STRCPY(m_mpxCamInfo.szMTSAccount, pDevInfo->szMTSAccount);
	m_mpxCamInfo.nReqStatus = 0;
	m_mpxCamInfo.nPlyStatus = 0;

	m_mpxCamInfo.nStatus = OFFLINE;
	m_mpxCamInfo.nToMtsStatus = OFFLINE;

	SAFE_STRCPY(m_mpxCamInfo.szServerID,	NETEC_Node::GetMCUID());
	SAFE_STRCPY(m_mpxCamInfo.szServerAddr,	NETEC_Node::GetServerIP());
	m_mpxCamInfo.nServerPort = NETEC_Node::GetServerPort();

	SAFE_STRCPY(m_mpxCamInfo.szNodeID,		NETEC_Node::GetNodeID());
	SAFE_STRCPY(m_mpxCamInfo.szLocalAddr,	NETEC_Node::GetLocalIP());
	SAFE_STRCPY(m_mpxCamInfo.szNatAddr,		NETEC_Node::GetNATIP());
	m_mpxCamInfo.nNatPort	= NETEC_Node::GetNATPort();
	m_mpxCamInfo.nLocalPort = NETEC_Node::GetLocalPort();


	if (m_hTransfer)
	{
		printf("CMonChannel::Create AVCON_STF_CloseSend \n");
		AVCON_STF_CloseSend(m_hTransfer);
		m_hTransfer = NULL;
	}

	m_hTransfer = AVCON_STF_OpenSend(INT2STR(nChnID).c_str(),
		m_mpxCamInfo.nAudioID, m_mpxCamInfo.nVideoID,TransferMessage,this);

	if (m_hTransfer==NULL)
	{
		return FALSE;
	}

	m_bWantToClose = false;
	if(m_pChannelSDK)
	{
		m_pChannelSDK->MON_CHNSDK_GetChannelStatus();
		//通道名字不再依赖设备，依赖网关，网关有同时设置前端名字接口,这样就达到同步显示
		//m_pChannelSDK->MON_CHNSDK_GetChannelName(m_mpxCamInfo.nChanID);
	}
	return TRUE;
}

void CMonChannel::Destory()
{
	m_bWantToClose = true;
	while (m_bDvrEventBusy)
	{
		Pause(10);
	}

	StopPlay();
	StopPlaySub();

	if (m_hTransfer)
	{
		printf("CMonChannel::Destory AVCON_STF_CloseSend \n");
		AVCON_STF_CloseSend(m_hTransfer);
		m_hTransfer = NULL;
	}
 	
	long hTemp = NULL;
	if (m_hVideoTranscode)
	{
		hTemp = m_hVideoTranscode;
		AVCON_STC_CloseTranscode(hTemp);
		m_hVideoTranscode=NULL;
	}
	if (m_hAudioTranscode)
	{
		hTemp = m_hAudioTranscode;
		AVCON_STC_CloseTranscode(hTemp);
		m_hAudioTranscode=NULL;
	}
	if (m_hSubTranscode)
	{
		hTemp = m_hSubTranscode;
		AVCON_STC_CloseTranscode(hTemp);
		m_hSubTranscode=NULL;
	}

	m_mpxCamInfo.nToMtsStatus = OFFLINE;
	m_mpxCamInfo.nPlyStatus=0;
	m_pChnCallback->OnMonChannelCallback_ChannelOffline(&m_mpxCamInfo);

	SaveToDatabase();
}

//transcode回调
void CMonChannel::OnTranscodePacket( long hHandle,const char*szKey,int nTranscodeType,int nStreamType,int nPacketType, unsigned char*pData,int nLen,unsigned char*pExtraData,int nExtraSize )
{
	if (m_bWantToClose)
	{
		return ;
	}
	//printf("CMonChannel::OnTranscodePacket szKey:%d  size:%d...\n", szKey, nLen);

	if ( m_hVideoTranscode==hHandle || m_hSubTranscode==hHandle || m_hAudioTranscode == hHandle )
	{
		if (nTranscodeType==ATT_3RD)
		{
			if (nPacketType==APT_VIDEO)
			{
				AVCON_STF_InputVideoPacket(m_hTransfer, pData, nLen, nStreamType);
			}
			else if (nPacketType==APT_AUDIO)
			{

				AVCON_STF_InputAudioPacket(m_hTransfer,pData,nLen);
			}
		}	

	}
}
//transfer回调
void CMonChannel::OnTransferMessage( long hHandle,const char*szKey,int nMessage,long wParam,long lParam )
{
	if (hHandle != m_hTransfer || m_bWantToClose)
	{
		return;
	}

	if (nMessage==MESSAGE_SEND)
	{
		if (wParam==REQUEST_TYPE_PLAY)
		{
			int nReqPlayStatus = AST_NONE;
			if (lParam&AST_AUDIO)
			{
				//音频流
				//nReqPlayStatus |= AST_AUDIO;
			}

			if (lParam&AST_VIDEO)
			{
				//视频主流
				nReqPlayStatus |= AST_VIDEO;
			}

			if ((lParam&AST_VSUB0) || (lParam&AST_VSUB1))
			{
				//视频子流
				nReqPlayStatus |= (AST_VSUB0|AST_VSUB1);
			}

			m_mpxCamInfo.nReqStatus = nReqPlayStatus;
		}
		else if (wParam==REQUEST_TYPE_REPLAY)
		{
#ifndef _DEBUG
			m_bWantToRePlay = true;
#endif
		}
		else if (wParam==REQUEST_TYPE_KEYFRAME)
		{
 			if (lParam&AST_VIDEO)
 			{
 				m_bMakeKeyframe = true;
 			}
 
 			if (lParam&(AST_VSUB0|AST_VSUB1))
 			{
 				m_bMakeSubKeyframe = true;
 			}
		}
	}

}

void CMonChannel::OnTransferStream( long hHandle,const char*szKey,int nDevType,int nPacketType,unsigned char*pData,int nLen )
{
	if (m_bWantToClose 
		|| m_pChnCallback==NULL)
	{	
		return ;
	}
}

void CMonChannel::SetChannelSDK( HPMON_IChannelSdk *pChannelSDK )
{
	m_pChannelSDK = pChannelSDK;
}

void CMonChannel::OnChannelSdkCallback_RecMessageCb( int nMessage, void *pBuf,int nBuflen )
{
	if (pBuf && nBuflen>0)
	{

		switch (nMessage)
		{
		case EM_DEVSDK_GET_CHN_STATUS:
			{
				int *pChnStatus = (int*)pBuf;
				SetChannelStatus(*pChnStatus);
			}
			break;
		case EM_DEVSDK_GET_CHN_VIDEOINFO:
			{
				MPX_VIDEO_INFO *pVideoInfo = (MPX_VIDEO_INFO*)pBuf;
				if (pVideoInfo && m_pChnCallback && pVideoInfo->nStructSize == nBuflen)
			 {
				 m_pChnCallback->OnMonChannelCallback_GetVideoInfo(pVideoInfo);
			 }
			}
			break;
		case EM_DEVSDK_GET_CHN_NAME:
			{
				char *szChnName = (char*)pBuf;

				//如果设备名字已经存在就不要再覆盖了
				int nStrLen=strlen(m_mpxCamInfo.szChanName);
				bool bNameExist=false;
				for (int i=0;i<nStrLen;i++)
				{
					//如果有非空格就代表名字非空
					if(!isspace((unsigned char)m_mpxCamInfo.szChanName[i]))
					{
						bNameExist=true;
						break;
					}
				}
				if (!bNameExist)
				{
					SAFE_STRCPY(m_mpxCamInfo.szChanName,szChnName);
					SaveToDatabase();
				}
			}
			break;
		}
	}
}

void CMonChannel::OnChannelSdkCallback_VodStreamCallback()
{
	if (m_pChnCallback)
	{
		m_pChnCallback->OnMonChannelCallback_UpdateLastOptTime();
		m_pChnCallback->OnMonChannelCallback_UpdateSdkAliveTime();
	}
}

void CMonChannel::OnChannelSdkCallback_Exception( int nErrorCode, void *pBuf,int nBuflen )
{

}

void CMonChannel::SetChannelStatus( int nChnStatus )
{
	//m_nChnStatus = nChnStatus;
	m_mpxCamInfo.nStatus = nChnStatus;
	SaveToDatabase();;
}

void CMonChannel::SetMemChannelName(const std::string & strName)
{
	if (strName.length()<=0)
	{
		printf("[GB28181]: The name length <=0![CMonChannel::SetMemChannelName]\n");
		return ;
	}
	SAFE_STRCPY(m_mpxCamInfo.szChanName,strName.c_str());
}

void CMonChannel::ProcessMtsEvents( bool bOnlined )
{
	if (m_bWantToClose)
	{
		return ;
	}


	if (bOnlined!=true)
	{
		if (m_mpxCamInfo.nToMtsStatus == ONLINE)
		{
			
			m_mpxCamInfo.nToMtsStatus = OFFLINE;
			SaveToDatabase();

			m_pChnCallback->OnMonChannelCallback_ChannelOffline(&m_mpxCamInfo);
		}
	}	
	else
	{
		if (m_mpxCamInfo.nToMtsStatus!=ONLINE && m_mpxCamInfo.nStatus == ONLINE)
		{
			
			//控制上线间隔
			unsigned long ulOnlineTimeInterval = XGetTimestamp()-m_nMtsLastOnlineTime;
			if (ulOnlineTimeInterval<TIMEOUT_ONLINE_INTERVAL)
			{
				return ;
			}

			//上一次上线,服务没反馈,超时后再重新上线
			if (m_mpxCamInfo.nToMtsStatus==ONLINE_BUSY 
				&& ulOnlineTimeInterval<TIMEOUT_CHANNEL_ONLINE)
			{
				return ;
			}
			SAFE_STRCPY(m_mpxCamInfo.szServerID,	NETEC_Node::GetMCUID());	
			SAFE_STRCPY(m_mpxCamInfo.szServerAddr,	NETEC_Node::GetServerIP());
			m_mpxCamInfo.nServerPort = NETEC_Node::GetServerPort();

			SAFE_STRCPY(m_mpxCamInfo.szNodeID,		NETEC_Node::GetNodeID());
			SAFE_STRCPY(m_mpxCamInfo.szLocalAddr,	NETEC_Node::GetLocalIP());
			SAFE_STRCPY(m_mpxCamInfo.szNatAddr,		NETEC_Node::GetNATIP());
			m_mpxCamInfo.nNatPort	= NETEC_Node::GetNATPort();
			m_mpxCamInfo.nLocalPort = NETEC_Node::GetLocalPort();

			m_mpxCamInfo.nToMtsStatus = ONLINE_BUSY;
			m_nMtsLastOnlineTime = XGetTimestamp();
			SaveToDatabase();

			m_pChnCallback->OnMonChannelCallback_ChannelOnline(&m_mpxCamInfo);
		}
	}

}

void CMonChannel::ProcessDvrEvents( void )
{
	if (m_bWantToClose)
	{
		return ;
	}

	m_bDvrEventBusy = true;
	{
		if (m_bWantToRePlay)
		{
			RePlay();
		}
		else
		{
			if (m_mpxCamInfo.nReqStatus!=m_mpxCamInfo.nPlyStatus)
			{
				DoPlay();
			}
		}

		if (m_bMakeKeyframe)
		{
			MakeKeyframe();
		}

		if (m_bMakeSubKeyframe)
		{
			MakeSubKeyframe();
		}

	}m_bDvrEventBusy = false;
}

int CMonChannel::SaveToDatabase( int nModifyMask/*=MM_NONE*/ )
{
	if (m_pChnCallback)
	{
		m_pChnCallback->OnMonChannelCallback_ChannelUpdate(&m_mpxCamInfo, nModifyMask);
	}
	return TRUE;
}
int CMonChannel::OnMtsParamChanged(const std::string& strMtsAccount)
{
	m_mpxCamInfo.nToMtsStatus = OFFLINE;
	m_pChnCallback->OnMonChannelCallback_ChannelOffline(&m_mpxCamInfo);

	SAFE_STRCPY(m_mpxCamInfo.szMTSAccount, strMtsAccount.c_str());
	if (strcmp(m_mpxCamInfo.szMTSAccount, "") == 0)
	{
		printf("szMTSAccount is not any char \n");
		SaveToDatabase();
	}

	return REQUEST_NO;
}

void CMonChannel::OnChannelOnline( int nErrCode )
{
	if (nErrCode==MPX_ERR_NO)
	{
		m_mpxCamInfo.nToMtsStatus = ONLINE;
	}
	else
	{
		m_mpxCamInfo.nToMtsStatus = nErrCode;
	}
	SaveToDatabase();
}

void CMonChannel::OnChannelOffline( int nErrCode )
{
	if (nErrCode==MPX_ERR_NO)
	{
		m_mpxCamInfo.nToMtsStatus = OFFLINE;
		SaveToDatabase();
	}

}


BOOL CMonChannel::PtzControl( int nPtzCmd,int nSpeed,void*pExtData,int nExtSize )
{
	if (m_pChannelSDK)
	{
		m_pChannelSDK->MON_CHNSDK_PtzControl(m_mpxCamInfo.nChanID, nPtzCmd, nSpeed, pExtData, nExtSize);
	}
	return TRUE;
}

void CMonChannel::DoPlay( void )
{
	int nDiff = (m_mpxCamInfo.nPlyStatus^m_mpxCamInfo.nReqStatus);
	bool bAudio = false;
	if (nDiff&AST_AUDIO)
	{
		if (m_mpxCamInfo.nReqStatus&AST_AUDIO)
		{
			bAudio=true;
		}
	}

	if (nDiff&AST_VIDEO)
	{
		if (m_mpxCamInfo.nReqStatus&AST_VIDEO)
		{
			StartPlay( false );
		}
		else
		{
			StopPlay();
		}
	}

	if (nDiff&(AST_VSUB0|AST_VSUB1))
	{
		if (m_mpxCamInfo.nReqStatus&(AST_VSUB0|AST_VSUB1))
		{
			StartPlaySub();
		}
		else
		{
			StopPlaySub();
		}
	}
}

void CMonChannel::RePlay( void )
{
	m_bWantToRePlay = false;

	StopPlaySub();	
	StopPlay();
}

int CMonChannel::StartPlay( bool bPlayAudio )
{
	if (IsPlay())
	{
		return m_mpxCamInfo.nPlyStatus;
	}

	if (m_pChnCallback)
	{
		m_pChnCallback->OnMonChannelCallback_UpdateLastOptTime();
	}
	int nRet =FALSE;
	if (m_pChannelSDK)
	{
		printf("[GBDev]: Start play %s at channel %d",m_mpxCamInfo.szDevID,m_mpxCamInfo.nChanID);
		int nRet = m_pChannelSDK->MON_CHNSDK_StartPlay( m_mpxCamInfo.nChanID, AST_VIDEO|AST_AUDIO );
		if (nRet)
		{
			m_mpxCamInfo.nPlyStatus |=( AST_VIDEO|AST_AUDIO) ;
		}
		else
		{
			m_mpxCamInfo.nPlyStatus ^= ( AST_VIDEO|AST_AUDIO) ;
		}
	}
	SaveToDatabase();
	return m_mpxCamInfo.nPlyStatus;

}

void CMonChannel::StopPlay( void )
{
	m_mpxCamInfo.nPlyStatus ^= ( AST_VIDEO|AST_AUDIO) ;

	if (m_pChnCallback)
	{
		m_pChnCallback->OnMonChannelCallback_UpdateLastOptTime();
	}

	if (m_pChannelSDK)
	{
		printf("[GBDev]: Stop play %s at channel %d",m_mpxCamInfo.szDevID,m_mpxCamInfo.nChanID);
		m_pChannelSDK->MON_CHNSDK_StopPlay(m_mpxCamInfo.nChanID, AST_VIDEO|AST_AUDIO);
	}
	SaveToDatabase();
}

bool CMonChannel::IsPlay( void )
{
	return m_mpxCamInfo.nPlyStatus&( AST_VIDEO|AST_AUDIO)?true:false;
}

int CMonChannel::StartPlaySub( void )
{
	if (IsPlaySub())
	{
		return m_mpxCamInfo.nPlyStatus;
	}

	if (m_pChnCallback)
	{
		m_pChnCallback->OnMonChannelCallback_UpdateLastOptTime();
	}

	int nRet =FALSE;
	if (m_pChannelSDK)
	{
		int nRet = m_pChannelSDK->MON_CHNSDK_StartPlay( m_mpxCamInfo.nChanID, AST_VSUB0|AST_VSUB1 );
		if (nRet)
		{
			m_mpxCamInfo.nPlyStatus |=( AST_VSUB0|AST_VSUB1) ;
		}
		else
		{
			m_mpxCamInfo.nPlyStatus ^= ( AST_VSUB0|AST_VSUB1) ;
		}
	}
	SaveToDatabase();
	return m_mpxCamInfo.nPlyStatus;
}

void CMonChannel::StopPlaySub( void )
{
	m_mpxCamInfo.nPlyStatus ^= ( AST_VSUB0|AST_VSUB1) ;

	if (m_pChnCallback)
	{
		m_pChnCallback->OnMonChannelCallback_UpdateLastOptTime();
	}

	if (m_pChannelSDK)
	{
		m_pChannelSDK->MON_CHNSDK_StopPlay(m_mpxCamInfo.nChanID, AST_VSUB0|AST_VSUB1);
	}
	SaveToDatabase();
}

bool CMonChannel::IsPlaySub( void )
{
	return m_mpxCamInfo.nPlyStatus&( AST_VSUB0|AST_VSUB1)?true:false;
}
//请求关键帧
void CMonChannel::MakeKeyframe( void )
{
	m_bMakeKeyframe = false;
// 	if (m_hPlayHandle)
// 	{
// 		//NET_DVR_MakeKeyFrame(m_hPlayHandle,m_mpxCamInfo.nRealChanID);
// 	}
}

void CMonChannel::MakeSubKeyframe( void )
{
	m_bMakeSubKeyframe = false;
// 	if (m_hPlaySubHandle)
// 	{
// 		//NET_DVR_MakeKeyFrame(m_hPlaySubHandle,m_mpxCamInfo.nRealChanID);
// 	}
}

void CMonChannel::OnChannelSdkCallback_RealPlayStreamCallback( unsigned int dwMediaType, unsigned char *pBuffer, unsigned int dwBufSize, 
															  unsigned char*pExtraData,unsigned int nExtraSize )
{
	if (m_bWantToClose)
	{
		return ;
	}

	if (m_pChnCallback)
	{
		m_pChnCallback->OnMonChannelCallback_UpdateLastOptTime();
	}
	

	//printf("CMonChannel::OnChannelSdkCallback_RealPlayStreamCallback MediaType:%d size:%d \n", dwMediaType, dwBufSize);

	if (m_mpxCamInfo.nDevType == RTSP_SIP)
	{
		int nEncodeType=*(int*)pExtraData;
		if (nEncodeType==GB_MEDIA_TYPE_PS)
		{
			//PS流
			if (m_hVideoTranscode==NULL)
			{//注意，这里主流、子流、辅流都是同一条流
				m_hVideoTranscode = AVCON_STC_OpenTranscode(UINT2STR(AST_VIDEO).c_str(),ATT_3RD,AST_VIDEO|AST_AUDIO|AST_VSUB0|AST_VSUB1,m_mpxCamInfo.nDevType,OnSTC_PacketCallback,this);
			}

			if (m_hVideoTranscode)
			{
				AVCON_STC_InputStandardPacket(m_hVideoTranscode,ASF_PS,pBuffer,dwBufSize);
			}
		}
		else if (nEncodeType==GB_MEDIA_TYPE_H264)
		{
			//H264
			if (m_hVideoTranscode==NULL)
			{//注意，这里主流、子流、辅流都是同一条流
				m_hVideoTranscode = AVCON_STC_OpenTranscode(UINT2STR(AST_VIDEO).c_str(),ATT_3RD,AST_VIDEO|AST_AUDIO|AST_VSUB0|AST_VSUB1,m_mpxCamInfo.nDevType,OnSTC_PacketCallback,this);
			}

			if (m_hVideoTranscode)
			{
				AVCON_STC_InputStandardPacket(m_hVideoTranscode,ASF_RTP,pBuffer,dwBufSize);
			}
		}
		return;
	}



	if ( dwMediaType == AST_VIDEO )
	{
		unsigned int nDataType = *(unsigned int*)pExtraData;
		if (m_hVideoTranscode==NULL)
		{
			m_hVideoTranscode = AVCON_STC_OpenTranscode(UINT2STR(AST_VIDEO).c_str(),ATT_3RD, AST_VIDEO, m_mpxCamInfo.nDevType, OnSTC_PacketCallback,this);
		}

		if (m_hVideoTranscode)
		{
			AVCON_STC_Input3RDPacket(m_hVideoTranscode,pBuffer,dwBufSize,pExtraData, nExtraSize);
		}

	}
	else if (dwMediaType == AST_AUDIO )
	{
		if (m_hAudioTranscode==NULL)
		{
			m_hAudioTranscode = AVCON_STC_OpenTranscode(UINT2STR(AST_AUDIO).c_str(),ATT_3RD, AST_AUDIO, m_mpxCamInfo.nDevType, OnSTC_PacketCallback,this);
		}

		if (m_hAudioTranscode)
		{
			AVCON_STC_Input3RDPacket(m_hAudioTranscode,pBuffer,dwBufSize,pExtraData, nExtraSize);
		}
	}
	else if ( dwMediaType == (AST_VSUB0|AST_VSUB1) )
	{
		if (m_hSubTranscode==NULL)
		{
			m_hSubTranscode = AVCON_STC_OpenTranscode(UINT2STR(AST_VSUB0|AST_VSUB1).c_str(),ATT_3RD, AST_VSUB0|AST_VSUB1, m_mpxCamInfo.nDevType, OnSTC_PacketCallback,this);
		}

		if (m_hSubTranscode)
		{
			AVCON_STC_Input3RDPacket(m_hSubTranscode,pBuffer,dwBufSize, pExtraData, nExtraSize);
		}
	}

}

int CMonChannel::GetChanID( void )
{
	return m_mpxCamInfo.nChanID;
}

unsigned long CMonChannel::GetVideoID( void )
{
	return m_mpxCamInfo.nVideoID;
}

unsigned long CMonChannel::GetAudioID( void )
{
	return m_mpxCamInfo.nAudioID;
}

void CMonChannel::GetAudioInfo( MPX_AUDIO_INFO*pAudioInfo )
{

}

void CMonChannel::SetAudioInfo( MPX_AUDIO_INFO*pAudioInfo )
{

}

void CMonChannel::GetVideoInfo( MPX_VIDEO_INFO*pVideoInfo )
{
	if (m_pChannelSDK)
	{
		m_pChannelSDK->MON_CHNSDK_GetVideoInfo(m_mpxCamInfo.nChanID, pVideoInfo);
	}
}

void CMonChannel::SetVideoInfo( MPX_VIDEO_INFO*pVideoInfo )
{
	if (m_pChannelSDK)
	{
		m_pChannelSDK->MON_CHNSDK_SetVideoInfo(m_mpxCamInfo.nChanID, pVideoInfo);
	}
}

int CMonChannel::SetChanParam( const std::string& strName )
{
	int nRet = REQUEST_NO;
	int nModifyMask = MM_NONE;
	BOOL bModified = FALSE;

	do 
	{
		if (strName.compare(m_mpxCamInfo.szChanName)!=0)
		{
			nRet = SaveNameToChannel(strName);
			if (nRet!=REQUEST_NO)
			{
				break;
			}

			SAFE_STRCPY(m_mpxCamInfo.szChanName,strName.c_str());
			nModifyMask = MM_NAME;
			bModified = TRUE;
		}
	} while (0);

	if (bModified)
	{
		nRet = SaveToDatabase(nModifyMask);
	}

	return nRet;
}

int CMonChannel::SaveNameToChannel( const std::string& strName )
{
	if (m_pChannelSDK)
	{
		//UpdateLastOptTime();
		m_pChannelSDK->MON_CHNSDK_SetChannelName(m_mpxCamInfo.nChanID, strName.c_str() );
	}

	return REQUEST_NO;
}

BOOL CMonChannel::SearchFile(long hRx, unsigned long ulType, const char* szStartTime, const char* szEndTime)
{
#ifdef _VOD_ENABLE
	if (m_pChannelSDK)
	{
		return m_pChannelSDK->VOD_CHNSDK_SearchFile(hRx,ulType,szStartTime,szEndTime);
	}
#endif
	return FALSE;
}

BOOL CMonChannel::PlayByFile(long hRx, const char* szFileName)
{
#ifdef _VOD_ENABLE
	if (m_pChannelSDK)
	{
		return m_pChannelSDK->VOD_CHNSDK_PlayByFile(hRx,szFileName);
	}
#endif
	return FALSE;
}

BOOL CMonChannel::PlayByTime(long hRx, const char* szStartTime, const char* szEndTime)
{
#ifdef _VOD_ENABLE
	if (m_pChannelSDK)
	{
		return m_pChannelSDK->VOD_CHNSDK_PlayByTime(hRx,szStartTime,szEndTime);
	}
#endif
	return FALSE;
}

BOOL CMonChannel::PlayByAbsoluteTime(long hRx, const char* szAbsoluteTime)
{
#ifdef _VOD_ENABLE
	if (m_pChannelSDK)
	{
		return m_pChannelSDK->VOD_CHNSDK_PlayByAbsoluteTime(hRx,szAbsoluteTime);
	}
#endif
	return FALSE;
}

BOOL CMonChannel::PlayPause(long hRx, long nState)
{
#ifdef _VOD_ENABLE
	if (m_pChannelSDK)
	{
		return m_pChannelSDK->VOD_CHNSDK_PlayPause(hRx,nState);
	}
#endif
	return FALSE;
}

BOOL CMonChannel::PlaySpeedControl(long hRx, double dSpeed)
{
#ifdef _VOD_ENABLE
	if (m_pChannelSDK)
	{
		return m_pChannelSDK->VOD_CHNSDK_PlaySpeedControl(hRx,dSpeed);
	}
#endif
	return FALSE;
}

BOOL CMonChannel::PlayFastForward(long hRx)
{
#ifdef _VOD_ENABLE
	if (m_pChannelSDK)
	{
		return m_pChannelSDK->VOD_CHNSDK_PlayFastForward(hRx);
	}
#endif
	return FALSE;
}

BOOL CMonChannel::PlaySlowDown(long hRx)
{
#ifdef _VOD_ENABLE
	if (m_pChannelSDK)
	{
		return m_pChannelSDK->VOD_CHNSDK_PlaySlowDown(hRx);
	}
#endif
	return FALSE;
}

BOOL CMonChannel::PlayNormal(long hRx)
{
#ifdef _VOD_ENABLE
	if (m_pChannelSDK)
	{
		return m_pChannelSDK->VOD_CHNSDK_PlayNormal(hRx);
	}
#endif
	return FALSE;
}

BOOL CMonChannel::PlayStop(long hRx)
{
#ifdef _VOD_ENABLE
	if (m_pChannelSDK)
	{
		return m_pChannelSDK->VOD_CHNSDK_PlayStop(hRx);
	}
#endif
	return FALSE;
}

BOOL CMonChannel::DownloadFileByName(unsigned long hDownload, const char* szFileName)
{
#ifdef _VOD_ENABLE
	if (m_pChannelSDK)
	{
		return m_pChannelSDK->VOD_CHNSDK_DownloadFileByFile(hDownload,szFileName);
	}
#endif	
	return FALSE;
}

BOOL CMonChannel::DownloadFileByTime(unsigned long hDownload, const char* szStartTime, const char* szEndTime)
{
#ifdef _VOD_ENABLE
	if (m_pChannelSDK)
	{
		return m_pChannelSDK->VOD_CHNSDK_DownloadFileByTime(hDownload,szStartTime,szEndTime);
	}
#endif	
	return FALSE;
}

BOOL CMonChannel::StopDownload(unsigned long hDownload)
{
#ifdef _VOD_ENABLE
	if (m_pChannelSDK)
	{
		return m_pChannelSDK->VOD_CHNSDK_StopDownload(hDownload);
	}
#endif	
	return FALSE;
}