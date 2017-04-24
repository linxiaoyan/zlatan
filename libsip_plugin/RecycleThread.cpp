#include "stdafx.h"
#include "RecycleThread.h"
#include "MonDevice.h"

RecycleThread::RecycleThread()
{

}

RecycleThread::~RecycleThread()
{
	for (std::list<MonDevTime>::iterator it=m_lstMonDevTime.begin();it!=m_lstMonDevTime.end();)
	{
		MonDevTime tMonDevTime=*it;
		delete tMonDevTime.pDevice;
		tMonDevTime.pDevice=NULL;
	}
	m_lstMonDevTime.clear();
}

bool RecycleThread::Start()
{
	m_bStop=false;
	return StartThread();
}

void RecycleThread::Stop()
{
	m_bStop=true;
	WaitForStop();
}

void RecycleThread::InsertMonDevTime(CMonDevice * pDev)
{
	MonDevTime tMonDevTime;
	tMonDevTime.nTime=time(NULL);
	tMonDevTime.pDevice=pDev;
	XAutoLock l(m_csMonDevTime);
	m_lstMonDevTime.push_back(tMonDevTime);
}
void RecycleThread::ThreadProcMain()
{
	m_bStop=false;
	while(!m_bStop)
	{
		time_t nCurrentTime=time(NULL);
		{
			XAutoLock l(m_csMonDevTime);
			for (std::list<MonDevTime>::iterator it=m_lstMonDevTime.begin();it!=m_lstMonDevTime.end();)
			{
				MonDevTime tMonDevTime=*it;
				if (nCurrentTime-tMonDevTime.nTime>TIME_FOR_LIVE)
				{
					printf("[GB28181]: Deleting the device %s !\n",tMonDevTime.pDevice->GetDeviceInfo()->szMTSAccount);
					delete tMonDevTime.pDevice;
					tMonDevTime.pDevice=NULL;
					it=m_lstMonDevTime.erase(it);//删除返回的是下一个值
				}
				else
				{
					it++;
				}
			}
		}
		XSleep(1000);
	}
}