 #pragma once

#include "KBASE.h"

#ifndef WIN32
#include <pthread.h>
#include <semaphore.h>
#endif

class thread_base
{
public:
	enum {WITH_EVENT,WITHOUT_EVENT};
	thread_base(int type);
	virtual ~thread_base(void);

	virtual bool start(void);
	virtual void stop(void);


#ifdef WIN32
	DWORD GetThreadID(void);
#else
	pthread_t GetThreadID(void);
#endif

protected:
	virtual void do_thread_proc(void)=0;

private:
	unsigned long thread_proc(void);

#ifdef WIN32
	static DWORD WINAPI init_proc(PVOID pObj)
	{
		return ((thread_base*)pObj)->thread_proc();
	}
#else
	static void* init_proc(void*pObj)
	{
		((thread_base*)pObj)->thread_proc();
		return 0;
	}
#endif

protected:
	int m_type;
#ifdef WIN32
	DWORD	m_threadid;
	HANDLE	m_thread;
	HANDLE	m_stopevent;
#else
	pthread_t m_thread;
	sem_t	  m_stopevent;
	bool	  m_stopped;
#endif

};


class IThreadCallback
{
public:
	virtual void OnThreadPool_OneThreadProc(char*pData, int nLen) = 0;
};

class thread_impl : public thread_base
{
public:
	thread_impl(IThreadCallback*pCallback);
	virtual ~thread_impl();
protected:
	IThreadCallback* m_pCallback;

public:
	bool start(void);
	void stop(void);

public:
	bool push_proc(char*pBuffer, int nLen);
	bool is_idle(void);
protected:
	KCritSec m_mutex;
	bool m_bIdle;

	char* m_pBuffer;
	int m_nBufLen;

protected:
	void do_thread_proc(void);
	bool m_bWantToStop;
};


class IThreadPoolCallback
{
public:
	virtual void OnThreadPool_OneThreadProc(char*pData, int nLen) = 0;
};

class thread_pool :
	public KThread,
	public KBufferPool,
	public IThreadCallback
{
public:
	thread_pool(void);
	virtual ~thread_pool(void);

public:
	bool create(IThreadPoolCallback*pCallback, int nMaxPoolSize=10);
	void destroy(void);
	bool is_created(void);
	int get_idle_number(void);
protected:
	int m_nMaxPoolSize;
	IThreadPoolCallback*m_pCallback;

public:
	void OnThreadPool_OneThreadProc(char*pData, int nLen);
	void push_proc(char* pData,int nLen);
protected:
	std::list <thread_impl*> m_pool;    

protected:
	void ThreadProcMain(void);
	bool process_pool(void);
protected:
	volatile bool m_bWantToStop;
};
