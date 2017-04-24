#ifndef GBALARM_H
#define GBALARM_H
#include "GBDefines.h"

class GB28181_API GBAlarm
{
public:
	GBAlarm();
	~GBAlarm();

	void SetCatalog(const Catalog & tCatalog);

	std::string m_strID;
	std::string m_strName;
	std::string m_strManufacturer;
	std::string m_strModel;
	std::string m_strOwner;
	std::string m_strCivilCode;
	std::string m_strBlock;
	std::string m_strAddress;
	int m_nParental;

	//这三个最重要，是用到调像的时候，ParentID国标是必选，其它两个在从设备获取的时候也可以设置进来
	std::string m_strParentID;
	std::string m_strParentIP;
	int m_nParentPort;

	int m_nSafetyWay;
	int m_nRegisterWay;
	std::string m_strCertNum;
	int m_nCertifiable;
	int m_nErrCode;
	std::string m_strEndTime;
	int m_nSecrecy;

	//IP和端口在国标里是可选的，也不知是父节点的还是IPC的
	std::string m_strIPAddress;
	int m_nPort;

	std::string m_strPassword;
	bool m_bStatus;
	double m_dLongitude;
	double m_dLatitude;

	int m_nAlarmNum;//从0开始

};

typedef std::map<int,GBAlarm*> HPID_GBALARM_MAP;
#endif