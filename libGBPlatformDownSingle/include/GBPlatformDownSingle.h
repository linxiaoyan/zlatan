#ifndef _GB_PLATFORM_DOWN_SINGLE_H_
#define _GB_PLATFORM_DOWN_SINGLE_H_

#include <event2/event.h>
#include "EXosipThread.h"
#include "StreamEventBaseThread.h"
#include "GBCheckHeartBeat.h"
#include "GBPlatformDownSingleEvent.h"
#include "On3PartyCallback.h"
#include "GBPlatformCallback.h"

class GBPlatform;
class StreamRecver;
class GBDeviceTalk;

typedef std::list<GBPlatform*> GBPlatformList;
typedef std::map<int, StreamRecver*> CidStreamRecverMap;
typedef std::map<int, GBDeviceTalk*> CidGBDeviceTalkMap;

class GB28181_API GBPlatformDownSingle:
	public EXosipThreadEvent,
	public GBCheckHeartBeatEvent,
	public GBPlatformCallback
{
private:
	GBPlatformDownSingle();

public:
	virtual ~GBPlatformDownSingle();

	static GBPlatformDownSingle *GetInstance();
	
	GBPlatform *FindGBPlatform(const std::string &plt_id);
	void DestroyGBDeviceVideoMapWhichDeviceID(const std::string &device_id);
	void DestroyGBDeviceVideoMapWhichPlatformID(const std::string &plt_id);
	void DestroyGBDeviceTalkMapWhichDeviceID(const std::string &device_id);
	void DestroyGBDeviceTalkMapWhichPlatformID(const std::string &plt_id);

public:
	bool Init(const int trace_level=1);
	void SetEventHandle(GBPlatformDownSingleEvent *gb_platform_down_single_event);
	void SetSipProtocol(const int protocol = GB_PROTOCOL_UDP);
	void SetSipLocalIP(const std::string &ip);
	void SetSipLocalPort(const int port = GB_SIP_PORT);
	void SetSipMapIP(const std::string &ip);
	void SetSipMapPort(const int port = GB_SIP_PORT);
	bool SetSipServerID(const std::string &id);

	bool Start();
	void Stop();

	void DisconnectGBPlatform(const std::string &plt_id);
	void RemoveGBPlatform(const std::string &plt_id);

	int  StartPlay(const std::string &plt_id, const std::string &device_id, const std::string &channel_id, const std::string &plt_ip, const int plt_port, VideoCallback *video_cb); 
	void StopPlay(const int cid, const int did);
	int  StartTalk(const std::string &plt_id, const std::string &device_id, const std::string &plt_ip, const int plt_port, AudioCallback *audio_cb);
	void StopTalk(const int cid, const int did); //由插件那边传过来
	void PtzControl(const std::string &plt_id, const std::string &channel_id, const std::string &plt_ip, const int plt_port, const int ptz_cmd, const int speed);
	void SendTalkStream(const int cid, unsigned char *data, int data_len);
	void OnGBCheckHeartBeatEvent_OffLine(const std::string &plt_id);

	int  SearchFile(const std::string &plt_id, const std::string &channel_id, const std::string &plt_ip, const int plt_port, const char *type, const char *start_time, const char *end_time);
	int  PlayByTime(const std::string &plt_id, const std::string &device_id, const std::string &channel_id, const std::string &plt_ip, const int plt_port, const int record_type, const std::string &start_time, const std::string &end_time, const unsigned long handle,VideoCallback *video_cb);
	void PlayByAbsoluteTime(const int cid, const int did, const int npt);
	void PlaybackStop(const int cid, const int did);
	void PlaybackPause(const int cid, const int did, const bool pause);
	void PlaybackSpeed(const int cid, const int did, const double speed);
	int  DownloadByTime(const std::string &plt_id, const std::string &device_id, const std::string &channel_id, const std::string &plt_ip, const int plt_port, const int record_type, const std::string &start_time, const std::string &end_time, const unsigned long handle,VideoCallback *video_cb);

private:
	std::string local_id_;
	char local_ip_[40];
	unsigned int sip_sn_;

	KCritSec gb_platform_cs_;
	GBPlatformList gb_platform_list_;

	KCritSec gb_device_video_cs_;
	CidStreamRecverMap gb_device_video_map_;

	KCritSec gb_device_talk_cs_;
	CidGBDeviceTalkMap gb_device_talk_map_;

	KCritSec playback_cs_;
	CidStreamRecverMap playback_map_;

	eXosip_t *exosip_;
	EXosipThread *exosip_thread_;
	GBCheckHeartBeat *gb_check_heartbeat_;
	GBPlatformDownSingleEvent *gb_platform_down_single_event_; //由GBPlatformSdk继承实现

	struct event_base *stream_event_base_;
	StreamEventBaseThread *stream_event_base_thread_;

	static GBPlatformDownSingle gb_platform_down_single_;

protected:
	virtual bool OnEXosipThreadEvent_IsUserRegistered(const std::string &plt_id);
	virtual bool OnEXosipThreadEvent_IsUserRegisteredByOtherIP(const std::string &plt_id, const std::string &plt_ip, const int plt_port);
	virtual bool OnEXosipThreadEvent_IsPasswordReady(const std::string &plt_id);
	virtual void OnEXosipThreadEvent_QueryPassword(const std::string &plt_id, std::string &plt_pwd);
	virtual void OnEXosipThreadEvent_Registered(const int reg_cseq, const std::string &plt_id, const std::string &plt_pwd, const std::string &plt_ip, const int plt_port, const int expires);
	virtual void OnEXosipThreadEvent_UnRegistered(const std::string &plt_id);
	virtual void OnEXosipThreadEvent_SubscribeAnswered(const std::string &plt_id, const int did);

	virtual void OnEXosipThreadEvent_Keeplive(const Keepalive &keepalive);
	virtual void OnEXosipThreadEvent_Catalog(const std::string &plt_id_or_dev_id, Catalog &catalog);
	virtual void OnEXosipThreadEvent_DeviceInfo(const DeviceInfo &device_info);
	virtual void OnEXosipThreadEvent_DeviceStatus(const DeviceStatus &device_status);
	virtual void OnEXosipThreadEvent_RecordInfo(const RecordInfo &record_info);
	virtual void OnEXosipThreadEvent_NotifyCatalog(const std::string &plt_id_or_dev_id, NotifyCatalog &notify_catalog);
	
	virtual void OnEXosipThreadEvent_StartPlaySucceeded(const int cid, const int did, const int stream_type, const int encoding_type, const int ssrc);
	virtual void OnEXosipThreadEvent_StartPlayFailed(const int cid);
	virtual void OnEXosipThreadEvent_StartTalkSucceeded(const int cid, const int did, const int encoding_type, const std::string &dest_ip, const unsigned short dest_port);
	virtual void OnEXosipThreadEvent_StartTalkFailed(const int cid);
	virtual void OnEXosipThreadEvent_PlaybackSucceeded(const int cid, const int did, const int stream_type, const int encoding_type, const int ssrc);
	virtual void OnEXosipThreadEvent_PlaybackFailed(const int cid);
	
	virtual void OnEXosipThreadEvent_TcpRequest(const int cid, const int did);

protected:
	virtual void SubscribeRefreshCatalog(const int did, const int expires, const std::string &device_id);
};
#endif
