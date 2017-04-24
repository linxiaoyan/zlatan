#ifndef _GB_PLATFORM_CALLBACK_H_
#define _GB_PLATFORM_CALLBACK_H_

class GBPlatformCallback
{
public:
	virtual ~GBPlatformCallback() {}

	virtual void SubscribeRefreshCatalog(const int did, const int expires, const std::string &device_id)=0;
};

#endif