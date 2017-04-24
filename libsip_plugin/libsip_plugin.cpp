// libsip_plugin.cpp : 定义 DLL 的初始化例程。
//

#include "stdafx.h"
#include "libsip_plugin.h"
#include "MonPlugin.h"

#ifdef WIN32
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//
//TODO: 如果此 DLL 相对于 MFC DLL 是动态链接的，
//		则从此 DLL 导出的任何调入
//		MFC 的函数必须将 AFX_MANAGE_STATE 宏添加到
//		该函数的最前面。
//
//		例如:
//
//		extern "C" BOOL PASCAL EXPORT ExportedFunction()
//		{
//			AFX_MANAGE_STATE(AfxGetStaticModuleState());
//			// 此处为普通函数体
//		}
//
//		此宏先于任何 MFC 调用
//		出现在每个函数中十分重要。这意味着
//		它必须作为函数中的第一个语句
//		出现，甚至先于所有对象变量声明，
//		这是因为它们的构造函数可能生成 MFC
//		DLL 调用。
//
//		有关其他详细信息，
//		请参阅 MFC 技术说明 33 和 58。
//

// Clibsip_pluginApp

BEGIN_MESSAGE_MAP(Clibsip_pluginApp, CWinApp)
END_MESSAGE_MAP()


// Clibsip_pluginApp 构造

Clibsip_pluginApp::Clibsip_pluginApp()
{
	// TODO: 在此处添加构造代码，
	// 将所有重要的初始化放置在 InitInstance 中
}


// 唯一的一个 Clibsip_pluginApp 对象

Clibsip_pluginApp theApp;


// Clibsip_pluginApp 初始化

BOOL Clibsip_pluginApp::InitInstance()
{
	CWinApp::InitInstance();

	return TRUE;
}
#endif

CMonPlugin* g_pHikPlugin = NULL;

AVCON_API BOOL CALLBACK AVCON_MPX_Startup(const char* pConfigFile,
										  AVCON_MPX_MESSAGE_CB pMessageCb,void*pMsgUser,
										  AVCON_MPX_ALARM_CB pAlarmCb,void*pAlarmUser,
										  AVCON_MPX_RECORD_CB pRecordCb,void*pRecUser)
{
	if (g_pHikPlugin==NULL)
	{
		g_pHikPlugin = new CMonPlugin();
	}

	if (g_pHikPlugin==NULL)
	{
		return FALSE;
	}

	return g_pHikPlugin->Startup(pConfigFile,pMessageCb,pMsgUser,pAlarmCb,pAlarmUser,pRecordCb,pRecUser);
}

AVCON_API void CALLBACK AVCON_MPX_Cleanup(void)
{
	if (g_pHikPlugin)
	{
		g_pHikPlugin->Cleanup();
		delete g_pHikPlugin;
		g_pHikPlugin = NULL;
	}
}

AVCON_API void CALLBACK AVCON_MPX_MtsStatusChanged(int nStatus)
{
	if (g_pHikPlugin)
	{
		g_pHikPlugin->ServerStatusChanged(nStatus);
	}
}

AVCON_API BOOL CALLBACK AVCON_MPX_PtzControl(const char*szDevID,int nChanID,int nPtzCmd,int nSpeed,void*pExtData,int nExtSize)
{
	if (g_pHikPlugin)
	{
		return g_pHikPlugin->PtzControl(szDevID,nChanID,nPtzCmd,nSpeed,pExtData,nExtSize);
	}
	return FALSE;
}	

AVCON_API BOOL CALLBACK AVCON_MPX_StartRecord(const char*szDevID,int nChanID)
{
	if (g_pHikPlugin)
	{
		return g_pHikPlugin->StartRecord(szDevID,nChanID);
	}
	return FALSE;
}

AVCON_API void CALLBACK AVCON_MPX_StopRecord(const char*szDevID,int nChanID)
{
	if (g_pHikPlugin)
	{
		g_pHikPlugin->StopRecord(szDevID,nChanID);
	}
}

AVCON_API BOOL CALLBACK AVCON_MPX_StartTalk(const char*szDevID,MPX_TALK_INFO*pTalkData)
{
	if (g_pHikPlugin)
	{
		return g_pHikPlugin->StartTalk(szDevID,pTalkData);
	}
	return FALSE;
}

AVCON_API void CALLBACK AVCON_MPX_StopTalk(const char*szDevID,const char *szNodeID)
{
	if (g_pHikPlugin)
	{
		g_pHikPlugin->StopTalk(szDevID,szNodeID);
	}
}

AVCON_API BOOL CALLBACK AVCON_MPX_StartListenAlarm(const char*szDevID)
{
	if (g_pHikPlugin)
	{
		return g_pHikPlugin->StartListenAlarm(szDevID);
	}
	return FALSE;
}	

AVCON_API void CALLBACK AVCON_MPX_StopListenAlarm(const char*szDevID)
{
	if (g_pHikPlugin)
	{
		g_pHikPlugin->StopListenAlarm(szDevID);
	}
}

AVCON_API void CALLBACK AVCON_MPX_OnDeviceOnline(const char*szDevID,int nErrCode)
{
	if (g_pHikPlugin)
	{
		g_pHikPlugin->OnDeviceOnline(szDevID,nErrCode);
	}
}

AVCON_API void CALLBACK AVCON_MPX_OnDeviceOffline(const char*szDevID,int nErrCode)
{
	if (g_pHikPlugin)
	{
		g_pHikPlugin->OnDeviceOffline(szDevID,nErrCode);
	}
}

AVCON_API void CALLBACK AVCON_MPX_OnChannelOnline(const char*szDevID,int nChanID,int nErrCode)
{
	if (g_pHikPlugin)
	{
		g_pHikPlugin->OnChannelOnline(szDevID,nChanID,nErrCode);
	}
}

AVCON_API void CALLBACK AVCON_MPX_OnChannelOffline(const char*szDevID,int nChanID,int nErrCode)
{
	if (g_pHikPlugin)
	{
		g_pHikPlugin->OnChannelOffline(szDevID,nChanID,nErrCode);
	}
}

AVCON_API long CALLBACK AVCON_MPX_GetLastError(void)
{
	if (g_pHikPlugin)
	{
		return g_pHikPlugin->GetLastError();
	}
	return -1;
}

AVCON_API BOOL CALLBACK AVCON_MPX_GetAudioInfo(const char*szDevID,int nChanID,MPX_AUDIO_INFO*pAudioInfo)
{
	if (g_pHikPlugin)
	{
		//return g_pHikPlugin->GetAudioInfo(szDevID,nChanID,pAudioInfo);
	}
	return FALSE;
}

AVCON_API BOOL CALLBACK AVCON_MPX_SetAudioInfo(const char*szDevID,int nChanID,MPX_AUDIO_INFO*pAudioInfo)
{
	if (g_pHikPlugin)
	{
		//return g_pHikPlugin->SetAudioInfo(szDevID,nChanID,pAudioInfo);
	}
	return FALSE;
}

AVCON_API BOOL CALLBACK AVCON_MPX_GetVideoInfo(const char*szDevID,int nChanID,MPX_VIDEO_INFO*pVideoInfo)
{
	if (g_pHikPlugin)
	{
		//return g_pHikPlugin->GetVideoInfo(szDevID,nChanID,pVideoInfo);
	}
	return FALSE;
}

AVCON_API BOOL CALLBACK AVCON_MPX_SetVideoInfo(const char*szDevID,int nChanID,MPX_VIDEO_INFO*pVideoInfo)
{
	if (g_pHikPlugin)
	{
		//return g_pHikPlugin->SetVideoInfo(szDevID,nChanID,pVideoInfo);
	}
	return FALSE;
}

AVCON_API BOOL CALLBACK AVCON_MPX_GetPtzInfo(const char*szDevID,int nChanID,MPX_PTZ_INFO*pPtzInfo)
{
	if (g_pHikPlugin)
	{
		//return g_pHikPlugin->GetPtzInfo(szDevID,nChanID,pPtzInfo);
	}
	return FALSE;
}

AVCON_API BOOL CALLBACK AVCON_MPX_SetPtzInfo(const char*szDevID,int nChanID,MPX_PTZ_INFO*pPtzInfo)
{
	if (g_pHikPlugin)
	{
		//return g_pHikPlugin->SetPtzInfo(szDevID,nChanID,pPtzInfo);
	}
	return FALSE;
}

AVCON_API void CALLBACK AVCON_MPX_TalkChanged(const char*szDevID,unsigned long nAudioID)
{
	if (g_pHikPlugin)
	{
		g_pHikPlugin->TalkChanged(szDevID,nAudioID);
	}
}

AVCON_API void CALLBACK AVCON_MPX_SwitchMatrixVideoSource(const char*szDevID,int nChanID, const char *szChanName, const char *szMatrixInput/*矩阵camera*/, void*pExtData,int nExtSize)
{
}

AVCON_API void CALLBACK AVCON_MPX_SendMatrixAllSourceName(const char* szFromNodeID)
{
}

AVCON_API void CALLBACK AVCON_MPX_ToolsOperator(int Message, AVCON_MPX_TOOLMESSAGE_CB pToolMessageCb, void*pExtData,int nExtSize, void* pSession)
{
	if (g_pHikPlugin)
	{
		g_pHikPlugin->OnToolsOperator( Message, pToolMessageCb, pExtData, nExtSize, pSession );
	}
}

//pb
AVCON_API void CALLBACK AVCON_MPX_GetMonDeviceLoginInfo(const char * szDevID,const char * szPassword,int nDevType)
{
	if (g_pHikPlugin)
	{
		g_pHikPlugin->OnGetMonDeviceLoginInfo(szDevID,szPassword,nDevType);
	}
}

AVCON_API void CALLBACK AVCON_MPX_OnDeviceBasicInfo(const char*szDevID, int nChannNum)
{

}

AVCON_API void CALLBACK AVCON_MPX_OnGetDevDiskInfo()
{
	
}