#ifndef CGBCHANNELSDK_H
#define CGBCHANNELSDK_H

#include<map>
#include "IDeviceSdk.h"
#include "GB28181Main.h"

typedef std::map<long,long> MAP_VODPLAY;
typedef std::map<long,long>::iterator IT_MAP_VODPLAY;

class CGBDeviceSdk;
class CGBChannelSdk:public HPMON_IChannelSdk,public GBChannelEvent
{
public:
	CGBChannelSdk(int nChannelNum,const std::string & strChannelID,CGBDeviceSdk * pCGBDeviceSdk);
	virtual ~CGBChannelSdk();

private:
	IChannelSdkCallback * m_pSdkCallBack;
	CGBDeviceSdk * m_pCGBDeviceSdk;
	int m_nPlayHandle;

	KCritSec	m_hMutex;
	MAP_VODPLAY	m_maphVod;

protected:
	void PushVODPlay(long hRx, long hPlay);
	long PopVODPlay(long hRx);
	long FindVODPlay(long hRx);

public:
//GBChannelEvent
	virtual void OnChannelPlaySucceed(int nPlayHandle);
	virtual void OnChannelPlayFailed(int nPlayHandle);
	virtual void OnChannelRecvStream(int nPlayHandle,int nStreamType,int nEncodeType,void * pData,int nLen,void * pUserData);
	virtual void OnChannelRecvRecordInfo(const int & nSN, const RecordInfo & tRecordInfo);

	void UpdateChannelIP(const char * szIPAddr,const int nPort);

//HPMON_IChannelSdk
public:
	virtual void MON_CHNSDK_Init();
	virtual void MON_CHNSDK_UnInit();
	virtual void MON_CHNSDK_SetMonChannel(IChannelSdkCallback *pChnCallback);
	virtual void MON_CHNSDK_GetChannelName(int nHpChnID);
	virtual void MON_CHNSDK_SetChannelName(int nHpChnID, const char* szChnName);
	virtual void MON_CHNSDK_GetChannelStatus();
	virtual int  MON_CHNSDK_StartPlay( int nHpChnID, int nReqPlayStatus );
	virtual void MON_CHNSDK_StopPlay(int nHpChnID, int nPlayStatus);
	virtual void MON_CHNSDK_PtzControl(int nHpChnID, int nPtzCmd,int nSpeed,void*pExtData,int nExtSize);
	virtual void MON_CHNSDK_GetVideoInfo(int nHpChnID, MPX_VIDEO_INFO*pVideoInfo);
	virtual void MON_CHNSDK_SetVideoInfo(int nHpChnID, MPX_VIDEO_INFO*pVideoInfo);
	virtual void MON_CHNSDK_StartTalk();
	virtual void MON_CHNSDK_StopTalk();
	virtual void MON_CHNSDK_SendTalkStream();

public:
	virtual BOOL VOD_CHNSDK_SearchFile(unsigned long hPlay, unsigned long ulType, const char* szStartTime, const char* szEndTime);
	virtual BOOL VOD_CHNSDK_PlayByFile(unsigned long hPlay, const char* szFileName);
	virtual BOOL VOD_CHNSDK_PlayByTime(unsigned long hPlay, const char* szStartTime, const char* szEndTime);
	virtual BOOL VOD_CHNSDK_PlayByAbsoluteTime(unsigned long hPlay, const char* szAbsoluteTime);
	virtual BOOL VOD_CHNSDK_PlayPause(unsigned long hPlay, long nState);
	virtual BOOL VOD_CHNSDK_PlaySpeedControl(unsigned long hPlay, double dSpeed);
	virtual BOOL VOD_CHNSDK_PlayFastForward(unsigned long hPlay);
	virtual BOOL VOD_CHNSDK_PlaySlowDown(unsigned long hPlay);
	virtual BOOL VOD_CHNSDK_PlayNormal(unsigned long hPlay);
	virtual BOOL VOD_CHNSDK_PlayStop(unsigned long hPlay);

	virtual BOOL VOD_CHNSDK_DownloadFileByFile(unsigned long hDownload, const char* szFileName);
	virtual BOOL VOD_CHNSDK_DownloadFileByTime(unsigned long hDownload, const char* szStartTime, const char* szEndTime);
	virtual BOOL VOD_CHNSDK_StopDownload(unsigned long hDownload);

private:
	GB28181Main * m_pGB28181Main;
	GBChannel * m_pChannel;
	int m_nChannel;
	int m_nCurrentStatus;

	//add by lxy
	unsigned long hPlayback;
};

typedef std::map<int,CGBChannelSdk*> ID_CHANNELSDK_MAP;

#endif