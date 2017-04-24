#include "stdafx.h"
#include <KBASE/XMLParser.h>
#include <KBASE/AutoLock.h>
#include "GBPlatformDownSingle.h"
#include "StreamRecver.h"
#include "GBDeviceTalk.h"
#include "GBPlatform.h"

static std::string GetChildNodeText(const XMLNode &xmlParent,const std::string &strNodeName)
{
	XMLNode xmlRet=xmlParent.GetChildNode(strNodeName.c_str());
	return xmlRet.IsEmpty() ?  "" : (xmlRet.GetText()==NULL ? "" : xmlRet.GetText());
}

GBPlatformDownSingle GBPlatformDownSingle::gb_platform_down_single_;

GBPlatformDownSingle *GBPlatformDownSingle::GetInstance()
{
	return &gb_platform_down_single_;
}

GBPlatformDownSingle::GBPlatformDownSingle():
	exosip_thread_(NULL),
	gb_platform_down_single_event_(NULL),
	gb_check_heartbeat_(NULL),
	exosip_(NULL),
	stream_event_base_(NULL),
	stream_event_base_thread_(NULL),
	sip_sn_(0)
{
}

GBPlatformDownSingle::~GBPlatformDownSingle()
{
}

bool GBPlatformDownSingle::Init(const int trace_level)
{
	osip_trace_initialize((osip_trace_level_t)trace_level,stdout);

	exosip_ = eXosip_malloc();
	if (exosip_ == NULL)
		return false;

	int nRet = eXosip_init(exosip_);
	if (nRet != OSIP_SUCCESS)
	{
		OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_ERROR,NULL,"Init the eXosip failed!\n"));
		return false;
	}
	
	if (exosip_thread_ == NULL)
	{
		exosip_thread_ = new EXosipThread(this);
		exosip_thread_->SetEXosip(exosip_);
	}

	if (gb_check_heartbeat_ == NULL)
		gb_check_heartbeat_ = new GBCheckHeartBeat(this);

	if (stream_event_base_ == NULL)
		stream_event_base_ = event_base_new();

	if (stream_event_base_thread_ == NULL)
		stream_event_base_thread_ = new StreamEventBaseThread(stream_event_base_);

	return true;
}

void GBPlatformDownSingle::SetEventHandle(GBPlatformDownSingleEvent *gb_platform_down_single_event)
{
	gb_platform_down_single_event_ = gb_platform_down_single_event;
}

void GBPlatformDownSingle::SetSipProtocol(const int protocol)
{
	if (exosip_thread_)
		exosip_thread_->SetProtocol(protocol);
}

void GBPlatformDownSingle::SetSipLocalIP(const std::string &ip)
{
	if (exosip_thread_)
	{
		if (0 == ip.size())
		{
			int ret = eXosip_guess_localip(exosip_, AF_INET, local_ip_, 40);
			if (ret != OSIP_SUCCESS)
			{
				OSIP_TRACE(osip_trace(__FILE__,__LINE__,OSIP_ERROR,NULL,"Guess the local ip failed!\n"));
			}
			else
			{
				exosip_thread_->SetLocalIP(local_ip_);
			}
		}
		else
		{	
			sprintf(local_ip_, ip.c_str());
			exosip_thread_->SetLocalIP(ip);
		}
	}
}

void GBPlatformDownSingle::SetSipLocalPort(const int port)
{
	if (exosip_thread_)
		exosip_thread_->SetLocalPort(port);
}

void GBPlatformDownSingle::SetSipMapIP(const std::string &ip)
{

}

void GBPlatformDownSingle::SetSipMapPort(const int port)
{

}

bool GBPlatformDownSingle::SetSipServerID(const std::string &id)
{
	if (!GB28181Utils::IsUserIDValidate(id))
		return false;

	local_id_ = id;

	if (exosip_thread_)
	{
		exosip_thread_->SetID(local_id_);
		exosip_thread_->SetRealm(local_id_.substr(0,10));
	}

	return true;
}

bool GBPlatformDownSingle::Start()
{
	if (!gb_check_heartbeat_ || !gb_check_heartbeat_->Start())
		return false;

	if (!exosip_thread_ || !exosip_thread_->Start())
		return false;

	if (!stream_event_base_thread_ || !stream_event_base_thread_->Start())
		return false;
	
	return true;
}

void GBPlatformDownSingle::Stop()
{
	if (stream_event_base_thread_)
	{
		stream_event_base_thread_->Stop();
		delete stream_event_base_thread_;
		stream_event_base_thread_ = NULL;
	}

	if (stream_event_base_)
		event_base_free(stream_event_base_);

	if (exosip_thread_)
	{
		exosip_thread_->Stop();
		delete exosip_thread_;
		exosip_thread_ = NULL;
	}

	if (gb_check_heartbeat_)
	{
		gb_check_heartbeat_->Stop();
		delete gb_check_heartbeat_;
		gb_check_heartbeat_ = NULL;
	}

	eXosip_quit(exosip_);
	osip_free(exosip_);
}

GBPlatform *GBPlatformDownSingle::FindGBPlatform(const std::string &plt_id)
{
	KAutoLock lock(gb_platform_cs_);
	GBPlatformList::iterator it = gb_platform_list_.begin();
	while (it != gb_platform_list_.end())
	{
		if (plt_id == (*it)->GetID())
			return *it;
		else
			++it;
	}
	return NULL;
}

void GBPlatformDownSingle::DisconnectGBPlatform(const std::string &plt_id)
{
	KAutoLock lock(gb_platform_cs_);
	GBPlatformList::iterator it = gb_platform_list_.begin();
	while (it != gb_platform_list_.end())
	{
		if (plt_id == (*it)->GetID())
		{
			(*it)->SetStatus(false);

			(*it)->KillTimerEvent((*it)->GetSubDid());

			eXosip_lock(exosip_);
			eXosip_subscribe_remove(exosip_, (*it)->GetSubDid());
			eXosip_unlock(exosip_);
			
			return;
		}

		++it;
	}
}

void GBPlatformDownSingle::RemoveGBPlatform(const std::string &plt_id)
{
	KAutoLock lock(gb_platform_cs_);
	GBPlatformList::iterator it = gb_platform_list_.begin();
	while (it != gb_platform_list_.end())
	{
		if (plt_id == (*it)->GetID())
		{
			(*it)->StopTimer();

			eXosip_lock(exosip_);
			eXosip_subscribe_remove(exosip_, (*it)->GetSubDid());
			eXosip_unlock(exosip_);

			delete (*it);
			gb_platform_list_.erase(it);
			return;
		}

		++it;
	}
}

void GBPlatformDownSingle::DestroyGBDeviceVideoMapWhichDeviceID(const std::string &device_id)
{
	KAutoLock lock(gb_device_video_cs_);
	CidStreamRecverMap::iterator it = gb_device_video_map_.begin();
	for (;it != gb_device_video_map_.end();)
	{
		if (it->second->GetGBDeviceID() == device_id)
		{
			delete it->second;
			gb_device_video_map_.erase(it++);
		}
	}
}

void GBPlatformDownSingle::DestroyGBDeviceVideoMapWhichPlatformID(const std::string &plt_id)
{
	KAutoLock lock(gb_device_video_cs_);
	CidStreamRecverMap::iterator it = gb_device_video_map_.begin();
	for (;it != gb_device_video_map_.end();)
	{
		if (it->second->GetGBPlatformID() == plt_id)
		{
			delete it->second;
			gb_device_video_map_.erase(it++);
		}
	}
}

void GBPlatformDownSingle::DestroyGBDeviceTalkMapWhichDeviceID(const std::string &device_id)
{
	KAutoLock lock(gb_device_talk_cs_);
	CidGBDeviceTalkMap::iterator it = gb_device_talk_map_.begin();
	for (;it != gb_device_talk_map_.end();)
	{
		if (it->second->gb_device_id_ == device_id)
		{
			delete it->second;
			gb_device_talk_map_.erase(it++);
		}
	}
}

void GBPlatformDownSingle::DestroyGBDeviceTalkMapWhichPlatformID(const std::string &plt_id)
{
	KAutoLock lock(gb_device_talk_cs_);
	CidGBDeviceTalkMap::iterator it = gb_device_talk_map_.begin();
	for (;it != gb_device_talk_map_.end();)
	{
		if (it->second->gb_platform_id_ == plt_id)
		{
			delete it->second;
			gb_device_talk_map_.erase(it++);
		}
	}
}

int GBPlatformDownSingle::StartPlay(const std::string &plt_id, const std::string &device_id, const std::string &channel_id, const std::string &plt_ip, const int plt_port, VideoCallback *video_cb)
{
	int cid = -1;
	StreamRecver *stream_recver = new StreamRecver(video_cb);
	if (stream_recver == NULL)
		return cid;

	stream_recver->SetLocalIP(local_ip_);
	stream_recver->SetStreamEventBase(stream_event_base_);

	int video_port = stream_recver->Open(GB_VIDEO);
	if (video_port < 0)
	{
		SDK_LOG("[StartPlay] Open Video StreamRecver[%s@%s_%s] failed\n", plt_id.c_str(), device_id.c_str(), channel_id.c_str());
		delete stream_recver;
		stream_recver = NULL;
		return cid;
	}

	cid = GB28181Utils::Play(exosip_, channel_id, plt_ip, plt_port, local_id_, local_ip_, video_port, 0);
	if (cid <= 0)
	{
		SDK_LOG("[StartPlay] GB28181Utils::Play[%s@%s_%s] failed\n", plt_id.c_str(), device_id.c_str(), channel_id.c_str());
		delete stream_recver;
		stream_recver = NULL;
		return cid;
	}

	{
		KAutoLock lock(gb_device_video_cs_);
		stream_recver->SetCallID(cid);
		stream_recver->SetGBPlatformID(plt_id);
		stream_recver->SetGBDeviceID(device_id);
		stream_recver->SetGBChannelID(channel_id);
		gb_device_video_map_[cid] = stream_recver;
	}
	
	return cid;
}

void GBPlatformDownSingle::StopPlay(const int cid, const int did)
{
	{
		KAutoLock lock(gb_device_video_cs_);
		CidStreamRecverMap::iterator it = gb_device_video_map_.find(cid);
		if (it != gb_device_video_map_.end())
		{
			it->second->Close();
			delete it->second;
			gb_device_video_map_.erase(it);
		}
	}

	GB28181Utils::StopPlay(exosip_, cid, did);
	SDK_LOG("[GBPlatformDownSingle] StopPlay cid[%d] did[%d]\n", cid, did);
}

int GBPlatformDownSingle::StartTalk(const std::string &plt_id, const std::string &device_id, const std::string &plt_ip, const int plt_port, AudioCallback *audio_cb)
{
	int cid = -1;
	GBDeviceTalk *gb_device_talk = new GBDeviceTalk();
	if (gb_device_talk == NULL)
	{
		SDK_LOG("[StartTalk] new GBDeviceTalk[%s@%s] failed\n", plt_id.c_str(), device_id.c_str());
		return cid;
	}

	gb_device_talk->SetLocalIP(local_ip_);
	gb_device_talk->SetStreamEventBase(stream_event_base_);

	int recv_port = gb_device_talk->CreateAudioRecver(audio_cb);
	if (recv_port < 0)
	{
		SDK_LOG("[StartTalk] Open Audio StreamRecver[%s@%s] failed\n", plt_id.c_str(), device_id.c_str());
		delete gb_device_talk;
		return cid;
	}

	int send_port = gb_device_talk->CreateAudioSender();
	if (send_port < 0)
	{
		SDK_LOG("[StartTalk] Create AudioSender[%s@%s] failed\n", plt_id.c_str(), device_id.c_str());
		delete gb_device_talk;
		return cid;
	}
	
	cid = GB28181Utils::TalkPlay(exosip_, device_id, plt_ip, plt_port, local_id_, local_ip_, recv_port, send_port, 0);
	if (cid <= 0)
	{
		SDK_LOG("[StartTalk] GB28181Utils::TalkPlay[%s@%s] failed", plt_id.c_str(), device_id.c_str());
		delete gb_device_talk;
		return cid;
	}

	{
		KAutoLock lock(gb_device_talk_cs_);
		gb_device_talk->gb_platform_id_ = plt_id;
		gb_device_talk->gb_device_id_ = device_id;
		gb_device_talk_map_[cid] = gb_device_talk;
	}

	return cid;
}

void GBPlatformDownSingle::StopTalk(const int cid, const int did)
{
	{
		KAutoLock lock(gb_device_talk_cs_);
		CidGBDeviceTalkMap::iterator it = gb_device_talk_map_.find(cid);
		if (it != gb_device_talk_map_.end())
		{
			it->second->Destroy();
			delete it->second;
			gb_device_talk_map_.erase(it);
		}
	}

	GB28181Utils::StopPlay(exosip_, cid, did);
}

void GBPlatformDownSingle::PtzControl(const std::string &plt_id, const std::string &channel_id, const std::string &plt_ip, const int plt_port, const int ptz_cmd, const int speed)
{
	GB28181Utils::PtzControl(exosip_, plt_id, plt_ip, plt_port, local_id_, ++sip_sn_, channel_id, ptz_cmd, speed);
}

void GBPlatformDownSingle::SendTalkStream(const int cid, unsigned char *data, int data_len)
{
	KAutoLock lock(gb_device_talk_cs_);
	if (gb_device_talk_map_[cid])
		gb_device_talk_map_[cid]->SendTalkStream((char *)data, data_len);
}

void GBPlatformDownSingle::OnGBCheckHeartBeatEvent_OffLine(const std::string &plt_id)
{
	{
		KAutoLock lock(gb_platform_cs_);
		GBPlatformList::iterator it = gb_platform_list_.begin();
		while (it != gb_platform_list_.end())
		{
			if (plt_id == (*it)->GetID())
			{
				(*it)->SetStatus(false);
				break;
			}
			++it;
		}
	}

	if (gb_platform_down_single_event_)
		gb_platform_down_single_event_->OnGBPlatformDownSingleEvent_Unregistered(plt_id);
}

int GBPlatformDownSingle::SearchFile(const std::string &plt_id, const std::string &channel_id, const std::string &plt_ip, const int plt_port, const char *type, const char *start_time, const char *end_time)
{
	int sip_sn = ++sip_sn_;
	int ret = GB28181Utils::QueryRecordFile(exosip_, plt_id, plt_ip, plt_port, local_id_, sip_sn, channel_id, start_time, end_time, type, "", "", "");
	if (ret > 0)
		return sip_sn;
	else
		return -1;
}

int GBPlatformDownSingle::PlayByTime(const std::string &plt_id, const std::string &device_id, const std::string &channel_id, const std::string &plt_ip, const int plt_port, 
									 const int record_type, const std::string &start_time, const std::string &end_time, const unsigned long handle, VideoCallback *video_cb)
{
	int cid = -1;
	StreamRecver *stream_recver = new StreamRecver(video_cb);
	if (stream_recver == NULL)
		return cid;

	stream_recver->SetLocalIP(local_ip_);
	stream_recver->SetStreamEventBase(stream_event_base_);

	int video_port = stream_recver->Open(GB_VIDEO);
	if (video_port < 0)
	{
		SDK_LOG("[PlayByTime] Open Video StreamRecver[%s@%s_%s] failed\n", plt_id.c_str(), device_id.c_str(), channel_id.c_str());
		delete stream_recver;
		return cid;
	}

	cid = GB28181Utils::Playback(exosip_, channel_id, plt_ip, plt_port, local_id_, local_ip_, record_type, start_time, end_time, video_port, 0);
	if (cid <= 0)
	{
		SDK_LOG("[PlayByTime] GB28181Utils::Playback[%s@%s_%s] failed\n", plt_id.c_str(), device_id.c_str(), channel_id.c_str());
		return cid;
	}

	{
		KAutoLock lock(playback_cs_);
		stream_recver->SetCallID(cid);
		stream_recver->SetGBPlatformID(plt_id);
		stream_recver->SetGBDeviceID(device_id);
		stream_recver->SetGBChannelID(channel_id);
		stream_recver->SetPlaybackHandle(handle);
		playback_map_[cid] = stream_recver;
	}

	return cid;
}

void GBPlatformDownSingle::PlayByAbsoluteTime(const int cid, const int did, const int npt)
{
	{
		KAutoLock lock(playback_cs_);
		CidStreamRecverMap::iterator it = playback_map_.find(cid);
		if (it == playback_map_.end())
		{
			SDK_LOG("[GBPlatformDownMulti] PlayByAbsoluteTime cid[%d] did[%d] not found\n", cid, did);
			return ;
		}
	}

	GB28181Utils::PlayByAbsoluteTime(exosip_, did, npt);
}

void GBPlatformDownSingle::PlaybackStop(const int cid, const int did)
{
	{
		KAutoLock lock(playback_cs_);
		CidStreamRecverMap::iterator it = playback_map_.find(cid);
		if (it != playback_map_.end())
		{
			it->second->Close();
			delete it->second;
			playback_map_.erase(it);
		}
		else
		{
			SDK_LOG("[GBPlatformDownMulti] PlaybackStop cid[%d] did[%d] not found\n", cid, did);
			return ;
		}
	}

	GB28181Utils::StopPlay(exosip_, cid, did);
}

void GBPlatformDownSingle::PlaybackPause(const int cid, const int did, const bool pause)
{
	{
		KAutoLock lock(playback_cs_);
		CidStreamRecverMap::iterator it = playback_map_.find(cid);
		if (it == playback_map_.end())
		{
			SDK_LOG("[GBPlatformDownMulti] PlaybackPause cid[%d] did[%d] not found\n", cid, did);
			return ;
		}
	}

	GB28181Utils::PlaybackPause(exosip_, did, pause);
}

void GBPlatformDownSingle::PlaybackSpeed(const int cid, const int did, const double speed)
{
	{
		KAutoLock lock(playback_cs_);
		CidStreamRecverMap::iterator it = playback_map_.find(cid);
		if (it == playback_map_.end())
		{
			SDK_LOG("[GBPlatformDownMulti] PlaybackSpeed cid[%d] did[%d] not found\n", cid, did);
			return ;
		}
	}

	GB28181Utils::PlaybackSpeed(exosip_, did, speed);
}

int GBPlatformDownSingle::DownloadByTime(const std::string &plt_id, const std::string &device_id, const std::string &channel_id, const std::string &plt_ip, const int plt_port, 
										 const int record_type, const std::string &start_time, const std::string &end_time, const unsigned long handle, VideoCallback *video_cb)
{
	int cid = -1;
	StreamRecver *stream_recver = new StreamRecver(video_cb);
	if (stream_recver == NULL)
		return cid;

	stream_recver->SetLocalIP(local_ip_);
	stream_recver->SetStreamEventBase(stream_event_base_);

	int video_port = stream_recver->Open(GB_VIDEO);
	if (video_port < 0)
	{
		SDK_LOG("[DownloadByTime] Open Video StreamRecver[%s@%s_%s] failed\n", plt_id.c_str(), device_id.c_str(), channel_id.c_str());
		delete stream_recver;
		return cid;
	}

	cid = GB28181Utils::Download(exosip_, channel_id, plt_ip, plt_port, local_id_, local_ip_, record_type, start_time, end_time, video_port, 0);
	if (cid <= 0)
	{
		SDK_LOG("[DownloadByTime] GB28181Utils::Download[%s@%s_%s] failed\n", plt_id.c_str(), device_id.c_str(), channel_id.c_str());
		return cid;
	}

	{
		KAutoLock lock(playback_cs_);
		stream_recver->SetCallID(cid);
		stream_recver->SetGBPlatformID(plt_id);
		stream_recver->SetGBDeviceID(device_id);
		stream_recver->SetGBChannelID(channel_id);
		stream_recver->SetPlaybackHandle(handle);
		playback_map_[cid] = stream_recver;
	}

	return cid;
}

bool GBPlatformDownSingle::OnEXosipThreadEvent_IsUserRegistered(const std::string &plt_id)
{
	KAutoLock lock(gb_platform_cs_);
	GBPlatformList::iterator it = gb_platform_list_.begin();
	while (it != gb_platform_list_.end())
	{
		if (plt_id == (*it)->GetID())
		{	
			if ((*it)->GetStatus())
				return true;
			else
				return false;
		}
		else
			++it;
	}
	return false;
}

bool GBPlatformDownSingle::OnEXosipThreadEvent_IsUserRegisteredByOtherIP(const std::string &plt_id, const std::string &plt_ip, const int plt_port)
{
	GBPlatform *gb_platform = FindGBPlatform(plt_id);
	if (gb_platform)
	{
		if (gb_platform->GetIP() != plt_ip || gb_platform->GetPort() != plt_port)
			return true;
	}
	return false;
}

bool GBPlatformDownSingle::OnEXosipThreadEvent_IsPasswordReady(const std::string &plt_id)
{
	if (gb_platform_down_single_event_)
		return gb_platform_down_single_event_->OnGBPlatformDownSingleEvent_IsPasswordReady(plt_id);
	return false;
}

void GBPlatformDownSingle::OnEXosipThreadEvent_QueryPassword(const std::string &plt_id, std::string &plt_pwd)
{
	if (gb_platform_down_single_event_)
		return gb_platform_down_single_event_->OnGBPlatformDownSingleEvent_QueryPassword(plt_id, plt_pwd);
}

void GBPlatformDownSingle::OnEXosipThreadEvent_Registered(const int reg_cseq, const std::string &plt_id, const std::string &plt_pwd, const std::string &plt_ip, const int plt_port, const int expires)
{
	GBPlatform *gb_platform = FindGBPlatform(plt_id);
	if (gb_platform == NULL)
	{
		gb_platform = new GBPlatform(this);
		if (gb_platform)
		{
			gb_platform->SetID(plt_id);
			gb_platform->SetIP(plt_ip);
			gb_platform->SetPort(plt_port);
			gb_platform->SetStatus(true);

			gb_platform->StartTimer();

			{
				KAutoLock lock(gb_platform_cs_);
				gb_platform_list_.push_back(gb_platform);
			}
		}
	}
	else
	{
		gb_platform->SetStatus(true);
	}

	if (reg_cseq == 2)
	{
		GB28181Utils::Query(exosip_, plt_id, plt_ip, plt_port, local_id_, ++sip_sn_, plt_id, "Catalog");
		GB28181Utils::SubscribeCatalog(exosip_, plt_id, plt_ip, plt_port, local_id_, ++sip_sn_, plt_id, SUBSCRIBE_CATALOG_EXPIRES);
		SDK_LOG("Subscribe catalog of [%s]\n", plt_id.c_str());

		if (gb_platform_down_single_event_)
			gb_platform_down_single_event_->OnGBPlatformDownSingleEvent_Registered(plt_id, plt_pwd, plt_ip, plt_port);
	}
}

void GBPlatformDownSingle::OnEXosipThreadEvent_UnRegistered(const std::string &plt_id)
{
	GBPlatform *gb_platform = FindGBPlatform(plt_id);
	if (gb_platform)
	{
		gb_platform->SetStatus(false);
	
		eXosip_lock(exosip_);
		eXosip_subscribe_remove(exosip_, gb_platform->GetSubDid());
		eXosip_unlock(exosip_);
	}

	DestroyGBDeviceVideoMapWhichPlatformID(plt_id);
	if (gb_platform_down_single_event_)
		gb_platform_down_single_event_->OnGBPlatformDownSingleEvent_Unregistered(plt_id);
}

void GBPlatformDownSingle::OnEXosipThreadEvent_SubscribeAnswered(const std::string &plt_id, const int did)
{
	GBPlatform *gb_platform = FindGBPlatform(plt_id);
	if (gb_platform)
	{
		gb_platform->SetSubDid(did);
		if (gb_platform->GetStatus())
			gb_platform->SetTimerEvent(did, SUBSCRIBE_CATALOG_EXPIRES * 1000);
	}
}

void GBPlatformDownSingle::OnEXosipThreadEvent_Keeplive(const Keepalive &keepalive)
{
	IDTimeType tIDTime;
	tIDTime.id = keepalive.device_id;
	tIDTime.time = time(NULL);

	gb_check_heartbeat_->SetIDTime(keepalive.device_id, tIDTime);
}

void GBPlatformDownSingle::OnEXosipThreadEvent_Catalog(const std::string &plt_id, Catalog &catalog)
{
	GBPlatform *gb_platform = FindGBPlatform(plt_id);
	if (gb_platform == NULL)
		return;

	if (plt_id == catalog.device_id)
	{
		std::list<CatalogItem>::iterator item_it = catalog.device_list.begin();

		for(; item_it != catalog.device_list.end(); item_it++)
		{
			if(!GB28181Utils::IsLeafNodeDevice(item_it->device_id))
				continue;

			item_it->ip_address = gb_platform->GetIP();
			item_it->port = gb_platform->GetPort();

			gb_platform->CreateGBDevice(plt_id, *item_it);
			gb_platform_down_single_event_->OnGBPlatformDownSingleEvent_DeviceCatalog(plt_id, *item_it);
			gb_platform_down_single_event_->OnGBPlatformDownSingleEvent_ChannelCatalog(plt_id, *item_it);
		}
	}
}

void GBPlatformDownSingle::OnEXosipThreadEvent_DeviceInfo(const DeviceInfo &device_info)
{

}

void GBPlatformDownSingle::OnEXosipThreadEvent_DeviceStatus(const DeviceStatus &device_status)
{
	if (device_status.status == "ON")
	{
		gb_platform_down_single_event_->OnGBPlatformDownSingleEvent_DeviceOnline(device_status.platform_id, device_status.device_id);
	}
	else if (device_status.status == "OFF")
	{
		gb_platform_down_single_event_->OnGBPlatformDownSingleEvent_DeviceOffline(device_status.platform_id, device_status.device_id);
	}
}

void GBPlatformDownSingle::OnEXosipThreadEvent_RecordInfo(const RecordInfo &record_info)
{
	if (gb_platform_down_single_event_)
		gb_platform_down_single_event_->OnGBPlatformDownSingleEvent_RecordInfo(record_info);
}

void GBPlatformDownSingle::OnEXosipThreadEvent_NotifyCatalog(const std::string &plt_id, NotifyCatalog &notify_catalog)
{
	GBPlatform *gb_platform = FindGBPlatform(plt_id);
	if (gb_platform == NULL)
		return;

	std::vector<NotifyCatalogItem>::iterator item_it = notify_catalog.device_list.begin();

	for(; item_it != notify_catalog.device_list.end(); item_it++)
	{
		if (gb_platform_down_single_event_)
		{
			if (item_it->event == "ON")
			{
				gb_platform_down_single_event_->OnGBPlatformDownSingleEvent_DeviceOnline(plt_id, item_it->catalog_item.device_id);
			}
			else if (item_it->event == "OFF")
			{
				gb_platform_down_single_event_->OnGBPlatformDownSingleEvent_DeviceOffline(plt_id, item_it->catalog_item.device_id);
			}
			else if (item_it->event == "ADD")
			{
				if(!GB28181Utils::IsLeafNodeDevice(item_it->catalog_item.device_id))
					continue;

				item_it->catalog_item.ip_address = gb_platform->GetIP();
				item_it->catalog_item.port = gb_platform->GetPort();

				gb_platform->CreateGBDevice(plt_id, item_it->catalog_item);
		
				gb_platform_down_single_event_->OnGBPlatformDownSingleEvent_DeviceAdd(plt_id, item_it->catalog_item);
			}
			else if (item_it->event == "DEL")
			{
				gb_platform_down_single_event_->OnGBPlatformDownSingleEvent_DeviceDel(plt_id, item_it->catalog_item.device_id);
			}
		}
	}
}

void GBPlatformDownSingle::OnEXosipThreadEvent_StartPlaySucceeded(const int cid, const int did, const int stream_type, const int encoding_type, const int ssrc)
{
	if (gb_platform_down_single_event_)
		gb_platform_down_single_event_->OnGBPlatformDownSingleEvent_StartPlaySucceeded(cid, did, stream_type, encoding_type);
}

void GBPlatformDownSingle::OnEXosipThreadEvent_StartPlayFailed(const int cid)
{
	{
		KAutoLock lock(gb_device_video_cs_);
		CidStreamRecverMap::iterator it = gb_device_video_map_.find(cid);
		if (it != gb_device_video_map_.end())
		{
			StreamRecver *stream_recver = it->second;
			if (stream_recver)
				delete stream_recver;

			gb_device_video_map_.erase(it);
		}
		else
			return;
	}

	if (gb_platform_down_single_event_)
			gb_platform_down_single_event_->OnGBPlatformDownSingleEvent_StartPlayFailed(cid);
}

void GBPlatformDownSingle::OnEXosipThreadEvent_TcpRequest(const int cid, const int did)
{

}

void GBPlatformDownSingle::OnEXosipThreadEvent_StartTalkSucceeded(const int cid, const int did, const int encoding_type, const std::string &dest_ip, const unsigned short dest_port)
{
	{
		KAutoLock lock(gb_device_talk_cs_);
		if (gb_device_talk_map_[cid])
		{
			gb_device_talk_map_[cid]->dest_audio_ip_ = dest_ip;
			gb_device_talk_map_[cid]->dest_audio_port_ = dest_port;
			gb_device_talk_map_[cid]->SetCallID(cid);
		}
	}

	if (gb_platform_down_single_event_)
		gb_platform_down_single_event_->OnGBPlatformDownSingleEvent_StartTalkSucceeded(cid, did, encoding_type);
}

void GBPlatformDownSingle::OnEXosipThreadEvent_StartTalkFailed(const int cid)
{
	{
		KAutoLock lock(gb_device_talk_cs_);
		CidGBDeviceTalkMap::iterator it = gb_device_talk_map_.find(cid);
		if (it != gb_device_talk_map_.end())
		{
			GBDeviceTalk *gb_device_talk = it->second;
			if (gb_device_talk)
				delete gb_device_talk;

			gb_device_talk_map_.erase(it);
		}
		else
			return;
	}

	if (gb_platform_down_single_event_)
		gb_platform_down_single_event_->OnGBPlatformDownSingleEvent_StartTalkFailed(cid);
}

void GBPlatformDownSingle::OnEXosipThreadEvent_PlaybackSucceeded(const int cid, const int did, const int stream_type, const int encoding_type, const int ssrc)
{
	unsigned long handle = 0;
	{
		KAutoLock lock(playback_cs_);
		if (playback_map_[cid])
		{
			handle = playback_map_[cid]->GetPlaybackHandle();
		}
	}

	if (gb_platform_down_single_event_)
		gb_platform_down_single_event_->OnGBPlatformDownSingleEvent_PlaybackSucceeded(handle, cid, did, stream_type, encoding_type);
}

void GBPlatformDownSingle::OnEXosipThreadEvent_PlaybackFailed(const int cid)
{
	unsigned long handle = 0;
	{
		KAutoLock lock(playback_cs_);
		CidStreamRecverMap::iterator it = playback_map_.find(cid);
		if (it != playback_map_.end())
		{
			handle = it->second->GetPlaybackHandle();

			StreamRecver *stream_recver = it->second;
			if (stream_recver)
				delete stream_recver;

			playback_map_.erase(it);
		}
		else
			return;
	}

	if (gb_platform_down_single_event_)
		gb_platform_down_single_event_->OnGBPlatformDownSingleEvent_PlaybackFailed(handle);
}


void GBPlatformDownSingle::SubscribeRefreshCatalog(const int did, const int expires, const std::string &device_id)
{
	GB28181Utils::SubscribeRefreshCatalog(exosip_, did, expires, ++sip_sn_, device_id);
}