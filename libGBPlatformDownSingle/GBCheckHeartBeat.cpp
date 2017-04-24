#include "stdafx.h"
#include <NETEC/XUtil.h>
#include "GBCheckHeartBeat.h"

GBCheckHeartBeat::GBCheckHeartBeat(GBCheckHeartBeatEvent *pGBCheckHeartBeatEvent):
	m_pGBCheckHeartBeatEvent(pGBCheckHeartBeatEvent)
{
}

GBCheckHeartBeat::~GBCheckHeartBeat()
{
}

bool GBCheckHeartBeat::Start()
{
	return StartThread();
}

void GBCheckHeartBeat::Stop()
{
	m_bRunning = false;
	WaitForStop();
}

//每次进来都覆盖掉
void GBCheckHeartBeat::SetIDTime(const std::string &strID,const IDTimeType &tIDTime)
{
	m_SafeMapClass.AddValue(strID,tIDTime);
}

void GBCheckHeartBeat::ReomveIDTime(const std::string &strID)
{
	m_SafeMapClass.DeleteFromKey(strID);
}

void GBCheckHeartBeat::ThreadProcMain()
{
	m_bRunning = true;
	IDTimeType tIDTime;
	time_t nNowTime;
	while(m_bRunning)
	{
		m_SafeMapClass.ReSet();
		while(m_SafeMapClass.GetNext(tIDTime))
		{
			if (!m_bRunning)
			{
				break;
			}
			nNowTime = time(NULL);
			if (nNowTime - tIDTime.time > MAX_TIME_OUT)//seconds
			{
				SDK_LOG("[GBCheckHeartBeat] Platform[%s] HeartBeat timeout\n", tIDTime.id.c_str());
				if (m_pGBCheckHeartBeatEvent)
				{
					m_pGBCheckHeartBeatEvent->OnGBCheckHeartBeatEvent_OffLine(tIDTime.id);
					m_SafeMapClass.DeleteFromKey(tIDTime.id);
				}
			}
			XSleep(10);
		}
		XSleep(10);
	}
}