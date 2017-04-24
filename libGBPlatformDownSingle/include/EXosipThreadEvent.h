#ifndef _EXOSIP_THREAD_EVENT_H_
#define _EXOSIP_THREAD_EVENT_H_

#include "GBDefines.h"

class EXosipThreadEvent
{
public:
	virtual ~EXosipThreadEvent(){}

	virtual bool OnEXosipThreadEvent_IsUserRegistered(const std::string &plt_id)=0;
	virtual bool OnEXosipThreadEvent_IsUserRegisteredByOtherIP(const std::string &plt_id, const std::string &plt_ip, const int plt_port)=0;
	virtual bool OnEXosipThreadEvent_IsPasswordReady(const std::string &plt_id)=0;
	virtual void OnEXosipThreadEvent_QueryPassword(const std::string &plt_id, std::string &plt_pwd)=0;
	virtual void OnEXosipThreadEvent_Registered(const int reg_cseq, const std::string &plt_id, const std::string &plt_pwd, const std::string &plt_ip, const int plt_port, const int expires)=0;
	virtual void OnEXosipThreadEvent_UnRegistered(const std::string &plt_id)=0;
	virtual void OnEXosipThreadEvent_SubscribeAnswered(const std::string &plt_id, const int did)=0;

	virtual void OnEXosipThreadEvent_Keeplive(const Keepalive &keeplive)=0;
	virtual void OnEXosipThreadEvent_Catalog(const std::string &plt_id_or_dev_id, Catalog &catalog)=0;
	virtual void OnEXosipThreadEvent_DeviceInfo(const DeviceInfo &device_info)=0;
	virtual void OnEXosipThreadEvent_DeviceStatus(const DeviceStatus &device_status)=0;
	virtual void OnEXosipThreadEvent_RecordInfo(const RecordInfo &record_info)=0;
	virtual void OnEXosipThreadEvent_NotifyCatalog(const std::string &plt_id_or_dev_id, NotifyCatalog &notify_catalog)=0;
	
	virtual void OnEXosipThreadEvent_StartPlaySucceeded(const int cid, const int did, const int stream_type, const int encoding_type, const int ssrc)=0;
	virtual void OnEXosipThreadEvent_StartPlayFailed(const int cid)=0;
	virtual void OnEXosipThreadEvent_StartTalkSucceeded(const int cid, const int did, const int encoding_type, const std::string &dest_ip, const unsigned short dest_port)=0;
	virtual void OnEXosipThreadEvent_StartTalkFailed(const int cid)=0;
	virtual void OnEXosipThreadEvent_PlaybackSucceeded(const int cid, const int did, const int stream_type, const int encoding_type, const int ssrc)=0;
	virtual void OnEXosipThreadEvent_PlaybackFailed(const int cid)=0;

	virtual void OnEXosipThreadEvent_TcpRequest(const int cid, const int did)=0;
};

#endif