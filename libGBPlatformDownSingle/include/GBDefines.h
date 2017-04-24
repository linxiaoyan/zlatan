#ifndef _GB_DEFINES_H_
#define _GB_DEFINES_H_

#ifdef WIN32

#ifdef GB28181_EXPORT
#define GB28181_API _declspec(dllexport)
#else
#define GB28181_API _declspec(dllimport)
#endif

#else
#define GB28181_API
#include "linux_include.h"
#ifndef SOCKET
#define SOCKET int 
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET	-1
#endif

#ifndef SOCKET_ERROR
#define SOCKET_ERROR	-1
#endif
#define closesocket(s) shutdown(s,SHUT_RDWR);close(s)
#endif

#include <eXosip2/eXosip.h>
#include <osip2/osip_mt.h>
#include <osip2/osip_time.h>
#include <osipparser2/internal.h>

#include <time.h>
#include <string>
#include <vector>
#include <map>
#include <list>

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

#define GB_ERROR_TYPE								-1

//前端设备
#define GB_DEVICE_DVR								111
#define GB_DEVICE_VIDEO_SERVER						112
#define GB_DEVICE_ENCODER							113
#define GB_DEVICE_DECODER							114
#define GB_DEVICE_VIDEO_MATRIX						115
#define GB_DEVICE_AUDIO_MATRIX						116
#define GB_DEVICE_ALARM_CONTROLLER					117
#define GB_DEVICE_NVR								118

//外围设备
#define GB_PERIPHERAL_CAMERA						131
#define GB_PERIPHERAL_IPC							132
#define GB_PERIPHERAL_MONITOR						133
#define GB_PERIPHERAL_ALARM_IN						134
#define GB_PERIPHERAL_ALARM_OUT						135
#define GB_PERIPHERAL_AUDIO_IN						136
#define GB_PERIPHERAL_AUDIO_OUT						137
#define GB_PERIPHERAL_MOBILE_DEVICE					128
#define GB_PERIPHERAL_OTHER_IN_OUT					139

//平台设备
#define GB_PLATFORM_SESSION_CONTROL_CENTER			200
#define GB_PLATFORM_WEB_SERVER						201
#define GB_PLATFORM_MEDIA_TRANSFER					202
#define GB_PLATFORM_PROXY_SERVER					203
#define GB_PLATFORM_SAFE_SERVER						204
#define GB_PLATFORM_ALARM_SERVER					205
#define GB_PLATFORM_DATABASE_SERVER					206
#define GB_PLATFORM_GIS_SERVER						207
#define GB_PLATFORM_MANAGE_SERVER					208
#define GB_PLATFORM_GATEWAY_SERVER					209
#define GB_PLATFORM_MEDIA_STORAGE_SERVER			210
#define GB_PLATFORM_SESSION_SAFE_GATEWAY			211


#define GB_INFO_NONE								0x0
#define GB_INFO_CATALOG								0x1
#define GB_INFO_DEVICEINFO							0x2
#define GB_INFO_DEVICESTATUS						0x4
#define GB_INFO_ALL									0x7

#define GB_DEFAULT_VIDEO_PORT						9000
#define GB_DEFAULT_AUDIO_PORT						7000

#define GB_DEVICE_VIDEO_PORT						10000
#define GB_DEVICE_AUDIO_PORT						20000

#define GB_PROTOCOL_TCP								1
#define GB_PROTOCOL_UDP								0
#define GB_SIP_PORT									5060

#define GB_PTZ_ZOOM_IN								1
#define GB_PTZ_ZOOM_OUT								2
#define GB_PTZ_UP									3
#define GB_PTZ_DOWN									4
#define GB_PTZ_LEFT									5
#define GB_PTZ_RIGHT								6
#define GB_PTZ_PRESET								7
#define GB_PTZ_CRUISE								8
#define GB_PTZ_SCAN									9
#define GB_PTZ_FOCUS_FAR							10
#define GB_PTZ_FOCUS_NEAR							11
#define GB_PTZ_IRIS_ENLARGE							12
#define GB_PTZ_IRIS_REDUCE							13

#define GB_PTZ_STOP_COMMON							14
#define GB_PTZ_STOP_FI								15

#define GB_MEDIA_TYPE_PCMA							8
#define GB_MEDIA_TYPE_PS							96
#define GB_MEDIA_TYPE_MPEG4							97
#define GB_MEDIA_TYPE_H264							98

#define GB_MEDIA_TYPE_DAHUA							99
#define GB_MEDIA_TYPE_HIKVISION						100

// add buj yuj 2013-5-16 
#define NODE_NAME_RESPONCES				"Response"
#define NODE_NAME_NOTIFY				"Notify"
#define NODE_NAME_CMDTYPE				"CmdType"
#define	CONTEN_TYPE_MANSCDPXML			"Application/MANSCDP+xml"
#define NODE_NAME_DEVICE_ID				"DeviceID"
#define NODE_NAME_STATUS				"Status"

#define CMD_TYPE_VALUE_DEVICE_CONTROL	"DeviceControl"
#define CMD_TYPE_VALUE_ALARM			"Alarm"
#define CMD_TYPE_VALUE_KEEPLIVE			"Keepalive"
#define CMD_TYPE_VALUE_CATALOG			"Catalog"
#define CMD_TYPE_VALUE_DEVICE_STATUS	"DeviceStatus"
#define CMD_TYPE_VALUE_DEVICE_INFO		"DeviceInfo"
#define CMD_TYPE_VALUE_RECORD_INFO		"RecordInfo"


typedef struct RecordItemTag
{
	std::string device_id;
	std::string name;
	std::string file_path;
	std::string address;
	std::string start_time;
	std::string end_time;
	unsigned int secrecy;
	std::string type;//time,manual,alarm
	std::string recorder_id;
}RecordItem;

typedef struct RecordInfoTag
{
	unsigned int sn;
	std::string device_id;
	std::string name;
	unsigned int sum_num;
	std::list<RecordItem> record_list;
}RecordInfo;

typedef struct CatalogItemTag
{
	std::string device_id;
	std::string name;
	std::string manufacturer;
	std::string model;
	std::string owner;
	std::string civil_code;
	std::string block;
	std::string address;
	unsigned int parental;
	std::string parent_id;
	unsigned int safety_way;
	unsigned int register_way;
	std::string cert_num;
	unsigned int certifiable;
	unsigned int err_code;
	std::string end_time;
	unsigned int secrecy;
	std::string ip_address;
	unsigned int port;
	std::string password;
	std::string status;
	double longitude;
	double latitude;
	unsigned int sum_num; //设备的通道数，自己扩展的
}CatalogItem;

typedef struct CatalogTag
{
	unsigned int sn;
	std::string device_id;
	unsigned int sum_num;
	std::list<CatalogItem> device_list;
}Catalog;

typedef struct DeviceInfoTag
{
	unsigned int sn;
	std::string device_id;
	std::string device_name;
	std::string result;
	std::string device_type;
	std::string manufacturer;
	std::string model;
	std::string firmware;
}DeviceInfo;

typedef DeviceInfo MANSCDPXml_DeviceInfo;

typedef struct DeviceStatusTag
{
	unsigned int sn;
	std::string device_id;
	std::string result;
	std::string online;
	std::string status;
	std::string reason;
	std::string encode;
	std::string recode;
	std::string device_time;
	std::string platform_id;
}DeviceStatus;

typedef struct KeepaliveTag
{
	std::string device_id;
	bool is_ok;
}Keepalive;

typedef struct NotifyCatalogItemTag
{
	std::string event;
	CatalogItem catalog_item;
}NotifyCatalogItem;

typedef struct NotifyCatalogTag
{
	std::string device_id; 
	unsigned int sum_num;
	std::vector<NotifyCatalogItem> device_list;
}NotifyCatalog;

typedef enum StreamType
{
	STREAM_TYPE_PLAY_MAIN,
	STREAM_TYPE_PLAY_SUB,
	STREAM_TYPE_PLAY_SUBSUB,
	STREAM_TYPE_PLAYBACK,
	STREAM_TYPE_DOWNLOAD
}StreamType;

typedef struct IDTimeTypeTag
{
	std::string id;
	time_t time;
}IDTimeType;

typedef struct GBAlarmTag
{
	std::string device_id;
	unsigned int alarm_priority;
	std::string alarm_time;		//yyyy-MM-ddThh:mm:ss
	unsigned int alarm_method;	//设备报警，视频报警
}GBAlarm;

typedef struct MsgBodyTag
{
	std::string id;
	std::string data;
}MsgBody;


#if defined(WIN32)
/* Windows is little endian */ 
#define __LITTLE_ENDIAN 1234
#define __BIG_ENDIAN    4321
#define __BYTE_ORDER __LITTLE_ENDIAN
#endif /*  defined(WIN32) */

struct GBRtpHeader
{
#if __BYTE_ORDER == __BIG_ENDIAN
	unsigned char v:2; 
	unsigned char p:1;
	unsigned char x:1;
	unsigned char cc:4;
	unsigned char m:1;
	unsigned char pt:7; 
#else
	unsigned char cc:4; 
	unsigned char x:1; 
	unsigned char p:1; 
	unsigned char v:2; 
	unsigned char pt:7;
	unsigned char m:1;
#endif
	unsigned short seq;
	unsigned tm;
	unsigned ssrc;
};

typedef struct tagRecvSIPMessageInfo
{
	osip_message_t * pMsg;
	char szHost[100];
	int nPort;
}RecvSIPMessageInfo;

#define CHARPTRTOSTR(sz)  (sz==0 ? "" : sz)


#endif