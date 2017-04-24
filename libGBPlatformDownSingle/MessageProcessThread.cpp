#include "stdafx.h"
#include <KBASE/AutoLock.h>
#include <NETEC/XUtil.h>
#include "MessageProcessThread.h"
#include "GB28181Utils.h"
#include "ManscdpXmlMessage.h"
#include "GBDefines.h"
#include "Func.h"

MessageProcessThread::MessageProcessThread(EXosipThreadEvent *exosip_thread_event)
	:exosip_thread_event_(exosip_thread_event)
	,quit_(false)	
{
}

MessageProcessThread::~MessageProcessThread(void)
{
}

bool MessageProcessThread::Start()
{
	return StartThread();
}

void MessageProcessThread::Stop()
{
	quit_ = true; 
	WaitForStop();
}

void MessageProcessThread::Push(MsgBody &msg_body)
{
	KAutoLock lock(msg_deque_cs_);
	if (msg_body.data.find("Keepalive") < 0)
		msg_deque_.push_back(msg_body);
	else
		msg_deque_.push_front(msg_body);
}

void MessageProcessThread::ProcessMsg(MsgBody &msg_body)
{
	if (GB28181Utils::IsPlatformType(msg_body.id))
	{
		MANSCDP_XML_MESSAGE msg_parser(exosip_thread_event_);
		msg_parser.SetFrom(msg_body.id);

		int ret = msg_parser.Parse(Func::GB2312toUTF8(msg_body.data.c_str()).c_str());
		if(ret < 0)
			SDK_LOG("[MANSCDP_XML] Parse xml failed\n");
	}
}

void MessageProcessThread::ThreadProcMain()
{
	MsgBody msg_body;
	while (!quit_)
	{
		XSleep(10); //为了不要阻塞主线程做Push操作
		msg_deque_cs_.Lock();
		if (!msg_deque_.empty())
		{
			msg_body = msg_deque_.front();
			msg_deque_.pop_front();
			msg_deque_cs_.Unlock();

			ProcessMsg(msg_body);
			continue;
		}
		msg_deque_cs_.Unlock();
	}
}
