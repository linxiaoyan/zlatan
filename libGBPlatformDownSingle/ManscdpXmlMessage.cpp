#include "stdafx.h"
#include "osipparser2/osip_port.h"
#include "ManscdpXmlMessage.h"
#include "GBDefines.h"


MANSCDP_XML_MESSAGE::MANSCDP_XML_MESSAGE(EXosipThreadEvent *exosip_thread_event):exosip_thread_event_(exosip_thread_event)
{
}

int MANSCDP_XML_MESSAGE::Parse(const char *xml_string)
{
	int ret = -1;
	XML_RESULTS xml_result;
	XMLNode xml_node = XMLNode::ParseString(xml_string, NULL, &xml_result);
	
	if(xml_result.error != eXMLErrorNone)
	{
		SDK_LOG("[MANSCDP_XML] ParseString failed\n");
		return ret;
	}

	XMLNode root_node  = xml_node.GetChildNode();
	if(root_node.IsEmpty())
		return ret;

	if(osip_strcasecmp(root_node.GetName(), NODE_NAME_RESPONCES) == 0)
	{
		XMLNode node = root_node.GetChildNode(NODE_NAME_CMDTYPE);
		if(node.IsEmpty())
			return ret;
		
		if (osip_strcasecmp(node.GetText(), CMD_TYPE_VALUE_CATALOG) == 0)
		{
			ret = ParseResponseCatalog(root_node);
		}
		else if (osip_strcasecmp(node.GetText(), CMD_TYPE_VALUE_DEVICE_STATUS) == 0)
		{
			ret = ParseDeviceStatus(root_node);
		}
		else if (osip_strcasecmp(node.GetText(), CMD_TYPE_VALUE_DEVICE_INFO) == 0)
		{
			ret = ParseDeviceInfo(root_node);
		}
		else if (osip_strcasecmp(node.GetText(), CMD_TYPE_VALUE_RECORD_INFO) == 0)
		{
			ret = ParseRecordInfo(root_node);
		}
	}
	else if(osip_strcasecmp(root_node.GetName(),NODE_NAME_NOTIFY) == 0)
	{
		XMLNode node =  root_node.GetChildNode(NODE_NAME_CMDTYPE);
		if(node.IsEmpty())
			return ret;

		if (osip_strcasecmp(node.GetText(), CMD_TYPE_VALUE_CATALOG) == 0)
		{
			ret = ParseNotifyCatalog(root_node);
		}
		else if (osip_strcasecmp(node.GetText(), CMD_TYPE_VALUE_KEEPLIVE) == 0)
		{
			ret = ParseKeeplive(root_node);
		}
	}

	return ret;
}

int MANSCDP_XML_MESSAGE::ParseResponseCatalog(XMLNode &parent_node)
{
	Catalog catalog;
	catalog.sn = atoi(GetChildNodeText(parent_node, "SN").c_str());
	catalog.device_id = GetChildNodeText(parent_node, "DeviceID");
	catalog.sum_num = atoi(GetChildNodeText(parent_node, "SumNum").c_str());

	XMLNode devicelist_node = parent_node.GetChildNode("DeviceList");
	XMLNode item_node;
	if (!devicelist_node.IsEmpty())
	{
		int item_count= devicelist_node.nChildNode("Item");
		for (int i=0; i < item_count; i++)
		{
			item_node = devicelist_node.GetChildNode("Item", i);
			CatalogItem catalog_item;
			catalog_item.device_id = GetChildNodeText(item_node,"DeviceID");
			catalog_item.name = GetChildNodeText(item_node,"Name");
			catalog_item.manufacturer = GetChildNodeText(item_node,"Manufacturer");
			catalog_item.model = GetChildNodeText(item_node,"Model");
			catalog_item.owner = GetChildNodeText(item_node,"Owner");
			catalog_item.civil_code = GetChildNodeText(item_node,"CivilCode");
			catalog_item.block = GetChildNodeText(item_node,"Block");
			catalog_item.address = GetChildNodeText(item_node,"Address");
			catalog_item.parental = atoi(GetChildNodeText(item_node,"Parental").c_str());
			catalog_item.parent_id = GetChildNodeText(item_node,"ParentID");
			catalog_item.safety_way = atoi(GetChildNodeText(item_node,"SafetyWay").c_str());
			catalog_item.register_way = atoi(GetChildNodeText(item_node,"RegisterWay").c_str());
			catalog_item.cert_num = GetChildNodeText(item_node,"CertNum");
			catalog_item.certifiable = atoi(GetChildNodeText(item_node,"Certifiable").c_str());
			catalog_item.err_code = atoi(GetChildNodeText(item_node,"ErrCode").c_str());
			catalog_item.end_time = GetChildNodeText(item_node,"EndTime");
			catalog_item.secrecy = atoi(GetChildNodeText(item_node,"Secrecy").c_str());
			catalog_item.password = GetChildNodeText(item_node,"Password");
			catalog_item.status = GetChildNodeText(item_node, "Status");
			catalog_item.longitude = atof(GetChildNodeText(item_node,"Longitude").c_str());
			catalog_item.latitude = atof(GetChildNodeText(item_node,"Latitude").c_str());
			catalog.device_list.push_back(catalog_item);
		}
	}

	if (exosip_thread_event_)
		exosip_thread_event_->OnEXosipThreadEvent_Catalog(from_, catalog);
	return 0;
}

int MANSCDP_XML_MESSAGE::ParseDeviceStatus(XMLNode &parent_node)
{
	DeviceStatus device_status;
	device_status.sn = atoi(GetChildNodeText(parent_node, "SN").c_str());
	device_status.device_id = GetChildNodeText(parent_node, "DeviceID");
	device_status.result = GetChildNodeText(parent_node,"Result").c_str();
	device_status.online = GetChildNodeText(parent_node,"Online").c_str();
	device_status.status = GetChildNodeText(parent_node,"Status").c_str();
	device_status.reason = GetChildNodeText(parent_node,"Reason").c_str();
	device_status.encode = GetChildNodeText(parent_node,"Encode").c_str();
	device_status.recode = GetChildNodeText(parent_node,"Recode").c_str();
	device_status.device_time = GetChildNodeText(parent_node,"DeviceTime").c_str();
	device_status.platform_id = from_;
	
	if (exosip_thread_event_)
		exosip_thread_event_->OnEXosipThreadEvent_DeviceStatus(device_status);
	return 0;
}

int MANSCDP_XML_MESSAGE::ParseDeviceInfo(XMLNode &parent_node)
{
	DeviceInfo devince_info;
	devince_info.sn = atoi(GetChildNodeText(parent_node, "SN").c_str());
	devince_info.device_id = GetChildNodeText(parent_node, "DeviceID");
	devince_info.device_name = GetChildNodeText(parent_node,"DeviceName");
	devince_info.result = GetChildNodeText(parent_node,"Result");
	devince_info.device_type = GetChildNodeText(parent_node,"DeviceType");
	devince_info.manufacturer = GetChildNodeText(parent_node,"Manufacturer");
	devince_info.model = GetChildNodeText(parent_node,"Model");
	devince_info.firmware = GetChildNodeText(parent_node,"Firmware");

	if (exosip_thread_event_)
		exosip_thread_event_->OnEXosipThreadEvent_DeviceInfo(devince_info);
	return 0;
}

int MANSCDP_XML_MESSAGE::ParseRecordInfo(XMLNode &parent_node)
{
	RecordInfo record_info;
	record_info.sn = atoi(GetChildNodeText(parent_node,"SN").c_str());
	record_info.device_id = GetChildNodeText(parent_node,"DeviceID");
	record_info.name = GetChildNodeText(parent_node,"Name");
	record_info.sum_num = atoi(GetChildNodeText(parent_node,"SumNum").c_str());
	
	XMLNode recordListNode = parent_node.GetChildNode("RecordList");
	XMLNode item_node;
	if (!recordListNode.IsEmpty())
	{
		int item_count=recordListNode.nChildNode("Item");
		for (int i=0; i<item_count; i++)
		{
			item_node=recordListNode.GetChildNode("Item",i);
			RecordItem record_item;
			record_item.device_id = GetChildNodeText(item_node,"DeviceID");
			record_item.name = GetChildNodeText(item_node,"Name");
			record_item.file_path = GetChildNodeText(item_node,"FilePath");
			record_item.address = GetChildNodeText(item_node,"Address");
			record_item.start_time = GetChildNodeText(item_node,"StartTime");
			record_item.end_time = GetChildNodeText(item_node,"EndTime");
			record_item.secrecy = atoi(GetChildNodeText(item_node,"Secrecy").c_str());
			record_item.type = GetChildNodeText(item_node,"Type");
			record_item.recorder_id = GetChildNodeText(item_node,"RecorderID");
			record_info.record_list.push_back(record_item);
		}
	}

	if (exosip_thread_event_)
		exosip_thread_event_->OnEXosipThreadEvent_RecordInfo(record_info);
	return 0;
}

std::string MANSCDP_XML_MESSAGE::GetChildNodeText(XMLNode& subnode,const char* pNodeName)
{
	XMLNode xmlRet = subnode.GetChildNode(pNodeName);
	return xmlRet.IsEmpty() ? "" : (xmlRet.GetText() == NULL ? "" : xmlRet.GetText());
}

int MANSCDP_XML_MESSAGE::ParseNotifyCatalog(XMLNode &parent_node)
{
	NotifyCatalog catalog;
	catalog.device_id = GetChildNodeText(parent_node, "DeviceID");
	catalog.sum_num = atoi(GetChildNodeText(parent_node,"SumNum").c_str());

	XMLNode deviceListNode = parent_node.GetChildNode("DeviceList");
	XMLNode item_node;
	if (!deviceListNode.IsEmpty())
	{
		int item_count = deviceListNode.nChildNode("Item");
		for (int i=0; i < item_count; i++)
		{
			item_node = deviceListNode.GetChildNode("Item", i);
			NotifyCatalogItem notify_item;
			notify_item.event = GetChildNodeText(item_node, "Event");
			notify_item.catalog_item.device_id = GetChildNodeText(item_node, "DeviceID");

			if (notify_item.event == "ADD")
			{
				notify_item.catalog_item.name = GetChildNodeText(item_node,"Name");
				notify_item.catalog_item.manufacturer = GetChildNodeText(item_node,"Manufacturer");
				notify_item.catalog_item.model = GetChildNodeText(item_node,"Model");
				notify_item.catalog_item.owner = GetChildNodeText(item_node,"Owner");
				notify_item.catalog_item.civil_code = GetChildNodeText(item_node,"CivilCode");
				notify_item.catalog_item.block = GetChildNodeText(item_node,"Block");
				notify_item.catalog_item.address = GetChildNodeText(item_node,"Address");
				notify_item.catalog_item.parental = atoi(GetChildNodeText(item_node,"Parental").c_str());
				notify_item.catalog_item.parent_id = GetChildNodeText(item_node,"ParentID");
				notify_item.catalog_item.safety_way = atoi(GetChildNodeText(item_node,"SafetyWay").c_str());
				notify_item.catalog_item.register_way = atoi(GetChildNodeText(item_node,"RegisterWay").c_str());
				notify_item.catalog_item.cert_num = GetChildNodeText(item_node,"CertNum");
				notify_item.catalog_item.certifiable = atoi(GetChildNodeText(item_node,"Certifiable").c_str());
				notify_item.catalog_item.err_code = atoi(GetChildNodeText(item_node,"ErrCode").c_str());
				notify_item.catalog_item.end_time = GetChildNodeText(item_node,"EndTime");
				notify_item.catalog_item.secrecy = atoi(GetChildNodeText(item_node,"Secrecy").c_str());
				notify_item.catalog_item.password = GetChildNodeText(item_node,"Password");
				notify_item.catalog_item.status = GetChildNodeText(item_node, "Status");
				notify_item.catalog_item.longitude = atof(GetChildNodeText(item_node,"Longitude").c_str());
				notify_item.catalog_item.latitude = atof(GetChildNodeText(item_node,"Latitude").c_str());
			}

			catalog.device_list.push_back(notify_item);
		}
	}

	if (exosip_thread_event_)
		exosip_thread_event_->OnEXosipThreadEvent_NotifyCatalog(from_, catalog);
	return 0;
}

int MANSCDP_XML_MESSAGE::ParseKeeplive(XMLNode &parent_node)
{
	XMLNode snode =  parent_node.GetChildNode(NODE_NAME_DEVICE_ID);
	if(snode.IsEmpty())
		return -1;

	Keepalive keepalive;
	keepalive.device_id = snode.GetText();

	if (exosip_thread_event_)
		exosip_thread_event_->OnEXosipThreadEvent_Keeplive(keepalive);
	return 0;
}