#ifndef _STREAM_EVENT_BASE_THREAD_H_
#define _STREAM_EVENT_BASE_THREAD_H_

#include <event2/event.h>
#include <NETEC/XCritSec.h>
#include <NETEC/XThreadBase.h>
#include "EXosipThreadEvent.h"

class StreamEventBaseThread: public XThreadBase
{
public:
	StreamEventBaseThread(struct event_base *event_base);
	virtual ~StreamEventBaseThread(void);

public:
	bool Start();
	void Stop();

protected:
	virtual void ThreadProcMain();

private:
	bool quit_;
	struct event_base *event_base_;
};

#endif