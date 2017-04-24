#ifndef _ALARM_SERVER_H_
#define _ALARM_SERVER_H_

#include "stdafx.h"

class AlarmSingleton
{
private:
	AlarmSingleton(){}
	AlarmSingleton(const AlarmSingleton &);
	AlarmSingleton & operator = (const AlarmSingleton &);

public:
	static AlarmSingleton &GetInstance()
	{
		static AlarmSingleton instance; 
		return instance;
	}

	void SetConfigFile(const char *szConfigFile) {strConfigFile = szConfigFile;}
	std::string  GetAlarmServerIP();
	unsigned int GetAlarmServerPort();

private:
	std::string strConfigFile;
};

#endif