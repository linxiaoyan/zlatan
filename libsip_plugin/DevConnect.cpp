#include "StdAfx.h"
#include "DevConnect.h"

#ifdef WIN32
#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#endif

CDevConnect::CDevConnect(DevConnectCallback *pCallback)
 : m_pConnectCallback( pCallback )
 , m_bWantToDestroy(false)
{
}

CDevConnect::~CDevConnect(void)
{
}

int CDevConnect::Create( int nThreadNum )
{
	m_bWantToDestroy = false;
	bool bret = XThreadBase::StartThread();

	return m_ConnectPool.create(this,nThreadNum);
}

void CDevConnect::Destroy( void )
{
	m_bWantToDestroy = true;
	XThreadBase::WaitForStop();

	m_ConnectPool.destroy();
	/*释放紧急队列*/
	do 
	{
		std::string strDevID = PopUrgency();
		if ( strDevID.empty() )
		{
			break;
		}
	} while (1);

	/*释放普通队列*/
	do 
	{
		std::string strDevID = PopNormal();
		if ( strDevID.empty() )
		{
			break;
		}

	} while (1);	


}

void SleepEx(unsigned long ulSleep,bool& bWantToStop)
{
	unsigned long ulCount = 0;

	while (bWantToStop!=true)
	{
		if (ulCount>=ulSleep)
		{
			return ;
		}

		XSleep(100);
		ulCount += 100;
	}
}

void CDevConnect::ThreadProcMain( void )
{
	while (m_bWantToDestroy!=true)
	{
		bool bQueueEmpty = true;
		/*优先处理紧急的设备*/
		do 
		{
			//MPX_DEVICE_INFO* pDevInfo = NULL;
			std::string strDevID ;
			strDevID = PopUrgency();
			if ( !strDevID.empty() )
			{
				bQueueEmpty = false;

				if (m_ConnectPool.get_idle_number()<=0)
				{
					/*没有工作线程可用*/
					break;
				}

				m_ConnectPool.push_proc((char*)strDevID.c_str(), strDevID.length()+1);
			}
			else
			{
				break;
			}
		} while (1);

		/*再处理普通的设备*/
		do 
		{
			//MPX_DEVICE_INFO* pDevInfo = NULL;
			std::string strDevID ;
			strDevID = PopNormal();
			if ( !strDevID.empty() )
			{
				bQueueEmpty = false;

				if (m_ConnectPool.get_idle_number()<=0)
				{
					/*没有工作线程可用*/
					break;
				}

				m_ConnectPool.push_proc((char*)strDevID.c_str(), strDevID.length()+1);
			}
			else
			{
				break;
			}
		} while (1);	

		if (bQueueEmpty)
		{
			m_pConnectCallback->OnDevConnectCallback_ConnectQueueEmpty();
		}

		SleepEx(10000,m_bWantToDestroy);
	}
}

void CDevConnect::OnThreadPool_OneThreadProc( char*pData, int nLen )
{
// 	MPX_DEVICE_INFO* pDevInfo = (MPX_DEVICE_INFO*)pData;
// 	if (pDevInfo==NULL || pDevInfo->nStructSize!=nLen)
// 	{
// 		return ;
// 	}

	if (m_pConnectCallback)
	{
		m_pConnectCallback->OnDevConnectCallback_Connect(pData, nLen);
	}

}

int CDevConnect::AddConnect( const std::string&strDevID,/*CMonDevice* pDev,*/int nUrgency/*=FALSE*/ )
{
	std::string strExist = FindUrgency( strDevID );
	if ( !strExist.empty() )
	{

		return FALSE;
	}
	else
	{
		RemoveNormal(strDevID);

		if (nUrgency)
		{
			PushUrgency(strDevID/*, pDev*/);
		}
		else
		{
			PushNormal(strDevID/*, pDev*/);
		}

		return TRUE;
	}
}

void CDevConnect::RemoveConnect( const std::string&strDevID )
{
	RemoveNormal(strDevID);
}

void CDevConnect::PushUrgency( const std::string&strDevID/*,std::string strDevID*/ )
{
	KAutoLock l(m_csPriority);
	m_mapPriority[strDevID] = strDevID;
}

std::string CDevConnect::PopUrgency( void )
{
	//CMonDevice *pDev = NULL;
	std::string strDevID;

	{
		KAutoLock l(m_csPriority);
		std::map<std::string,std::string>::iterator it = m_mapPriority.begin();
		if (it!=m_mapPriority.end())
		{
			strDevID = it->second;
			m_mapPriority.erase(it);
		}
	}

	return strDevID;
}

std::string CDevConnect::FindUrgency( const std::string&strDevID )
{
	//CMonDevice*pDev = NULL;
	std::string strID;
	{
		KAutoLock l(m_csPriority);
		std::map<std::string,std::string>::iterator it = m_mapPriority.find(strDevID);
		if (it!=m_mapPriority.end())
		{
			strID = it->second;
		}
	}

	return strID;
}

void CDevConnect::PushNormal( const std::string&strDevID/*,CMonDevice*pDev*/ )
{
	KAutoLock l(m_csNormal);
	m_mapNormal[strDevID] = strDevID;
}

std::string CDevConnect::PopNormal( void )
{
	//CMonDevice*pDev = NULL;
	std::string strID;
	{
		KAutoLock l(m_csNormal);
		std::map<std::string,std::string>::iterator it = m_mapNormal.begin();
		if (it!=m_mapNormal.end())
		{
			strID = it->second;
			m_mapNormal.erase(it);
		}
	}

	return strID;
}

void CDevConnect::RemoveNormal( const std::string&strDevID )
{
	do 
	{
		//CMonDevice*pDev = NULL;
		std::string strID;
		{
			KAutoLock l(m_csNormal);
			std::map<std::string,std::string>::iterator it = m_mapNormal.find(strDevID);
			if (it!=m_mapNormal.end())
			{
				//strID = it->second;
				m_mapNormal.erase(it);
			}
			else
			{
				break;
			}
		}

// 		if (pDev)
// 		{
// 			delete pDev;
// 		}
	} while (1);
}