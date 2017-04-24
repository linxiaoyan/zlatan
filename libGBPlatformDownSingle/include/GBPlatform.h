#ifndef _GB_PLATFORM_H_
#define _GB_PLATFORM_H_

#include <KBASE/Timer.h>
#include <KBASE/CritSec.h>
#include "GBDefines.h"
#include "GBDevice.h"
#include "GBPlatformCallback.h"

#define SUBSCRIBE_CATALOG_EXPIRES 3600

typedef std::map<std::string, GBDevice*> GBDeviceMap;

class GBPlatform:public KTimer
{
public:
	GBPlatform(GBPlatformCallback *gb_platform_cb);
	~GBPlatform();

	void CreateGBDevice(const std::string &device_id, CatalogItem &catalog_item);
	bool CheckGBDeviceIn(const std::string &device_id);
	void DestroyGBDevices();

	void SetID(const std::string &id) {id_ = id;}
	void SetIP(const std::string &ip) {ip_ = ip;}
	void SetPort(const int port) {port_ = port;}
	void SetStatus(bool status) {status_ = status;}
	std::string GetID() {return id_;}
	std::string GetIP() {return ip_;}
	int  GetPort() {return port_;}
	bool GetStatus() {return status_;}
	void SetSubDid(const int did) {sub_did_ = did;}
	int  GetSubDid() {return sub_did_;}

	virtual void OnTimerEvent(unsigned int nEventID);

private:
	GBPlatformCallback *gb_platform_callback_;

	std::string id_;
	std::string ip_;
	int  port_;
	bool status_; //true means online, false means offline
	int  sub_did_;

	KCritSec gb_device_cs_;
	GBDeviceMap gb_device_map_;

	DeviceInfo   platform_info_;
	DeviceStatus platform_status_;
};
#endif