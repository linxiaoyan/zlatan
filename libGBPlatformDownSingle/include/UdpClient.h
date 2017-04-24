#ifndef _UDP_CLIENT_H_
#define _UDP_CLIENT_H_
#include "GBDefines.h"

class UdpClient
{
public:
	UdpClient();
	virtual ~UdpClient();

	virtual int  Create(const std::string &src_addr, unsigned short src_port);
	virtual void Close(void);
	virtual int  SendTo(const std::string &dest_addr, unsigned short dest_port, char *data, int data_len);
	unsigned short GetLocalPort(void);

protected:
	SOCKET socket_;
};
#endif