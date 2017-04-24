#ifndef _MANSCDP_XML_MESSAGE_H_
#define _MANSCDP_XML_MESSAGE_H_

#include <string>
#include <KBASE/XMLParser.h>
#include "EXosipThread.h"

class MANSCDP_XML_MESSAGE
{
public:
	MANSCDP_XML_MESSAGE(EXosipThreadEvent *exosip_thread_event);
	void SetFrom(const std::string &from){from_ = from;}
	int Parse(const char* xml_string);
	int ParseKeeplive(XMLNode &parent_node);
	int ParseResponseCatalog(XMLNode &parent_node);
	int ParseNotifyCatalog(XMLNode &parent_node);
	int ParseDeviceStatus(XMLNode &parent_node);
	int ParseDeviceInfo(XMLNode &parent_node);
	int ParseRecordInfo(XMLNode &parent_node);

	std::string GetChildNodeText(XMLNode& subnode,const char* pNodeName);

protected:
	std::string from_;
	EXosipThreadEvent *exosip_thread_event_;
};

#endif