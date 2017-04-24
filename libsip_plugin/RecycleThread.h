#ifndef RECYCLETHREAD_H
#define RECYCLETHREAD_H

#include "NETEC/XThreadBase.h"
#include "NETEC/XUtil.h"
#include "NETEC/XAutoLock.h"
#include "NETEC/XCritSec.h"

class CMonDevice;
#include <list>
#include <time.h>

#define TIME_FOR_LIVE 5//Second
typedef struct tagMonDevTime
{
	time_t nTime;
	CMonDevice * pDevice;
}MonDevTime;

class RecycleThread:public XThreadBase
{
public:
	RecycleThread();
	~RecycleThread();

	bool Start();
	void Stop();

	void InsertMonDevTime(CMonDevice * pDev);

protected:
	virtual void ThreadProcMain();

private:
	bool m_bStop;
	XCritSec m_csMonDevTime;
	std::list<MonDevTime> m_lstMonDevTime;
};
#endif