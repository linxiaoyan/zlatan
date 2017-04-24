#include "StdAfx.h"
#include "MonPlugin.h"

#ifdef WIN32
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

AlarmSingleton &CMonPlugin::m_AlarmSingleton = AlarmSingleton::GetInstance();

CMonPlugin::CMonPlugin(void)
: m_bWantToStop(false)
, m_bLoginMtsSuccessed(false)
//, m_pDevSDk(NULL)
, m_pMsgCb(NULL)
, m_pAlarmCb(NULL)
, m_pRecCb(NULL)
, m_pMsgUser(NULL)
, m_pAlarmUser(NULL)
, m_pRecUser(NULL)
, m_Connect(this)
{
	m_pPDB = CPluginDB_Manage::CreatePluginDB("sip.dat");
	m_pSDKInit = HPMON_ISDK::Create(this);
	m_pRecycleThread=NULL;

#ifdef _PING_SNMP
	m_pDeviceStateManager = new DeviceStateManager(this);
	m_pDeviceStateManager->Start();
#endif
}

CMonPlugin::~CMonPlugin(void)
{

	if (m_pPDB)
	{
		CPluginDB_Manage::ReleasePluginDB(m_pPDB);
		m_pPDB = NULL;
	}
	if (m_pSDKInit)
	{
		delete m_pSDKInit;
		m_pSDKInit=NULL;
	}
#ifdef _PING_SNMP
	if (m_pDeviceStateManager)
	{
		m_pDeviceStateManager->Stop();
		delete m_pDeviceStateManager;
		m_pDeviceStateManager = NULL;
	}
#endif

	if (m_pRecycleThread)
	{
		delete m_pRecycleThread;
		m_pRecycleThread=NULL;
	}
}

void CMonPlugin::ThreadProcMain( void )
{

	while (!m_bWantToStop)
	{
		//先获取到所有key,新插入的下次遍历会生效
		std::list<string> lstDevID;
		{
			KAutoLock l(m_csDevices);
			for (IT_MAP_MONDEVICES it=m_mapDevices.begin();it!=m_mapDevices.end();it++)
			{
				lstDevID.push_back(it->first);
			}
		}

		for (std::list<string>::iterator it=lstDevID.begin();it!=lstDevID.end();it++)
		{
			std::string strID=*it;

			CMonDevice* pDev = NULL;
			{
				KAutoLock l(m_csDevices);
				IT_MAP_MONDEVICES it=m_mapDevices.find(strID);
				if (it!=m_mapDevices.end())
				{
					pDev=it->second;
				}
			}

			if (pDev)
			{
				pDev->ProcessDvrEvents();
				pDev->ProcessMtsEvents(m_bLoginMtsSuccessed);
			}
		}
		XSleep(10);
	}

}

BOOL CMonPlugin::Startup( const char* pConfigFile, AVCON_MPX_MESSAGE_CB pMessageCb,void*pMsgUser, AVCON_MPX_ALARM_CB pAlarmCb,void*pAlarmUser, AVCON_MPX_RECORD_CB pRecordCb,void*pRecUser )
{
	m_AlarmSingleton.SetConfigFile(pConfigFile);
	
	m_pMsgCb = pMessageCb;
	m_pAlarmCb = pAlarmCb;
	m_pRecCb = pRecordCb;

	m_pMsgUser = pMsgUser;
	m_pAlarmUser = pAlarmUser;
	m_pRecUser = pRecUser;
	//m_Connect.Create(10);//SIP插件不主动连设备

	AVCON_STC_Init();
	AVCON_STF_Init(FALSE);
	//DevicesToConnectQueue();
	CreateAllDevs();
	//SIP服务要求先创建好所有设备才启动服务
	m_pSDKInit->InitSDK();
#ifdef _VOD_ENABLE
	VODTX_Startup(VOD_RTSP_SIP,VOD_ATT_3RD,VOD_AST_VIDEO|VOD_AST_AUDIO,this);
#endif

	if(m_pRecycleThread==NULL)
	{
		m_pRecycleThread=new RecycleThread();

		if (m_pRecycleThread)
		{
			if(m_pRecycleThread->Start()==false)
			{
				printf("RecycleThread start failed!\n");
			}
		}
	}

	return XThreadBase::StartThread();

}

void CMonPlugin::Cleanup( void )
{
#ifdef _VOD_ENABLE
	VODTX_Cleanup(this);
	//CNodeTx::Destroy();
#endif
// 	if (m_pDevSDk)
// 	{
// 		m_pDevSDk->UnInit();
// 	}
	m_bWantToStop = true;
	XThreadBase::WaitForStop();

	AVCON_STF_Cleanup();
	AVCON_STC_Cleanup();

	ClearDevices();
	m_Connect.Destroy();
	m_pSDKInit->UnInitSDK();

	if (m_pRecycleThread)
	{
		m_pRecycleThread->Stop();
	}
}

long CMonPlugin::GetLastError( void )
{
	return 0;
}

//login mts result
void CMonPlugin::ServerStatusChanged( int nStatus )
{
	bool bLoginSuccessed = false;
	switch (nStatus)
	{
	case MPX_ERR_CS_BUSY:
	case MPX_ERR_CS_RECONNECTED:
	case MPX_ERR_CS_CONNECTING:
	case MPX_ERR_CS_IDLE:
		return ;
	case MPX_ERR_LOGINSUCCESSFUL:
	case MPX_ERR_MON_MODULELIMIT:
		{			
			bLoginSuccessed = true;
		}
		break;
	case MPX_ERR_MON_PROTOCOL:
	case MPX_ERR_MON_REUSESERIAL:
	case MPX_ERR_MON_INVALIDACCOUNT:
	case MPX_ERR_MON_INVALIDPASSWORD:
	case MPX_ERR_MON_INVALIDSERIAL:
	case MPX_ERR_MON_ALREADYLOGIN:
	case MPX_ERR_MON_INVALIDTYPE:
	case MPX_ERR_CS_DISCONNECTED:
	case MPX_ERR_MON_LOGOUT:
		{
			bLoginSuccessed = false;

			/*通知离线*/
			//这个在ThreadProMain里处理了，为什么还放到这里来？因为这个状态变得很快
			//{
			//	CMonDevice* pDev = NULL;
			//	IT_MAP_MONDEVICES it;
			//	{
			//		KAutoLock l(m_csDevices);
			//		it = m_mapDevices.begin();
			//		if (it!=m_mapDevices.end())
			//		{
			//			pDev = it->second;
			//		}
			//	}

			//	while (pDev)
			//	{
			//		pDev->ProcessMtsEvents(false);
			//		{
			//			KAutoLock l(m_csDevices);
			//			it++;
			//			if (it!=m_mapDevices.end())
			//			{
			//				pDev = it->second;
			//			}
			//			else
			//			{
			//				pDev=NULL;
			//				break;
			//			}

			//		}
			//	}
			//}

			std::list<string> lstDevID;
			{
				KAutoLock l(m_csDevices);
				for (IT_MAP_MONDEVICES it=m_mapDevices.begin();it!=m_mapDevices.end();it++)
				{
					lstDevID.push_back(it->first);
				}
			}

			for (std::list<string>::iterator it=lstDevID.begin();it!=lstDevID.end();it++)
			{
				std::string strID=*it;

				CMonDevice* pDev = NULL;
				{
					KAutoLock l(m_csDevices);
					IT_MAP_MONDEVICES it=m_mapDevices.find(strID);
					if (it!=m_mapDevices.end())
					{
						pDev=it->second;
					}
				}

				if (pDev)
				{
					pDev->ProcessMtsEvents(false);
				}
			}
		}
		break;
	}

	m_bLoginMtsSuccessed = bLoginSuccessed;
}

BOOL CMonPlugin::PtzControl( const char*szDevID,int nChanID,int nPtzCmd,int nSpeed,void*pExtData,int nExtSize )
{
	CMonDevice* pDev = FindDevice(szDevID,TRUE);
	if (pDev)
	{
		return pDev->PtzControl(nChanID,nPtzCmd,nSpeed,pExtData,nExtSize);
	}
	return TRUE;
}

BOOL CMonPlugin::StartRecord( const char*szDevID,int nChanID )
{

	return TRUE;
}

void CMonPlugin::StopRecord( const char*szDevID,int nChanID )
{

}

BOOL CMonPlugin::StartTalk( const char*szDevID,MPX_TALK_INFO*pTalkData )
{

	CMonDevice* pDev = FindDevice(szDevID,TRUE);
	if (pDev)
	{
		return pDev->StartTalk(pTalkData);
	}
	return FALSE;
}

void CMonPlugin::StopTalk( const char*szDevID,const char *szNodeID)
{
	CMonDevice* pDev = FindDevice(szDevID,TRUE);
	if (pDev)
	{
		pDev->StopTalk(szNodeID);
	}
}

void CMonPlugin::TalkChanged( const char*szDevID,unsigned long nAudioID )
{
	CMonDevice* pDev = FindDevice(szDevID,TRUE);
	if (pDev)
	{
		pDev->OnChangedAudio(nAudioID);
	}
}

BOOL CMonPlugin::StartListenAlarm( const char*szDevID )
{

	return TRUE;
}

void CMonPlugin::StopListenAlarm( const char*szDevID )
{

}

void CMonPlugin::OnDeviceOnline( const char*szDevID,int nErrCode )
{
	CMonDevice* pDev = FindDevice(szDevID,TRUE);
	if (pDev && m_pPDB)
	{
		pDev->OnDeviceOnline(nErrCode);
	}	
}

void CMonPlugin::OnDeviceOffline( const char*szDevID,int nErrCode )
{
	CMonDevice* pDev = FindDevice(szDevID,TRUE);
	if (pDev && m_pPDB)
	{
		pDev->OnDeviceOffline(nErrCode);
	}	
}

void CMonPlugin::OnChannelOnline( const char*szDevID,int nChanID,int nErrCode )
{
	CMonDevice* pDev = FindDevice(szDevID,TRUE);
	if (pDev && m_pPDB)
	{
		pDev->OnChannelOnline(nChanID, nErrCode);
	}	
}

void CMonPlugin::OnChannelOffline( const char*szDevID,int nChanID,int nErrCode )
{
	CMonDevice* pDev = FindDevice(szDevID,TRUE);
	if (pDev && m_pPDB)
	{
		pDev->OnChannelOffline(nChanID, nErrCode);
	}	
}

void CMonPlugin::OnToolsOperator( int nMessage, AVCON_MPX_TOOLMESSAGE_CB pToolMessageCb, void*pExtData,int nExtSize, void* pSession )
{
	switch( nMessage )
	{
	case MTL_VAL_CMD_SETDVRINFO:
		{
			SetDvrInfo(nMessage, pToolMessageCb, pExtData, nExtSize, pSession);
		}
		break;
	case MTL_VAL_CMD_GETDVRINFO:
		{
			GetDvrInfo(nMessage, pToolMessageCb, pExtData, nExtSize, pSession);
		}
		break;
	case MTL_VAL_CMD_GETCHNINFO:
		{
			GetChnInfo(nMessage, pToolMessageCb, pExtData, nExtSize, pSession);
		}
		break;
	case MTL_VAL_CMD_GETLOGINFO:
		{
			GetLogInfo(nMessage, pToolMessageCb, pExtData, nExtSize, pSession);
		}
		break;
	case MTL_VAL_CMD_DELDVR:
		{
			DelDvrInfo(nMessage, pToolMessageCb, pExtData, nExtSize, pSession);
		}
		break;
	case MTL_VAL_CMD_MODIFY_CHANNEL:
		{
			ModifyChannelInfo(nMessage, pToolMessageCb, pExtData, nExtSize, pSession);
		}
		break;
	}
}

void CMonPlugin::OnGetMonDeviceLoginInfo(const char * szDevID,const char * szPassword,int nDevType)
{
	//szDevID是一个MTS账号，带域的
	//写数据库
	printf("The %s password is %s! [%d]\n",szDevID,szPassword,nDevType);
	MPX_DEVICE_INFO tDeviceInfo;
	SAFE_STRCPY(tDeviceInfo.szMTSAccount,szDevID);
	SAFE_STRCPY(tDeviceInfo.szMTSPass,szPassword);
	KUID uid(szDevID);
	std::string strUserID=uid.GetUserID();
	SAFE_STRCPY(tDeviceInfo.szDevID,strUserID.c_str());
	SAFE_STRCPY(tDeviceInfo.szDevPass,szPassword);
	SAFE_STRCPY(tDeviceInfo.szDevUser,strUserID.c_str());
	tDeviceInfo.nDevType=nDevType;

	if (m_pPDB)
	{
		m_pPDB->SetDvrInfo(tDeviceInfo);
	}

}

void CMonPlugin::SetDvrInfo( int nMessage, AVCON_MPX_TOOLMESSAGE_CB pToolMessageCb, void*pExtData,int nExtSize, void* pSession )
{
	//只让使用者更改设备账号和密码以外的信息
	int nRet = REQUEST_FAILED;
	MPX_DEVICE_INFO respData;

	if (pExtData && nExtSize>0)
	{
		MPX_DEVICE_INFO* pDvrInfo = (MPX_DEVICE_INFO*)pExtData;
		if (m_pPDB)
		{
			CMonDevice* pDev = FindDevice(pDvrInfo->szDevID);
			int nWantToReconnect = FALSE;
			if (pDev)
			{
				//如果网关有设备
				{
					m_mapKeyAccount[pDvrInfo->szMTSAccount] =pDvrInfo->szDevID;
				}

				do 
				{
					nRet = pDev->SetMtsParam(pDvrInfo->szMTSAccount,pDvrInfo->szMTSPass);
					if (nRet!=REQUEST_NO)
					{
						break;
					}

					nRet = pDev->SetDevParam(pDvrInfo->szDevName,pDvrInfo->szDevUser,pDvrInfo->szDevPass,nWantToReconnect);
				} while (0);

				if (nRet==REQUEST_NO)
				{
					memcpy(&respData,pDev->GetDeviceInfo(),sizeof(MPX_DEVICE_INFO));
				}

				//国标插件不运许修改设备的登陆账号和密码
 				if (nWantToReconnect)
 				{
					DelDev(pDvrInfo->szDevID);
 				}
			}
			else
			{
				//如果网关没有设备
				nRet=m_pPDB->SetDvrInfoFromTool(*pDvrInfo);
				nWantToReconnect = TRUE;

				if (nRet==REQUEST_NO)
				{
					memcpy(&respData,pDvrInfo,sizeof(MPX_DEVICE_INFO));
				}
				respData.nToDvrStatus=CONNECTING;
			}

			if (nWantToReconnect)
			{
				CreateDev(&respData);
			}

		}// if(m_pPDB) 
	}

	if (nRet==REQUEST_NO)
	{
		pToolMessageCb(nMessage, nRet==0?0:1, (void*)&respData, sizeof(MPX_DEVICE_INFO), pSession);
	}
	else
	{
		pToolMessageCb(nMessage, nRet==0?0:1, pExtData, nExtSize, pSession);
	}

}

void CMonPlugin::GetDvrInfo( int nMessage, AVCON_MPX_TOOLMESSAGE_CB pToolMessageCb, void*pExtData,int nExtSize, void* pSession )
{
	MAP_DEVICE_INFO mapDvrInfo;
	bool bRet = false;
	if (m_pPDB)
	{
		bRet = m_pPDB->GetDvrInfos(mapDvrInfo, ORDERBY_ASC, 0, "");
	}
	pToolMessageCb(nMessage, bRet?0:1, (void*)&mapDvrInfo, mapDvrInfo.size(), pSession);
}

void CMonPlugin::GetChnInfo( int nMessage, AVCON_MPX_TOOLMESSAGE_CB pToolMessageCb, void*pExtData,int nExtSize, void* pSession )
{
	if (pExtData && nExtSize>0)
	{
		char *szDvrID= (char*)pExtData;
		MAP_CAMERA_INFO CameraInfo;
		bool bRet = false;
		if (m_pPDB)
		{
			bRet = m_pPDB->GetChannelInfo(CameraInfo, "where DevID='%s'", szDvrID);
		}
		pToolMessageCb(nMessage, bRet?0:1, (void*)&CameraInfo, CameraInfo.size(), pSession);
	}
}

void CMonPlugin::GetLogInfo( int nMessage, AVCON_MPX_TOOLMESSAGE_CB pToolMessageCb, void*pExtData,int nExtSize, void* pSession )
{
	if(pExtData && nExtSize > 0 )
	{
		OPTLOG_QUERY *pQuery = (OPTLOG_QUERY*)pExtData;
		VEC_OPT_LOG vecLog;
		int nPageTotal=0;
		if (pQuery)
		{
			bool bRet = m_pPDB->GetVecLog(vecLog, nPageTotal, *pQuery);
			if (bRet)
			{
				pToolMessageCb(MTL_VAL_CMD_LOG_PAGETOTAL, bRet?0:1,  (void*)&nPageTotal, nPageTotal, pSession);
				pToolMessageCb(nMessage, bRet?0:1, (void*)&vecLog, vecLog.size(),  pSession);
			}
		}
	}
}

void CMonPlugin::DelDvrInfo( int nMessage, AVCON_MPX_TOOLMESSAGE_CB pToolMessageCb, void*pExtData,int nExtSize, void* pSession )
{
	if (pExtData && nExtSize>1)
	{
		char *szDvrID= (char*)pExtData;
		bool bRet = false;
		MPX_DEVICE_INFO Dvrinfo;
		SAFE_STRCPY( Dvrinfo.szDevID, szDvrID );
		if (m_pPDB)
		{
			bRet = m_pPDB->DelDvrInfo( Dvrinfo );
		}
		pToolMessageCb(nMessage, bRet?0:1, (void*)&szDvrID, strlen(szDvrID)+1, pSession);

		if (bRet)
		{
			m_Connect.RemoveConnect(Dvrinfo.szDevID);
			DelDev(Dvrinfo.szDevID);
			
		}
	}
}

void CMonPlugin::ModifyChannelInfo( int nMessage, AVCON_MPX_TOOLMESSAGE_CB pToolMessageCb, void*pExtData,int nExtSize, void* pSession )
{
	if (pExtData && nExtSize>1)
	{
		MPX_CAMERA_INFO *pChannelInfo = (MPX_CAMERA_INFO*)pExtData;
		if (m_pPDB)
		{
			CMonDevice* pDev = FindDevice(pChannelInfo->szDevID);
			if (pDev)
			{
				pDev->SetChanParam(pChannelInfo->nChanID, pChannelInfo->szChanName);
			}

		}
	}
}

void CMonPlugin::OnDevConnectCallback_ConnectQueueEmpty( void )
{

}

void CMonPlugin::OnDevConnectCallback_Connect( char*pData, int nLen )
{
	//不需要DveConnect了
	//if ( pData==NULL ||( strlen(pData)+1) !=nLen )
	//{
	//	return ;
	//}
	//std::string strDevID = pData;
	//m_Connect.RemoveConnect(strDevID);

	//CMonDevice*pDev= FindDevice(strDevID);
	//if ( pDev && !pDev->IsConnect() )
	//{
	//	//正式去连接设备
	//	pDev->Connect();
	//}
}

// void CMonPlugin::OnDevSdkCallback_ConnectResult( int nMessage, int nErrCode,  std::string strDevID, void *pbuffer, int nbufLen )
// {
// 	if (nMessage == EM_CONNECT_SUCCESS)
// 	{
// 		if (pbuffer && nbufLen >0)
// 		{
// 			MPX_DEVICE_INFO *pDevInfo = (PMPX_DEVICE_INFO)pbuffer;
// 			if (pDevInfo)
// 			{
// 				CMonDevice *pDev = FindDevice(strDevID);
// 				if (pDev)
// 				{
// 					pDev->SetloginSessionID(pDevInfo->hLogin);
// 					pDev->SetSeriaNo(pDevInfo->szSerialNo);
// 					pDev->SetChnNum(pDevInfo->nAnalogNum );
// 					
// 
// 				}
// 
// 
// 			}
// 		}
// 	}
// 	else if (nMessage == EM_CONNECT_FAILED)
// 	{
// 
// 	}
// 
// }

bool CMonPlugin::DevicesToConnectQueue(const std::string strDevID,int nUrgency/*=FALSE*/ )
{
	bool bConnecting = false;
// 	if (m_Connect.AddConnect( strDevID,nUrgency) )
// 	{
// 		bConnecting = true;
// 	}
	return bConnecting;
}

void CMonPlugin::PushDevice( const std::string strDevID, CMonDevice *pDev )
{
	{
		m_mapKeyAccount[pDev->GetDeviceInfo()->szMTSAccount] = strDevID;
	}
	
	{
		KAutoLock l(m_csDevices);
		IT_MAP_MONDEVICES it = m_mapDevices.find(strDevID);
		if (it!=m_mapDevices.end())
		{
			CMonDevice* pTmpDev = it->second;
			if (pTmpDev)
			{
				pTmpDev->Destroy();
				//delete pTmpDev;
				//pTmpDev = NULL;
				if (m_pRecycleThread)
				{
					m_pRecycleThread->InsertMonDevTime(pTmpDev);
				}
			}
			m_mapDevices.erase(it);
		}
		m_mapDevices[strDevID] = pDev;
	}

}

//ClearDevices是在WaitForStop之后，所以不用管它对map怎么操作
void CMonPlugin::ClearDevices(void)
{
	do 
	{
		CMonDevice* pDev = NULL;

		{
			KAutoLock l(m_csDevices);
			IT_MAP_MONDEVICES it = m_mapDevices.begin();
			if (it!=m_mapDevices.end())
			{
				pDev = it->second;
				m_mapDevices.erase(it);
			}
			else
			{
				break;
			}
		}
		if (pDev)
		{
			//最后的就不用放在垃圾回收线程
			pDev->Destroy();
			delete pDev;
			pDev = NULL;

		}
		
	} while (1);

}
CMonDevice * CMonPlugin::PopDevice( const std::string strDevID )
{
	CMonDevice* pDev = NULL;
	{
		KAutoLock l(m_csDevices);
		IT_MAP_MONDEVICES it = m_mapDevices.find(strDevID);
		if (it!=m_mapDevices.end())
		{
			pDev = it->second;
			m_mapDevices.erase(it);
		}
	}

	if (pDev)
	{
		{
			IT_MAP_MTSUSER_DEVID it = m_mapKeyAccount.find(pDev->GetDeviceInfo()->szMTSAccount);
			if (it!=m_mapKeyAccount.end())
			{
				m_mapKeyAccount.erase(it);
			}
		}

	}
	return pDev;

}

CMonDevice * CMonPlugin::FindDevice( const std::string strDevID, BOOL bAccount/*=FALSE*/ )
{
	string strDevKey = strDevID;
	if (bAccount)
	{
		strDevKey = FindDevID(strDevID);
	}

	{
		KAutoLock l(m_csDevices);
		IT_MAP_MONDEVICES it = m_mapDevices.find(strDevKey);
		if (it!=m_mapDevices.end())
		{
			return it->second;
		}
	}

	return NULL;
}

std::string CMonPlugin::FindDevID( string strMtsAccount )
{
	string strSecondKey = strMtsAccount;

	IT_MAP_MTSUSER_DEVID it = m_mapKeyAccount.find(strMtsAccount);
	if (it!=m_mapKeyAccount.end())
	{
		strSecondKey = it->second;
	}

	return strSecondKey;
}

bool CMonPlugin::CreateAllDevs( void )
{
	if (m_pPDB==NULL)
	{
		return false;
	}

	MAP_DEVICE_INFO mapDevs;
	if ( !m_pPDB->GetDvrInfos( mapDevs,ORDERBY_DESC,0,"" ) )
	{
		return false;
	}

	bool bConnecting = false;
	MAP_DEVICE_INFO::iterator it = mapDevs.begin();
	while (it!=mapDevs.end())
	{
		//MPX_DEVICE_INFO* pDevInfo = new MPX_DEVICE_INFO();
		MPX_DEVICE_INFO DevInfo = it->second;
		CreateDev(&DevInfo);
		it++;
	}

	return TRUE;
}

bool CMonPlugin::CreateDev( MPX_DEVICE_INFO *pDevInfo )
{
	CMonDevice* pDev = new CMonDevice(this);
	if (pDev)
	{
		if ( pDev->Create(INSTANCE_NULL, pDevInfo) )
		{
			PushDevice(pDevInfo->szDevID,pDev);
#ifdef _PING_SNMP
			if (m_pDeviceStateManager)
			{
				m_pDeviceStateManager->SetLoopInterval( m_mapDevices.size()*10000 );
				m_pDeviceStateManager->AddDeviceState(pDevInfo->szDevAddr, pDevInfo->szDevID, false);
			}
#endif
		}
		else
		{
#ifdef _PING_SNMP
			m_pDeviceStateManager->RemoveDevice(pDevInfo->szDevID);
#endif
			//没插进去的就不用放在垃圾回收线程
			pDev->Destroy();
			delete pDev;
		}
	}

	return true;
}

int CMonPlugin::OnMonDeviceCallback_ToConnectQueue( std::string strDevID, int nUrgency/*=FALSE*/ )
{
	if ( m_Connect.AddConnect( strDevID,nUrgency) )
		return TRUE;

	return FALSE;

}

int CMonPlugin::OnMonDeviceCallback_DeviceUpdate( const MPX_DEVICE_INFO*pDevInfo,int nModifyMask )
{
	if (m_pPDB)
	{
		if (!m_pPDB->UpdateDvrInfo(*pDevInfo))
		{
			LOG::ERR("OnHIKDeviceCallback_DeviceUpdate SetDvrInfo Err[%s]\n",pDevInfo->szDevAddr);
		}

		if (nModifyMask&MM_NAME)
		{
			if (m_pMsgCb)
			{
				m_pMsgCb(MESSAGE_MPX_UPDATEDEVNAME,(void*)pDevInfo,pDevInfo->nStructSize,m_pMsgUser);
			}
		}

		return REQUEST_NO;
	}

	return -1;
}

void CMonPlugin::OnMonDeviceCallback_DeviceOnline( const MPX_DEVICE_INFO*pDevInfo )
{
	if (m_pMsgCb)
	{
		m_pMsgCb(MESSAGE_MPX_DEVONLINE,(void*)pDevInfo,pDevInfo->nStructSize,m_pMsgUser);
	}
}

void CMonPlugin::OnMonDeviceCallback_DeviceOffline( const MPX_DEVICE_INFO*pDevInfo )
{
	if (m_pMsgCb)
	{
		m_pMsgCb(MESSAGE_MPX_DEVOFFLINE,(void*)pDevInfo,pDevInfo->nStructSize,m_pMsgUser);
	}
}

int CMonPlugin::OnMonDeviceCallback_ChannelDel(const MPX_CAMERA_INFO*pCamInfo)
{
	int nRet = -1;
	if (m_pPDB && pCamInfo)
	{
		if (!m_pPDB->DelChannelInfo(*pCamInfo))
		{
			LOG::ERR("OnHIKDeviceCallback_ChannelUpdate DelChannelInfo Err[%s]\n",pCamInfo->szDevID);
		}
		nRet = REQUEST_NO;
	}
	return nRet;
}

bool CMonPlugin::OnMonDeviceCallback_GetDBCameraInfo(const std::string& strDevID,MAP_CAMERA_INFO & CameraInfo)
{
	bool bRet = false;
	if (m_pPDB)
	{
		bRet = m_pPDB->GetChannelInfo(CameraInfo, " WHERE DevID='%s'", strDevID.c_str());
	}
	return bRet;
}

int CMonPlugin::OnMonDeviceCallback_ChannelUpdate( const MPX_CAMERA_INFO*pCamInfo,int nModifyMask )
{
	int nRet = -1;

	if (m_pPDB && pCamInfo)
	{
		if (!m_pPDB->SetChannelInfo(*pCamInfo))
		{
			LOG::ERR("OnHIKDeviceCallback_ChannelUpdate SetChannelInfo Err[%s]\n",pCamInfo->szChanName);
		}

		if (nModifyMask&MM_NAME)
		{
			if (m_pMsgCb)
			{
				m_pMsgCb(MESSAGE_MPX_UPDATECNANNELNAME,(void*)pCamInfo,pCamInfo->nStructSize,m_pMsgUser);
			}
		}

		nRet = REQUEST_NO;
	}

	return nRet;
}

void CMonPlugin::OnMonDeviceCallback_ChannelOnline( const MPX_CAMERA_INFO*pCamInfo )
{
	if (m_pMsgCb)
	{
		m_pMsgCb(MESSAGE_MPX_CAMONLINE,(void*)pCamInfo,pCamInfo->nStructSize,m_pMsgUser);
	}
}

void CMonPlugin::OnMonDeviceCallback_ChannelOffline( const MPX_CAMERA_INFO*pCamInfo )
{
	if (m_pMsgCb)
	{
		m_pMsgCb(MESSAGE_MPX_CAMOFFLINE,(void*)pCamInfo,pCamInfo->nStructSize,m_pMsgUser);
	}
}

void CMonPlugin::OnMonDeviceCallback_RespondTalk( const MPX_TALK_INFO*pTalkData )
{
	if (m_pMsgCb)
	{
		m_pMsgCb(MESSAGE_MPX_ON_STARTTALK,(void*)pTalkData,pTalkData->nStructSize+pTalkData->nUserLength,m_pMsgUser);
	}
}

void CMonPlugin::OnMonDeviceCallback_DevChannelChange( MPX_TALK_DEVCHANNLECHANGE* pDevChannelChange )
{
	if (m_pMsgCb)
	{
		m_pMsgCb(MESSAGE_MPX_DEVCHANNLECHANGE,(void*)pDevChannelChange,pDevChannelChange->nStructSize,m_pMsgUser);
	}
}

void CMonPlugin::OnMonDeviceCallback_SetDeviceStatus(  std::string strDevID, std::string strDevIP, bool bDevStatus )
{
#ifdef _PING_SNMP
	if (m_pDeviceStateManager)
	{
		m_pDeviceStateManager->AddDeviceState( strDevIP, strDevID, bDevStatus );
	}
	#endif
}

void CMonPlugin::OnMonDeviceCallback_GetVideoInfo( const MPX_VIDEO_INFO*pVideoInfo )
{
	if (m_pMsgCb)
	{
		m_pMsgCb(MESSAGE_MPX_VIDEOINFO,(void*)pVideoInfo,pVideoInfo->nStructSize,m_pMsgUser);
	}
}

std::string CMonPlugin::OnMonDeviceCallback_GetAlarmServerIP()
{
	return m_AlarmSingleton.GetAlarmServerIP();
}

unsigned int CMonPlugin::OnMonDeviceCallback_GetAlarmServerPort()
{
	return m_AlarmSingleton.GetAlarmServerPort();
}

//ping 回调处理
#ifdef _PING_SNMP
void CMonPlugin::OnStateChangedEvent( std::string strID,bool bState )
{
	printf("ping 回调处理, strDevID:%s %s\n", strID.c_str(), bState?"在线":"不在线" );

	CMonDevice *pDev=FindDevice(strID);
	if (pDev)
	{
		pDev->DevStatusChange(bState);
	}
	

}
#endif

void CMonPlugin::DelDev( char *szDevID )
{
	CMonDevice *pDev = PopDevice(szDevID);
	if (pDev)
	{
#ifdef _PING_SNMP
		if (m_pDeviceStateManager)
		{
			m_pDeviceStateManager->RemoveDevice(szDevID);
		}
#endif
		pDev->Destroy();
		//放进回收线程
		//delete pDev;
		//pDev = NULL;
		if (m_pRecycleThread)
		{
			m_pRecycleThread->InsertMonDevTime(pDev);
		}
	}
}

BOOL CMonPlugin::GetAudioInfo( const char*szDevID,int nChanID,MPX_AUDIO_INFO*pAudioInfo )
{
	return TRUE;
}

BOOL CMonPlugin::SetAudioInfo( const char*szDevID,int nChanID,MPX_AUDIO_INFO*pAudioInfo )
{
	return TRUE;
}

BOOL CMonPlugin::GetVideoInfo( const char*szDevID,int nChanID,MPX_VIDEO_INFO*pVideoInfo )
{
	CMonDevice* pDev = FindDevice(szDevID,TRUE);
	if (pDev && m_pPDB)
	{
		pDev->GetVideoInfo(nChanID, pVideoInfo);
	}	
	return TRUE;
}

BOOL CMonPlugin::SetVideoInfo( const char*szDevID,int nChanID,MPX_VIDEO_INFO*pVideoInfo )
{
	CMonDevice* pDev = FindDevice(szDevID,TRUE);
	if (pDev && m_pPDB)
	{
		pDev->SetVideoInfo(nChanID, pVideoInfo);
	}	
	return TRUE;
}

void CMonPlugin::OnSDKCallback_RegisterDevice( char *szIPAddr, int nPort, char* szDevUser, char *szPwd, int nDevType, void*pUserData,int nLen )
{
	if (strcmp(szDevUser, "") == 0)
	{
		return;
	}
	CMonDevice *pDev = FindDevice(szDevUser);
	if (pDev == NULL)
	{
		MPX_DEVICE_INFO tempDvrInfo;
		if (m_pPDB)
		{
			if(!m_pPDB->GetDvrInfo(tempDvrInfo,"where DevID='%s'",szDevUser))
			{
				//数据库里不还没设备信息
				MPX_DEVICE_INFO Devinfo;
				SAFE_STRCPY(Devinfo.szDevID, szDevUser);
				SAFE_STRCPY(Devinfo.szDevAddr, szIPAddr);
				SAFE_STRCPY(Devinfo.szDevUser, szDevUser);
				SAFE_STRCPY(Devinfo.szDevPass, szPwd);
				SAFE_STRCPY(Devinfo.szSerialNo, szDevUser);
				Devinfo.nDevPort = nPort;
				Devinfo.nDevType = nDevType;
				Devinfo.nToDvrStatus=CONNECTED;
				if (m_pPDB)
				{
					m_pPDB->SetDvrInfoFromTool(Devinfo);
				}
				CreateDev(&Devinfo);
			}
			else
			{
				//数据里已经有了就用数据库里的数据进行创建
				//update ip and port
				SAFE_STRCPY(tempDvrInfo.szDevAddr,szIPAddr);
				tempDvrInfo.nDevPort=nPort;
				tempDvrInfo.nToDvrStatus=CONNECTED;
				if (m_pPDB)
				{
					m_pPDB->UpdateDvrInfo(tempDvrInfo);
				}
				CreateDev(&tempDvrInfo);
			}
		}
	}
	else
	{
		//如果网关已经有设备了,就上线
		pDev->Connect();

	}
}

void CMonPlugin::OnSDKCallback_UnRegisterDevice( char *szDevID )
{
	printf("The %s is unregister!",szDevID);
	DelDev(szDevID);
}

bool CMonPlugin::OnSDKCallback_IsPasswordReady(char * szDevID)
{
	if (m_pPDB)
	{
		MPX_DEVICE_INFO tempDvrInfo;
		if(m_pPDB->GetDvrInfo(tempDvrInfo,"where DevID='%s'",szDevID))
		{
			return true;
		}
		else
		{
			//Query password
			SAFE_STRCPY(tempDvrInfo.szDevID,szDevID);
			tempDvrInfo.nDevType=RTSP_SIP;
			if (m_pMsgCb)
			{
				printf("Query device info...%s\r\n", szDevID);
				m_pMsgCb(MESSAGE_MPX_GETMONDEVINFO,(void*)&tempDvrInfo,tempDvrInfo.nStructSize,m_pMsgUser);
			}
			return false;
		}
	}
	return false;
}

std::string CMonPlugin::OnSDKCallback_QueryPassword(char * szDevID)
{
	if (m_pPDB)
	{
		MPX_DEVICE_INFO tempDvrInfo;
		if(m_pPDB->GetDvrInfo(tempDvrInfo,"where DevID='%s'",szDevID))
		{
			return tempDvrInfo.szDevPass;
		}
	}
	return "";
}

void CMonPlugin::OnVOD_SearchFile(unsigned long hPlay, const char* szDevID, int nChanID, unsigned long ulType, const char* szStartTime, const char* szEndTime)
{
	CMonDevice* pDev = FindDevice(szDevID,TRUE);
	if (pDev && m_pPDB)
	{
		pDev->SearchFile(hPlay,nChanID,ulType,szStartTime,szEndTime);
	}
}

void CMonPlugin::OnVOD_PlayByFile(unsigned long hPlay, const char* szDevID, int nChanID, unsigned int nWndID, const char* szFileName)
{
	CMonDevice* pDev = FindDevice(szDevID,TRUE);
	if (pDev && m_pPDB)
	{
		pDev->PlayByFile(hPlay,nChanID,nWndID,szFileName);
	}
}

void CMonPlugin::OnVOD_PlayByTime(unsigned long hPlay, const char* szDevID, int nChanID, unsigned int nWndID, const char* szStartTime, const char* szEndTime)
{
	CMonDevice* pDev = FindDevice(szDevID,TRUE);
	if (pDev && m_pPDB)
	{
		pDev->PlayByTime(hPlay,nChanID,nWndID,szStartTime,szEndTime);
	}
}

void CMonPlugin::OnVOD_PlayByAbsoluteTime(unsigned long hPlay, const char* szDevID, int nChanID, unsigned int nWndID, const char* szAbsoluteTime)
{
	CMonDevice* pDev = FindDevice(szDevID,TRUE);
	if (pDev && m_pPDB)
	{
		pDev->PlayByAbsoluteTime(hPlay,nChanID,nWndID,szAbsoluteTime);
	}
}

void CMonPlugin::OnVOD_PlayPause(unsigned long hPlay, const char* szDevID, int nChanID, unsigned int nWndID, long nState)
{
	CMonDevice* pDev = FindDevice(szDevID,TRUE);
	if (pDev && m_pPDB)
	{
		pDev->PlayPause(hPlay,nChanID,nWndID,nState);
	}
}

void CMonPlugin::OnVOD_PlaySpeedControl(unsigned long hPlay, const char* szDevID, int nChanID, unsigned int nWndID, double dSpeed)
{
	CMonDevice* pDev = FindDevice(szDevID,TRUE);
	if (pDev && m_pPDB)
	{
		pDev->PlaySpeedControl(hPlay,nChanID,nWndID,dSpeed);
	}
}

void CMonPlugin::OnVOD_PlayFastForward(unsigned long hPlay, const char* szDevID, int nChanID, unsigned int nWndID)
{
	CMonDevice* pDev = FindDevice(szDevID,TRUE);
	if (pDev && m_pPDB)
	{
		pDev->PlayFastForward(hPlay,nChanID,nWndID);
	}
}

void CMonPlugin::OnVOD_PlaySlowDown(unsigned long hPlay, const char* szDevID, int nChanID, unsigned int nWndID)
{
	CMonDevice* pDev = FindDevice(szDevID,TRUE);
	if (pDev && m_pPDB)
	{
		pDev->PlaySlowDown(hPlay,nChanID,nWndID);
	}
}

void CMonPlugin::OnVOD_PlayNormal(unsigned long hPlay, const char* szDevID, int nChanID, unsigned int nWndID)
{
	CMonDevice* pDev = FindDevice(szDevID,TRUE);
	if (pDev && m_pPDB)
	{
		pDev->PlayNormal(hPlay,nChanID,nWndID);
	}
}

void CMonPlugin::OnVOD_PlayStop(unsigned long hPlay, const char* szDevID, int nChanID, unsigned int nWndID)
{
	CMonDevice* pDev = FindDevice(szDevID,TRUE);
	if (pDev && m_pPDB)
	{
		pDev->PlayStop(hPlay,nChanID,nWndID);
	}
}

void CMonPlugin::OnVOD_DownloadFile(unsigned long hDownload, const char* szDevID, int nChanID, unsigned int nWndID, const char* szFileName)
{
	CMonDevice* pDev = FindDevice(szDevID,TRUE);
	if (pDev && m_pPDB)
	{
		pDev->DownloadFileByName(hDownload,nChanID,nWndID,szFileName);
	}
}

void CMonPlugin::OnVOD_DownloadFile(unsigned long hDownload, const char* szDevID, int nChanID, unsigned int nWndID, const char* szStartTime, const char* szEndTime)
{
	CMonDevice* pDev = FindDevice(szDevID,TRUE);
	if (pDev && m_pPDB)
	{
		pDev->DownloadFileByTime(hDownload,nChanID,nWndID,szStartTime,szEndTime);
	}
}

void CMonPlugin::OnVOD_StopDownload(unsigned long hDownload, const char* szDevID, int nChanID, unsigned int nWndID)
{
	CMonDevice* pDev = FindDevice(szDevID,TRUE);
	if (pDev && m_pPDB)
	{
		pDev->StopDownload(hDownload,nChanID,nWndID);
	}
}
