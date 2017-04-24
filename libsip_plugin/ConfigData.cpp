#include "stdafx.h"
#include "ConfigData.h"

ConfigData ConfigData::m_ConfigData;

ConfigData::ConfigData()
{
	
}

ConfigData::~ConfigData()
{

}

ConfigData * ConfigData::GetInstance()
{
	return &m_ConfigData;
}
