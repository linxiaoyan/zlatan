// libsip_plugin.h : libsip_plugin DLL 的主头文件
//

#pragma once

#ifdef WIN32
#ifndef __AFXWIN_H__
	#error "在包含此文件之前包含“stdafx.h”以生成 PCH 文件"
#endif

#include "resource.h"		// 主符号


// Clibsip_pluginApp
// 有关此类实现的信息，请参阅 libsip_plugin.cpp
//

class Clibsip_pluginApp : public CWinApp
{
public:
	Clibsip_pluginApp();

// 重写
public:
	virtual BOOL InitInstance();

	DECLARE_MESSAGE_MAP()
};
#endif