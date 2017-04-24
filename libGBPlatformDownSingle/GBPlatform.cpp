#include "stdafx.h"
#include <KBASE/AutoLock.h>
#include "GB28181Utils.h"
#include "GBPlatform.h"

GBPlatform::GBPlatform(GBPlatformCallback *gb_platform_cb):
	gb_platform_callback_(gb_platform_cb)
{
	
}

GBPlatform::~GBPlatform()
{
	//KAutoLock lock(gb_device_cs_);
	GBDeviceMap::iterator it = gb_device_map_.begin();
	while (it != gb_device_map_.end())
	{
		if (it->second)
			delete it->second;
		++it;
	}
	gb_device_map_.clear();
}

bool GBPlatform::CheckGBDeviceIn(const std::string &id)
{
	KAutoLock lock(gb_device_cs_);
	GBDeviceMap::iterator it = gb_device_map_.find(id);
	if(it != gb_device_map_.end())
	{
		return true;
	}
	return false;
}

void GBPlatform::CreateGBDevice(const std::string &device_id, CatalogItem &catalog_item)
{
	KAutoLock lock(gb_device_cs_);
	GBDeviceMap::iterator it = gb_device_map_.find(catalog_item.device_id);
	if(it != gb_device_map_.end())
	{
		it->second->set_catalog_item(catalog_item);
		return;
	}

	GBDevice *gb_device  = new GBDevice();
	if (gb_device)
	{
		gb_device->set_catalog_item(catalog_item);
		gb_device_map_[catalog_item.device_id] = gb_device;
	}
}

void GBPlatform::DestroyGBDevices()
{
	KAutoLock lock(gb_device_cs_);
	for (GBDeviceMap::iterator it = gb_device_map_.begin(); it != gb_device_map_.end(); ++it)
	{
		GBDevice *gb_device = it->second;
		if (gb_device)
		{
			delete gb_device;
			gb_device = NULL;
		}
	}
	gb_device_map_.clear();
}


void GBPlatform::OnTimerEvent(unsigned int nEventID)
{
	if (nEventID == sub_did_)
		gb_platform_callback_->SubscribeRefreshCatalog(nEventID, SUBSCRIBE_CATALOG_EXPIRES, id_);
}