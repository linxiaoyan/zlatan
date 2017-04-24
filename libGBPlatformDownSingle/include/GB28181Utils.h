#ifndef _GB28181_UTILS_H_
#define _GB28181_UTILS_H_

#include "GBDefines.h"

class GB28181Utils
{
public:
	GB28181Utils();
	virtual ~GB28181Utils();

	//函数介绍:		查询设备
	//dest_id:	    目标平台ID
	//dest_ip:  	目标平台的IP地址
	//dest_port:	目标平台的端口
	//src_id:	    服务ID
	//sn:           XML序号
	//device_id:    目标平台或多通道设备的ID
	//query_type:	查询类型，可以为"Catalog","DeviceInfo","DeviceStatus"

	static void Query(eXosip_t *exosip,
					  const std::string &dest_id,
					  const std::string &dest_ip,
					  const int dest_port,
					  const std::string &src_id,
					  const unsigned int sn,
					  const std::string &device_id,
					  const std::string &query_type);
	
	//函数介绍:		接实时流
	//dest_id:	    接像的设备ID,到现在为止，这个只能是通道ID
	//src_id:	    服务ID
	//dest_ip:  	设备所在平台的IP地址
	//dest_port:	设备所在平台的端口

	static int PlayTCP(eXosip_t *exosip,
		const std::string &dest_id,
		const std::string &dest_ip,
		const int dest_port,
		const std::string &src_id,
		const std::string &src_ip,
		const int video_port,
		const unsigned int ssrc);

	static int Play(eXosip_t *exosip,
		const std::string &dest_id,
		const std::string &dest_ip,
		const int dest_port,
		const std::string &src_id,
		const std::string &src_ip,
		const int video_port,
		const unsigned int ssrc);

	static int TalkPlay(eXosip_t *exosip,
		const std::string &dest_id,
		const std::string &dest_ip,
		const int dest_port,
		const std::string &src_id,
		const std::string &src_ip,
		const int recv_port,
		const int send_port,
		const unsigned int ssrc);

	static int Playback(eXosip_t *exosip,
		const std::string &dest_id,
		const std::string &dest_ip,
		const int dest_port,
		const std::string &src_id,
		const std::string &src_ip,
		const int record_type,
		const std::string &start_time,
		const std::string &end_time,
		const int recv_port,
		const unsigned int ssrc);

	static int Download(eXosip_t *exosip,
		const std::string &dest_id,
		const std::string &dest_ip,
		const int dest_port,
		const std::string &src_id,
		const std::string &src_ip,
		const int record_type,
		const std::string &start_time,
		const std::string &end_time,
		const int recv_port,
		const unsigned int ssrc);

	static bool StopPlay(eXosip_t *exosip, const int cid, const int did);

	static void PtzControl(eXosip_t *exosip,
		const std::string &dest_id,
		const std::string &dest_ip,
		const int dest_port,
		const std::string &src_id,
		const unsigned int sn,
		const std::string &device_id,
		const int ptz_cmd,
		const int speed);


	//录像
	static void Record(eXosip_t * pEXosip,
		const std::string & strDestID,
		const std::string & strSourceID,
		const std::string & strDestIP,
		const int & nPort,
		const bool & bStop);

	static int QueryRecordFile(eXosip_t *exosip,
		const std::string &dest_id,
		const std::string &dest_ip,
		const int dest_port,
		const std::string &src_id,
		const unsigned int sn,
		const std::string &device_id,
		const std::string &start_time,
		const std::string &end_time,
		const std::string &type,
		const std::string &file_path,
		const std::string &record_id,
		const std::string &address);

	static void PlaybackPause(eXosip_t *exosip, const int did, const bool pause);
	static void PlayByAbsoluteTime(eXosip_t *exosip, const int did, const int npt);
	static void PlaybackSpeed(eXosip_t *exosip, const int did, const double speed);//包括快进、慢放

	//远程重启
	static void TeleBoot(eXosip_t * pEXosip,
		const std::string & strDestID,
		const std::string & strSourceID,
		const std::string & strDestIP,
		const int & nPort);

	//布防
	static void SetGuard(eXosip_t * pEXosip,
		const std::string & strDestID,
		const std::string & strSourceID,
		const std::string & strDestIP,
		const int & nPort,
		const bool & bSetGuard);
	//报警复位
	static void ResetAlarm(eXosip_t * pEXosip,
		const std::string & strDestID,
		const std::string & strSourceID,
		const std::string & strDestIP,
		const int & nPort);

	static void SubscribeAlarm(eXosip_t * pEXosip,
		const std::string & strDestID,
		const std::string & strSourceID,
		const std::string & strDestIP,
		const int & nPort,
		const std::string & strAlarmSrcID,
		const int & nStartAlarmPriority,
		const int & nEndAlarmPriority,
		const int & nAlarmMethod,
		const std::string & strStartTime,
		const std::string & strEndTime,
		int & nExpires);

	//Catalog订阅
	static void SubscribeCatalog(eXosip_t *exosip, 
		const std::string &dest_id, 
		const std::string &dest_ip, 
		const int dest_port,
		const std::string &src_id, 
		const unsigned int sn, 
		const std::string &device_id, 
		const int expires);

	static void SubscribeRefreshCatalog(eXosip_t *exosip, 
		const int did,
		const int expires,
		const unsigned int sn, 
		const std::string &device_id);

	
	//此处用:yyyy-MM-dd hh:mm:ss
	static unsigned long StrToTimeStamp(const std::string & strTime);
	static std::string PtzToHexStr(const int & nPtzType,const int & nSpeed);
	static std::string PtzToHexStr2(const int & nTiltType,const int & nTiltSpeed,const int & nPanType,const int & nPanSpeed){return "";}

	static bool IsUserIDValidate(const std::string & strID);
	static int GetIDType(const std::string & strID);
	static int GetIDNumber(const std::string & strID);

	static bool IsLeafNodeDevice(const std::string & strDevID);
	static bool IsPlatformType(const std::string &id);
	static bool GBDateStringAddSecs(const char *s,size_t stCch, time_t tSec,char * pOut,size_t nOutLen);
	//国标格式:yyyy-MM-ddThh:mm:ss
	static std::string GetCurrentDate();
	static std::string GetCurrentDateNoMs();
};
#endif