#include "StdAfx.h"
#include "MonDevice.h"

#ifdef WIN32
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

CMonDevice::CMonDevice(IMonDeviceCallback *pCallback)
: m_pDevCallback(pCallback)
, m_hLogin(INSTANCE_NULL)
, m_ulLastOptTime(0)
, m_bWantToDestroy(false)
, m_pDeviceSdk(NULL)
, m_nTalkCodecID(X_AUDIO_CODEC_G711A)
, m_bTalkTimerout(false)
, m_pTalk(NULL)
, m_nMtsOnlineTime(0)
{
	m_pDeviceSdk = HPMON_IDeviceSdk::Create(this);
}

CMonDevice::~CMonDevice(void)
{
	if (m_pDeviceSdk)
	{
		delete m_pDeviceSdk;
		m_pDeviceSdk = NULL;
	}

	for(std::list<CMonChannel*>::iterator it=m_lstMonChannel.begin();it!=m_lstMonChannel.end();it++)
	{
		CMonChannel * pChan=*it;
		if (pChan!=NULL)
		{
			delete pChan;
			pChan=NULL;
		}
	}
}

void CMonDevice::OnDevSdkCallback_ConnectResult( int nMessage, int nErrCode, std::string strDevID, void *pbuffer, int nBuflen )
{
	if (nMessage == EM_DEVSDK_CONNECT_SUCCESS)
	{

		m_mpxDevInfo.nToDvrStatus = ONLINE;
		if (m_pDeviceSdk)
		{
			m_pDeviceSdk->MON_DEVSDK_GetDevName();
			m_pDeviceSdk->MON_DEVSDK_GetDevStatus();
			m_pDeviceSdk->MON_DEVSDK_GetModelType();
			m_pDeviceSdk->MON_DEVSDK_GetSerialNo();
			m_pDeviceSdk->MON_DEVSDK_GetChannelNum();
			m_pDeviceSdk->MON_DEVSDK_GetTalkCodecID();

		}
		if (m_pDevCallback)
		{
			m_pDevCallback->OnMonDeviceCallback_SetDeviceStatus( m_mpxDevInfo.szDevID, m_mpxDevInfo.szDevAddr, true);
		}
	}
	else if (nMessage == EM_DEVSDK_CONNECT_FAILED)
	{
		if (m_pDevCallback)
		{
			printf("CONNECT_FAILED szdeviD:%s \n", m_mpxDevInfo.szDevID);
			m_pDevCallback->OnMonDeviceCallback_SetDeviceStatus( m_mpxDevInfo.szDevID, m_mpxDevInfo.szDevAddr, false);
		}
		m_mpxDevInfo.nToDvrStatus = OFFLINE;
	}

	SaveToDatabase();
}

void CMonDevice::OnDevSdkCallback_DisConnect( )
{	
	m_mpxDevInfo.nToDvrStatus = OFFLINE;
	SaveToDatabase();
	printf("DisConnect szdeviD:%s \n", m_mpxDevInfo.szDevID);
	if (m_pDevCallback)
	{
		m_pDevCallback->OnMonDeviceCallback_SetDeviceStatus( m_mpxDevInfo.szDevID, m_mpxDevInfo.szDevAddr, false);
	}
}

void CMonDevice::OnDevSdkCallback_RecMessageCb( int nMessage, void *pBuf,int nBuflen )
{
	if ( pBuf && nBuflen>0)
	{
		switch (nMessage)
		{

		case EM_DEVSDK_SET_DEV_NAME:

			break;
		case EM_DEVSDK_GET_DEV_NAME:
			{
				//如果设备名字已经存在就不要再覆盖了
				int nStrLen=strlen(m_mpxDevInfo.szDevName);
				bool bNameExist=false;
				for (int i=0;i<nStrLen;i++)
				{
					//如果有非空格就代表名字非空,
					//对于win32,char 是signed char ,是一个负数，转成isspace时int就变成一个很大的数，大于256
					if(!isspace((unsigned char)m_mpxDevInfo.szDevName[i]))
					{
						bNameExist=true;
						break;
					}
				}
				if (!bNameExist)
				{
					SAFE_STRCPY(m_mpxDevInfo.szDevName, (char*)pBuf);
				}
			}
			break;
		case EM_DEVSDK_GET_DEV_CHANNELNUM:
			{
				int *pChnNum = (int*)pBuf;
				SetChnNum( *pChnNum );
				if (m_pDeviceSdk)
				{
					m_pDeviceSdk->MON_DEVSDK_GetChannelInfo();
				}
			}
			break;
		case EM_DEVSDK_GET_DEV_SERIALNO:
			{	
				SAFE_STRCPY(m_mpxDevInfo.szSerialNo, (char*)pBuf);
			}
			break;
		case EM_DEVSDK_GET_DEV_MODELTYPE:

			break;
		case EM_DEVSDK_GET_TALKCODECID:	//获取对讲音频格式 
			{
				int *pTalkCodecID = (int*)pBuf;
				m_nTalkCodecID  = *pTalkCodecID;
				printf("CMonDevice TalkCodecType:%d\r\n",m_nTalkCodecID);

			}
			break;
		case EM_DEVSDK_UPDATE_DEV_IP:
			{
				MPX_DEVICE_INFO * tDeviceInfo=(MPX_DEVICE_INFO *)pBuf;
				if (sizeof(MPX_DEVICE_INFO)==nBuflen)
				{
					SAFE_STRCPY(m_mpxDevInfo.szDevAddr,tDeviceInfo->szDevAddr);
					m_mpxDevInfo.nDevPort=tDeviceInfo->nDevPort;
				}
			}
			break;
		}
		SaveToDatabase();
	}
}

void CMonDevice::OnDevSdkCallback_RecChannelInfo( int nHPChnID, int nChnStatus, HPMON_IChannelSdk* pChnSDK )
{
	CreateMonChannel(nHPChnID,nChnStatus, pChnSDK);
}

void CMonDevice::OnDevSdkCallback_Exception( int nErrorCode, void *pBuf,int nBuflen )
{
	

}

void CMonDevice::OnDevSdkCallback_DevTalkResult( int nErrCode, std::string strDevID, void *pBuffer, int nBufLen )
{
	if (m_pTalk)
	{
		m_pTalk->OnOpenDevTalkChn(nErrCode, pBuffer, nBufLen);
	}
}

void CMonDevice::OnDevSdkCallback_DevTalkVoiceStream( unsigned int dwMediaType, unsigned char *pBuffer, unsigned int dwBufSize, unsigned char*pExtraData,unsigned int nExtraSize )
{
	if (m_pTalk)
	{
		m_pTalk->OnVoiceStream(dwMediaType, pBuffer, dwBufSize, pExtraData,nExtraSize );
	}
}

void CMonDevice::OnDevSdkCallback_DevAlarmNotify(int nAlarmID,int nAlarmType,int nAlarmSubType,std::string strAlarmTime,void * pExtraData,unsigned int nExtraSize)
{
	if (m_mpxDevInfo.nToMtsStatus!=ONLINE)
	{
		printf("[GB28181] The device is not online(alarm)!\n");
		return ;
	}

	KCmdPacket outPacket(MONALARM_KEY_MSGID, MONALARM_CMD_UPLOAD_INFO, "" );
	outPacket.SetAttrib(MONALARM_KEY_USERCMD, CMD_MONALARM_UPLOAD_ALARMINFO);
	outPacket.SetAttribUN( MONALARM_KEY_PROTO_VERSION, MONALARM_PROTOCAL_VERSION );
	outPacket.SetAttrib( MONALARM_KEY_DEVID, m_mpxDevInfo.szMTSAccount);
	outPacket.SetAttrib( MONALARM_KEY_DEVNAME,m_mpxDevInfo.szDevName);

	switch(nAlarmType)
	{
	case 2://设备IO输入报警
		{
			outPacket.SetAttribUN(MONALARM_KEY_MAIN_ALARMTYPE, MONALARM_MAIN_IOINPUT);
			outPacket.SetAttribUN( MONALARM_KEY_SUB_ALARMTYPE,MONALARM_SUB_MANUALLY);	//默认子类型是手动报警
		}
		break;
	case 5://视频报警,没区分输入还是输出
		{
			outPacket.SetAttribUN(MONALARM_KEY_MAIN_ALARMTYPE, MONALARM_MAIN_VIDEOINPUT);
			if (nAlarmSubType==1)
			{
				outPacket.SetAttribUN( MONALARM_KEY_SUB_ALARMTYPE,MONALARM_SUB_MONITOR);	//1是移动侦测报警
			}
		}
		break;
	default:
		printf("[GB28181] alarm type[%d] is not handle\n", nAlarmType);
		//暂时没有的报警类型就不上报了
		return;
	}


	
	outPacket.SetAttribUN( MONALARM_KEY_CHNID,nAlarmID);
	int nLoc=strAlarmTime.find('T');
	if (nLoc!=string::npos)
	{
		strAlarmTime.replace(nLoc,1," ");
	}
	outPacket.SetAttrib(MONALARM_KEY_ALARMTIME,strAlarmTime.c_str());
	std::string strSendData =  outPacket.GetString();

	KUDPSocket tUDPSocket;
	std::string strIP="";
	int nPort=0;
	//Fun::ReadAlarmServerString("AlarmServer","IP",strIP,"");
	//Fun::ReadAlarmServerInt("AlarmServer","Port",nPort,0);
	strIP = m_pDevCallback->OnMonDeviceCallback_GetAlarmServerIP();
	nPort = m_pDevCallback->OnMonDeviceCallback_GetAlarmServerPort();

	if(strIP !="" && nPort !=0 && tUDPSocket.Connect(strIP,nPort))
	{
		tUDPSocket.SendTo(strIP.c_str(), nPort,(char*)strSendData.c_str(), strSendData.length()+1);
	}
}

int CMonDevice::Create( long hLogin, const MPX_DEVICE_INFO *pDevInfo )
{
	if ( pDevInfo==NULL 
		|| pDevInfo->nStructSize!=m_mpxDevInfo.nStructSize)
	{
		return FALSE;
	}
	UpdateLastOptTime();
	if (m_pDeviceSdk)
	{
		m_pDeviceSdk->MON_DEVSDK_Init();
	}

	m_bWantToDestroy = false;
 	m_hLogin = hLogin;
 
 	memcpy(&m_mpxDevInfo,pDevInfo,sizeof(MPX_DEVICE_INFO));
 	m_mpxDevInfo.hLogin = m_hLogin;
 	//m_mpxDevInfo.nDevType = HIK_DVR;
 	m_mpxDevInfo.nToDvrStatus = CONNECTING;
 	m_mpxDevInfo.nToMtsStatus = OFFLINE;

	//把数据库里的所有通道名字保存起来
	if (m_pDevCallback)
	{
		MAP_CAMERA_INFO mapCameraInfo;
		if (m_pDevCallback->OnMonDeviceCallback_GetDBCameraInfo(m_mpxDevInfo.szDevID,mapCameraInfo))
		{
			printf("[GB28181]:Get [%s] the camera info![CMonDevice::Create]\n",m_mpxDevInfo.szDevID);
			IT_MAP_CAMERA_INFO it=mapCameraInfo.begin();
			for (;it!=mapCameraInfo.end();it++)
			{
				printf("[GB28181]:Get the camera info from the database %d:%s ![CMonDevice::Create]\n",it->second.nChanID,it->second.szChanName);
				m_mapIDDBName[it->second.nChanID]=NONULLSTR(it->second.szChanName);
			}
		}
		else
		{
			printf("[GB28181]:Get the camera info failed from the database![CMonDevice::Create]\n");
		}
	}
	
 	if (m_pDevCallback && pDevInfo->nToDvrStatus==CONNECTED)
 	{
 		Connect();//连接进去就可以连接
 	}
	return TRUE;
}

void CMonDevice::Destroy( void )
{
	printf("CMonDevice::Destroy..\n");

	//对讲时删除设备要先停止对讲
	if (m_pTalk)
	{
		StopTalk(NULL);
	}
	m_bWantToDestroy = true;
	UpdateLastOptTime();

	ClearMonChns();
	if (m_pDeviceSdk)
	{
		m_pDeviceSdk->MON_DEVSDK_DisConnect();
		m_pDeviceSdk->MON_DEVSDK_UnInit();
	}

	m_pDevCallback->OnMonDeviceCallback_DeviceOffline(&m_mpxDevInfo);

	m_mpxDevInfo.nToMtsStatus = OFFLINE;
	m_mpxDevInfo.nToDvrStatus = DISCONNECTED;
	SaveToDatabase();

}

MPX_DEVICE_INFO * CMonDevice::GetDeviceInfo()
{
	return &m_mpxDevInfo;
}

void CMonDevice::ProcessMtsEvents( bool bLoginSuccessed )
{
	if (m_bWantToDestroy)
	{
		return ;
	}

	if (bLoginSuccessed!=true)
	{
		if (m_mpxDevInfo.nToMtsStatus==ONLINE)
		{
			//通道下线
			AllProcessChannelEvent(false);
			m_pDevCallback->OnMonDeviceCallback_DeviceOffline(&m_mpxDevInfo);

			m_mpxDevInfo.nToMtsStatus = OFFLINE;
			SaveToDatabase();

		}
	}	
	else
	{
		//发布上线
		//控制上线间隔
		unsigned long ulOnlineTimeInterval = XGetTimestamp()-m_nMtsOnlineTime;
		if (ulOnlineTimeInterval<TIMEOUT_ONLINE_INTERVAL)
		{
			return ;
		}
		//上一次上线,服务没反馈,超时后再重新上线
		if (m_mpxDevInfo.nToMtsStatus==ONLINE_BUSY 
			&& ulOnlineTimeInterval<TIMEOUT_DEVICE_ONLINE)
		{
			return ;
		}

		if (m_mpxDevInfo.nToDvrStatus!=ONLINE)
		{
			if ( m_mpxDevInfo.nToMtsStatus == ONLINE)
			{
				if (m_mpxDevInfo.nToDvrStatus==CONNECTING)
				{
					return ;
				}

				m_pDevCallback->OnMonDeviceCallback_DeviceOffline(&m_mpxDevInfo);
				m_mpxDevInfo.nToDvrStatus = DISCONNECTED;
				SaveToDatabase();

			}
		}
		else
		{
			if (m_mpxDevInfo.nToMtsStatus!=ONLINE)
			{
				if (m_mpxDevInfo.nToDvrStatus == ONLINE)
				{
					
					m_nMtsOnlineTime = XGetTimestamp();
					//SaveToDatabase();
					m_mpxDevInfo.nToMtsStatus = ONLINE_BUSY;
					SaveToDatabase();
					m_pDevCallback->OnMonDeviceCallback_DeviceOnline(&m_mpxDevInfo);
					
				}
			}
			else
			{
				AllProcessChannelEvent(m_mpxDevInfo.nToMtsStatus==ONLINE);
			}
		}

	}

}

void CMonDevice::ProcessDvrEvents( void )
{
	if (m_bWantToDestroy)
	{
		return ;
	}

	if (m_mpxDevInfo.nToDvrStatus!=CONNECTED)
	{
		return ;
	}

	//控制该设备长时间没操作，就是断链
	unsigned long ulOpertorTimeInterval = XGetTimestamp()-m_ulLastOptTime;
	if ( ulOpertorTimeInterval > TIMEOUT_DEVICE_NOTOPERTOR && m_mpxDevInfo.nToDvrStatus == ONLINE )
	{
		UpdateLastOptTime();
		m_pDeviceSdk->MON_DEVSDK_TimeOutNotOperatorDisConnect();
	}


	if (m_bTalkTimerout)
	{
		//对讲超时处理
		StopTalk(NULL);
		m_bTalkTimerout = false;
	}

	std::list<int> lstChanID;
	{
		KAutoLock l(m_csMonChannel);
		for (IT_MAP_MONCHANNEL it=m_mapMonChannel.begin();it!=m_mapMonChannel.end();it++)
		{
			lstChanID.push_back(it->first);
		}
	}

	CMonChannel * pChan=NULL;
	for (std::list<int>::iterator it=lstChanID.begin();it!=lstChanID.end();it++)
	{
		int nChanID=*it;
		{
			KAutoLock l(m_csMonChannel);
			IT_MAP_MONCHANNEL it=m_mapMonChannel.find(nChanID);
			if (it!=m_mapMonChannel.end())
			{
				pChan=it->second;
			}
		}

		if (pChan!=NULL)
		{
			pChan->ProcessDvrEvents();
		}
	}
}

int CMonDevice::SaveToDatabase( int nModifyMask/*=MM_NONE*/ )
{
	UpdateLastOptTime();
	if (m_pDevCallback)
	{
		return m_pDevCallback->OnMonDeviceCallback_DeviceUpdate(&m_mpxDevInfo, nModifyMask);
	}

	return false;
}

void CMonDevice::UpdateLastOptTime()
{
	m_ulLastOptTime = XGetTimestamp();
}

void CMonDevice::SetloginSessionID( long hLogin )
{
	m_mpxDevInfo.hLogin = hLogin;
}

void CMonDevice::SetChnNum( int nChnNum )
{
	m_mpxDevInfo.nAnalogNum = nChnNum;
}

void CMonDevice::SetSeriaNo( string strSerialNo )
{
	SAFE_STRCPY( m_mpxDevInfo.szSerialNo, strSerialNo.c_str() );
}

void CMonDevice::PushMonChannel( int nChnID, CMonChannel *pChn )
{
	KAutoLock l(m_csMonChannel);
	m_mapMonChannel[nChnID] = pChn;
}

CMonChannel * CMonDevice::FindMonChn( int nChnID )
{
	CMonChannel *pResltChn = NULL;
	{
		KAutoLock l(m_csMonChannel);
		IT_MAP_MONCHANNEL it = m_mapMonChannel.find(nChnID);
		if (it!=m_mapMonChannel.end())
		{
			pResltChn = it->second;
		}
	}
	return pResltChn;

}
void CMonDevice::ClearMonChns()
{
	do 
	{
		CMonChannel* pResltChn = NULL;
		{
			KAutoLock l(m_csMonChannel);
			IT_MAP_MONCHANNEL it = m_mapMonChannel.begin();
			if (it!=m_mapMonChannel.end())
			{
				pResltChn = it->second;
				m_mapMonChannel.erase(it);
			}
			else
			{
				break;
			}
		}

		if (pResltChn)
		{
			pResltChn->Destory();
			m_lstMonChannel.push_back(pResltChn);
		}

	} while (1);

}

void CMonDevice::CreateMonChannel( int nChnID,int nChnStatus, HPMON_IChannelSdk *pChannelSdk )
{
//	if ( m_mapMonChannel.size()==0 )
	{
		//for (int i=0; i<nChnNum; i++)
		{
			//内存泄漏
			CMonChannel *pMonChn = new CMonChannel(this, pChannelSdk);
			printf("[GB28181]: Set the channel memory name to db name![CMonDevice::CreateMonChannel]\n");
			pMonChn->SetMemChannelName(m_mapIDDBName[nChnID]);
			if (pMonChn->Create(nChnID, &m_mpxDevInfo))
			{
				pMonChn->SetChannelStatus(nChnStatus);
				PushMonChannel(nChnID, pMonChn);
			}
			else
			{
				pMonChn->Destory();
				delete pMonChn;
			}
		}

	}

}

bool CMonDevice::IsNeedDisConnect()
{
	return false;
}

void CMonDevice::Connect()
{
	UpdateLastOptTime();
	if (m_pDeviceSdk)
	{
		m_pDeviceSdk->MON_DEVSDK_Connect(m_mpxDevInfo.szDevID, m_mpxDevInfo.szDevAddr, m_mpxDevInfo.szDevUser, m_mpxDevInfo.szDevPass,
			m_mpxDevInfo.nDevPort, NULL, 0);
	}
}

void CMonDevice::DisConnect( void )
{
	UpdateLastOptTime();
	if (m_pDeviceSdk)
	{
		m_pDeviceSdk->MON_DEVSDK_DisConnect();
	}
}

bool CMonDevice::IsConnect()
{
	return m_mpxDevInfo.nToDvrStatus==ONLINE?true:false;
}

int CMonDevice::SetMtsParam( const std::string strMtsUser, const std::string strMtsPass )
{
	int nRet = REQUEST_NO;
	if (strMtsUser.compare(m_mpxDevInfo.szMTSAccount)!=0
		|| strMtsPass.compare(m_mpxDevInfo.szMTSPass)!=0)
	{
		//通知通道修改MTS账号(MTS通道下线)
		std::list<int> lstChanID;
		{
			KAutoLock l(m_csMonChannel);
			for (IT_MAP_MONCHANNEL it=m_mapMonChannel.begin();it!=m_mapMonChannel.end();it++)
			{
				lstChanID.push_back(it->first);
			}
		}

		CMonChannel * pChan=NULL;
		for (std::list<int>::iterator it=lstChanID.begin();it!=lstChanID.end();it++)
		{
			int nChanID=*it;
			{
				KAutoLock l(m_csMonChannel);
				IT_MAP_MONCHANNEL it=m_mapMonChannel.find(nChanID);
				if (it!=m_mapMonChannel.end())
				{
					pChan=it->second;
				}
			}

			if (pChan!=NULL)
			{
				pChan->OnMtsParamChanged(strMtsUser);
			}
		}

		//通知MTS设备下线	
		m_pDevCallback->OnMonDeviceCallback_DeviceOffline(&m_mpxDevInfo);
		
		//m_mpxDevInfo.nToMtsStatus=OFFLINE;
		SAFE_STRCPY(m_mpxDevInfo.szMTSAccount, strMtsUser.c_str());
		SAFE_STRCPY(m_mpxDevInfo.szMTSPass, strMtsPass.c_str());

		nRet = SaveToDatabase();
	}

	return nRet;
}

int CMonDevice::SetDevParam( const std::string strDevName, const std::string strDevUser, const std::string strDevPwd, int& nWantToReconnect )
{
	int nRet = REQUEST_NO;
	nWantToReconnect = FALSE;
	BOOL bModified = FALSE;
	int nModeifyMask = MM_NONE;

	do 
	{
		if (strDevName.compare(m_mpxDevInfo.szDevName)!=0)
		{
			nRet = SaveNameToDevice(strDevName);
			if (nRet!=REQUEST_NO)
			{
				break;
			}

			SAFE_STRCPY(m_mpxDevInfo.szDevName,strDevName.c_str());
			bModified = TRUE;
			nModeifyMask = MM_NAME;
		}

		if (strDevUser.compare(m_mpxDevInfo.szDevUser)!=0
			|| strDevPwd.compare(m_mpxDevInfo.szDevPass)!=0)
		{
			//需求断掉重连

 			SAFE_STRCPY(m_mpxDevInfo.szDevUser,strDevUser.c_str());
 			SAFE_STRCPY(m_mpxDevInfo.szDevPass,strDevPwd.c_str());
 			m_mpxDevInfo.nToMtsStatus = OFFLINE;
 			m_mpxDevInfo.nToDvrStatus = DISCONNECTED;
 
 			bModified = TRUE;
 			nWantToReconnect = TRUE;
		}

	} while (0);

	if (bModified)
	{
		nRet = SaveToDatabase(nModeifyMask);
	}

	return nRet;
}

int CMonDevice::SaveNameToDevice( const std::string&strName )
{
	if (m_pDeviceSdk)
	{
		UpdateLastOptTime();
		m_pDeviceSdk->MON_DEVSDK_SetDevName( strName.c_str() );
	}

	return REQUEST_NO;
	
}

void CMonDevice::OnMonChannelCallback_ChannelUpdate( const MPX_CAMERA_INFO*pCamInfo,int nModifyMask )
{
	if (m_pDevCallback)
	{
		m_pDevCallback->OnMonDeviceCallback_ChannelUpdate(pCamInfo, nModifyMask);
	}

}

void CMonDevice::OnMonChannelCallback_ChannelDel( const MPX_CAMERA_INFO*pCamInfo)
{
	if (m_pDevCallback)
	{
		m_pDevCallback->OnMonDeviceCallback_ChannelDel(pCamInfo);
	}

}

void CMonDevice::OnMonChannelCallback_ChannelOnline( const MPX_CAMERA_INFO*pCamInfo )
{
	if (m_pDevCallback)
	{
		m_pDevCallback->OnMonDeviceCallback_ChannelOnline(pCamInfo);
	}
}

void CMonDevice::OnMonChannelCallback_ChannelOffline( const MPX_CAMERA_INFO*pCamInfo )
{
	if (m_pDevCallback)
	{
		m_pDevCallback->OnMonDeviceCallback_ChannelOffline(pCamInfo);
	}
}

void CMonDevice::OnDeviceOnline( int nErrCode )
{
	bool bOnlined = false;
	if (nErrCode!=MPX_ERR_NO)
	{
		bOnlined = false;
		m_mpxDevInfo.nToMtsStatus = nErrCode;
	}
	else
	{
		m_mpxDevInfo.nToMtsStatus = ONLINE;
		bOnlined = true;
	}
	//CMonPluginHik::LogSystem(nErrCode,m_mpxDevInfo.szDevID,"设备上线");
	SaveToDatabase();

	AllProcessChannelEvent(bOnlined);
}

void CMonDevice::OnDeviceOffline( int nErrCode )
{
	bool bOnlined = false;
	if (nErrCode!=nErrCode)
	{
	}
	else
	{
		bOnlined = false;
		m_mpxDevInfo.nToMtsStatus = OFFLINE;
		//CMonPluginHik::LogSystem(nErrCode,m_mpxDevInfo.szDevID,"设备下线");

		SaveToDatabase();
	}

	AllProcessChannelEvent(bOnlined);
}

void CMonDevice::OnChannelOnline( int nChanID,int nErrCode )
{
	CMonChannel* pCapChan = FindMonChn(nChanID);
	if (pCapChan)
	{
		pCapChan->OnChannelOnline(nErrCode);
	}
}

void CMonDevice::OnChannelOffline( int nChanID,int nErrCode )
{
	CMonChannel* pCapChan = FindMonChn(nChanID);
	if (pCapChan)
	{
		pCapChan->OnChannelOffline(nErrCode);
	}
}

BOOL CMonDevice::StartTalk( MPX_TALK_INFO*pTalkData )
{
	UpdateLastOptTime();
	if (pTalkData->nDevType == HIK_DVR || pTalkData->nDevType == DAH_DVR
		|| pTalkData->nDevType==RTSP_SIP)
	{
		//对讲通道是一个独立通道
		//与设备对讲
		if (m_pTalk==NULL)
		{
			m_pTalk = new CDevTalk(this, m_nTalkCodecID);
			if (m_pTalk)
			{
				if (m_pTalk->StartTalk(pTalkData))
				{
					printf("StartTalk success\n");
					return TRUE;
				}
				else
				{
					printf("StartTalk error\n");
					m_pTalk->RespondTalkFailed();
				}
			}
		}
		else
		{
			printf("StartTalk busy\n");
			m_pTalk->RespondTalkBusy(pTalkData);
		}
	}
// 	else if()
// 	{
// 		//设备没有专有的音频对讲通道
// 
// 	}
	return FALSE;

}

void CMonDevice::StopTalk( const char *szNodeID )
{
	UpdateLastOptTime();
	if (m_pTalk)
	{
		m_pTalk->StopTalk(szNodeID);
		delete m_pTalk;
		m_pTalk = NULL;
	}
}

void CMonDevice::OnChangedAudio( unsigned long nAudioID )
{
	UpdateLastOptTime();
	if (m_pTalk)
	{
		m_pTalk->TalkChanged(nAudioID);
	}
}

BOOL CMonDevice::PtzControl( int nChanID,int nPtzCmd,int nSpeed,void*pExtData,int nExtSize )
{
	UpdateLastOptTime();
	CMonChannel* pCapChan = FindMonChn(nChanID);
	if (pCapChan)
	{
		return pCapChan->PtzControl(nPtzCmd,nSpeed,pExtData,nExtSize);
	}
	return TRUE;
}

bool CMonDevice::OnDevTalkCallback_StartTalkSDK( int nChnID )
{
	UpdateLastOptTime();
	if ( m_pDeviceSdk )
	{
		m_pDeviceSdk->MON_DEVSDK_StartTalk(m_mpxDevInfo.szDevID, nChnID);
		return true;
	}
	return false;
}
void CMonDevice::OnDevTalkCallback_StopTalkSDK( int nChnID )
{
	UpdateLastOptTime();
	if ( m_pDeviceSdk )
	{
		m_pDeviceSdk->MON_DEVSDK_StopTalk(m_mpxDevInfo.szDevID, nChnID);
	}
}

void CMonDevice::OnDevTalkCallback_RespondTalk( const MPX_TALK_INFO*pTalkData )
{
	UpdateLastOptTime();
	if (m_pDevCallback)
	{
		m_pDevCallback->OnMonDeviceCallback_RespondTalk(pTalkData);
	}
}

void CMonDevice::OnDevTalkCallback_TalkTimerout( void )
{
	UpdateLastOptTime();
	m_bTalkTimerout = true;
}

void CMonDevice::OnDevTalkCallback_DevChannelChange( unsigned long ulAudioID, bool bStartTalk/*=true*/ )
{
	UpdateLastOptTime();
	IT_MAP_MONCHANNEL it = m_mapMonChannel.begin();

	while (it!=m_mapMonChannel.end())
	{
		CMonChannel *pMonChn = it->second;
		if (pMonChn)
		{
			int nChanID = pMonChn->GetChanID();

			//char szMonChannleID[256] = {0};
			//snprintf(szMonChannleID, MAXLEN(szMonChannleID), "%s_%02d", m_mpxDevInfo.szMTSAccount, nChanID);

			MPX_TALK_DEVCHANNLECHANGE* pDevChannelChange = new MPX_TALK_DEVCHANNLECHANGE;
			memset(pDevChannelChange, 0, sizeof(MPX_TALK_DEVCHANNLECHANGE));
			SAFE_STRCPY(pDevChannelChange->szMTSAccount, m_mpxDevInfo.szMTSAccount);
			snprintf(pDevChannelChange->szMTSChannelID, MAXLEN(pDevChannelChange->szMTSChannelID), "%s_%02d", m_mpxDevInfo.szMTSAccount, nChanID);
			pDevChannelChange->ulVideoID = pMonChn->GetVideoID();
			if (bStartTalk)
			{
				pDevChannelChange->ulAudioID = ulAudioID;
			}
			else
			{
				pDevChannelChange->ulAudioID = pMonChn->GetAudioID();
			}

			if (m_pDevCallback)
			{
				m_pDevCallback->OnMonDeviceCallback_DevChannelChange( pDevChannelChange );
			}

			delete pDevChannelChange;
		}

		it++;
	}

}

void CMonDevice::OnDevTalkCallback_SendVoiceToDevice( int nStreamType, unsigned char*pData,int nLen )
{
	UpdateLastOptTime();
	if (m_pDeviceSdk)
	{
		m_pDeviceSdk->MON_DEVSDK_SendTalkStream(nStreamType, pData, nLen);

	}
}

void CMonDevice::DevStatusChange( bool bStatus )
{
	//if (bStatus)
	//{
	//	//在线
	//	if ( m_mpxDevInfo.nToDvrStatus != ONLINE )
	//	{
	//		Connect();
	//	}
	//	
	//}
	//else
	//{
	//	if (m_mpxDevInfo.nToDvrStatus == ONLINE)
	//		DisConnect();
	//}
}

void CMonDevice::OnMonChannelCallback_UpdateLastOptTime()
{
	UpdateLastOptTime();
}

void CMonDevice::OnMonChannelCallback_UpdateSdkAliveTime()
{
	//m_ulLastUTCTime = XGetTimestamp();
}

void CMonDevice::OnMonChannelCallback_GetVideoInfo( const MPX_VIDEO_INFO*pVideoInfo )
{
	if (m_pDevCallback)
	{
		m_pDevCallback->OnMonDeviceCallback_GetVideoInfo(pVideoInfo);
	}

}

BOOL CMonDevice::GetAudioInfo( int nChanID,MPX_AUDIO_INFO*pAudioInfo )
{
	return TRUE;
}

BOOL CMonDevice::SetAudioInfo( int nChanID,MPX_AUDIO_INFO*pAudioInfo )
{
	return TRUE;
}

BOOL CMonDevice::GetVideoInfo( int nChanID,MPX_VIDEO_INFO*pVideoInfo )
{
	CMonChannel* pCapChan = FindMonChn(nChanID);
	if (pCapChan)
	{
		pCapChan->GetVideoInfo(pVideoInfo);
	}

	return TRUE;
}

BOOL CMonDevice::SetVideoInfo( int nChanID,MPX_VIDEO_INFO*pVideoInfo )
{
	CMonChannel* pCapChan = FindMonChn(nChanID);
	if (pCapChan)
	{
		pCapChan->SetVideoInfo(pVideoInfo);
	}

	return TRUE;
}

int CMonDevice::SetChanParam( const int nChanID, const string &strChanName )
{
	int nRet = REQUEST_NO;
	IT_MAP_MONCHANNEL it = m_mapMonChannel.find(nChanID);
	if ( it != m_mapMonChannel.end() )
	{
		CMonChannel* pHikCapChan = it->second;
		if(pHikCapChan)
			nRet= pHikCapChan->SetChanParam(strChanName);
	}
	return nRet;
}

BOOL CMonDevice::SearchFile(long hRx, int nChanID, unsigned long ulType, const char* szStartTime, const char* szEndTime)
{
	IT_MAP_MONCHANNEL it = m_mapMonChannel.find(nChanID);
	if ( it != m_mapMonChannel.end() )
	{
		CMonChannel* pHikCapChan = it->second;
		if(pHikCapChan)
			return pHikCapChan->SearchFile(hRx,ulType,szStartTime,szEndTime);
	}

	return FALSE;
}

BOOL CMonDevice::PlayByFile(long hRx, int nChanID, int nWndID, const char* szFileName)
{
	IT_MAP_MONCHANNEL it = m_mapMonChannel.find(nChanID);
	if ( it != m_mapMonChannel.end() )
	{
		CMonChannel* pHikCapChan = it->second;
		if(pHikCapChan)
			return pHikCapChan->PlayByFile(hRx,szFileName);
	}

	return FALSE;
}

BOOL CMonDevice::PlayByTime(long hRx, int nChanID, int nWndID, const char* szStartTime, const char* szEndTime)
{
	IT_MAP_MONCHANNEL it = m_mapMonChannel.find(nChanID);
	if ( it != m_mapMonChannel.end() )
	{
		CMonChannel* pHikCapChan = it->second;
		if(pHikCapChan)
			return pHikCapChan->PlayByTime(hRx,szStartTime,szEndTime);
	}

	return FALSE;
}

BOOL CMonDevice::PlayByAbsoluteTime(long hRx, int nChanID, int nWndID, const char* szAbsoluteTime)
{
	IT_MAP_MONCHANNEL it = m_mapMonChannel.find(nChanID);
	if ( it != m_mapMonChannel.end() )
	{
		CMonChannel* pHikCapChan = it->second;
		if(pHikCapChan)
			return pHikCapChan->PlayByAbsoluteTime(hRx,szAbsoluteTime);
	}

	return FALSE;
}

BOOL CMonDevice::PlayPause(long hRx, int nChanID, int nWndID, long nState)
{
	IT_MAP_MONCHANNEL it = m_mapMonChannel.find(nChanID);
	if ( it != m_mapMonChannel.end() )
	{
		CMonChannel* pHikCapChan = it->second;
		if(pHikCapChan)
			return pHikCapChan->PlayPause(hRx,nState);
	}

	return FALSE;
}

BOOL CMonDevice::PlaySpeedControl(long hRx, int nChanID, int nWndID, double dSpeed)
{
	IT_MAP_MONCHANNEL it = m_mapMonChannel.find(nChanID);
	if ( it != m_mapMonChannel.end() )
	{
		CMonChannel* pHikCapChan = it->second;
		if(pHikCapChan)
			return pHikCapChan->PlaySpeedControl(hRx,dSpeed);
	}

	return FALSE;
}

BOOL CMonDevice::PlayFastForward(long hRx, int nChanID, int nWndID)
{
	IT_MAP_MONCHANNEL it = m_mapMonChannel.find(nChanID);
	if ( it != m_mapMonChannel.end() )
	{
		CMonChannel* pHikCapChan = it->second;
		if(pHikCapChan)
			return pHikCapChan->PlayFastForward(hRx);
	}

	return FALSE;
}

BOOL CMonDevice::PlaySlowDown(long hRx, int nChanID, int nWndID)
{
	IT_MAP_MONCHANNEL it = m_mapMonChannel.find(nChanID);
	if ( it != m_mapMonChannel.end() )
	{
		CMonChannel* pHikCapChan = it->second;
		if(pHikCapChan)
			return pHikCapChan->PlaySlowDown(hRx);
	}

	return FALSE;
}

BOOL CMonDevice::PlayNormal(long hRx, int nChanID, int nWndID)
{
	IT_MAP_MONCHANNEL it = m_mapMonChannel.find(nChanID);
	if ( it != m_mapMonChannel.end() )
	{
		CMonChannel* pHikCapChan = it->second;
		if(pHikCapChan)
			return pHikCapChan->PlayNormal(hRx);
	}

	return FALSE;
}

BOOL CMonDevice::PlayStop(long hRx, int nChanID, int nWndID)
{
	IT_MAP_MONCHANNEL it = m_mapMonChannel.find(nChanID);
	if ( it != m_mapMonChannel.end() )
	{
		CMonChannel* pHikCapChan = it->second;
		if(pHikCapChan)
			return pHikCapChan->PlayStop(hRx);
	}

	return FALSE;
}

BOOL CMonDevice::DownloadFileByName(unsigned long hDownload, int nChanID, unsigned int nWndID, const char* szFileName)
{
	IT_MAP_MONCHANNEL it = m_mapMonChannel.find(nChanID);
	if ( it != m_mapMonChannel.end() )
	{
		CMonChannel* pHikCapChan = it->second;
		if(pHikCapChan)
			return pHikCapChan->DownloadFileByName(hDownload,szFileName);
	}

	return FALSE;
}

BOOL CMonDevice::DownloadFileByTime(unsigned long hDownload, int nChanID, unsigned int nWndID, const char* szStartTime, const char* szEndTime)
{
	IT_MAP_MONCHANNEL it = m_mapMonChannel.find(nChanID);
	if ( it != m_mapMonChannel.end() )
	{
		CMonChannel* pHikCapChan = it->second;
		if(pHikCapChan)
			return pHikCapChan->DownloadFileByTime(hDownload,szStartTime,szEndTime);
	}
	return FALSE;
}

BOOL CMonDevice::StopDownload(unsigned long hDownload, int nChanID, unsigned int nWndID)
{
	IT_MAP_MONCHANNEL it = m_mapMonChannel.find(nChanID);
	if ( it != m_mapMonChannel.end() )
	{
		CMonChannel* pHikCapChan = it->second;
		if(pHikCapChan)
			return pHikCapChan->StopDownload(hDownload);
	}
	return FALSE;
}

void CMonDevice::AllProcessChannelEvent( bool bOnlined )
{
	std::list<int> lstChanID;
	{
		KAutoLock l(m_csMonChannel);
		for (IT_MAP_MONCHANNEL it=m_mapMonChannel.begin();it!=m_mapMonChannel.end();it++)
		{
			lstChanID.push_back(it->first);
		}
	}

	CMonChannel * pChan=NULL;
	for (std::list<int>::iterator it=lstChanID.begin();it!=lstChanID.end();it++)
	{
		int nChanID=*it;
		{
			KAutoLock l(m_csMonChannel);
			IT_MAP_MONCHANNEL it=m_mapMonChannel.find(nChanID);
			if (it!=m_mapMonChannel.end())
			{
				pChan=it->second;
			}
		}

		if (pChan!=NULL)
		{
			pChan->ProcessMtsEvents(bOnlined);
		}
	}
}