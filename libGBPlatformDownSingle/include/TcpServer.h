#ifndef _TCP_SERVER_H_
#define _TCP_SERVER_H_
#include <NETEC/NETEC_TCPServer.h>
#include <NETEC/XCritSec.h>

#define MAX_TCP_BUFFER_SZIE 65536

typedef struct tagRTSPHeader
{
	unsigned char  ucMagic;
	unsigned char  ucChannel;
	unsigned short usLength;
}RTSPHeader;

class TcpServerEvent
{
public:
	virtual ~TcpServerEvent(){}
	virtual void OnRecvTcpData(unsigned char *data, unsigned int data_len)=0;
};

class TcpServer:public NETEC_TCPServerNotify,public NETEC_TCPServerStreamCallback
{
public:
	TcpServer(TcpServerEvent *tcp_server_event);
	virtual ~TcpServer(void);

	virtual int Open(unsigned short nPort);
	virtual void Close(void);

	static int SendData(SOCKET hSocket,struct sockaddr *addr, int addrlen, const char *pData, int nDataLen);
protected:
	virtual bool OnNETEC_TCPServerNotifyConnected(SOCKET hSocket,const char*cszLocalIP,const char*cszPeerIP);
	
	virtual void OnNETEC_TCPServerStreamCallbackRecvdData(const char*pData,int nLen);
	virtual void OnNETEC_TCPServerStreamCallbackDisconnected(void);

protected:
	NETEC_TCPServer *tcp_server_;
	TcpServerEvent  *tcp_server_event_;
	NETEC_TCPServerStream *tcp_server_stream_;
	int port_;

	unsigned char *buffer_;
	int buffer_size_;
	int data_len_;
};

#endif