#ifndef CGBDEVICESDK_H
#define CGBDEVICESDK_H

#ifndef ONLINE
#define ONLINE 1
#endif

#include "kbase/CritSec.h"
#include "kbase/AutoLock.h"
#include "GB28181Main.h"
#include "IDeviceSdk.h"
#include "CGBChannelSdk.h"
#include "align_left.h"

//#define __BYTE_ORDER __LITTLE_ENDIAN

#if defined(WIN32)
/* Windows is little endian */ 
#define __LITTLE_ENDIAN 1234
#define __BIG_ENDIAN    4321
#define __BYTE_ORDER __LITTLE_ENDIAN
#endif /*  defined(WIN32) */

struct RTPHeader
{
#if __BYTE_ORDER == __BIG_ENDIAN
	unsigned char v:2; 
	unsigned char p:1;
	unsigned char x:1;
	unsigned char cc:4;
	unsigned char m:1;
	unsigned char pt:7; 
#else
	unsigned char cc:4; 
	unsigned char x:1; 
	unsigned char p:1; 
	unsigned char v:2; 
	unsigned char pt:7;
	unsigned char m:1;
#endif
	unsigned short seq;
	unsigned tm;
	unsigned ssrc;
};

#define RTPHEADER_LEN 12

class CGBSDK:public HPMON_ISDK,public GB28181Event 
{
public:
	CGBSDK(ISDKCallback * pCallBack);
	virtual ~CGBSDK();

	virtual int InitSDK();
	virtual void UnInitSDK();
	virtual void ConnectServer(char *szServerID, char *szIPAddr, int nPort , char *szDomain, char *szUser, char *szPwd ){}
	virtual void DisConnectServer(){}
	virtual void SetBandInfo(int nBandwidth,int nStrategy,bool bBalance){};
protected:
	void OnException(int nErrorCode, void *pBuf,int nBuflen);
	void OnGB28181SIPEntityRegistered(const std::string & strID,const std::string & strIP,const int &nPort);
	void OnGB28181SIPEntityUnregistered(const std::string &strID);
	void OnGB28181SIPEntityReady(const std::string & strID,const std::string & strIP,const int &nPort);
	bool OnGB28181SIPEntityPasswordReady(const std::string & strID);
	std::string OnGB28181SIPEntityQueryPassword(const std::string & strID);
protected:
	GB28181Main * m_pGB28181Main;
private:
	ISDKCallback * m_pCallBack;
};

class CGBDeviceSdk:public HPMON_IDeviceSdk,public GBDeviceEvent,public align_left_bytes_callback
{
public:
	CGBDeviceSdk(IDeviceSdkCallback * pSdkCallBack);
	virtual ~CGBDeviceSdk();

	GBDevice * GetGBDevice(){return m_pDevice;}

protected:
	IDeviceSdkCallback * m_pSdkCallBack;

protected:
	GB28181Main * m_pGB28181Main;
	bool m_bConnected;
	GBDevice * m_pDevice;
	std::string m_strGBDeviceID;

private:
	void DisConnect();
//-------------------------------Channel--------------------------//
protected:
	KCritSec		  m_csChannelSdk;
	ID_CHANNELSDK_MAP m_mapChannelSdk;
	bool CreateChannelSDK(int nHPChannelID,const std::string & strChannelID,CGBDeviceSdk * pGBDeviceSdk);
	void PushChannelSDK( int nHPChnID, CGBChannelSdk *pChannelSdk );
	void ClearChannelSDK();
	void ReSetChannelSdkSessionID(int hSession);
	CGBChannelSdk *FindChnSDK(int nHPChnID);

	KCritSec		 m_csmapGBAlarm;
	HPID_GBALARM_MAP m_mapGBAlarm;
	bool CreateGBAlarm(int nHPAlarmID,const std::string & strAlarmID);
	void PushGBAlarm(int nHPAlarmID,GBAlarm * pAlarm);
	GBAlarm * FindGBAlarm(int nHPAlarmID);
	void ClearGBAlarm();




//---------------------------HPMON_IDeviceSdk----------------------//
public:
	virtual bool MON_DEVSDK_Init();
	virtual void MON_DEVSDK_UnInit();
	virtual bool MON_DEVSDK_Connect(char *szDevID, char* szIPAddr, char* szDevUser, char* szDevPwd, unsigned short nPort, void *pUserData, int nLen );
	virtual void MON_DEVSDK_DisConnect();
	virtual void MON_DEVSDK_TimeOutNotOperatorDisConnect();
	virtual bool MON_DEVSDK_IsConnect();
	virtual void MON_DEVSDK_GetChannelInfo();
	virtual void MON_DEVSDK_GetDevName();
	virtual void MON_DEVSDK_SetDevName(const char *szDevName);
	virtual void MON_DEVSDK_GetChannelNum();
	virtual void MON_DEVSDK_GetDevStatus();
	virtual void MON_DEVSDK_GetSerialNo();
	virtual void MON_DEVSDK_GetModelType();
	virtual void MON_DEVSDK_GetTalkCodecID();//获取对讲音频编码ID
	virtual void MON_DEVSDK_StartTalk(char *szDevID, int nTalkChnID);
	virtual void MON_DEVSDK_StopTalk(char *szDevID, int nTalkChnID);
	virtual void MON_DEVSDK_SendTalkStream(int nStreamType, unsigned char*pData,int nLen);
	virtual void MON_DEVSDK_SetDevID(char *szDevID);

protected:
	virtual void OnDeviceVoicePlaySucceed(int nPlayHandle);
	virtual void OnDeviceVoicePlayFailed(int nPlayHandle);
	virtual void OnDeviceRecvAudioStream(int nPlayHandle,void * pData,int nLen);
	virtual void OnDeviceAlarmCallBack(const GBAlarmInfo & tGBAlarmInfo);
	virtual void OnDeviceChannelOnline(const std::string & strChannelID,const int & nChannelNum);
	virtual void OnDeviceAlarmOnline(const std::string & strAlarmID,const int & nAlarmNum);
	virtual void OnDeviceIPChanged(const char * szIPaddr,const int nPort);

	virtual void OnAlignLeftCallback_OneLeftBytes(align_left_bytes* pLeft,unsigned char*pBytes,int nBytes);

private:
	int m_nVoiceHandle;
	unsigned short m_nSeq;
	unsigned int m_nTimeStamp;
	align_left_bytes * m_pAlignLeft;
	unsigned char m_VoiceBuffer[1024];//12+data >512就清一次
	int m_nVoiceBufferLen;
	unsigned long m_nCurrentTime;
	int m_nVoiceCodecID;
	int m_nChannelCount;

};

#endif