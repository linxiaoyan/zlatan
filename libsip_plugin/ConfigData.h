#ifndef CONFIGDATA_H
#define CONFIGDATA_H
#include <string>
typedef struct tagDevicePlugin
{
	std::string strLocalIP;
	int nLocalPort;
	std::string strLocalID;
	std::string strMapIP;
	int nMapPort;
	int nOsipTraceLevel;
	bool bStaticPwd;
}DevicePlugin;

typedef struct tagPlatformPlugin
{
	std::string strLocalIP;
	int nLocalPort;
	std::string strLocalID;
	std::string strMapIP;
	int nMapPort;
	int nOsipTraceLevel;
	bool bStaticPwd;
}PlatformPlugin;

class ConfigData
{
private:
	ConfigData();
public:
	~ConfigData();
	static ConfigData * GetInstance();

	DevicePlugin m_DevicePluginInfo;
	PlatformPlugin m_PlatformPluginInfo;
private:
	static ConfigData m_ConfigData;
};
#endif