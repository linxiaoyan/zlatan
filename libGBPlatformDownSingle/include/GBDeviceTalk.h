#ifndef _GB_DEVICE_TALK_H_
#define _GB_DEVICE_TALK_H_

#include "GBDefines.h"
#include "GB28181Utils.h"
#include "UdpClient.h"
#include "UdpServer.h"
#include "StreamRecver.h"

class GBDeviceTalk
{
public:
	GBDeviceTalk();
	virtual ~GBDeviceTalk();

	int	 CreateAudioRecver(AudioCallback *audio_cb);
	void DestroyAudioRecver();
	int  CreateAudioSender();
	void DestroyAudioSender();

	void SendTalkStream(char *data, int data_len);
	void SetCallID(const int cid);

	void SetLocalIP(const std::string local_ip);
	void SetStreamEventBase(struct event_base *event_base);

	void Destroy();
	
public:
	std::string    dest_audio_ip_;
	unsigned short dest_audio_port_;
	std::string    gb_platform_id_;
	std::string    gb_device_id_;

private:
	UdpClient	 *audio_sender_;
	StreamRecver *audio_recver_;
	int call_id_;

	std::string local_ip_;
	struct event_base *event_base_;
};
#endif