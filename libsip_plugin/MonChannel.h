#pragma once
#ifndef _MONCHANNEL_H_
#define _MONCHANNEL_H_

#include "IDeviceSdk.h"
#include "libStreamTranscode.h"
#include "libStreamTransfer.h"

class IMonChannelCallback
{
public:
	virtual ~IMonChannelCallback(void){};
	virtual void OnMonChannelCallback_ChannelUpdate(const MPX_CAMERA_INFO*pCamInfo,int nModifyMask) = 0;
	virtual void OnMonChannelCallback_ChannelOnline(const MPX_CAMERA_INFO*pCamInfo) = 0;
	virtual void OnMonChannelCallback_ChannelOffline(const MPX_CAMERA_INFO*pCamInfo) = 0;
	virtual void OnMonChannelCallback_GetVideoInfo(const MPX_VIDEO_INFO*pVideoInfo) = 0;

	virtual void OnMonChannelCallback_UpdateLastOptTime()=0;
	virtual void OnMonChannelCallback_ChannelDel( const MPX_CAMERA_INFO*pCamInfo) = 0;

	virtual void OnMonChannelCallback_UpdateSdkAliveTime()=0;

// 	virtual int OnMonChannelCallback_StartPlay(string strDevID, int nChnID, long nReqStatus, KdmChannel *pKdmChannel) =0;
// 	virtual int OnMonChannelCallback_StopPlay(string strDevID, int nChnID) =0;
// 
// 	virtual void OnMonChannelCallback_StartTalk(string strDevID,const int nChnID, KdmChannel *pKdmChannel ) =0;
// 	virtual void OnMonChannelCallback_StopTalk(string strDevID, const int nChnID )=0;
// 	virtual void OnMonChannelCallback_RespondTalk(const MPX_TALK_INFO*pTalkData)=0;

};

class CMonChannel 
	: public IChannelSdkCallback
{
public:
	CMonChannel(IMonChannelCallback *pCallback, HPMON_IChannelSdk *pChannelSDK);
	~CMonChannel(void);

protected:
	IMonChannelCallback *m_pChnCallback;
	HPMON_IChannelSdk	*m_pChannelSDK;
	MPX_CAMERA_INFO		m_mpxCamInfo;

	int					m_nChnStatus;
	unsigned long		m_nMtsLastOnlineTime;

public:
	int Create(int nChnID,  const MPX_DEVICE_INFO* pDevInfo);
	void Destory();
	void SetChannelSDK(HPMON_IChannelSdk *pChannelSDK);
	void SetChannelStatus(int nChnStatus);
	void SetMemChannelName(const std::string & strName);

	void GetAudioInfo(MPX_AUDIO_INFO*pAudioInfo);
	void SetAudioInfo(MPX_AUDIO_INFO*pAudioInfo);
	void GetVideoInfo(MPX_VIDEO_INFO*pVideoInfo);
	void SetVideoInfo(MPX_VIDEO_INFO*pVideoInfo);

public:
	void ProcessMtsEvents(bool bOnlined);
	void ProcessDvrEvents(void);
	int SetChanParam(const std::string& strName);
	int SaveToDatabase(int nModifyMask=MM_NONE);
	int OnMtsParamChanged(const std::string& strMtsAccount);
	void OnChannelOnline(int nErrCode);
	void OnChannelOffline(int nErrCode);
	BOOL PtzControl(int nPtzCmd,int nSpeed,void*pExtData,int nExtSize);

	int GetChanID(void);
	unsigned long GetVideoID(void);
	unsigned long GetAudioID(void);

public:
	BOOL SearchFile(long hRx, unsigned long ulType, const char* szStartTime, const char* szEndTime);
	BOOL PlayByFile(long hRx, const char* szFileName);
	BOOL PlayByTime(long hRx, const char* szStartTime, const char* szEndTime);
	BOOL PlayByAbsoluteTime(long hRx, const char* szAbsoluteTime);
	BOOL PlayPause(long hRx, long nState);
	BOOL PlaySpeedControl(long hRx, double dSpeed);
	BOOL PlayFastForward(long hRx);
	BOOL PlaySlowDown(long hRx);
	BOOL PlayNormal(long hRx);
	BOOL PlayStop(long hRx);

	BOOL DownloadFileByName(unsigned long hDownload, const char* szFileName);
	BOOL DownloadFileByTime(unsigned long hDownload, const char* szStartTime, const char* szEndTime);
	BOOL StopDownload(unsigned long hDownload);

protected:
	bool m_bWantToRePlay;
	bool m_bMakeKeyframe;
	bool m_bMakeSubKeyframe;

	bool m_bWantToClose;
	bool m_bDvrEventBusy;

protected:
	void DoPlay(void);
	void RePlay(void);

	int StartPlay(bool bPlayAudio);
	void StopPlay(void);
	bool IsPlay(void);

	int StartPlaySub(void);
	void StopPlaySub(void);
	bool IsPlaySub(void);

	void MakeKeyframe(void);
	void MakeSubKeyframe(void);
	
	int SaveNameToChannel(const std::string& strName);

//IChannelSdkCallback
public:
	virtual void OnChannelSdkCallback_RecMessageCb(int nMessage, void *pBuf,int nBuflen);
	virtual void OnChannelSdkCallback_Exception(int nErrorCode, void *pBuf,int nBuflen );
	virtual void OnChannelSdkCallback_RealPlayStreamCallback(unsigned int dwMediaType, unsigned char *pBuffer, unsigned int dwBufSize, unsigned char*pExtraData,unsigned int nExtraSize);
	virtual void OnChannelSdkCallback_VodStreamCallback();

public://Transcode
	void OnTranscodePacket(long hHandle,const char*szKey,int nTranscodeType,int nStreamType,int nPacketType,
		unsigned char*pData,int nLen,unsigned char*pExtraData,int nExtraSize);
protected:
	long m_hVideoTranscode;
	long m_hSubTranscode;
	long m_hAudioTranscode;

public://Transfer
	void OnTransferMessage(long hHandle,const char*szKey,int nMessage,long wParam,long lParam);
	void OnTransferStream(long hHandle,const char*szKey,int nDevType,int nPacketType,unsigned char*pData,int nLen);
protected:
	long m_hTransfer;

};


typedef std::map<int, CMonChannel*> MAP_MONCHANNEL;
typedef MAP_MONCHANNEL::iterator IT_MAP_MONCHANNEL;

#endif
