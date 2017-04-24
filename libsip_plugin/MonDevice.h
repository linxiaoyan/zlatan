#pragma once
#ifndef _MONDEVICE_H_
#define _MONDEVICE_H_

#include "IDeviceSdk.h"
#include "MonChannel.h"
#include "DevTalk.h"
#include "knet/UDPSocket.h"

class IMonDeviceCallback
{
public:
	virtual ~IMonDeviceCallback(){};

	virtual int OnMonDeviceCallback_ToConnectQueue(std::string strDevID, int nUrgency/*=FALSE*/)=0;
	virtual int OnMonDeviceCallback_DeviceUpdate(const MPX_DEVICE_INFO*pDevInfo,int nModifyMask) = 0;
	virtual void OnMonDeviceCallback_DeviceOnline(const MPX_DEVICE_INFO*pDevInfo) = 0;
	virtual void OnMonDeviceCallback_DeviceOffline(const MPX_DEVICE_INFO*pDevInfo) = 0;

 	virtual int OnMonDeviceCallback_ChannelUpdate(const MPX_CAMERA_INFO*pCamInfo,int nModifyMask) = 0;
 	virtual void OnMonDeviceCallback_ChannelOnline(const MPX_CAMERA_INFO*pCamInfo) = 0;
 	virtual void OnMonDeviceCallback_ChannelOffline(const MPX_CAMERA_INFO*pCamInfo) = 0;
	virtual void OnMonDeviceCallback_GetVideoInfo(const MPX_VIDEO_INFO*pVideoInfo) = 0;
	virtual int  OnMonDeviceCallback_ChannelDel(const MPX_CAMERA_INFO*pCamInfo) = 0;

	virtual bool  OnMonDeviceCallback_GetDBCameraInfo(const std::string& strDevID,MAP_CAMERA_INFO & CameraInfo)=0;
// 
// 	virtual void OnMonDeviceCallback_SetDevName(const char*szDevID,int nErrCode) = 0;
// 
 	virtual void OnMonDeviceCallback_RespondTalk(const MPX_TALK_INFO*pTalkData) = 0;
	virtual void OnMonDeviceCallback_DevChannelChange(MPX_TALK_DEVCHANNLECHANGE* pDevChannelChange)=0;
	//用于设置ping模块的设备实际状态
	virtual void OnMonDeviceCallback_SetDeviceStatus(const std::string strDevID, const std::string strDevIP, bool bDevStatus ) = 0;

	virtual std::string  OnMonDeviceCallback_GetAlarmServerIP() = 0;
	virtual unsigned int OnMonDeviceCallback_GetAlarmServerPort() = 0;
};


class CMonDevice 
	: public IDeviceSdkCallback
	, public IMonChannelCallback
	, public IDevTalkCallback
{
public:
	CMonDevice(IMonDeviceCallback *pCallback);
	~CMonDevice(void);

// IDeviceSdkCallback
public:
	virtual void OnDevSdkCallback_ConnectResult( int nMessage, int nErrCode, std::string strDevID, void *pbuffer, int nBuflen ) ;
	virtual void OnDevSdkCallback_DisConnect( );
	virtual void OnDevSdkCallback_RecMessageCb(int nMessage, void *pBuf,int nBuflen);
	virtual void OnDevSdkCallback_Exception(int nErrorCode, void *pBuf,int nBuflen );
	virtual void OnDevSdkCallback_RecChannelInfo(int nHPChnID,int nChnStatus,  HPMON_IChannelSdk* pChnSDK);
	virtual void OnDevSdkCallback_DevTalkResult(int nErrCode, std::string strDevID, void *pBuffer, int nBufLen) ;
	virtual void OnDevSdkCallback_DevTalkVoiceStream(unsigned int dwMediaType, unsigned char *pBuffer, unsigned int dwBufSize, unsigned char*pExtraData,unsigned int nExtraSize);
	virtual void OnDevSdkCallback_DevAlarmNotify(int nAlarmID,int nAlarmType,int nAlarmSubType,std::string strAlarmTime,void * pExtraData,unsigned int nExtraSize);

//IMonChannelCallback
public:
	virtual void OnMonChannelCallback_ChannelUpdate(const MPX_CAMERA_INFO*pCamInfo,int nModifyMask) ;
	virtual void OnMonChannelCallback_ChannelOnline(const MPX_CAMERA_INFO*pCamInfo) ;
	virtual void OnMonChannelCallback_ChannelOffline(const MPX_CAMERA_INFO*pCamInfo) ;
	virtual void OnMonChannelCallback_UpdateLastOptTime();
	virtual void OnMonChannelCallback_GetVideoInfo(const MPX_VIDEO_INFO*pVideoInfo);
	virtual void OnMonChannelCallback_ChannelDel( const MPX_CAMERA_INFO*pCamInfo);
	virtual void OnMonChannelCallback_UpdateSdkAliveTime();
//IDevTalkCallback
public:
	virtual bool OnDevTalkCallback_StartTalkSDK(int nChnID);
	virtual void OnDevTalkCallback_StopTalkSDK(int nChnID);
	virtual void OnDevTalkCallback_RespondTalk(const MPX_TALK_INFO*pTalkData) ;
	virtual void OnDevTalkCallback_TalkTimerout(void) ;
	virtual void OnDevTalkCallback_DevChannelChange(unsigned long ulAudioID, bool bStartTalk=true);
	virtual void OnDevTalkCallback_SendVoiceToDevice(int nStreamType, unsigned char*pData,int nLen );


protected:
	IMonDeviceCallback *m_pDevCallback;
	HPMON_IDeviceSdk   *m_pDeviceSdk;
	bool				m_bWantToDestroy;

	MPX_DEVICE_INFO m_mpxDevInfo;
	long			m_hLogin;

	unsigned long	m_ulLastOptTime;		//记录最后操作的时刻值
	bool			m_bIsNeedDisconnect;	//标识是否要与设备断链，true: 需要断链, 反之为否
	bool			m_bTalkTimerout;		//对讲超时处理

	int 			m_nRealDevStatus;	//真实当前设备状态
	unsigned int	m_nMtsOnlineTime;

	std::list<CMonChannel*> m_lstMonChannel;
	std::map<int,std::string> m_mapIDDBName;
	MAP_MONCHANNEL	m_mapMonChannel;
	KCritSec		m_csMonChannel;
	void PushMonChannel(int nChnID, CMonChannel *pChn);
	CMonChannel *FindMonChn(int nChnID);
	void ClearMonChns(void);

public:
	int Create(long hLogin, const MPX_DEVICE_INFO *pDevInfo );
	void Destroy(void);
	void Connect(void);
	void DisConnect(void);
	bool IsConnect(void);

	void SetloginSessionID(long hLogin);
	void SetChnNum(int nChnNum);
	void SetSeriaNo(string strSerialNo);
	MPX_DEVICE_INFO *GetDeviceInfo();
	
	bool IsNeedDisConnect();
	void UpdateLastOptTime();	//更新最近一次操作时间
	void DevStatusChange(bool bStatus);

	void AllProcessChannelEvent(bool bOnlined);
public:
	BOOL SearchFile(long hRx, int nChanID, unsigned long ulType, const char* szStartTime, const char* szEndTime);
	BOOL PlayByFile(long hRx, int nChanID, int nWndID, const char* szFileName);
	BOOL PlayByTime(long hRx, int nChanID, int nWndID, const char* szStartTime, const char* szEndTime);
	BOOL PlayByAbsoluteTime(long hRx, int nChanID, int nWndID, const char* szAbsoluteTime);
	BOOL PlayPause(long hRx, int nChanID, int nWndID, long nState);
	BOOL PlaySpeedControl(long hRx, int nChanID, int nWndID, double dSpeed);
	BOOL PlayFastForward(long hRx, int nChanID, int nWndID);
	BOOL PlaySlowDown(long hRx, int nChanID, int nWndID);
	BOOL PlayNormal(long hRx, int nChanID, int nWndID);
	BOOL PlayStop(long hRx, int nChanID, int nWndID);

	BOOL DownloadFileByName(unsigned long hDownload, int nChanID, unsigned int nWndID, const char* szFileName);
	BOOL DownloadFileByTime(unsigned long hDownload, int nChanID, unsigned int nWndID, const char* szStartTime, const char* szEndTime);
	BOOL StopDownload(unsigned long hDownload, int nChanID, unsigned int nWndID);
public:
	void ProcessMtsEvents(bool bLoginSuccessed);
	void ProcessDvrEvents(void);
	void OnDeviceOnline(int nErrCode);
	void OnDeviceOffline(int nErrCode);
	void OnChannelOnline(int nChanID,int nErrCode);
	void OnChannelOffline(int nChanID,int nErrCode);
 	BOOL GetAudioInfo(int nChanID,MPX_AUDIO_INFO*pAudioInfo);
 	BOOL SetAudioInfo(int nChanID,MPX_AUDIO_INFO*pAudioInfo);
 	BOOL GetVideoInfo(int nChanID,MPX_VIDEO_INFO*pVideoInfo);
 	BOOL SetVideoInfo(int nChanID,MPX_VIDEO_INFO*pVideoInfo);
// 
// 	BOOL StartListenAlarm(void);
// 	void StopListenAlarm(void);

	BOOL StartTalk(MPX_TALK_INFO*pTalkData);
	void StopTalk(const char *szNodeID);
	void OnChangedAudio(unsigned long nAudioID);
	BOOL PtzControl(int nChanID,int nPtzCmd,int nSpeed,void*pExtData,int nExtSize);


	int SaveToDatabase(int nModifyMask=MM_NONE);
	int SetMtsParam(const std::string strMtsUser, const std::string strMtsPass);
	int SetDevParam(const std::string strDevName, const std::string strDevUser, 
		const std::string strDevPwd, int& nWantToReconnect );
	int SetChanParam(const int nChanID, const string &strChanName);

protected:
	void CreateMonChannel(int nChnID, int nChnStatus, HPMON_IChannelSdk *pChannelSdk);
	int SaveNameToDevice(const std::string&strName);

protected:
	CDevTalk *m_pTalk;
	int		  m_nTalkCodecID;


};


typedef std::map<std::string, CMonDevice*>  MAP_MONDEVICES;
typedef MAP_MONDEVICES::iterator IT_MAP_MONDEVICES;

#endif
