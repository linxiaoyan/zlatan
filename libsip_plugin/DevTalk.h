#pragma once
#ifndef _DEVTALK_H_
#define _DEVTALK_H_

class IDevTalkCallback
{
public:
	virtual ~IDevTalkCallback(){};
	virtual bool OnDevTalkCallback_StartTalkSDK(int nChnID) = 0;
	virtual void OnDevTalkCallback_StopTalkSDK(int nChnID) = 0;
	virtual void OnDevTalkCallback_RespondTalk(const MPX_TALK_INFO*pTalkData) = 0;
	virtual void OnDevTalkCallback_TalkTimerout(void) = 0;
	virtual void OnDevTalkCallback_DevChannelChange(unsigned long ulAudioID, bool bStartTalk=true) = 0;
	virtual void OnDevTalkCallback_SendVoiceToDevice(int nStreamType, unsigned char*pData,int nLen )= 0;
};


class CDevTalk
{
public:
	CDevTalk( IDevTalkCallback* pITalkCallback, int nTalkCodecID );
	~CDevTalk(void);
protected:
	IDevTalkCallback*		m_pDevTalkCallback;
	bool					m_bWantToStop;

	MPX_TALK_INFO 	*m_pTalkRespond;
	int				m_nDevType;
	std::string		m_strUserData;

	unsigned long		m_ulTalkAudioID;
	int					m_nCodecID;
	long				m_hTransferSend;
	long				m_hTransferRecv;
	long				m_hAVCto3Party;
	long				m_h3PartyToAVC;

public:
	void RespondTalkSuccess();
	void RespondTalkFailed();
	void RespondTalkBusy(MPX_TALK_INFO *pTalkData);

public:
	void OnVoiceStream(unsigned int dwMediaType, unsigned char *pBuffer, unsigned int dwBufSize, 
		unsigned char*pExtraData,unsigned int nExtraSize);

	bool StartTalk( MPX_TALK_INFO*pTalkData);
	void StopTalk(const char *szNodeID);
	void TalkChanged(unsigned long nAudioID);
	void OnOpenDevTalkChn(int nErrCode, void *pBuffer, int nBufLen);

public:
	//向平台发数据
	void OnAVCPacket2MTS(long hHandle,int nStreamType,unsigned char*pData,int nLen);
	//向设备端发送数据
	void OnMonTo3RDDevice(long hHandle,int nStreamType,unsigned char*pData,int nLen);
	//收流
	void OnTransferStream(long hHandle,const char*szKey,int nDevType,int nPacketType,unsigned char*pData,int nLen);
	//
	void OnSendTransferMessage(long hHandle,const char*szKey,int nMessage,long wParam,long lParam);
	void OnRecvTransferMessage(long hHandle,const char*szKey,int nMessage,long wParam,long lParam); 

};

#endif
