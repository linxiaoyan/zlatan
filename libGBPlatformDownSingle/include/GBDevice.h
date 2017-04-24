#ifndef _GB_DEVICE_H_
#define _GB_DEVICE_H_

#include "GBDefines.h"

#define Gb28181_DEVICE_ONLINE 1
#define Gb28181_DEVICE_OFFLINE 2
#define Gb28181_DEVICE_TIMEOUT 3

class GBDevice
{
public:
	GBDevice()
	{
		timeout_ = false;
		ask_status_time_ = time(NULL);
		last_online_time_ = time(NULL);
	}
	
	~GBDevice(){}

	std::string get_device_id()
	{
		return catalog_item_.device_id;
	}

	void set_catalog_item(const CatalogItem &catalog_item)
	{
		catalog_item_ = catalog_item;
	}

	void set_last_online_time(time_t last_online_time)
	{
		last_online_time_ = last_online_time;
	}

private:
	CatalogItem  catalog_item_;
	time_t ask_status_time_;
	time_t last_online_time_;
	bool   timeout_;
};

#endif