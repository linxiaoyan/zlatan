#include "AlarmSingleton.h"

std::string AlarmSingleton::GetAlarmServerIP()
{
	std::string strSrvAddr = "";

	XMLNode xConfigNode = XMLNode::OpenFile(strConfigFile.c_str(),"config");
	if(xConfigNode.IsEmpty())
	{
		printf("[AlarmSingleton] config error <config>\n");
		return strSrvAddr;
	}

	char szSrvAddr[64];

	XMLNode xAlarmNode = xConfigNode.GetChildNode("alarm");
	if(!xAlarmNode.IsEmpty())
	{
		XMLNode xServeripNode = xAlarmNode.GetChildNode("alarmsrvip");
		if(xServeripNode.IsEmpty())
		{
			printf("[AlarmSingleton] config error <config/alarm/alarmsrvip>.\n");
			return strSrvAddr;
		}

		SAFE_STRCPY(szSrvAddr, NONULLSTR(xServeripNode.GetText()).c_str());
		strSrvAddr = szSrvAddr;
	}

	return strSrvAddr;
}

unsigned int AlarmSingleton::GetAlarmServerPort()
{
	unsigned int nPort = 0;

	XMLNode xConfigNode = XMLNode::OpenFile(strConfigFile.c_str(),"config");
	if(xConfigNode.IsEmpty())
	{
		printf("[AlarmSingleton] config error <config>\n");
		return nPort;
	}

	XMLNode xAlarmNode = xConfigNode.GetChildNode("alarm");
	if(!xAlarmNode.IsEmpty())
	{
		XMLNode xPortNode = xAlarmNode.GetChildNode("alarmsrvport");
		if(xPortNode.IsEmpty())
		{
			printf("[AlarmSingleton] config error <config/alarm/alarmsrvport>.\n");
			return nPort;
		}

		nPort = STR2UINT(NONULLSTR(xPortNode.GetText()));
	}

	return nPort;
}