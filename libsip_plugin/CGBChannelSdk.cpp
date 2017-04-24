#include "stdafx.h"
#include "CGBChannelSdk.h"
#include "CGBDeviceSdk.h"
#include "PtzDefine.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//MonPlugin变一条流的思想:
//1.就是StartPlayXX进去都进去StartPlay,检测到正在播放就保存申请的播放流类型，然后再返回，
//2.StopPlay的时候就检测一下是否符合当前的播放类型，如果符合才让它去停止
CGBChannelSdk::CGBChannelSdk(int nChannelNum,const std::string & strChannelID,CGBDeviceSdk * pCGBDeviceSdk)
:m_pCGBDeviceSdk(pCGBDeviceSdk)
{
	m_pGB28181Main=GB28181Main::GetInstance();
	m_pCGBDeviceSdk=pCGBDeviceSdk;
	if(!m_pGB28181Main->GetChannel(pCGBDeviceSdk->GetGBDevice(),strChannelID,&m_pChannel))
	{
		printf("[GB28181]:Get the channel from the device failed!\n");
	}
	else
	{
		printf("[GB28181]:Get the channel from the device succeed!\n");
	}
	if (m_pChannel)
	{
		m_pChannel->SetChannelEvent(this);
	}
	m_nChannel=nChannelNum;
	m_nPlayHandle=-1;
	m_nCurrentStatus=AST_NONE;
}

CGBChannelSdk::~CGBChannelSdk()
{
	if (m_pGB28181Main && m_pChannel)
	{
		if (m_nPlayHandle!=-1)
		{
			m_pGB28181Main->ChannelStopPlay(m_pChannel,m_nPlayHandle);
		}
		m_pGB28181Main->FreeChannel(&m_pChannel);
	}
}

//-------------------------------------HPMON_IChannelSdk-----------------------------------//
void CGBChannelSdk::MON_CHNSDK_Init()
{

}

void CGBChannelSdk::MON_CHNSDK_UnInit()
{
	//清理时要先把所有的回调置0
	//MON_CHNSDK_SetMonChannel(NULL);
}

void CGBChannelSdk::PushVODPlay(long hRx, long hPlay)
{
	KAutoLock l(m_hMutex);
	m_maphVod[hRx] = hPlay;
	//printf(">>>>>>>>>Push m_maphVod[%ld]=%ld<<<<<<<<<\n", hRx, hPlay);
}

long CGBChannelSdk::PopVODPlay(long hRx)
{
	//printf("<<<<<<<<<<<<<<<<hRx:%ld\n", hRx);
	long hVod = INSTANCE_NULL;
	{
		KAutoLock l(m_hMutex);
		IT_MAP_VODPLAY it = m_maphVod.find(hRx);
		if (it!=m_maphVod.end())
		{
			hVod = it->second;
			m_maphVod.erase(it);
			//printf(">>>>>>>>>Pop m_maphVod[%ld]=%ld<<<<<<<<<\n", hRx, hVod);
		}
	}

	return hVod;
}

long CGBChannelSdk::FindVODPlay(long hRx)
{
	long hVod = INSTANCE_NULL;
	{
		KAutoLock l(m_hMutex);
		IT_MAP_VODPLAY it = m_maphVod.find(hRx);
		if (it!=m_maphVod.end())
		{
			hVod = it->second;
		}
	}

	return hVod;
}

void CGBChannelSdk::MON_CHNSDK_SetMonChannel(IChannelSdkCallback *pChnCallback)
{
	m_pSdkCallBack=pChnCallback;
}

void CGBChannelSdk::MON_CHNSDK_GetChannelName(int nHpChnID)
{
	if (m_pChannel && m_pSdkCallBack)
	{
		m_pSdkCallBack->OnChannelSdkCallback_RecMessageCb(EM_DEVSDK_GET_CHN_NAME,(void*)m_pChannel->m_strName.c_str(),m_pChannel->m_strName.length()+1);
		return ;
	}
}

void CGBChannelSdk::MON_CHNSDK_SetChannelName(int nHpChnID, const char* szChnName)
{
	


}

void CGBChannelSdk::MON_CHNSDK_GetChannelStatus()
{
	if (m_pChannel && m_pSdkCallBack)
	{
		int nStatus=m_pChannel->m_bStatus ? 1 : 0;
		m_pSdkCallBack->OnChannelSdkCallback_RecMessageCb( EM_DEVSDK_GET_CHN_STATUS, (void*)&nStatus, sizeof(int) );
		return ;
	}
}

int  CGBChannelSdk::MON_CHNSDK_StartPlay( int nHpChnID, int nReqPlayStatus )
{
	//printf("Start play status is :%d\n",nReqPlayStatus);
	if (m_nChannel!=nHpChnID)
	{
		return false;
	}

	if (m_nPlayHandle>0)
	{
		//视频正在播放,但都要记录当前转到哪条流
		m_nCurrentStatus=nReqPlayStatus;
		return true;
	}

	if (m_pChannel)
	{
		m_pGB28181Main->ChannelPlay(m_pChannel,m_nPlayHandle,NULL);
		if (m_nPlayHandle<0)
		{
			printf("[GB28181]: Channel send command failed!\n");
			return false;
		}
	}
	printf("[GB28181]:Start the channnel play and Handle is :%d\n",m_nPlayHandle);
	m_nCurrentStatus=nReqPlayStatus;
	return true;
}


void CGBChannelSdk::MON_CHNSDK_StopPlay(int nHpChnID, int nPlayStatus)
{
	if (m_nChannel!=nHpChnID)
	{
		return ;
	}

	if (!(m_nCurrentStatus & nPlayStatus))
	{
		//不是当前的流就不停止
		return ;
	}

	if (m_nPlayHandle<0)
	{
		//视频没有播放
		return ;
	}

	if (m_pChannel==NULL)
	{
		return ;
	}
	printf("[GB28181]:Stop handle is :%d\n",m_nPlayHandle);
	m_pGB28181Main->ChannelStopPlay(m_pChannel,m_nPlayHandle);//用cid来做一些异步回调？告诉停止失败？
	m_nPlayHandle=-1;
	
	return ;
}

void CGBChannelSdk::MON_CHNSDK_PtzControl(int nHpChnID, int nPtzCmd,int nSpeed,void*pExtData,int nExtSize)
{
	if (m_nChannel!=nHpChnID)
	{
		return ;
	}

	if (!m_pChannel || !m_pGB28181Main)
	{
		printf("[GB28181]: The channel is empty(MON_CHNSDK_PtzControl)!\n");
		return ;
	}

	int ptz_speed;
	//景阳的云台速度最大是64，并且放大缩小没有倍速可言
	int nCommonSpeed=255;
	int nZoomSpeed=15;
	switch(nPtzCmd)
	{
	case CMD_UP:
		{
			ptz_speed = (int)(nSpeed*nCommonSpeed/100.0);
			ptz_speed = (ptz_speed<=0) ? 1 : ptz_speed;
			m_pGB28181Main->ChannelPtzControl(m_pChannel,GB_PTZ_UP,ptz_speed);
		}
		break;
	case CMD_DOWN:
		{
			ptz_speed = (int)(nSpeed*nCommonSpeed/100.0);
			ptz_speed = (ptz_speed<=0) ? 1 : ptz_speed;
			m_pGB28181Main->ChannelPtzControl(m_pChannel,GB_PTZ_DOWN,ptz_speed);
		}
		break;
	case CMD_LEFT:
		{
			ptz_speed = (int)(nSpeed*nCommonSpeed/100.0);
			ptz_speed = (ptz_speed<=0) ? 1 : ptz_speed;
			m_pGB28181Main->ChannelPtzControl(m_pChannel,GB_PTZ_LEFT,ptz_speed);
		}
		break;
	case CMD_RIGHT:
		{
			ptz_speed = (int)(nSpeed*nCommonSpeed/100.0);
			ptz_speed = (ptz_speed<=0) ? 1 : ptz_speed;
			m_pGB28181Main->ChannelPtzControl(m_pChannel,GB_PTZ_RIGHT,ptz_speed);
		}
		break;
	case CMD_DN:
		{
			ptz_speed = (int)(nSpeed*nZoomSpeed/100.0);
			ptz_speed = (ptz_speed<=0) ? 1 : ptz_speed;
			m_pGB28181Main->ChannelPtzControl(m_pChannel,GB_PTZ_ZOOM_OUT,ptz_speed);
		}
		break;
	case CMD_DF:
		{
			ptz_speed = (int)(nSpeed*nZoomSpeed/100.0);
			ptz_speed = (ptz_speed<=0) ? 1 : ptz_speed;
			m_pGB28181Main->ChannelPtzControl(m_pChannel,GB_PTZ_ZOOM_IN,ptz_speed);
		}
		break;
	case CMD_STOP_UP:
	case CMD_STOP_DOWN:
	case CMD_STOP_LEFT:
	case CMD_STOP_RIGHT:
	case CMD_STOP_DN:
	case CMD_STOP_DF:
		{
			m_pGB28181Main->ChannelPtzControl(m_pChannel,GB_PTZ_STOP_COMMON,0);
		}
		break;
	case CMD_HB:
		{
			ptz_speed = (int)(nSpeed*nCommonSpeed/100.0);
			ptz_speed = (ptz_speed<=0) ? 1 : ptz_speed;
			m_pGB28181Main->ChannelPtzControl(m_pChannel,GB_PTZ_IRIS_ENLARGE,ptz_speed);
		}
		break;
	case CMD_HS:
		{
			ptz_speed = (int)(nSpeed*nCommonSpeed/100.0);
			ptz_speed = (ptz_speed<=0) ? 1 : ptz_speed;
			m_pGB28181Main->ChannelPtzControl(m_pChannel,GB_PTZ_IRIS_REDUCE,ptz_speed);
		}
		break;
	case CMD_FB:
		{
			ptz_speed = (int)(nSpeed*nCommonSpeed/100.0);
			ptz_speed = (ptz_speed<=0) ? 1 : ptz_speed;
			m_pGB28181Main->ChannelPtzControl(m_pChannel,GB_PTZ_FOCUS_NEAR,ptz_speed);
		}
		break;
	case CMD_FS:
		{
			ptz_speed = (int)(nSpeed*nCommonSpeed/100.0);
			ptz_speed = (ptz_speed<=0) ? 1 : ptz_speed;
			m_pGB28181Main->ChannelPtzControl(m_pChannel,GB_PTZ_FOCUS_FAR,ptz_speed);
		}
		break;
	case CMD_STOP_HB:
	case CMD_STOP_HS:
	case CMD_STOP_FB:
	case CMD_STOP_FS:
		m_pGB28181Main->ChannelPtzControl(m_pChannel,GB_PTZ_STOP_FI,0);
		break;
	default:
		{
			if (nPtzCmd>=CMD_PREST_CALL && nPtzCmd<=CMD_PREST_CALL+256)
			{
				printf("[TDGBPlatform]:Goto preset point!\n");
				int nPoint = nPtzCmd-CMD_PREST_CALL/*调用号从0开始*/;
				m_pGB28181Main->ChannelPtzControl(m_pChannel,GB_PTZ_PRESET,nPoint);
			}
		}
		break;
	}


}

void CGBChannelSdk::MON_CHNSDK_GetVideoInfo(int nHpChnID,MPX_VIDEO_INFO*pVideoInfo)
{

}

void CGBChannelSdk::MON_CHNSDK_SetVideoInfo(int nHpChnID, MPX_VIDEO_INFO*pVideoInfo)
{

}

void CGBChannelSdk::MON_CHNSDK_StartTalk()
{

}

void CGBChannelSdk::MON_CHNSDK_StopTalk()
{

}

void CGBChannelSdk::MON_CHNSDK_SendTalkStream()
{

}

void CGBChannelSdk::OnChannelPlaySucceed(int nPlayHandle)
{
	printf("[GBDev]: Start the play succeed!\n");
	if (m_nPlayHandle!=nPlayHandle)
	{
		printf("[GB28181]:The play succed handle is not correct!\n");
		m_nPlayHandle=nPlayHandle;
	}
}

void CGBChannelSdk::OnChannelPlayFailed(int nPlayHandle)
{
	printf("[GBDev]: Start the play failed!\n");
	if (m_nPlayHandle!=nPlayHandle)
	{
		printf("[GB28181]:The play failed handle is not correct!\n");
		return ;
	}
	m_nPlayHandle=-1;
}

void CGBChannelSdk::OnChannelRecvStream(int nPlayHandle,int nStreamType,int nEncodeType,void * pData,int nLen,void * pUserData)
{
	if (m_pSdkCallBack!=NULL)
	{
		if (nStreamType==STREAM_TYPE_PLAY_MAIN || 
			nStreamType==STREAM_TYPE_PLAY_SUB ||
			nStreamType==STREAM_TYPE_PLAY_SUBSUB)
		{
			unsigned char * pTemData=(unsigned char *)pData;
			m_pSdkCallBack->OnChannelSdkCallback_RealPlayStreamCallback(AST_VIDEO,(unsigned char *)&pTemData[12],nLen-12,(unsigned char*)&nEncodeType,sizeof(int));
		}
		else if (nStreamType==STREAM_TYPE_PLAYBACK)
		{
			//NET_DVR_TIME time = {0};
			//BOOL bt = NET_DVR_GetPlayBackOsdTime(lRealHandle,&time);
			//int err = NET_DVR_GetLastError();
#ifdef _VOD_ENABLE
			long hRx = (long)pUserData;
			AVCON_PLAY_FRAME play = {0};
			play.nFilePos =  0;
			play.uDataType = AST_VIDEO;

			unsigned char * pTemData=(unsigned char *)pData;
			//VODTX_InputStandardPacket((unsigned long)hRx,ASF_PS,(unsigned char*)pData,nLen,(unsigned char*)&play,sizeof(AVCON_PLAY_FRAME));
			VODTX_InputStandardPacket((unsigned long)hRx,ASF_PS,(unsigned char*)&pTemData[12],nLen-12);
			m_pSdkCallBack->OnChannelSdkCallback_VodStreamCallback();
#endif
		}
		else if (nStreamType==STREAM_TYPE_DOWNLOAD)
		{
#ifdef _VOD_ENABLE
			long hRx = (long)pUserData;
			AVCON_PLAY_FRAME play = {0};
			play.nFilePos =  0;
			play.uDataType = AST_VIDEO;

			struct tm *timeinfo;
			time_t rawtime;
			time(&rawtime);
			timeinfo = localtime(&rawtime);
			play.nYear = timeinfo->tm_year+1900;
			play.nMonth = timeinfo->tm_mon+1;
			play.nDay = timeinfo->tm_mday;
			play.nHour = timeinfo->tm_hour;
			play.nMinute = timeinfo->tm_min;
			play.nSecond = timeinfo->tm_sec;
			GBRTPHeader * pRTPHeader=(GBRTPHeader *)pData;
			int nFileTime=0;
			if (m_pChannel)
			{
				nFileTime=m_pChannel->GetStreamFileTime(nPlayHandle);
			}
			unsigned long nTM=ntohl(pRTPHeader->tm);

			play.nFilePos=(1.00000/(double)90000.0*nTM)/nFileTime;
			//printf("######[GBSDK]: The timestamp is :%d and %d and %d\n",nTM,nFileTime,play.nFilePos);

			unsigned char * pTemData=(unsigned char *)pData;
			VODTX_InputStandardPacket((unsigned long)hRx,ASF_PS,(unsigned char*)&pTemData[12],nLen-12,(unsigned char*)&play,sizeof(AVCON_PLAY_FRAME));
			m_pSdkCallBack->OnChannelSdkCallback_VodStreamCallback();
#endif
		}
	}
}

void CGBChannelSdk::OnChannelRecvRecordInfo(const int & nSN, const RecordInfo & tRecordInfo)
{
	//add by lxy
#ifdef _VOD_ENABLE
	LIST_VODFILE Filelist;
	char  start_time[20];
	char  end_time[20];
	unsigned int len = 0;

	std::list<RecordItem>::const_iterator p_lstRecordItem;
	for (p_lstRecordItem = tRecordInfo.lstRecordItem.begin(); p_lstRecordItem != tRecordInfo.lstRecordItem.end(); ++p_lstRecordItem)
	{
		VODFILE file;
		memset(start_time, 0, 20);
		memset(end_time, 0, 20);

		SAFE_STRCPY(start_time, p_lstRecordItem->strStartTime.c_str());
		SAFE_STRCPY(end_time, p_lstRecordItem->strEndTime.c_str());
		len = strlen(p_lstRecordItem->strStartTime.c_str());

		for (unsigned int i = 0; i < len; ++i)
		{
			if (start_time[i] == 'T' && end_time[i] == 'T')
			{
				start_time[i] = ' ';
				end_time[i] = ' ';
				break;
			}
		}

		SAFE_STRCPY(file.szBeginTime, start_time);
		SAFE_STRCPY(file.szEndTime, end_time);
		SAFE_STRCPY(file.szFileName, p_lstRecordItem->strName.c_str());
		file.usFileType = 0;
		file.ulFileSize = 1000000;

		Filelist.push_back(file);
	}
	VODTX_EventLoopback(hPlayback,VOD_EVE_SEARCHFILE,(void*)&Filelist,sizeof(Filelist));
	return ;
#endif
	return ;
}

void CGBChannelSdk::UpdateChannelIP(const char * szIPAddr,const int nPort)
{
	m_pChannel->m_strParentIP=szIPAddr;
	m_pChannel->m_nParentPort=nPort;
}

BOOL CGBChannelSdk::VOD_CHNSDK_SearchFile(unsigned long hPlay, unsigned long ulType, const char* szStartTime, const char* szEndTime)
{
#ifdef _VOD_ENABLE
	char  start_time[20];
	char  end_time[20];
	unsigned int len = 0;

	SAFE_STRCPY(start_time, szStartTime);
	SAFE_STRCPY(end_time, szEndTime);
	len = strlen(szStartTime);

	for (unsigned int i = 0; i < len; ++i)
	{
		if (start_time[i] == ' ' && end_time[i] == ' ')
		{
			start_time[i] = 'T';
			end_time[i] = 'T';
			break;
		}
	}

	if (m_pGB28181Main)
	{
		int nSession=-1;
		m_pGB28181Main->ChannelQueryRecordFile(m_pChannel,start_time,end_time,"all","","","",nSession);
	}
	//printf("--------------------->[hPlayback:%lu]<-----------------\n", hPlay);
	hPlayback = hPlay;
	return TRUE;
#endif
	return FALSE;
}

BOOL CGBChannelSdk::VOD_CHNSDK_PlayByFile(unsigned long hPlay, const char* szFileName)
{
#ifdef _VOD_ENABLE
	return TRUE;
#endif
	return FALSE;
}

BOOL CGBChannelSdk::VOD_CHNSDK_PlayByTime(unsigned long hPlay, const char* szStartTime, const char* szEndTime)
{
#ifdef _VOD_ENABLE
	//printf(">>>>>>>>>>>>>>>>>>[VOD_CHNSDK_PlayByTime:%lu]<<<<<<<<<<<<<<<<<<<<\n", hPlay);
	if (m_pChannel==NULL)
	{
		printf("[GB28181]:The channel is empty![CGBChannelSdk::VOD_CHNSDK_PlayByTime]\n");
		return FALSE;
	}

	long hVodPlay = INSTANCE_NULL;
	/*
	if (hVodPlay!=INSTANCE_NULL)
	{
		if (m_pGB28181Main)
		{
			m_pGB28181Main->ChannelStopPlay(m_pChannel,hVodPlay);//异步的，会不会有问题？
		}
		//Sleep?
	}
	*/
	if (m_pGB28181Main)
	{
			m_pGB28181Main->ChannelPlayback(m_pChannel,szStartTime,szEndTime,hVodPlay,(void*)hPlay);
	}
	//printf(">>>>>>>>>>>>befor push hPlay:%lu,hVodPlay:%ld\n", hPlay, hVodPlay);
	PushVODPlay(hPlay,hVodPlay);
	return TRUE;
#endif
	return FALSE;
}

BOOL CGBChannelSdk::VOD_CHNSDK_PlayByAbsoluteTime(unsigned long hPlay, const char* szAbsoluteTime)
{
#ifdef _VOD_ENABLE
	long hVodPlay=PopVODPlay(hPlay);
	if (hVodPlay!=INSTANCE_NULL)
	{
		VODTX_EventLoopback(hPlay,VOD_EVE_REPLAY,(void*)hVodPlay,sizeof(long));
		return TRUE;
	}
	return FALSE;
#endif
	return FALSE;
}

BOOL CGBChannelSdk::VOD_CHNSDK_PlayPause(unsigned long hPlay, long nState)
{
#ifdef _VOD_ENABLE
	long hVodPlay=FindVODPlay(hPlay);
	if (hVodPlay!=INSTANCE_NULL)
	{
		if (m_pGB28181Main)
		{
			m_pGB28181Main->ChannelPlaybackPause(m_pChannel,hVodPlay,nState==1 ? true : false);
			return TRUE;
		}
	}
	return FALSE;
#endif
	return FALSE;
}

BOOL CGBChannelSdk::VOD_CHNSDK_PlaySpeedControl(unsigned long hPlay, double dSpeed)
{
#ifdef _VOD_ENABLE
/*
	int nPos = (int)dSpeed;
	if (dSpeed<0)
		nPos = 0;
	else if (dSpeed>=100)
		nPos = 99;
	long hVodPlay=PopVODPlay(hPlay);
	if (hVodPlay!= INSTANCE_NULL)
	{
		if (m_pGB28181Main)
		{
			m_pGB28181Main->ChannelPlaybackSpeed(m_pChannel,hVodPlay,dSpeed);
		}
		VODTX_EventLoopback(hPlay,VOD_EVE_REPLAY,(void*)hVodPlay,sizeof(long));
		return TRUE;
	}
	return FALSE;
*/
	return TRUE;
#endif
	return FALSE;
}

BOOL CGBChannelSdk::VOD_CHNSDK_PlayFastForward(unsigned long hPlay)
{
#ifdef _VOD_ENABLE
/*
	long hVodPlay=PopVODPlay(hPlay);
	if (hVodPlay!=INSTANCE_NULL)
	{
		if (m_pGB28181Main)
		{
			m_pGB28181Main->ChannelPlaybackFast(m_pChannel,hVodPlay);
		}
		return TRUE;
	}
	return FALSE;
*/
	return TRUE;
#endif
	return FALSE;
}

BOOL CGBChannelSdk::VOD_CHNSDK_PlaySlowDown(unsigned long hPlay)
{
#ifdef _VOD_ENABLE
/*
	long hVodPlay=PopVODPlay(hPlay);
	if (hVodPlay!=INSTANCE_NULL)
	{
		if (m_pGB28181Main)
		{
			m_pGB28181Main->ChannelPlaybackSlow(m_pChannel,hVodPlay);
		}
	}
	return FALSE;
*/
	return TRUE;
#endif
	return FALSE;
}

BOOL CGBChannelSdk::VOD_CHNSDK_PlayNormal(unsigned long hPlay)
{
#ifdef _VOD_ENABLE
/*
	long hVodPlay=PopVODPlay(hPlay);
	if (hVodPlay!=INSTANCE_NULL)
	{
		if (m_pGB28181Main)
		{
			m_pGB28181Main->ChannelPlaybackSlow(m_pChannel,hVodPlay);
		}
		return TRUE;
	}
	return FALSE;
*/
	return TRUE;
#endif
	return FALSE;
}

BOOL CGBChannelSdk::VOD_CHNSDK_PlayStop(unsigned long hPlay)
{
#ifdef _VOD_ENABLE
	//printf(">>>>>>>>>>>>>>>>[VOD_CHNSDK_PlayStop:%lu]<<<<<<<<<<<<<<<<\n", hPlay);
	XSleep(1500);
	if (m_pChannel==NULL)
	{
		printf("[GB28181]:The channel is empty![CGBChannelSdk::VOD_CHNSDK_PlayStop]\n");
		return FALSE;
	}
	long hVodPlay=PopVODPlay(hPlay);
	//printf("hVodPlay:%ld\n", hVodPlay);
	if (hVodPlay!=INSTANCE_NULL)
	{
		if (m_pGB28181Main)
		{
			m_pGB28181Main->ChannelStopPlay(m_pChannel,hVodPlay);
		}
		hPlayback = 0;
		return TRUE;
	}
	return FALSE;
#endif
	return FALSE;
}

BOOL CGBChannelSdk::VOD_CHNSDK_DownloadFileByFile(unsigned long hDownload, const char* szFileName)
{
#ifdef _VOD_ENABLE

#endif
	return TRUE;
}

BOOL CGBChannelSdk::VOD_CHNSDK_DownloadFileByTime(unsigned long hDownload, const char* szStartTime, const char* szEndTime)
{
#ifdef _VOD_ENABLE
	if (m_pChannel==NULL)
	{
		printf("[GB28181]:The channel is empty![CGBChannelSdk::VOD_CHNSDK_PlayByTime]\n");
		return FALSE;
	}

	long hVodPlay = INSTANCE_NULL;
	if (m_pGB28181Main)
	{
		printf("[GB28181]: Download by time [%s-%s|%d]!\n",szStartTime,szEndTime,hDownload);
		m_pGB28181Main->ChannelDownload(m_pChannel,szStartTime,szEndTime,hVodPlay,(void*)hDownload);
	}

	if (hVodPlay==-1)
	{
		printf("[GB28181]: Send download command failed![CGBChannelSdk::VOD_CHNSDK_DownloadFileByTime]\n");
		return FALSE;
	}
	PushVODPlay(hDownload,hVodPlay);
#endif
	return TRUE;
}

BOOL CGBChannelSdk::VOD_CHNSDK_StopDownload(unsigned long hDownload)
{
#ifdef _VOD_ENABLE
	if (m_pChannel==NULL)
	{
		printf("[GB28181]:The channel is empty![CGBChannelSdk::VOD_CHNSDK_StopDownload]\n");
		return FALSE;
	}
	long hVodPlay=PopVODPlay(hDownload);
	if (hVodPlay!=INSTANCE_NULL)
	{
		if (m_pGB28181Main)
		{
			m_pGB28181Main->ChannelStopPlay(m_pChannel,hVodPlay);
		}
		hPlayback = 0;
		return TRUE;
	}
	else
	{
		printf("[GB28181]: Can't find the download handle![CGBChannelSdk::VOD_CHNSDK_StopDownload]\n");
	}
	return FALSE;
#endif
	return TRUE;
}