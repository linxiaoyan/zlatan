#include "stdafx.h"
#include "GBDeviceTalk.h"

GBDeviceTalk::GBDeviceTalk():
	audio_sender_(NULL),
	audio_recver_(NULL),
	call_id_(-1)
{
}

GBDeviceTalk::~GBDeviceTalk()
{
	
}

void GBDeviceTalk::Destroy()
{
	if (audio_recver_)
	{
		audio_recver_->Close();
		delete audio_recver_;
		audio_recver_ = NULL;
	}

	if (audio_sender_)
	{
		audio_sender_->Close();
		delete audio_sender_;
		audio_sender_ = NULL;
	}
}

int GBDeviceTalk::CreateAudioRecver(AudioCallback *audio_cb)
{
	int recv_port = -1;
	if (audio_recver_)
		return recv_port;

	audio_recver_ = new StreamRecver(audio_cb);
	if (audio_recver_)
	{
		audio_recver_->SetLocalIP(local_ip_);
		audio_recver_->SetStreamEventBase(event_base_);
		recv_port = audio_recver_->Open(GB_AUDIO);
	}
	return recv_port;
}

void GBDeviceTalk::DestroyAudioRecver()
{
	if (audio_recver_)
	{
		audio_recver_->Close();
		delete audio_recver_;
		audio_recver_ = NULL;
	}
}

int GBDeviceTalk::CreateAudioSender()
{
	int send_port = -1;
	if (audio_sender_)
		return send_port;

	audio_sender_ = new UdpClient;
	if(audio_sender_->Create("", 0) < 0)
	{
		SDK_LOG("Create UdpClient failed\n");
		delete audio_sender_;
		audio_sender_ = NULL;
		return send_port;
	}

	send_port = audio_sender_->GetLocalPort();
	return send_port;
}

void GBDeviceTalk::DestroyAudioSender()
{
	if (audio_sender_)
	{
		audio_sender_->Close();
		delete audio_sender_;
		audio_sender_ = NULL;
		call_id_ = -1;//关闭流一定要赋为-1
	}
}

void GBDeviceTalk::SendTalkStream(char *data, int data_len)
{
	if (audio_sender_)
	{
		printf("Send Talk Stream To [%s][%d][%d]\n", dest_audio_ip_.c_str(), dest_audio_port_, data_len);
		audio_sender_->SendTo(dest_audio_ip_, dest_audio_port_, data, data_len);
	}
}

void GBDeviceTalk::SetCallID(const int cid)
{
	call_id_ = cid;
	audio_recver_->SetCallID(cid);
}

void GBDeviceTalk::SetLocalIP(const std::string local_ip)
{
	local_ip_ = local_ip;
}

void GBDeviceTalk::SetStreamEventBase(struct event_base *event_base)
{
	event_base_ = event_base;
}
