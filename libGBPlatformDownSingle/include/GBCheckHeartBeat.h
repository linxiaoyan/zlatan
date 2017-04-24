#ifndef _GB_CHECK_HEARTBEAT_H_
#define _GB_CHECK_HEARTBEAT_H_

#include <KBASE/Thread.h>
#include "GBDefines.h"
#include "SafeMap.h"

#define MAX_TIME_OUT 180//seconds,3 minutes time out

class GBCheckHeartBeatEvent
{
public:
	virtual void OnGBCheckHeartBeatEvent_OffLine(const std::string & strID)=0;
};

class GBCheckHeartBeat:public KThread
{
public:
	GBCheckHeartBeat(GBCheckHeartBeatEvent * pGBCheckHeartBeatEvent);
	virtual ~GBCheckHeartBeat();

	bool Start();
	void Stop();

	void SetIDTime(const std::string & strID,const IDTimeType & tIDTime);
	void ReomveIDTime(const std::string & strID);

protected:
	virtual void ThreadProcMain();

private:
	SafeMap<std::string,IDTimeType> m_SafeMapClass;
	bool m_bRunning;
	GBCheckHeartBeatEvent * m_pGBCheckHeartBeatEvent;
};
#endif