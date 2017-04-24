#pragma once
#ifndef _MONPLUGIN_H_
#define _MONPLUGIN_H_

#include "stdafx.h"
#include "CMonPlugin.h"
#include "IDeviceSdk.h"
#include "DevConnect.h"
#include "NETEC/XThreadBase.h"
#include "MonDevice.h"
#include "RecycleThread.h"
#include "AlarmSingleton.h"

#ifdef _PING_SNMP
#include "StateManager.h"
#include "DeviceStateTable.h"
#endif

typedef std::map<string,string> MAP_MTSUSER_DEVID;
typedef MAP_MTSUSER_DEVID::iterator IT_MAP_MTSUSER_DEVID;

class CMonPlugin 
	: public IMonPlugin
	, public XThreadBase
	, public DevConnectCallback
	, public ISDKCallback
	, public IMonDeviceCallback
#ifdef _VOD_ENABLE
	, public IEventTx
#endif
#ifdef _PING_SNMP
	, public DeviceStateEvent
#endif
{
public:
	CMonPlugin(void);
	~CMonPlugin(void);

protected:
	static AlarmSingleton &m_AlarmSingleton;
	
	//HPMON_IDeviceSdk	*m_pDevSDk;
	CDevConnect			m_Connect;
	HPMON_ISDK			*m_pSDKInit;

	bool				m_bWantToStop;
	bool				m_bLoginMtsSuccessed;
	IPluginDB			*m_pPDB;

	AVCON_MPX_MESSAGE_CB m_pMsgCb;
	AVCON_MPX_ALARM_CB   m_pAlarmCb;
	AVCON_MPX_RECORD_CB  m_pRecCb;

	void *m_pMsgUser;
	void *m_pAlarmUser;
	void *m_pRecUser;
#ifdef _PING_SNMP
	DeviceStateManager *m_pDeviceStateManager;
#endif
//XThreadBase
public:
	virtual void ThreadProcMain(void);

// IMonPlugin
public:
	virtual BOOL Startup(const char* pConfigFile,
		AVCON_MPX_MESSAGE_CB pMessageCb,void*pMsgUser,
		AVCON_MPX_ALARM_CB pAlarmCb,void*pAlarmUser,
		AVCON_MPX_RECORD_CB pRecordCb,void*pRecUser);
	virtual void Cleanup(void) ;
	virtual long GetLastError(void) ;


	virtual void ServerStatusChanged(int nStatus) ;

	virtual BOOL PtzControl(const char*szDevID,int nChanID,int nPtzCmd,int nSpeed,void*pExtData,int nExtSize) ;

	virtual BOOL StartRecord(const char*szDevID,int nChanID) ;
	virtual void StopRecord(const char*szDevID,int nChanID) ;

	virtual BOOL StartTalk(const char*szDevID,MPX_TALK_INFO*pTalkData) ;
	virtual void StopTalk(const char*szDevID,const char *szNodeID) ;
	virtual void TalkChanged(const char*szDevID,unsigned long nAudioID);

	virtual BOOL StartListenAlarm(const char*szDevID) ;
	virtual void StopListenAlarm(const char*szDevID) ;

	virtual void OnDeviceOnline(const char*szDevID,int nErrCode) ;
	virtual void OnDeviceOffline(const char*szDevID,int nErrCode) ;
	virtual void OnChannelOnline(const char*szDevID,int nChanID,int nErrCode) ;
	virtual void OnChannelOffline(const char*szDevID,int nChanID,int nErrCode) ;

	virtual BOOL GetAudioInfo(const char*szDevID,int nChanID,MPX_AUDIO_INFO*pAudioInfo);
	virtual BOOL SetAudioInfo(const char*szDevID,int nChanID,MPX_AUDIO_INFO*pAudioInfo);
	virtual BOOL GetVideoInfo(const char*szDevID,int nChanID,MPX_VIDEO_INFO*pVideoInfo);
	virtual BOOL SetVideoInfo(const char*szDevID,int nChanID,MPX_VIDEO_INFO*pVideoInfo);

public:
	virtual void OnToolsOperator(int nMessage, AVCON_MPX_TOOLMESSAGE_CB pToolMessageCb, void*pExtData,int nExtSize, void* pSession);
	virtual void OnGetMonDeviceLoginInfo(const char * szDevID,const char * szPassword,int nDevType);
	void SetDvrInfo( int nMessage, AVCON_MPX_TOOLMESSAGE_CB pToolMessageCb, void*pExtData,int nExtSize, void* pSession );
	void GetDvrInfo( int nMessage, AVCON_MPX_TOOLMESSAGE_CB pToolMessageCb, void*pExtData,int nExtSize, void* pSession );	
	void GetChnInfo( int nMessage, AVCON_MPX_TOOLMESSAGE_CB pToolMessageCb, void*pExtData,int nExtSize, void* pSession );
	void GetLogInfo( int nMessage, AVCON_MPX_TOOLMESSAGE_CB pToolMessageCb, void*pExtData,int nExtSize, void* pSession );
	void DelDvrInfo( int nMessage, AVCON_MPX_TOOLMESSAGE_CB pToolMessageCb, void*pExtData,int nExtSize, void* pSession );
	void ModifyChannelInfo( int nMessage, AVCON_MPX_TOOLMESSAGE_CB pToolMessageCb, void*pExtData,int nExtSize, void* pSession );

//DevConnectCallback
public:
	virtual void OnDevConnectCallback_ConnectQueueEmpty(void) ;
	virtual void OnDevConnectCallback_Connect(char*pData, int nLen) ;

//IDeviceSdkCallback
public:
	//virtual void OnDevSdkCallback_ConnectResult( int nMessage, int nErrCode, std::string strDevID,  void *pbuffer, int nbufLen );

//IMonDeviceCallback
public:
	virtual int OnMonDeviceCallback_ToConnectQueue(std::string strDevID,int nUrgency/*=FALSE*/);
	virtual int OnMonDeviceCallback_DeviceUpdate(const MPX_DEVICE_INFO*pDevInfo,int nModifyMask);
	virtual void OnMonDeviceCallback_DeviceOnline(const MPX_DEVICE_INFO*pDevInfo) ;
	virtual void OnMonDeviceCallback_DeviceOffline(const MPX_DEVICE_INFO*pDevInfo) ;

	virtual int OnMonDeviceCallback_ChannelUpdate(const MPX_CAMERA_INFO*pCamInfo,int nModifyMask) ;
	virtual void OnMonDeviceCallback_ChannelOnline(const MPX_CAMERA_INFO*pCamInfo) ;
	virtual void OnMonDeviceCallback_ChannelOffline(const MPX_CAMERA_INFO*pCamInfo) ;
	virtual void OnMonDeviceCallback_RespondTalk(const MPX_TALK_INFO*pTalkData) ;
	virtual void OnMonDeviceCallback_DevChannelChange(MPX_TALK_DEVCHANNLECHANGE* pDevChannelChange);
	virtual void OnMonDeviceCallback_SetDeviceStatus(const std::string strDevID,const std::string strDevIP, bool bDevStatus );

	virtual void OnMonDeviceCallback_GetVideoInfo(const MPX_VIDEO_INFO*pVideoInfo);
	virtual int  OnMonDeviceCallback_ChannelDel(const MPX_CAMERA_INFO*pCamInfo);
	virtual bool OnMonDeviceCallback_GetDBCameraInfo(const std::string& strDevID,MAP_CAMERA_INFO & CameraInfo);

	virtual std::string  OnMonDeviceCallback_GetAlarmServerIP();
	virtual unsigned int OnMonDeviceCallback_GetAlarmServerPort();
//DeviceStateEvent
#ifdef _PING_SNMP
public:
	virtual void OnStateChangedEvent(std::string strID,bool bState);
#endif

protected:
	bool DevicesToConnectQueue(const std::string strDevID,int nUrgency/*=FALSE*/);
	bool CreateAllDevs(void);
	bool CreateDev( MPX_DEVICE_INFO *pDevInfo );
	void DelDev(char *szDevID);


protected:
	//
	RecycleThread * m_pRecycleThread;
	MAP_MONDEVICES	m_mapDevices;
	KCritSec		m_csDevices;
	void PushDevice(const std::string strDevID, CMonDevice *pDev);
	CMonDevice * PopDevice( const std::string strDevID );
	CMonDevice *FindDevice(const std::string strDevID, BOOL bAccount=FALSE);
	void ClearDevices(void);

	MAP_MTSUSER_DEVID m_mapKeyAccount;
	std::string FindDevID(string strMtsAccount);

//IMonSDKCallback
public:
	virtual void OnSDKCallback_RegisterDevice(char *szIPAddr, int nPort, char* szDevUser, char *szPwd,int nDevType, void*pUserData,int nLen);
	virtual void OnSDKCallback_UnRegisterDevice( char *szDevID ) ;
	virtual bool OnSDKCallback_IsPasswordReady(char * szDevID);
	virtual std::string OnSDKCallback_QueryPassword(char * szDevID);
	virtual void OnSDKCallback_Exception(int nErrorCode, void *pBuf,int nBuflen ){};

protected:
	virtual void OnVOD_SearchFile(unsigned long hPlay, const char* szDevID, int nChanID, unsigned long ulType, const char* szStartTime, const char* szEndTime);
	virtual void OnVOD_PlayByFile(unsigned long hPlay, const char* szDevID, int nChanID, unsigned int nWndID, const char* szFileName);
	virtual void OnVOD_PlayByTime(unsigned long hPlay, const char* szDevID, int nChanID, unsigned int nWndID, const char* szStartTime, const char* szEndTime);
	virtual void OnVOD_PlayByAbsoluteTime(unsigned long hPlay, const char* szDevID, int nChanID, unsigned int nWndID, const char* szAbsoluteTime);
	virtual void OnVOD_PlayPause(unsigned long hPlay, const char* szDevID, int nChanID, unsigned int nWndID, long nState);
	virtual void OnVOD_PlaySpeedControl(unsigned long hPlay, const char* szDevID, int nChanID, unsigned int nWndID, double dSpeed);
	virtual void OnVOD_PlayFastForward(unsigned long hPlay, const char* szDevID, int nChanID, unsigned int nWndID);
	virtual void OnVOD_PlaySlowDown(unsigned long hPlay, const char* szDevID, int nChanID, unsigned int nWndID);
	virtual void OnVOD_PlayNormal(unsigned long hPlay, const char* szDevID, int nChanID, unsigned int nWndID);
	virtual void OnVOD_PlayStop(unsigned long hPlay, const char* szDevID, int nChanID, unsigned int nWndID);

	virtual void OnVOD_DownloadFile(unsigned long hPlay, const char* szDevID, int nChanID, unsigned int nWndID, const char* szFileName);
	virtual void OnVOD_DownloadFile(unsigned long hPlay, const char* szDevID, int nChanID, unsigned int nWndID, const char* szStartTime, const char* szEndTime);
	virtual void OnVOD_StopDownload(unsigned long hPlay, const char* szDevID, int nChanID, unsigned int nWndID);
};

#endif
