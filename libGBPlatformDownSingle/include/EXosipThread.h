#ifndef _EXOSIP_THREAD_H_
#define _EXOSIP_THREAD_H_

#include "GBDefines.h"
#include "EXosipThreadEvent.h"
#include "MessageProcessThread.h"

#define NONCE_LEN 256
#define SIZE_CHAR 40

class EXosipThread:public KThread
{
public:
	EXosipThread(EXosipThreadEvent *exosip_thread_event = 0);
	virtual ~EXosipThread();

	void SetEXosip(eXosip_t *exosip){exosip_ = exosip;}
	void SetUserAgent(std::string agent);
	void SetLocalPort(int port = GB_SIP_PORT){local_port_ = port;}
	void SetLocalIP(const std::string &ip){local_ip_ = ip;}
	void SetID(const std::string &id){id_ = id;}
	void SetRealm(const std::string &realm){realm_ = realm;}
	void SetProtocol(int protocol){protocol_ = protocol;} //tcp 非0,udp 0

	virtual bool Start();
	virtual void Stop();

private:
	int  ProcessRegister(const eXosip_event_t *je);      //做成成员函数，主要是为了方便在里面回调
	void RemoveDoubleQuote(char str[],char remove='"');
	char *GenerateNonce();
	int  GetMediaPort(sdp_message_t *sdp, const char *type);

protected:
	virtual void ThreadProcMain();

private:
	MessageProcessThread *message_process_thread_;
	eXosip_t *exosip_;

	EXosipThreadEvent *exosip_thread_event_;
	bool running;
	int local_port_;
	int protocol_;
	std::string local_ip_;
	std::string realm_;
	std::string id_;
};
#endif