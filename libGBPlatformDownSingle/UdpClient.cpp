#include "stdafx.h"
#include "UdpClient.h"

UdpClient::UdpClient():socket_(INVALID_SOCKET)
{
	
}

UdpClient::~UdpClient()
{

}

int UdpClient::Create(const std::string &src_addr, unsigned short src_port)
{
	sockaddr_in addr;
	addr.sin_family=AF_INET;

	if(src_addr.empty() || src_addr == "0.0.0.0")
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
	else
		addr.sin_addr.s_addr = inet_addr(src_addr.c_str());
	
	addr.sin_port = htons(src_port);

	socket_ = ::socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
	if(socket_ == INVALID_SOCKET)
		return -1;

	if(::bind(socket_, (struct sockaddr*)&addr, sizeof(sockaddr_in)) == SOCKET_ERROR)
	{   
		Close();
		return -1;
	}

	return 0;
}

void UdpClient::Close(void)
{
	if(socket_ == INVALID_SOCKET) 
		return;

	closesocket(socket_);
	socket_ = INVALID_SOCKET;
}

int UdpClient::SendTo(const std::string &dest_addr, unsigned short dest_port, char *data, int data_len)
{
	if(socket_ == INVALID_SOCKET) 
		return -1;

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(dest_addr.c_str());
	addr.sin_port   = htons(dest_port);

	return ::sendto(socket_, (const char*)data, data_len, MSG_NOSIGNAL, (struct sockaddr*)&addr, sizeof(sockaddr_in));
}

unsigned short UdpClient::GetLocalPort(void)
{
	if (socket_ == INVALID_SOCKET)
		return 0;

	sockaddr_in addr;
#ifdef WIN32	
	int addr_len = sizeof (sockaddr);
#else
	socklen_t addr_len = sizeof (sockaddr);
#endif
	if (::getsockname(socket_, (sockaddr*)&addr, &addr_len) == SOCKET_ERROR)
		return 0;

	return ntohs(addr.sin_port);
}