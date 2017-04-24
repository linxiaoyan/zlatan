#include "stdafx.h"
#include "GB28181Utils.h"

#ifdef WIN32
#define strncasecmp(s1, s2, n) _strnicmp(s1, s2, n)
static const char *abb_weekdays[] = { 
	"Sun", 
	"Mon", 
	"Tue", 
	"Wed", 
	"Thu", 
	"Fri", 
	"Sat", 
	NULL 
}; 

static const char *full_weekdays[] = {
	"Sunday", 
	"Monday", 
	"Tuesday", 
	"Wednesday", 
	"Thursday", 
	"Friday", 
	"Saturday", 
	NULL 
}; 

static const char *abb_month[] = { 
	"Jan", 
	"Feb", 
	"Mar", 
	"Apr", 
	"May", 
	"Jun", 
	"Jul", 
	"Aug", 
	"Sep", 
	"Oct", 
	"Nov", 
	"Dec", 
	NULL 
}; 

static const char *full_month[] = { 
	"January", 
	"February", 
	"Mars", 
	"April", 
	"May", 
	"June", 
	"July", 
	"August", 
	"September", 
	"October", 
	"November", 
	"December", 
	NULL, 
}; 

static const char *ampm[] = { 
	"am", 
	"pm", 
	NULL 
}; 

/* 
* Try to match `*buf' to one of the strings in `strs'.  Return the 
* index of the matching string (or -1 if none).  Also advance buf. 
*/ 

static int 
match_string(const char **buf, const char **strs) { 
	int i = 0; 
	for (i = 0; strs[i] != NULL; ++i) { 
		int len = (int) strlen(strs[i]); 
		if (strncasecmp(*buf, strs[i], len) == 0) { 
			*buf += len; 
			return i; 
		} 
	} 
	return -1; 
} 

/* 
* tm_year is relative this year */ 

const int tm_year_base = 1900; 

/* 
* Return TRUE iff `year' was a leap year. 
*/ 

static int 
is_leap_year(int year) { 
	return (year % 4) == 0 && ((year % 100) != 0 || (year % 400) == 0); 
} 

/* 
* Return the weekday [0,6] (0 = Sunday) of the first day of `year' 
*/ 

static int 
first_day(int year) { 
	int ret = 4; 

	for (; year > 1970; --year) 
		ret = (ret + 365 + is_leap_year(year) ? 1 : 0) % 7; 
	return ret; 
} 

/* 
* Set `timeptr' given `wnum' (week number [0, 53]) 
*/ 

static void 
set_week_number_sun(struct tm *timeptr, int wnum) { 
	int fday = first_day(timeptr->tm_year + tm_year_base); 

	timeptr->tm_yday = wnum * 7 + timeptr->tm_wday - fday; 
	if (timeptr->tm_yday < 0) { 
		timeptr->tm_wday = fday; 
		timeptr->tm_yday = 0; 
	} 
} 

/* 
* Set `timeptr' given `wnum' (week number [0, 53]) 
*/ 

static void 
set_week_number_mon(struct tm *timeptr, int wnum) { 
	int fday = (first_day(timeptr->tm_year + tm_year_base) + 6) % 7; 

	timeptr->tm_yday = wnum * 7 + (timeptr->tm_wday + 6) % 7 - fday; 
	if (timeptr->tm_yday < 0) { 
		timeptr->tm_wday = (fday + 1) % 7; 
		timeptr->tm_yday = 0; 
	} 
} 

/* 
* Set `timeptr' given `wnum' (week number [0, 53]) 
*/ 

static void 
set_week_number_mon4(struct tm *timeptr, int wnum) { 
	int fday = (first_day(timeptr->tm_year + tm_year_base) + 6) % 7; 
	int offset = 0; 

	if (fday < 4) 
		offset += 7; 

	timeptr->tm_yday = offset + (wnum - 1) * 7 + timeptr->tm_wday - fday; 
	if (timeptr->tm_yday < 0) { 
		timeptr->tm_wday = fday; 
		timeptr->tm_yday = 0; 
	} 
} 

char * strptime(const char *buf, const char *fmt, struct tm *timeptr) { 
	char c; 
	for (; (c = *fmt) != '\0'; ++fmt) { 
		char *s; 
		int ret; 

		if (isspace(c)) { 
			while (isspace(*buf)) 
				++buf; 
		} else if (c == '%' && fmt[1] != '\0') { 
			c = *++fmt; 
			if (c == 'E' || c == 'O') 
				c = *++fmt; 
			switch (c) { 
								case 'A': 
									ret = match_string(&buf, full_weekdays); 
									if (ret < 0) 
										return NULL; 
									timeptr->tm_wday = ret; 
									break; 
								case 'a': 
									ret = match_string(&buf, abb_weekdays); 
									if (ret < 0) 
										return NULL; 
									timeptr->tm_wday = ret; 
									break; 
								case 'B': 
									ret = match_string(&buf, full_month); 
									if (ret < 0) 
										return NULL; 
									timeptr->tm_mon = ret; 
									break; 
								case 'b': 
								case 'h': 
									ret = match_string(&buf, abb_month); 
									if (ret < 0) 
										return NULL; 
									timeptr->tm_mon = ret; 
									break; 
								case 'C': 
									ret = strtol(buf, &s, 10); 
									if (s == buf) 
										return NULL; 
									timeptr->tm_year = (ret * 100) - tm_year_base; 
									buf = s; 
									break; 
								case 'c': 
									abort(); 
								case 'D': /* %m/%d/%y */ 
									s = strptime(buf, "%m/%d/%y", timeptr); 
									if (s == NULL) 
										return NULL; 
									buf = s; 
									break; 
								case 'd': 
								case 'e': 
									ret = strtol(buf, &s, 10); 
									if (s == buf) 
										return NULL; 
									timeptr->tm_mday = ret; 
									buf = s; 
									break; 
								case 'H': 
								case 'k': 
									ret = strtol(buf, &s, 10); 
									if (s == buf) 
										return NULL; 
									timeptr->tm_hour = ret; 
									buf = s; 
									break; 
								case 'I': 
								case 'l': 
									ret = strtol(buf, &s, 10); 
									if (s == buf) 
										return NULL; 
									if (ret == 12) 
										timeptr->tm_hour = 0; 
									else 
										timeptr->tm_hour = ret; 
									buf = s; 
									break; 
								case 'j': 
									ret = strtol(buf, &s, 10); 
									if (s == buf) 
										return NULL; 
									timeptr->tm_yday = ret - 1; 
									buf = s; 
									break; 
								case 'm': 
									ret = strtol(buf, &s, 10); 
									if (s == buf) 
										return NULL; 
									timeptr->tm_mon = ret - 1; 
									buf = s; 
									break; 
								case 'M': 
									ret = strtol(buf, &s, 10); 
									if (s == buf) 
										return NULL; 
									timeptr->tm_min = ret; 
									buf = s; 
									break; 
								case 'n': 
									if (*buf == '\n') 
										++buf; 
									else 
										return NULL; 
									break; 
								case 'p': 
									ret = match_string(&buf, ampm); 
									if (ret < 0) 
										return NULL; 
									if (timeptr->tm_hour == 0) { 
										if (ret == 1) 
											timeptr->tm_hour = 12; 
									} else 
										timeptr->tm_hour += 12; 
									break; 
								case 'r': /* %I:%M:%S %p */ 
									s = strptime(buf, "%I:%M:%S %p", timeptr); 
									if (s == NULL) 
										return NULL; 
									buf = s; 
									break; 
								case 'R': /* %H:%M */ 
									s = strptime(buf, "%H:%M", timeptr); 
									if (s == NULL) 
										return NULL; 
									buf = s; 
									break; 
								case 'S': 
									ret = strtol(buf, &s, 10); 
									if (s == buf) 
										return NULL; 
									timeptr->tm_sec = ret; 
									buf = s; 
									break; 
								case 't': 
									if (*buf == '\t') 
										++buf; 
									else 
										return NULL; 
									break; 
								case 'T': /* %H:%M:%S */ 
								case 'X': 
									s = strptime(buf, "%H:%M:%S", timeptr); 
									if (s == NULL) 
										return NULL; 
									buf = s; 
									break; 
								case 'u': 
									ret = strtol(buf, &s, 10); 
									if (s == buf) 
										return NULL; 
									timeptr->tm_wday = ret - 1; 
									buf = s; 
									break; 
								case 'w': 
									ret = strtol(buf, &s, 10); 
									if (s == buf) 
										return NULL; 
									timeptr->tm_wday = ret; 
									buf = s; 
									break; 
								case 'U': 
									ret = strtol(buf, &s, 10); 
									if (s == buf) 
										return NULL; 
									set_week_number_sun(timeptr, ret); 
									buf = s; 
									break; 
								case 'V': 
									ret = strtol(buf, &s, 10); 
									if (s == buf) 
										return NULL; 
									set_week_number_mon4(timeptr, ret); 
									buf = s; 
									break; 
								case 'W': 
									ret = strtol(buf, &s, 10); 
									if (s == buf) 
										return NULL; 
									set_week_number_mon(timeptr, ret); 
									buf = s; 
									break; 
								case 'x': 
									s = strptime(buf, "%Y:%m:%d", timeptr); 
									if (s == NULL) 
										return NULL; 
									buf = s; 
									break; 
								case 'y': 
									ret = strtol(buf, &s, 10); 
									if (s == buf) 
										return NULL; 
									if (ret < 70) 
										timeptr->tm_year = 100 + ret; 
									else 
										timeptr->tm_year = ret; 
									buf = s; 
									break; 
								case 'Y': 
									ret = strtol(buf, &s, 10); 
									if (s == buf) 
										return NULL; 
									timeptr->tm_year = ret - tm_year_base; 
									buf = s; 
									break; 
								case 'Z': 
									abort(); 
								case '\0': 
									--fmt; 
									/* FALLTHROUGH */ 
								case '%': 
									if (*buf == '%') 
										++buf; 
									else 
										return NULL; 
									break; 
								default: 
									if (*buf == '%' || *++buf == c) 
										++buf; 
									else 
										return NULL; 
									break; 
			} 
		} else { 
			if (*buf == c) 
				++buf; 
			else 
				return NULL; 
		} 
	} 
	return (char *) buf; 
} 
#endif

GB28181Utils::GB28181Utils()
{

}

GB28181Utils::~GB28181Utils()
{

}

//函数介绍:查询设备目录信息
void GB28181Utils::Query(eXosip_t *exosip,
						 const std::string &dest_id,
						 const std::string &dest_ip,
						 const int dest_port,
						 const std::string &src_id,
						 const unsigned int sn,
						 const std::string &device_id,
						 const std::string &query_type)
{
	osip_message_t *message;
	char tmp[1024];
	char dest_call[100],source_call[100],route_call[100];
	
	std::string realm = src_id.substr(0,10);
	
	snprintf(dest_call,100,"sip:%s@%s", dest_id.c_str(), realm.c_str());
	snprintf(source_call,100,"sip:%s@%s", src_id.c_str(), realm.c_str());
	snprintf(route_call,100,"<sip:%s@%s:%d;lr>", dest_id.c_str(), dest_ip.c_str(), dest_port);

	eXosip_message_build_request(exosip, &message, "MESSAGE", dest_call, source_call, route_call);

	snprintf(tmp, 1024, "<?xml version=\"1.0\"?>\r\n"
						"<Query>\r\n"
			        	"<CmdType>%s</CmdType>\r\n"
						"<SN>%d</SN>\r\n"
						"<DeviceID>%s</DeviceID>\r\n"
						"</Query>\r\n",
						query_type.c_str(),
						sn,
						device_id.c_str());

	osip_message_set_body(message, tmp, strlen(tmp));  
	osip_message_set_content_type(message, "Application/MANSCDP+xml");
	eXosip_lock(exosip);
	eXosip_message_send_request(exosip, message);
	eXosip_unlock(exosip);
}

int GB28181Utils::PlayTCP(eXosip_t *exosip,
					      const std::string &dest_id,
					      const std::string &dest_ip,
					      const int dest_port,
					      const std::string &src_id,
					      const std::string &src_ip,
					      const int video_port,
					      const unsigned int ssrc)
{
	int ret;
	osip_message_t *message;
	char tmp[1024];
	char dest_call[100],source_call[100],route_call[100];
	std::string realm = src_id.substr(0,10);

	snprintf(dest_call,100, "sip:%s@%s", dest_id.c_str(), realm.c_str());
	snprintf(source_call,100, "sip:%s@%s", src_id.c_str(), realm.c_str());
	snprintf(route_call,100, "<sip:%s@%s:%d;lr>", dest_id.c_str(), dest_ip.c_str(), dest_port);

	ret = eXosip_call_build_initial_invite (exosip, &message, dest_call, source_call, route_call, NULL);
	if (ret != 0)
		return -1;

	snprintf(tmp,1024, "v=0\r\n"
		"o=%s 0 0 IN IP4 %s\r\n"
		"s=Play\r\n"
		"c=IN IP4 %s\r\n"
		"t=0 0\r\n"
		"m=video %d RTP/AVP/TCP 96 98 97\r\n"
		"a=recvonly\r\n"
		"a=rtpmap:96 PS/90000\r\n"
		"a=rtpmap:98 H264/90000\r\n"
		"a=rtpmap:97 MPEG4/90000\r\n"
		"y=%010u\r\n"
		"f=\r\n",
		src_id.c_str(),
		src_ip.c_str(),
		src_ip.c_str(),
		video_port,
		ssrc);

	osip_message_set_body(message, tmp, strlen(tmp));
	osip_message_set_content_type(message, "application/sdp");

	eXosip_lock(exosip);
	ret = eXosip_call_send_initial_invite(exosip, message);
	eXosip_unlock(exosip);
	return ret;
}

int GB28181Utils::Play(eXosip_t *exosip,
					   const std::string &dest_id,
					   const std::string &dest_ip,
					   const int dest_port,
					   const std::string &src_id,
					   const std::string &src_ip,
					   const int video_port,
					   const unsigned int ssrc)
{
	int ret;
	osip_message_t *message;
	char tmp[1024];
	char dest_call[100],source_call[100],route_call[100];
	std::string realm = src_id.substr(0,10);

	snprintf(dest_call,100, "sip:%s@%s", dest_id.c_str(), realm.c_str());
	snprintf(source_call,100, "sip:%s@%s", src_id.c_str(), realm.c_str());
	snprintf(route_call,100, "<sip:%s@%s:%d;lr>", dest_id.c_str(), dest_ip.c_str(), dest_port);

	ret = eXosip_call_build_initial_invite (exosip, &message, dest_call, source_call, route_call, NULL);
	if (ret != 0)
		return -1;
	
	snprintf(tmp,1024, "v=0\r\n"
		               "o=%s 0 0 IN IP4 %s\r\n"
		               "s=Play\r\n"
		               "c=IN IP4 %s\r\n"
		               "t=0 0\r\n"
					   "m=video %d RTP/AVP 96 98 97\r\n"
					   "a=recvonly\r\n"
					   "a=rtpmap:96 PS/90000\r\n"
					   "a=rtpmap:98 H264/90000\r\n"
					   "a=rtpmap:97 MPEG4/90000\r\n"
				       "y=%010u\r\n"
		               "f=\r\n",
					   src_id.c_str(),
					   src_ip.c_str(),
					   src_ip.c_str(),
					   video_port,
					   ssrc);

	osip_message_set_body(message, tmp, strlen(tmp));
	osip_message_set_content_type(message, "application/sdp");

	eXosip_lock(exosip);
	ret = eXosip_call_send_initial_invite(exosip, message);
	eXosip_unlock(exosip);
	return ret;
}

int GB28181Utils::TalkPlay(eXosip_t *exosip,
						   const std::string &dest_id,
						   const std::string &dest_ip,
						   const int dest_port,
						   const std::string &src_id,
						   const std::string &src_ip,
						   const int recv_port,
						   const int send_port,
						   const unsigned int ssrc)
{
	int ret;
	osip_message_t *message;
	char tmp[1024];
	char dest_call[100], source_call[100], route_call[100];
	std::string realm = src_id.substr(0,10);

	snprintf(dest_call,100, "sip:%s@%s", dest_id.c_str(), realm.c_str());
	snprintf(source_call,100, "sip:%s@%s", src_id.c_str(), realm.c_str());
	snprintf(route_call,100, "<sip:%s@%s:%d;lr>", dest_id.c_str(), dest_ip.c_str(), dest_port);

	ret = eXosip_call_build_initial_invite (exosip, &message, dest_call, source_call, route_call, NULL);
	if (ret != 0)
		return -1;

	snprintf(tmp, 1024, "v=0\r\n"
						"o=%s 0 0 IN IP4 %s\r\n"
						"s=Talk\r\n"
						"c=IN IP4 %s\r\n"
						"t=0 0\r\n"
						"m=audio %d RTP/AVP 8\r\n"
						"a=recvonly\r\n"
						"a=rtpmap:8 PCMA/8000/1\r\n"
						"m=audio %d RTP/AVP 8\r\n"
						"a=sendonly\r\n"
						"a=rtpmap:8 PCMA/8000/1\r\n"
						"y=%010u\r\n"
						"f=\r\n",
						src_id.c_str(),
						src_ip.c_str(),
						src_ip.c_str(),
						recv_port,
						send_port,
						ssrc);

	osip_message_set_body(message, tmp, strlen(tmp));
	osip_message_set_content_type(message, "application/sdp");

	eXosip_lock(exosip);
	ret = eXosip_call_send_initial_invite (exosip,message);
	eXosip_unlock (exosip);
	return ret;
}

int GB28181Utils::Playback(eXosip_t *exosip,
						   const std::string &dest_id,
						   const std::string &dest_ip,
						   const int dest_port,
						   const std::string &src_id,
						   const std::string &src_ip,
						   const int record_type,
						   const std::string &start_time,
						   const std::string &end_time,
						   const int recv_port,
						   const unsigned int ssrc)
{
	int ret;
	osip_message_t *message;
	char tmp[1024];
	char dest_call[100], source_call[100], route_call[100];
	std::string realm = src_id.substr(0,10);

	snprintf(dest_call,100, "sip:%s@%s", dest_id.c_str(), realm.c_str());
	snprintf(source_call,100, "sip:%s@%s", src_id.c_str(), realm.c_str());
	snprintf(route_call,100, "<sip:%s@%s:%d;lr>", dest_id.c_str(), dest_ip.c_str(), dest_port);

	ret = eXosip_call_build_initial_invite (exosip, &message, dest_call, source_call, route_call, NULL);
	if (ret != 0)
		return -1;

	unsigned long ul_start_time = StrToTimeStamp(start_time);
	unsigned long ul_end_time = StrToTimeStamp(end_time);
	
	snprintf(tmp, 4096, "v=0\r\n"
						"o=%s 0 0 IN IP4 %s\r\n"
						"s=Playback\r\n"
						"u=%s:%d\r\n"
						"c=IN IP4 %s\r\n"
						"t=%lu %lu\r\n"
						"m=video %d RTP/AVP 96 98 97\r\n"
						"a=recvonly\r\n"
						"a=rtpmap:96 H264/90000\r\n"
						"a=rtpmap:98 H264/90000\r\n"
						"a=rtpmap:97 MPEG4/90000\r\n"
						"y=%u\r\n"
						"f=\r\n",
						src_id.c_str(),
						src_ip.c_str(),
						dest_id.c_str(),
						record_type,
						src_ip.c_str(),
						ul_start_time,
						ul_end_time,
						recv_port,
						ssrc);

	osip_message_set_body(message, tmp, strlen(tmp));
	osip_message_set_content_type(message, "application/sdp");

	eXosip_lock(exosip);
	ret = eXosip_call_send_initial_invite (exosip,message);
	eXosip_unlock (exosip);
	return ret;
}

int GB28181Utils::Download(eXosip_t *exosip,
						   const std::string &dest_id,
						   const std::string &dest_ip,
						   const int dest_port,
						   const std::string &src_id,
						   const std::string &src_ip,
						   const int record_type,
						   const std::string &start_time,
						   const std::string &end_time,
						   const int recv_port,
						   const unsigned int ssrc)
{
	int ret;
	osip_message_t *message;
	char tmp[1024];
	char dest_call[100], source_call[100], route_call[100];
	std::string realm = src_id.substr(0,10);

	snprintf(dest_call,100, "sip:%s@%s", dest_id.c_str(), realm.c_str());
	snprintf(source_call,100, "sip:%s@%s", src_id.c_str(), realm.c_str());
	snprintf(route_call,100, "<sip:%s@%s:%d;lr>", dest_id.c_str(), dest_ip.c_str(), dest_port);

	ret = eXosip_call_build_initial_invite (exosip, &message, dest_call, source_call, route_call, NULL);
	if (ret != 0)
		return -1;

	unsigned long ul_start_time = StrToTimeStamp(start_time);
	unsigned long ul_end_time = StrToTimeStamp(end_time);

	snprintf(tmp, 4096, "v=0\r\n"
		                 "o=%s 0 0 IN IP4 %s\r\n"
		                 "s=Download\r\n"
		                 "u=%s:%d\r\n"
		                 "c=IN IP4 %s\r\n"
		                 "t=%lu %lu\r\n"
		                 "m=video %d RTP/AVP 96 98 97\r\n"
		                 "a=recvonly\r\n"
						 "a=rtpmap:96 H264/90000\r\n"
						 "a=rtpmap:98 H264/90000\r\n"
						 "a=rtpmap:97 MPEG4/90000\r\n"
						 "y=%u\r\n"
						 "f=\r\n",
						 src_id.c_str(),
						 src_ip.c_str(),
						 dest_id.c_str(),
						 record_type,
						 src_ip.c_str(),
						 ul_start_time,
						 ul_end_time,
						 recv_port,
						 ssrc);

	osip_message_set_body(message, tmp, strlen(tmp));
	osip_message_set_content_type(message, "application/sdp");

	eXosip_lock(exosip);
	ret = eXosip_call_send_initial_invite (exosip,message);
	eXosip_unlock (exosip);
	return ret;
}

bool GB28181Utils::StopPlay(eXosip_t *exosip, const int cid, const int did)
{
	int ret = -1;
	eXosip_lock(exosip);
	ret = eXosip_call_terminate(exosip, cid, did);
	eXosip_unlock (exosip);
	if (ret != OSIP_SUCCESS)
		return false;
	return true;
}

void GB28181Utils::PtzControl(eXosip_t *exosip,
							  const std::string &dest_id,
							  const std::string &dest_ip,
							  const int dest_port,
							  const std::string &src_id,
							  const unsigned int sn,
							  const std::string &device_id,
							  const int ptz_cmd,
							  const int speed)
{
	osip_message_t *message;
	char tmp[1024];
	char dest_call[100], source_call[100], route_call[100];
	std::string realm = src_id.substr(0,10);

	snprintf(dest_call,100, "sip:%s@%s", dest_id.c_str(), realm.c_str());
	snprintf(source_call,100, "sip:%s@%s", src_id.c_str(), realm.c_str());
	snprintf(route_call,100, "<sip:%s@%s:%d;lr>", dest_id.c_str(), dest_ip.c_str(), dest_port);

	std::string ptz_hex = PtzToHexStr(ptz_cmd, speed);

	eXosip_message_build_request (exosip, &message, "MESSAGE", dest_call, source_call, route_call);   
	
	snprintf(tmp, 1024, "<?xml version=\"1.0\"?>\r\n"
						"<Control>\r\n"
						"<CmdType>DeviceControl</CmdType>\r\n"
						"<SN>%d</SN>\r\n"
						"<DeviceID>%s</DeviceID>\r\n"
						"<PTZCmd>%s</PTZCmd>\r\n"
						//"<Info>\r\n"
						//"<ControlPriority>5</ControlPriority>\r\n"
						//"</Info>\r\n"
						"</Control>\r\n",
						sn,
						device_id.c_str(),
						ptz_hex.c_str());

	osip_message_set_body(message, tmp, strlen(tmp));   
	osip_message_set_content_type(message, "Application/MANSCDP+xml"); 
	eXosip_lock(exosip);
	eXosip_message_send_request(exosip, message);
	eXosip_unlock(exosip);
}

void GB28181Utils::Record(eXosip_t * exosip,
						  const std::string & dest_id,
						  const std::string & src_id,
						  const std::string & dest_ip,
						  const int & dest_port,
						  const bool & bStop)
{
	osip_message_t * message;
	int nCSeq=-1;
	osip_cseq_t * pCSeq=NULL;
	char tmp[1024];
	char dest_call[100],source_call[100],route_call[100];
	std::string realm=src_id.substr(0,10);
	std::string strStop;
	if (bStop)
	{
		strStop="StopRecord";
	}
	else
	{
		strStop="Record";
	}
	snprintf(dest_call,100,"sip:%s@%s",dest_id.c_str(),realm.c_str());
	snprintf(source_call,100,"sip:%s@%s",src_id.c_str(),realm.c_str());
	snprintf(route_call,100,"<sip:%s@%s:%d;lr>",dest_id.c_str(),dest_ip.c_str(),dest_port);

	eXosip_message_build_request (exosip,&message, "MESSAGE", dest_call, source_call, route_call);
	pCSeq=osip_message_get_cseq(message);
	if (pCSeq!=NULL && pCSeq->number!=NULL)
	{
		nCSeq=atoi(pCSeq->number);
	}
	else
	{
		return ;
	}
	snprintf (tmp, 1024,   
		"<?xml version=\"1.0\"?>\r\n"
		"<Control>\r\n"
		"<CmdType>DeviceControl</CmdType>\r\n"
		"<SN>%d</SN>\r\n"
		"<DeviceID>%s</DeviceID>\r\n"
		"<RecordCmd>%s</RecordCmd>\r\n"
		"</Control>\r\n",nCSeq,dest_id.c_str(),strStop.c_str());
	osip_message_set_body (message, tmp, strlen(tmp));   
	osip_message_set_content_type (message, "Application/MANSCDP+xml"); 
	eXosip_lock (exosip);
	eXosip_message_send_request (exosip,message);
	eXosip_unlock (exosip);
}

int GB28181Utils::QueryRecordFile(eXosip_t *exosip,
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
								   const std::string &address)
{
	osip_message_t *message;
	char tmp[1024];
	char dest_call[100], source_call[100], route_call[100];
	std::string realm = src_id.substr(0,10);

	snprintf(dest_call, 100, "sip:%s@%s", dest_id.c_str(), realm.c_str());
	snprintf(source_call, 100, "sip:%s@%s", src_id.c_str(), realm.c_str());
	snprintf(route_call, 100, "<sip:%s@%s:%d;lr>", dest_id.c_str(), dest_ip.c_str(), dest_port);

	eXosip_message_build_request(exosip, &message, "MESSAGE", dest_call, source_call, route_call);
	snprintf(tmp, 1024, "<?xml version=\"1.0\"?>r\n"
						"<Query>\r\n"
						"<CmdType>RecordInfo</CmdType>\r\n"
						"<SN>%d</SN>\r\n"
						"<DeviceID>%s</DeviceID>\r\n"
						"<StartTime>%s</StartTime>\r\n"
						"<EndTime>%s</EndTime>\r\n"
						"<FilePath>%s</FilePath>\r\n"
						"<Address>%s</Address>\r\n"
						"<Secrecy>0</Secrecy>\r\n"
						"<Type>%s</Type>\r\n"
						"<RecorderID>%s</RecorderID>\r\n"
						"</Query>\r\n",
						sn,
						device_id.c_str(),
						start_time.c_str(),
						end_time.c_str(),
						file_path.c_str(),
						address.c_str(),
						type.c_str(),
						record_id.c_str());

	osip_message_set_body(message, tmp, strlen(tmp));   
	osip_message_set_content_type(message, "Application/MANSCDP+xml"); 
	eXosip_lock(exosip);
	int ret = eXosip_message_send_request(exosip,message);
	eXosip_unlock(exosip);
	return ret;
}

void GB28181Utils::PlaybackPause(eXosip_t *exosip, const int did, const bool pause)
{
	osip_message_t *message = NULL;
	osip_cseq_t *cseq = NULL;
	int ncseq = 1;
	char tmp[1024];

	eXosip_call_build_info(exosip, did, &message);
	cseq = osip_message_get_cseq(message);
	if (cseq != NULL && cseq->number != NULL)
	{
		ncseq = atoi(cseq->number);
	}

	if (pause)
	{
		snprintf(tmp, 1024, "PAUSE RTSP/1.0\r\n"
							"CSeq: %d\r\n"
							"PauseTime: now\r\n",
							ncseq);
	}
	else
	{
		snprintf(tmp, 1024, "PLAY RTSP/1.0\r\n"
							"CSeq: %d\r\n"
							"Range: npt=now-\r\n",
							ncseq);
	}

	osip_message_set_body(message, tmp, strlen(tmp));   
	osip_message_set_content_type(message, "Application/MANSCDP+xml"); 
	eXosip_lock (exosip);
	eXosip_message_send_request(exosip,message);
	eXosip_unlock (exosip);
}

void GB28181Utils::PlayByAbsoluteTime(eXosip_t *exosip, const int did, const int npt)
{
	osip_message_t *message = NULL;
	osip_cseq_t *cseq = NULL;
	int ncseq = 1;
	char tmp[1024];

	eXosip_call_build_info(exosip, did, &message);
	cseq = osip_message_get_cseq(message);
	if (cseq != NULL && cseq->number != NULL)
	{
		ncseq = atoi(cseq->number);
	}

	snprintf(tmp, 1024, "PLAY RTSP/1.0\r\n"
						"CSeq: %d\r\n"
						"Range: npt=%d-\r\n",
						ncseq,
						npt);

	osip_message_set_body(message, tmp, strlen(tmp));   
	osip_message_set_content_type(message, "Application/MANSRTSP"); 
	eXosip_lock(exosip);
	eXosip_call_send_request(exosip, did, message);
	eXosip_unlock (exosip);
}

void GB28181Utils::PlaybackSpeed(eXosip_t *exosip, const int did, const double speed)
{
	osip_message_t *message = NULL;
	osip_cseq_t *cseq = NULL;
	int ncseq = 1;
	char tmp[1024];

	eXosip_call_build_info(exosip, did, &message);
	cseq = osip_message_get_cseq(message);
	if (cseq != NULL && cseq->number != NULL)
	{
		ncseq = atoi(cseq->number);
	}

	snprintf(tmp, 1024, "PLAY MANSRTSP/1.0\r\n"
						"CSeq: %d\r\n"
						"Scale: %f\r\n",
						ncseq,
						speed);

	osip_message_set_body(message, tmp, strlen(tmp));   
	osip_message_set_content_type(message, "Application/MANSRTSP"); 
	eXosip_lock(exosip);
	eXosip_call_send_request(exosip, did, message);
	eXosip_unlock (exosip);
}

void GB28181Utils::TeleBoot(eXosip_t * exosip,
							const std::string & dest_id,
							const std::string & src_id,
							const std::string & dest_ip,
							const int & dest_port)
{
	osip_message_t * message;
	int nCSeq=-1;
	osip_cseq_t * pCSeq=NULL;
	char tmp[1024];
	char dest_call[100],source_call[100],route_call[100];
	std::string realm=src_id.substr(0,10);
	snprintf(dest_call,100,"sip:%s@%s",dest_id.c_str(),realm.c_str());
	snprintf(source_call,100,"sip:%s@%s",src_id.c_str(),realm.c_str());
	snprintf(route_call,100,"<sip:%s@%s:%d;lr>",dest_id.c_str(),dest_ip.c_str(),dest_port);

	eXosip_message_build_request (exosip,&message, "MESSAGE", dest_call, source_call, route_call);   
	pCSeq=osip_message_get_cseq(message);
	if (pCSeq!=NULL && pCSeq->number!=NULL)
	{
		nCSeq=atoi(pCSeq->number);
	}
	else
	{
		//printf("Get the CSeq failed\n");
		return ;
	}
	snprintf (tmp, 1024,   
		"<?xml version=\"1.0\"?>\r\n"
		"<Control>\r\n"
		"<CmdType>DeviceControl</CmdType>\r\n"
		"<SN>%d</SN>\r\n"
		"<DeviceID>%s</DeviceID>\r\n"
		"<TeleBoot>Boot</TeleBoot>\r\n"
		//"<Info>\r\n"
		//"<ControlPriority>5</ControlPriority>\r\n"
		//"</Info>\r\n"
		"</Control>\r\n",nCSeq,dest_id.c_str());

	osip_message_set_body (message, tmp, strlen(tmp));   
	osip_message_set_content_type (message, "Application/MANSCDP+xml"); 
	eXosip_lock (exosip);
	eXosip_message_send_request (exosip,message);
	eXosip_unlock (exosip);
}

void GB28181Utils::SetGuard(eXosip_t * exosip,
							const std::string & dest_id,
							const std::string & src_id,
							const std::string & dest_ip,
							const int & dest_port,
							const bool & bSetGuard)
{
	osip_message_t * message;
	int nCSeq=-1;
	osip_cseq_t * pCSeq=NULL;
	char tmp[1024];
	char dest_call[100],source_call[100],route_call[100];
	std::string realm=src_id.substr(0,10);
	std::string strSetGuard;
	if (bSetGuard)
	{
		strSetGuard="SetGuard";
	}
	else
	{
		strSetGuard="ResetGuard";
	}
	snprintf(dest_call,100,"sip:%s@%s",dest_id.c_str(),realm.c_str());
	snprintf(source_call,100,"sip:%s@%s",src_id.c_str(),realm.c_str());
	snprintf(route_call,100,"<sip:%s@%s:%d;lr>",dest_id.c_str(),dest_ip.c_str(),dest_port);

	eXosip_message_build_request (exosip,&message, "MESSAGE", dest_call, source_call, route_call);   
	pCSeq=osip_message_get_cseq(message);
	if (pCSeq!=NULL && pCSeq->number!=NULL)
	{
		nCSeq=atoi(pCSeq->number);
	}
	else
	{
		//printf("Get the CSeq failed\n");
		return ;
	}
	snprintf (tmp, 1024,   
		"<?xml version=\"1.0\"?>\r\n"
		"<Control>\r\n"
		"<CmdType>DeviceControl</CmdType>\r\n"
		"<SN>%d</SN>\r\n"
		"<DeviceID>%s</DeviceID>\r\n"
		"<GuardCmd>%s</GuardCmd>\r\n"
		//"<Info>\r\n"
		//"<ControlPriority>5</ControlPriority>\r\n"
		//"</Info>\r\n"
		"</Control>\r\n",nCSeq,dest_id.c_str(),strSetGuard.c_str());

	osip_message_set_body (message, tmp, strlen(tmp));   
	osip_message_set_content_type (message, "Application/MANSCDP+xml"); 
	eXosip_lock (exosip);
	eXosip_message_send_request (exosip,message);
	eXosip_unlock (exosip);
}

void GB28181Utils::ResetAlarm(eXosip_t * exosip,
							  const std::string &dest_id,
							  const std::string & src_id,
							  const std::string & dest_ip,
							  const int & dest_port)
{
	osip_message_t * message;
	int nCSeq=-1;
	osip_cseq_t * pCSeq=NULL;
	char tmp[1024];
	char dest_call[100],source_call[100],route_call[100];
	std::string realm=src_id.substr(0,10);
	snprintf(dest_call,100,"sip:%s@%s",dest_id.c_str(),realm.c_str());
	snprintf(source_call,100,"sip:%s@%s",src_id.c_str(),realm.c_str());
	snprintf(route_call,100,"<sip:%s@%s:%d;lr>",dest_id.c_str(),dest_ip.c_str(),dest_port);

	eXosip_message_build_request (exosip,&message, "MESSAGE", dest_call, source_call, route_call);   
	pCSeq=osip_message_get_cseq(message);
	if (pCSeq!=NULL && pCSeq->number!=NULL)
	{
		nCSeq=atoi(pCSeq->number);
	}
	else
	{
		//printf("Get the CSeq failed\n");
		return ;
	}
	snprintf (tmp, 1024,   
		"<?xml version=\"1.0\"?>\r\n"
		"<Control>\r\n"
		"<CmdType>DeviceControl</CmdType>\r\n"
		"<SN>%d</SN>\r\n"
		"<DeviceID>%s</DeviceID>\r\n"
		"<AlarmCmd>ResetAlarm</AlarmCmd>\r\n"
		"<Info>\r\n"//以下三行对于景阳设备不能没
		"<AlarmMethod>2</AlarmMethod>\r\n"
		"</Info>\r\n"
		"</Control>\r\n",nCSeq,dest_id.c_str());

	osip_message_set_body (message, tmp, strlen(tmp));   
	osip_message_set_content_type (message, "Application/MANSCDP+xml"); 
	eXosip_lock (exosip);
	eXosip_message_send_request (exosip,message);
	eXosip_unlock (exosip);
}

void GB28181Utils::SubscribeAlarm(eXosip_t * exosip, const std::string & dest_id, const std::string & src_id, const std::string & dest_ip, const int & dest_port, const std::string & strAlarmSrcID, const int & nStartAlarmPriority, const int & nEndAlarmPriority, const int & nAlarmMethod, const std::string & strStartTime, const std::string & strEndTime,int & nExpires)
{
	osip_message_t * message;
	int nCSeq=-1;
	osip_cseq_t * pCSeq=NULL;
	char tmp[1024];
	char dest_call[100],source_call[100],route_call[100];
	std::string realm=src_id.substr(0,10);
	snprintf(dest_call,100,"sip:%s@%s",dest_id.c_str(),realm.c_str());
	snprintf(source_call,100,"sip:%s@%s",src_id.c_str(),realm.c_str());
	snprintf(route_call,100,"<sip:%s@%s:%d;lr>",dest_id.c_str(),dest_ip.c_str(),dest_port);

	eXosip_subscribe_build_initial_request(exosip,&message, dest_call, source_call, route_call,"prensence",nExpires);   
	pCSeq=osip_message_get_cseq(message);
	if (pCSeq!=NULL && pCSeq->number!=NULL)
	{
		nCSeq=atoi(pCSeq->number);
	}
	else
	{
		//printf("Get the CSeq failed\n");
		return ;
	}
	snprintf (tmp, 1024,   
		"<?xml version=\"1.0\"?>\r\n"
		"<Query>\r\n"
		"<CmdType>Alarm</CmdType>\r\n"
		"<SN>%d</SN>\r\n"
		"<DeviceID>%s</DeviceID>\r\n"
		"<StartAlarmPriority>%d</StartAlarmPriority>\r\n"
		"<EndAlarmPriority>%d</EndAlarmPriority>\r\n"
		"<AlarmMethod>%d</AlarmMethod>\r\n"
		"<StartTime>%s</StartTime>\r\n"
		"<EndTime>%s</EndTime>\r\n"
		"</Query>\r\n",nCSeq,strAlarmSrcID.c_str(),nStartAlarmPriority,nEndAlarmPriority,nAlarmMethod,strStartTime.c_str(),strEndTime.c_str());

	osip_message_set_body (message, tmp, strlen(tmp));   
	osip_message_set_content_type (message, "Application/MANSCDP+xml"); 
	eXosip_lock (exosip);
	eXosip_subscribe_send_initial_request(exosip,message);
	eXosip_unlock (exosip);
}

void GB28181Utils::SubscribeCatalog(eXosip_t *exosip, const std::string &dest_id, const std::string &dest_ip, const int dest_port, const std::string &src_id, const unsigned int sn, const std::string &device_id, const int expires)
{
	osip_message_t *message;
	char tmp[1024];
	char dest_call[100],source_call[100],route_call[100];
	std::string realm = src_id.substr(0,10);

	snprintf(dest_call,100, "sip:%s@%s", dest_id.c_str(), realm.c_str());
	snprintf(source_call,100, "sip:%s@%s", src_id.c_str(), realm.c_str());
	snprintf(route_call,100, "<sip:%s@%s:%d;lr>", dest_id.c_str(), dest_ip.c_str(), dest_port);

	eXosip_subscribe_build_initial_request(exosip, &message, dest_call, source_call, route_call, "Catalog;id=123", expires);

	snprintf (tmp, 1024, "<?xml version=\"1.0\"?>\r\n"
		                 "<Query>\r\n"
		                 "<CmdType>Catalog</CmdType>\r\n"
		                 "<SN>%d</SN>\r\n"
		                 "<DeviceID>%s</DeviceID>\r\n"
		                 "</Query>\r\n",
						 sn, 
						 device_id.c_str());

	osip_message_set_body (message, tmp, strlen(tmp));   
	osip_message_set_content_type (message, "Application/MANSCDP+xml"); 
	eXosip_lock (exosip);
	eXosip_subscribe_send_initial_request(exosip,message);
	eXosip_unlock (exosip);
}

void GB28181Utils::SubscribeRefreshCatalog(eXosip_t *exosip, const int did, const int expires, const unsigned int sn, const std::string &device_id)
{
	osip_message_t *message;
	char tmp[1024];
	char value[20];

	eXosip_subscribe_build_refresh_request(exosip, did, &message);

	snprintf (tmp, 1024, "<?xml version=\"1.0\"?>\r\n"
		"<Query>\r\n"
		"<CmdType>Catalog</CmdType>\r\n"
		"<SN>%d</SN>\r\n"
		"<DeviceID>%s</DeviceID>\r\n"
		"</Query>\r\n",
		sn, 
		device_id.c_str());

	osip_message_set_body (message, tmp, strlen(tmp));   
	osip_message_set_content_type (message, "Application/MANSCDP+xml"); 

	memset(value, 0, sizeof(value));
	sprintf(value, "%d", expires);
	osip_message_set_header(message, "Expires", value);

	eXosip_lock (exosip);
	eXosip_subscribe_send_refresh_request(exosip, did, message);
	eXosip_unlock (exosip);
}

//原来这是从1970-1-1 8:00:00开始算的，不知Linux是否一样
unsigned long GB28181Utils::StrToTimeStamp(const std::string & strTime)
{
	struct tm tm;
	strptime(strTime.c_str(),"%Y-%m-%d %H:%M:%S", &tm) ;
	return mktime(&tm);
}

//上下左右速度取nSpeed的低8位(0-255)，放大缩小取nSpeed的低4位(0-15)
std::string GB28181Utils::PtzToHexStr(const int & nPtzType,const int & nSpeed)
{
	unsigned char ptzCmd[8];
	memset(ptzCmd,0,8);
	ptzCmd[0]=0xA5;
	ptzCmd[1]=0x0F;//暂时固定，因为版本升级也不知道怎么变
	//地址码固定是0x001，景阳技术人员说没用到，宝信技术人员也说没作用
	ptzCmd[2]=0x01;
	//cmd
	//ptzCmd[3]
	//水平速度
	//ptzCmd[4]
	//垂直速度
	//ptzCmd[5]
	//变倍速度，因为地址(0x001)的高4位是0x0,所以这里肯定是0xX0
	//ptzCmd[6]
	//检验码
	//ptzCmd[7]
	//toString
	switch (nPtzType)
	{
	case GB_PTZ_ZOOM_IN:
		{
			ptzCmd[3]=0x20;
			ptzCmd[6]=(unsigned char)((nSpeed <<4) & 0xF0) | 0x0;//0x0为地址码的高4位
		}
		break;
	case GB_PTZ_ZOOM_OUT:
		{
			ptzCmd[3]=0x10;
			ptzCmd[6]=(unsigned char)((nSpeed <<4) & 0xF0) | 0x0;
		}
		break;
	case GB_PTZ_LEFT:
		{
			ptzCmd[3]=0x02;
			ptzCmd[4]=(unsigned char)(nSpeed & 0xFF);
			//ptzCmd[6]|=0x0;//0x0为地址码的高4位
		}
		break;
	case GB_PTZ_RIGHT:
		{
			ptzCmd[3]=0x01;
			ptzCmd[4]=(unsigned char)(nSpeed & 0xFF);
			//ptzCmd[6]|=0x0;//0x0为地址码的高4位
		}
		break;
	case GB_PTZ_UP:
		{
			ptzCmd[3]=0x08;
			ptzCmd[5]=(unsigned char)(nSpeed & 0xFF);
			//ptzCmd[6]|=0x0;//0x0为地址码的高4位
		}
		break;
	case GB_PTZ_DOWN:
		{
			ptzCmd[3]=0x04;
			ptzCmd[5]=(unsigned char)(nSpeed & 0xFF);
			//ptzCmd[6]|=0x0;//0x0为地址码的高4位
		}
		break;
	case GB_PTZ_FOCUS_NEAR:
		{
			ptzCmd[3]=0x42;
			ptzCmd[4]=(unsigned char)(nSpeed & 0xFF);
		}
		break;
	case GB_PTZ_FOCUS_FAR:
		{
			ptzCmd[3]=0x41;
			ptzCmd[4]=(unsigned char)(nSpeed & 0xFF);
		}
		break;
	case GB_PTZ_IRIS_ENLARGE:
		{
			ptzCmd[3]=0x44;
			ptzCmd[5]=(unsigned char)(nSpeed & 0xFF);
		}
		break;
	case GB_PTZ_IRIS_REDUCE:
		{
			ptzCmd[3]=0x48;
			ptzCmd[5]=(unsigned char)(nSpeed & 0xFF);
		}
		break;
	case GB_PTZ_PRESET:
		{
			ptzCmd[3]=0x82;
			ptzCmd[5]=(unsigned char)(nSpeed & 0xFF);
		}
	case GB_PTZ_STOP_COMMON:
		break;
	case GB_PTZ_STOP_FI:
		{
			ptzCmd[3]=0x40;
		}
		break;
	}

	//算校验码
	int nCode=0;
	for (int i=0;i<7;i++)
	{
		nCode+=ptzCmd[i];
	}
	ptzCmd[7]=(unsigned char)(nCode & 0xFF);
	//转字符串
	std::string strRet;
	for (int i=0;i<8;i++)
	{
		unsigned char cFourBit=(ptzCmd[i] >> 4) & 0xF;
		if (cFourBit <=9)
		{
			strRet.push_back((unsigned char)cFourBit+48);
		}
		else
		{
			strRet.push_back((unsigned char)cFourBit+55);
		}
		cFourBit=ptzCmd[i] & 0x0F;
		if (cFourBit <=9)
		{
			strRet.push_back((unsigned char)cFourBit+48);
		}
		else
		{
			strRet.push_back((unsigned char)cFourBit+55);
		}
	}
	return strRet;
}

bool GB28181Utils::IsUserIDValidate(const std::string & strID)
{
	if (strID.length()!=20) return false;
	char c;
	for (int i=0;i<20;i++)
	{
		c=strID.at(i);
		if (c<'0' || c>'9')
		{
			return false;
		}
	}
	return true;
}

int GB28181Utils::GetIDType(const std::string & strID)
{
	if (!IsUserIDValidate(strID))
	{
		return GB_ERROR_TYPE;
	}
	int nType=atoi(strID.substr(10,3).c_str());
	return nType !=0 ?nType : GB_ERROR_TYPE;
}

int GB28181Utils::GetIDNumber(const std::string & strID)
{
	if (!IsUserIDValidate(strID))
	{
		return GB_ERROR_TYPE;
	}
	int nNum=atoi(strID.substr(13,7).c_str());
	return nNum !=0 ? nNum : GB_ERROR_TYPE;
}

bool GB28181Utils::IsLeafNodeDevice(const std::string &id)
{
	int type = GetIDType(id);
	if (type != GB_ERROR_TYPE)
	{
		if(type == GB_PERIPHERAL_CAMERA || type == GB_PERIPHERAL_IPC)
			return true;
	}
	return false;
}

bool GB28181Utils::IsPlatformType(const std::string &id)
{
	int type = GetIDType(id);
	if (type != GB_ERROR_TYPE)
	{
		if(type == GB_PLATFORM_SESSION_CONTROL_CENTER)
			return true;
	}
	return false;
}

std::string GB28181Utils::GetCurrentDate()
{
	char buffer[80] = {'\0'};
#ifdef WIN32
	SYSTEMTIME sys;
	GetLocalTime(&sys);
	sprintf(buffer,"%4d-%02d-%02dT%02d:%02d:%02d.%03d",sys.wYear,sys.wMonth,sys.wDay,sys.wHour,sys.wMinute, sys.wSecond,sys.wMilliseconds);
#else
	struct timeval nowtimeval;
	gettimeofday(&nowtimeval,0);
	time_t now;
	struct tm *timenow;
	time(&now);
	timenow = localtime(&now);
	sprintf(buffer,
		"%4d-%02d-%02dT%02d:%02d:%02d.%03d",
		timenow->tm_year + 1900,
		timenow->tm_mon + 1,
		timenow->tm_mday,
		timenow->tm_hour,
		timenow->tm_min,
		timenow->tm_sec,
		nowtimeval.tv_usec/1000);
#endif
	return buffer;
}

std::string GB28181Utils::GetCurrentDateNoMs()
{
	char buffer[80] = {'\0'};
#ifdef WIN32
	SYSTEMTIME sys;
	GetLocalTime(&sys);
	sprintf(buffer,"%4d-%02d-%02dT%02d:%02d:%02d",sys.wYear,sys.wMonth,sys.wDay,sys.wHour,sys.wMinute, sys.wSecond);
#else
	time_t now;
	struct tm *timenow;
	time(&now);
	timenow = localtime(&now);
	sprintf(buffer,
		"%4d-%02d-%02dT%02d:%02d:%02d",
		timenow->tm_year + 1900,
		timenow->tm_mon + 1,
		timenow->tm_mday,
		timenow->tm_hour,
		timenow->tm_min,
		timenow->tm_sec);
#endif
	return buffer;
}

bool GB28181Utils::GBDateStringAddSecs(const char *s,size_t stCch, time_t tSec,char * pOut,size_t nOutLen)
{
	struct tm atm;
	if(sscanf(s,"%d-%d-%d%*[T]%d:%d:%d",&atm.tm_year,&atm.tm_mon,&atm.tm_mday,
		&atm.tm_hour,&atm.tm_min,&atm.tm_sec) != 6)
		return false;
	atm.tm_isdst = -1;
	atm.tm_mon -= 1;
	atm.tm_year -= 1900;
	tSec += mktime(&atm);
	atm=*localtime(&tSec);
	return strftime(pOut, nOutLen, "%Y-%m-%dT%H:%M:%S", &atm);
}


