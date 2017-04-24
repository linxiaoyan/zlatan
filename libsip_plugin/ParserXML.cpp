#include "stdafx.h"
#include "ParserXML.h"

bool ParserXML::ParserFile(const std::string & strFileName,ConfigData & rConfigData)
{
	XMLNode xConfigNode = XMLNode::OpenFile(strFileName.c_str(),"config");
	if(xConfigNode.IsEmpty())
	{
		printf("[GB28181]: config error: <config>[ParserXML::ParserFile].[ParserXML::ParserFile]\n");
		return false;
	}

	//device
	XMLNode xDeviceNode = xConfigNode.GetChildNode("device");
	if(!xDeviceNode.IsEmpty())
	{
		//config/device/localip
		XMLNode xIPNode = xDeviceNode.GetChildNode("localip");
		if(xIPNode.IsEmpty())
		{
			printf("[GB28181]: config error <config/device/localip>.[ParserXML::ParserFile]\n");
			return false;
		}
		rConfigData.m_DevicePluginInfo.strLocalIP = NONULLSTR(xIPNode.GetText());

		//config/device/localport
		XMLNode xPortNode = xDeviceNode.GetChildNode("localport");
		if(xPortNode.IsEmpty())
		{
			printf("[GB28181]: config error <config/device/localport>.[ParserXML::ParserFile]\n");
			return false;
		}
		rConfigData.m_DevicePluginInfo.nLocalPort= STR2INT(NONULLSTR(xPortNode.GetText()));

		//config/device/id
		XMLNode xIDNode = xDeviceNode.GetChildNode("id");
		if(xIDNode.IsEmpty())
		{
			printf("[GB28181]: config error <config/device/id>.[ParserXML::ParserFile]\n");
			return false;
		}
		rConfigData.m_DevicePluginInfo.strLocalID = NONULLSTR(xIDNode.GetText());

		//config/device/mapip
		XMLNode xMapIPNode = xDeviceNode.GetChildNode("mapip");
		if(xMapIPNode.IsEmpty())
		{
			printf("[GB28181]: config error <config/device/mapip>.[ParserXML::ParserFile]\n");
			return false;
		}
		rConfigData.m_DevicePluginInfo.strMapIP = NONULLSTR(xMapIPNode.GetText());

		//config/device/mapport
		XMLNode xMapPortNode = xDeviceNode.GetChildNode("mapport");
		if(xMapPortNode.IsEmpty())
		{
			printf("[GB28181]: config error <config/device/mapport>.[ParserXML::ParserFile]\n");
			return false;
		}
		rConfigData.m_DevicePluginInfo.nMapPort = STR2INT(NONULLSTR(xMapPortNode.GetText()));

		//config/device/tracelevel
		XMLNode xOsipTraceLevelNode = xDeviceNode.GetChildNode("tracelevel");
		if(xOsipTraceLevelNode.IsEmpty())
		{
			printf("[GB28181]: config error <config/device/tracelevel>.[ParserXML::ParserFile]\n");
			return false;
		}
		rConfigData.m_DevicePluginInfo.nOsipTraceLevel = STR2INT(NONULLSTR(xOsipTraceLevelNode.GetText()));

		//config/device/staticpwd
		XMLNode xStaticPwdNode = xDeviceNode.GetChildNode("staticpwd");
		if(xStaticPwdNode.IsEmpty())
		{
			printf("[GB28181]: config error <config/device/staticpwd>.[ParserXML::ParserFile]\n");
			return false;
		}
		rConfigData.m_DevicePluginInfo.bStaticPwd = STR2INT(NONULLSTR(xStaticPwdNode.GetText()))==0 ? false : true;

	}

	XMLNode xPlatformNode = xConfigNode.GetChildNode("platform");
	if(!xPlatformNode.IsEmpty())
	{
		//config/device/localip
		XMLNode xIPNode = xPlatformNode.GetChildNode("localip");
		if(xIPNode.IsEmpty())
		{
			printf("[GB28181]: config error <config/platform/localip>.[ParserXML::ParserFile]\n");
			return false;
		}
		rConfigData.m_PlatformPluginInfo.strLocalIP = NONULLSTR(xIPNode.GetText());

		//config/platform/localport
		XMLNode xPortNode = xPlatformNode.GetChildNode("localport");
		if(xPortNode.IsEmpty())
		{
			printf("[GB28181]: config error <config/platform/localport>.[ParserXML::ParserFile]\n");
			return false;
		}
		rConfigData.m_PlatformPluginInfo.nLocalPort= STR2INT(NONULLSTR(xPortNode.GetText()));

		//config/platform/id
		XMLNode xIDNode = xPlatformNode.GetChildNode("id");
		if(xIDNode.IsEmpty())
		{
			printf("[GB28181]: config error <config/platform/id>.[ParserXML::ParserFile]\n");
			return false;
		}
		rConfigData.m_PlatformPluginInfo.strLocalID = NONULLSTR(xIDNode.GetText());

		//config/platform/mapip
		XMLNode xMapIPNode = xPlatformNode.GetChildNode("mapip");
		if(xMapIPNode.IsEmpty())
		{
			printf("[GB28181]: config error <config/platform/mapip>.[ParserXML::ParserFile]\n");
			return false;
		}
		rConfigData.m_PlatformPluginInfo.strMapIP = NONULLSTR(xMapIPNode.GetText());

		//config/platform/mapport
		XMLNode xMapPortNode = xPlatformNode.GetChildNode("mapport");
		if(xMapPortNode.IsEmpty())
		{
			printf("[GB28181]: config error <config/platform/mapport>.[ParserXML::ParserFile]\n");
			return false;
		}
		rConfigData.m_PlatformPluginInfo.nMapPort = STR2INT(NONULLSTR(xMapPortNode.GetText()));

		//config/platform/tracelevel
		XMLNode xOsipTraceLevelNode = xPlatformNode.GetChildNode("tracelevel");
		if(xOsipTraceLevelNode.IsEmpty())
		{
			printf("[GB28181]: config error <config/platform/tracelevel>.[ParserXML::ParserFile]\n");
			return false;
		}
		rConfigData.m_PlatformPluginInfo.nOsipTraceLevel = STR2INT(NONULLSTR(xOsipTraceLevelNode.GetText()));

		//config/platform/staticpwd
		XMLNode xStaticPwdNode = xPlatformNode.GetChildNode("staticpwd");
		if(xStaticPwdNode.IsEmpty())
		{
			printf("[GB28181]: config error <config/platform/staticpwd>.[ParserXML::ParserFile]\n");
			return false;
		}
		rConfigData.m_PlatformPluginInfo.bStaticPwd = STR2INT(NONULLSTR(xStaticPwdNode.GetText()))==0 ? false : true;

	}
	return true;
}

bool ParserXML::GetChildNodeValue(const XMLNode & xParent,const char * szChildName,std::string & strRetValue)
{
	strRetValue="";
	XMLNode xChildNode = xParent.GetChildNode(szChildName);
	if(xChildNode.IsEmpty())
	{
		return false;
	}
	strRetValue=NONULLSTR(xChildNode.GetText());
	return true;
}