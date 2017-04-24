#include "stdafx.h"
#include <NETEC/XUtil.h>
#include "StreamEventBaseThread.h"

StreamEventBaseThread::StreamEventBaseThread(struct event_base *event_base):
	event_base_(event_base),
	quit_(false)
{
}

StreamEventBaseThread::~StreamEventBaseThread()
{
}

bool StreamEventBaseThread::Start()
{
	return StartThread();
}

void StreamEventBaseThread::Stop()
{
	quit_ = true; 
	WaitForStop();
}


void StreamEventBaseThread::ThreadProcMain()
{
	while (!quit_)
	{
		XSleep(10);
		event_base_dispatch(event_base_);
	}
}