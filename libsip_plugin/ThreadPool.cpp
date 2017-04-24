#include "stdafx.h"
#include "ThreadPool.h"

#ifdef WIN32
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

thread_base::thread_base(int type) 
: m_type(type)
{
	if (type == WITH_EVENT)
	{
#ifdef WIN32
		m_stopevent = CreateEvent(NULL,TRUE,TRUE,NULL);
		SetEvent(m_stopevent);
#else
		sem_init(&m_stopevent,0,1);
#endif
	}

#ifdef WIN32
	m_threadid=0;
	m_thread=NULL;
#else
	m_thread = -1;
	m_stopped=true;
#endif

	m_type = type;
};
thread_base::~thread_base(void)
{
	if (m_type == WITH_EVENT)
	{
#ifdef WIN32
		CloseHandle(m_stopevent);
#else
		sem_destroy(&m_stopevent);
#endif
	}
}

bool thread_base::start(void)
{
#ifdef WIN32
	m_thread = CreateThread(NULL, 0, init_proc, this, 0, &m_threadid);
	if (m_thread==NULL)
	{
		return false;
	}

	if (m_type == WITH_EVENT)
	{
		ResetEvent(m_stopevent);
	}
	return true;
#else
	if (pthread_create(&m_thread,NULL,init_proc,(void*)this)!=0)
	{
		return false;
	}

	if (m_type == WITH_EVENT)
	{
		sem_wait(&m_stopevent);
	}
	m_stopped=false;
	return true;
#endif
}

void thread_base::stop(void)
{
#ifdef WIN32
	if (m_type == WITH_EVENT)
	{
		WaitForSingleObject(m_stopevent,INFINITE);
	}
	HANDLE hThread = (HANDLE)InterlockedExchange((LONG *)&m_thread, 0);
	if (hThread) 
	{
		if (m_type == WITH_EVENT)
		{
			WaitForSingleObject(hThread, INFINITE);
		}
		CloseHandle(hThread);
	}
#else
	if (m_stopped==false)
	{
		if (m_type == WITH_EVENT)
		{
			sem_wait(&m_stopevent);
		}
		m_stopped=true;
		pthread_join(m_thread,NULL);
	}
#endif
}

#ifdef WIN32
DWORD thread_base::GetThreadID(void)	
{
	return m_threadid;
}
#else
pthread_t thread_base::GetThreadID(void)
{
	return m_thread;
}
#endif

unsigned long thread_base::thread_proc(void)
{
	do_thread_proc();
	if (m_type == WITH_EVENT)
	{
#ifndef WIN32
		sem_post(&m_stopevent);
#else
		SetEvent(m_stopevent);
#endif
	}
	return 0;
}


thread_impl::thread_impl(IThreadCallback*pCallback) 
: thread_base(thread_base::WITH_EVENT)
, m_pCallback(pCallback)
, m_bWantToStop(false)
, m_bIdle(true)
{
	m_pBuffer = (char*)malloc(1024);
	m_nBufLen = 1024;
}

thread_impl::~thread_impl() 
{
	free(m_pBuffer);
	m_nBufLen = 0;
}

bool thread_impl::start(void)
{
	m_bWantToStop = false;
	return thread_base::start();
}

void thread_impl::stop(void)
{
	m_bWantToStop = true;
	thread_base::stop();
}

bool thread_impl::push_proc(char*pBuffer, int nLen)
{
	if (m_bWantToStop)
	{
		return false;
	}

	if (!is_idle())
	{
		return false;
	}
	else
	{
		if (m_nBufLen<nLen)
		{
			m_pBuffer = (char*)realloc(m_pBuffer, nLen);
			m_nBufLen = nLen;
		}

		if (m_pBuffer)
		{
			memcpy(m_pBuffer, pBuffer, nLen);
			m_nBufLen = nLen;
		}

		{
			KAutoLock l(m_mutex);
			m_bIdle = false;
		}
		return true;
	}
}

bool thread_impl::is_idle(void)
{
	KAutoLock l(m_mutex);
	return m_bIdle;
}

void thread_impl::do_thread_proc(void)
{
	while (!m_bWantToStop)
	{
		if (!is_idle())
		{
			if (m_pBuffer)
			{
				m_pCallback->OnThreadPool_OneThreadProc(m_pBuffer, m_nBufLen);
			}

			{
				KAutoLock l(m_mutex);
				m_bIdle = true;
			}
		}
		Pause(10);
	}
}


thread_pool::thread_pool(void)
: m_pCallback(NULL)
, m_nMaxPoolSize(10)
, m_bWantToStop(false)
{
}

thread_pool::~thread_pool(void)
{
}

bool thread_pool::create(IThreadPoolCallback*pCallback, int nMaxPoolSize)
{
	if (pCallback==NULL) 
	{
		return false;
	}

	m_pCallback = pCallback;
	m_nMaxPoolSize = (nMaxPoolSize>20) ? 20 : ((nMaxPoolSize<1)?1:nMaxPoolSize);

	//先启动线程，减少不断启动/关闭线程带来的开销
	for (int i=0; i<m_nMaxPoolSize; i++)
	{
		thread_impl*pProc = new thread_impl(this);
		if (pProc && pProc->start())
		{
			m_pool.push_back(pProc);
		}
		Pause(250);//线程启动之间建议间隔250毫秒
	}

	m_bWantToStop = false;
	return StartThread();
}

void thread_pool::destroy(void)
{
	m_bWantToStop = true;
	KThread::WaitForStop();
	Clear();/*清空缓冲区*/

	{//释放线程
		std::list<thread_impl*>::iterator it = m_pool.begin();
		while (it != m_pool.end())
		{
			thread_impl*pProc = m_pool.front();
			if (pProc)
			{
				pProc->stop();
				delete pProc;
				pProc = NULL;
			}
			m_pool.pop_front();
			it = m_pool.begin();
		}
	}
}

bool thread_pool::is_created(void)
{
	return (m_bWantToStop==false);
}

int thread_pool::get_idle_number(void)
{
	return m_nMaxPoolSize-GetSize();
}

void thread_pool::ThreadProcMain(void)
{
	while (!m_bWantToStop)
	{
		process_pool();
		Pause(10);
	}
}

void thread_pool::push_proc(char* pData,int nLen)
{
	if (m_bWantToStop)
	{
		return ;
	}

	KBuffer* pBuffer=new KBuffer();
	if(pBuffer)
	{
		if(pBuffer->SetBuffer(pData,nLen))
		{
			Push(pBuffer);
		}
	}
}

bool thread_pool::process_pool(void)
{
	if (GetSize()<=0)
	{
		return false;
	}
	else
	{
		list <thread_impl*>::iterator it = m_pool.begin();
		while (GetSize()>0 && it!=m_pool.end())
		{
			thread_impl*pProc = m_pool.front();
			if (pProc && pProc->is_idle())
			{
				KBuffer* pBuffer = Pop();
				if(pBuffer)
				{
					pProc->push_proc((char*)pBuffer->GetBuffer(), pBuffer->GetSize());
					delete pBuffer;
					pBuffer=NULL;
				}
			}
			it++;
		}
		return true;
	}
}

void thread_pool::OnThreadPool_OneThreadProc(char*pData, int nLen)
{
	if (m_bWantToStop)
	{
		return ;
	}

	m_pCallback->OnThreadPool_OneThreadProc(pData, nLen);
}
