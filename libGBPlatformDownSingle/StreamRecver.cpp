#include "stdafx.h"
#include <KBASE/AutoLock.h>
#include <event2/event.h>
#include <event2/buffer.h>
#include "StreamRecver.h"
#include "RtpReorder.h"

int StreamRecver::start_port_ = GB_DEFAULT_VIDEO_PORT;

void OnRecvVideoData(struct bufferevent *bev, void *vcb)
{
#ifdef RTP_REORDER
	RtpReorder *rtp_reorder = (RtpReorder *)vcb;
	char buf[2048];
	int len = bufferevent_read(bev, buf, 2048);
	if (len > 0)
	{
		rtp_reorder->InputRtpPacket(buf, len);
	}
#else
	VideoCallback  *pvcb = (VideoCallback *)vcb;
	char buf[2048];
	int len = bufferevent_read(bev, buf, 2048);
	if (len > 0)
		pvcb->On3PartyVideoCallback((unsigned char *)buf, len);
#endif
}

void OnRecvAudioData(struct bufferevent *bev, void *acb)
{
#ifdef RTP_REORDER
	RtpReorder *rtp_reorder = (RtpReorder *)acb;
	char buf[2048];
	int len = bufferevent_read(bev, buf, 2048);
	if (len > 0)
	{
		rtp_reorder->InputRtpPacket(buf, len);
	}
#else
	AudioCallback *pacb = (AudioCallback *)acb;
	char buf[2048];
	int len = bufferevent_read(bev, buf, 2048);
	if (len > 0)
		pacb->On3PartyAudioCallback((unsigned char *)buf, len);
#endif
}

StreamRecver::StreamRecver(VideoCallback *video_cb)
	:video_cb_(video_cb)
	,audio_cb_(NULL)
	,rtp_buffev_(NULL)
	,rtcp_buffev_(NULL)
#ifdef RTP_REORDER
	,rtp_reorder_(NULL)
#endif
{
#ifdef RTP_REORDER
	rtp_reorder_ = new RtpReorder(video_cb);
#endif
}

StreamRecver::StreamRecver(AudioCallback *audio_cb)
	:audio_cb_(audio_cb)
	,video_cb_(NULL)
	,rtp_buffev_(NULL)
	,rtcp_buffev_(NULL)
#ifdef RTP_REORDER
	,rtp_reorder_(NULL)
#endif
{
#ifdef RTP_REORDER
	rtp_reorder_ = new RtpReorder(audio_cb);
#endif
}

StreamRecver::~StreamRecver()
{
	Close();
}

bool StreamRecver::CheckPortInvalid(const std::string &local_ip, const int port)
{
	return true;
}

int StreamRecver::Open(int media_type)
{
	int rtp_port = -1;

	if (rtp_buffev_ || rtcp_buffev_)
		return rtp_port;
	
	{
		KAutoLock l(port_cs_);
		if (start_port_ == GB_DEFAULT_VIDEO_PORT + 1000)
			start_port_ = GB_DEFAULT_VIDEO_PORT;

		rtp_port = start_port_;
		start_port_ += 2;
	}

	int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	evutil_make_socket_nonblocking(sock);

	struct sockaddr_in addr_in;
	memset(&addr_in, 0, sizeof(addr_in));
	addr_in.sin_family = AF_INET;
	addr_in.sin_addr.s_addr = inet_addr(local_ip_.c_str());
	addr_in.sin_port = htons(rtp_port);
	if (bind(sock, (struct sockaddr*)&addr_in, sizeof(addr_in)) < 0)
		return -1;

	rtp_buffev_ = bufferevent_socket_new(stream_event_base_, sock, BEV_OPT_CLOSE_ON_FREE);

#ifdef RTP_REORDER
	if (media_type == GB_VIDEO)
		bufferevent_setcb(rtp_buffev_, OnRecvVideoData, NULL, NULL, rtp_reorder_);
	else if (media_type == GB_AUDIO)
		bufferevent_setcb(rtp_buffev_, OnRecvAudioData, NULL, NULL, rtp_reorder_);
#else
	if (media_type == GB_VIDEO)
		bufferevent_setcb(rtp_buffev_, OnRecvVideoData, NULL, NULL, video_cb_);
	else if (media_type == GB_AUDIO)
		bufferevent_setcb(rtp_buffev_, OnRecvAudioData, NULL, NULL, audio_cb_);
#endif

	bufferevent_enable(rtp_buffev_, EV_READ);

	/*
	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	evutil_make_socket_nonblocking(sock);
	addr_in.sin_port = htons(rtp_port+1);
	if (bind(sock, (struct sockaddr*)&addr_in, sizeof(addr_in)) < 0)
		return -1;

	rtcp_buffev_ = bufferevent_socket_new(stream_event_base_, sock, BEV_OPT_CLOSE_ON_FREE);
	
	if (media_type == GB_VIDEO)
		bufferevent_setcb(rtcp_buffev_, OnRecvVideoData, NULL, NULL, video_cb_);
	else if (media_type == GB_AUDIO)
		bufferevent_setcb(rtcp_buffev_, OnRecvAudioData, NULL, NULL, audio_cb_);

	bufferevent_enable(rtcp_buffev_, EV_READ);
	*/
	return rtp_port;
}

void StreamRecver::Close()
{
	if (rtcp_buffev_)
	{
		bufferevent_free(rtcp_buffev_);
		rtcp_buffev_ = NULL;
	}

	if (rtp_buffev_)
	{
		bufferevent_free(rtp_buffev_);
		rtp_buffev_ = NULL;
	}

#ifdef RTP_REORDER
	if (rtp_reorder_)
	{
		rtp_reorder_->Destroy();
		delete rtp_reorder_;
		rtp_reorder_ = NULL;
	}
#endif

	video_cb_ = NULL;
	audio_cb_ = NULL;
}
