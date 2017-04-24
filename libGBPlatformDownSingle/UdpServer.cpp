#include "stdafx.h"
#include "GBDefines.h"
#include "UdpServer.h"

UdpServer::UdpServer(UdpServerEvent *udp_server_event):udp_server_event_(udp_server_event)
{

}

UdpServer::~UdpServer()
{
	Close();
}

int UdpServer::Open(unsigned short port)
{
	udp_server_ = NETEC_UDPServerCreate(*this, port);
	
	if (udp_server_)
		port_ = port;
	else
		port_ = -1;
	
	return port_;
}

void UdpServer::Close(void)
{
	if (udp_server_)
	{
		udp_server_->ReleaseConnections();
		delete udp_server_;
		udp_server_ = NULL;
	}
}

bool UdpServer::OnNETEC_UDPServerNotifyRecvdPacket(UDPServerPacket *pUDPServerPacket)
{
	size_t packet_len = pUDPServerPacket->GetDataLen();
	unsigned char *packet = (unsigned char *)pUDPServerPacket->GetData();
	packet[packet_len]='\0';

	if (udp_server_event_)
	{
		udp_server_event_->OnRecvUdpData(packet, packet_len);
		return true;
	}
	return false;
}

int UdpServer::SendData(SOCKET hSocket, sockaddr *addr, int addrlen, const char *pData, int nDataLen)
{
	UDPServerPacket tPacket(hSocket,addr,addrlen,pData,nDataLen);
	tPacket.Send();
	return 0;
}


