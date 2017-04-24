#ifndef _STREAM_RECVER_H_
#define _STREAM_RECVER_H_

#include <event2/bufferevent.h>
#include <KBASE/AutoLock.h>
#include "GBDefines.h"
#include "On3PartyCallback.h"
#include "RtpReorder.h"

typedef enum {
	GB_NONE  = 0x00,
	GB_AUDIO = 0x01,
	GB_VIDEO = 0x02,
}GB_STREAM_TYPE;

class StreamRecver
{
public:
	StreamRecver(VideoCallback *video_cb);
	StreamRecver(AudioCallback *audio_cb);
	~StreamRecver();

	std::string GetGBDeviceID() {return gb_device_id_;}
	std::string GetGBPlatformID() {return gb_platform_id_;}
	void SetCallID(const int cid) {call_id_ = cid;}
	void SetLocalIP(const std::string local_ip) {local_ip_ = local_ip;}
	void SetGBPlatformID(const std::string gb_plt_id) {gb_platform_id_ = gb_plt_id;}
	void SetGBDeviceID(const std::string gb_dev_id) {gb_device_id_ = gb_dev_id;}
	void SetGBChannelID(const std::string gb_chan_id) {gb_channel_id_ = gb_chan_id;}
	void SetPlaybackHandle(unsigned long handle) {playback_handle_ = handle;}
	unsigned long GetPlaybackHandle() {return playback_handle_;}
	void SetStreamEventBase(struct event_base *event_base) {stream_event_base_ = event_base;}

	bool CheckPortInvalid(const std::string &local_ip, const int port);

	int  Open(int media_type);
	void Close();

private:
	KCritSec   port_cs_;
	static int start_port_;

	int call_id_;
	std::string local_ip_;
	std::string gb_platform_id_;
	std::string gb_device_id_;
	std::string gb_channel_id_;

	unsigned long playback_handle_;

	VideoCallback *video_cb_;
	AudioCallback *audio_cb_;

	struct bufferevent *rtp_buffev_;
	struct bufferevent *rtcp_buffev_;

	struct event_base *stream_event_base_;

#ifdef RTP_REORDER
	RtpReorder *rtp_reorder_;
#endif
};

#endif