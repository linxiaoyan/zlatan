#ifndef _DEVCONNECT_H_
#define _DEVCONNECT_H_
#pragma once
#include "stdafx.h"
#include "NETEC/XThreadBase.h"
#include "NETEC/XUtil.h"
#include "ThreadPool.h"

class DevConnectCallback
{
public:
	virtual void OnDevConnectCallback_ConnectQueueEmpty(void) = 0;
	virtual void OnDevConnectCallback_Connect(char*pData, int nLen) = 0;
};


class CDevConnect: 
	public XThreadBase,
	public IThreadPoolCallback
{
public:
	CDevConnect( DevConnectCallback *pCallback );
	~CDevConnect(void);

protected:
	bool m_bWantToDestroy;

	thread_pool m_ConnectPool;

	DevConnectCallback *m_pConnectCallback;

public:
	int Create(int nThreadNum);
	void Destroy(void);
public:
	void ThreadProcMain(void);
	void OnThreadPool_OneThreadProc(char*pData, int nLen);

public:
	int AddConnect(const std::string&strDevID, /*CMonDevice* pDev, */int nUrgency=FALSE);
	void RemoveConnect(const std::string&strDevID);	
	
protected:
	void PushUrgency(const std::string&strDevID/*, std::string strDevID*/);
	std::string PopUrgency(void);
	std::string FindUrgency(const std::string&strDevID);

protected:
	KCritSec m_csPriority;
	std::map<std::string,std::string> m_mapPriority; //MAP_PDEVICE_INFO m_mapPriority;

protected:
	void PushNormal(const std::string&strDevID/*,std::string strDevID*/);
	std::string PopNormal(void);
	void RemoveNormal(const std::string&strDevID);
protected:
	KCritSec m_csNormal;
	std::map<std::string,std::string> m_mapNormal; //MAP_PDEVICE_INFO m_mapNormal;


};

#endif
