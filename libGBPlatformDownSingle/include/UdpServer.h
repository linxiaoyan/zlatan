#ifndef _UDP_SERVER_H_
#define _UDP_SERVER_H_
#include <NETEC/NETEC_UDPServer.h>
#include <NETEC/XCritSec.h>

class UdpServerEvent
{
public:
	virtual ~UdpServerEvent(){}
	virtual void OnRecvUdpData(unsigned char *data, int data_len)=0;
};

class UdpServer:public NETEC_UDPServerNotify
{
public:
	UdpServer(UdpServerEvent *stream_recver_event);
	virtual ~UdpServer();

	int  Open(unsigned short port);
	void Close(void);

	static int   SendData(SOCKET hSocket, struct sockaddr *addr, int addrlen, const char* data, int data_len);
	virtual bool OnNETEC_UDPServerNotifyRecvdPacket(UDPServerPacket *udp_server_packet);

protected:
	NETEC_UDPServer *udp_server_;
	UdpServerEvent  *udp_server_event_;
	int port_;
};


#endif