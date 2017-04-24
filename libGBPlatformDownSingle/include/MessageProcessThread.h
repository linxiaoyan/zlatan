#ifndef _MESSAGE_PROCESS_THREAD_H_
#define _MESSAGE_PROCESS_THREAD_H_

#include <deque>
#include <KBASE/CritSec.h>
#include <KBASE/Thread.h>
#include "EXosipThreadEvent.h"

class MessageProcessThread:public KThread
{
public:
	MessageProcessThread(EXosipThreadEvent *exosip_thread_event = NULL);
	virtual ~MessageProcessThread(void);

public:
	bool Start();
	void Stop();
	void Push(MsgBody &msg_body);

protected:
	virtual void ThreadProcMain();
	void ProcessMsg(MsgBody &msg_body);

private:
	bool quit_;
	KCritSec msg_deque_cs_;
	std::deque<MsgBody> msg_deque_;
	EXosipThreadEvent *exosip_thread_event_;
};

#endif