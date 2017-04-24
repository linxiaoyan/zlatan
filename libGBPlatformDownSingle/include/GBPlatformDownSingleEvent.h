#ifndef _GB_PLATFORM_DOWN_SINGLE_EVENT_H_
#define _GB_PLATFORM_DOWN_SINGLE_EVENT_H_

#include "GBDefines.h"

class GB28181_API GBPlatformDownSingleEvent
{
public:
	virtual ~GBPlatformDownSingleEvent(){}

	virtual bool OnGBPlatformDownSingleEvent_IsPasswordReady(const std::string &plt_id)=0;
	virtual void OnGBPlatformDownSingleEvent_QueryPassword(const std::string &plt_id, std::string &plt_pwd)=0;
	virtual void OnGBPlatformDownSingleEvent_Registered(const std::string &plt_id, const std::string &plt_pwd, const std::string &plt_ip, const int plt_port)=0;
	virtual void OnGBPlatformDownSingleEvent_Unregistered(const std::string &plt_id)=0;

	virtual void OnGBPlatformDownSingleEvent_DeviceCatalog(const std::string &plt_id, const CatalogItem &catalog_item)=0;
	virtual void OnGBPlatformDownSingleEvent_ChannelCatalog(const std::string &plt_id, const CatalogItem &catalog_item)=0;
	
	virtual void OnGBPlatformDownSingleEvent_StartPlaySucceeded(const int cid, const int did, const int stream_type, const int encoding_type)=0;
	virtual void OnGBPlatformDownSingleEvent_StartPlayFailed(const int cid)=0;
	
	virtual void OnGBPlatformDownSingleEvent_StartTalkSucceeded(const int cid, const int did, const int encoding_type)=0;
	virtual void OnGBPlatformDownSingleEvent_StartTalkFailed(const int cid)=0;

	virtual void OnGBPlatformDownSingleEvent_PlaybackSucceeded(unsigned long handle, const int cid, const int did, const int stream_type, const int encoding_type)=0;
	virtual void OnGBPlatformDownSingleEvent_PlaybackFailed(unsigned long handle)=0;

	virtual void OnGBPlatformDownSingleEvent_DeviceOnline(const std::string &plt_id, const std::string &device_id)=0;
	virtual void OnGBPlatformDownSingleEvent_DeviceOffline(const std::string &plt_id, const std::string &device_id)=0;
	virtual void OnGBPlatformDownSingleEvent_ChannelOnline(const std::string &plt_id, const std::string &device_id, const std::string &channel_id)=0;
	virtual void OnGBPlatformDownSingleEvent_ChannelOffline(const std::string &plt_id, const std::string &device_id, const std::string &channel_id)=0;
	virtual void OnGBPlatformDownSingleEvent_DeviceUpdate(const Catalog &catalog)=0;
	virtual void OnGBPlatformDownSingleEvent_DeviceAdd(const std::string &plt_id, const CatalogItem &catalog_item)=0;
	virtual void OnGBPlatformDownSingleEvent_DeviceDel(const std::string &plt_id, const std::string &device_id)=0;

	virtual void OnGBPlatformDownSingleEvent_RecordInfo(const RecordInfo &record_info)=0;
};

#endif