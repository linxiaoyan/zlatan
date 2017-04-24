// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件

#pragma once

#ifdef WIN32

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // 从 Windows 头中排除极少使用的资料
#endif

#include "targetver.h"

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // 某些 CString 构造函数将是显式的

#include <afxwin.h>         // MFC 核心组件和标准组件
#include <afxext.h>         // MFC 扩展

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxole.h>         // MFC OLE 类
#include <afxodlgs.h>       // MFC OLE 对话框类
#include <afxdisp.h>        // MFC 自动化类
#endif // _AFX_NO_OLE_SUPPORT

#ifndef _AFX_NO_DB_SUPPORT
#include <afxdb.h>                      // MFC ODBC 数据库类
#endif // _AFX_NO_DB_SUPPORT

#ifndef _AFX_NO_DAO_SUPPORT
#include <afxdao.h>                     // MFC DAO 数据库类
#endif // _AFX_NO_DAO_SUPPORT

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>           // MFC 对 Internet Explorer 4 公共控件的支持
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>                     // MFC 对 Windows 公共控件的支持
#endif // _AFX_NO_AFXCMN_SUPPORT


#pragma comment(lib,"libSQLData.lib")
#pragma comment(lib,"KBASE.lib")
#pragma comment(lib,"NETEC.lib")
#pragma comment(lib,"KNET.lib")
#pragma comment(lib,"libstream_transcode.lib")
#pragma comment(lib,"libstream_transfer.lib")
#pragma comment(lib,"libGB28181.lib")

#ifdef _VOD_ENABLE
#pragma comment(lib,"VOD_TX.lib")
#endif

#else

#include "linux_include.h"


#endif

#include "KBASE.h"
#include "Fun.h"
#include "IPluginDB.h"
#include "IMonDefine.h"
#include "libStreamTransfer.h"
#include "libStreamTranscode.h"
#include "NETEC/NETEC_Node.h"
#include "NETEC/XUtil.h"
#include "VIDEC/VIDEC_Header.h"
#include "AUDEC/AUDEC_CodecID.h"
#include "MonToolDef.h"
#include "MonAlarmDefine.h"

#ifdef _VOD_ENABLE
#include "VMSP/VMSPDefine.h"
#include "VODTX.h"
#include "VODType.h"
#include "VODPublic.h"
#endif