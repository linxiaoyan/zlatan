#ifndef PARSERXML_H
#define PARSERXML_H

#include "ConfigData.h"

class ParserXML
{
public:
	static bool ParserFile(const std::string & strFileName,ConfigData & rConfigData);
	static bool GetChildNodeValue(const XMLNode & xParent,const char * szChildName,std::string & strRetValue);
};
#endif