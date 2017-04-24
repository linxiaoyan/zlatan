#include "stdafx.h"
#include "GBDefines.h"
#include "TcpServer.h"
#define RTP_HEADER_LEN 12

TcpServer::TcpServer(TcpServerEvent *tcp_server_event):
	tcp_server_event_(tcp_server_event),
	tcp_server_(NULL),
	tcp_server_stream_(NULL),
	buffer_(NULL),
	buffer_size_(0),
	data_len_(0)
{

}

TcpServer::~TcpServer()
{
	Close();
	if (buffer_)
		free(buffer_);
}


int TcpServer::Open(unsigned short port)
{
	tcp_server_ = NETEC_TCPServerCreate(*this, port);
	if (tcp_server_ == NULL)
		return -1;
	port_ = port;
	return 0;
}

void TcpServer::Close(void)
{
	if (tcp_server_stream_)
	{
		tcp_server_stream_->Close();
		delete tcp_server_stream_;
		tcp_server_stream_ = NULL;
	}

	if (tcp_server_)
	{
		tcp_server_->ReleaseConnections();
		delete tcp_server_;
		tcp_server_ = NULL;
	}
}

void TcpServer::OnNETEC_TCPServerStreamCallbackRecvdData(const char *pData,int nLen)
{
	if (buffer_size_ <= data_len_+nLen)
	{
		unsigned char *ptempbuf = (unsigned char *)malloc(data_len_ + nLen + 1024);
		if (ptempbuf)
		{
			if (buffer_)
			{
				if (data_len_ > 0)
					memcpy(ptempbuf,buffer_,data_len_);

				free(buffer_);
			}

			buffer_ = ptempbuf;
			buffer_size_ = data_len_+nLen+1024;
		}
	}
	memcpy(buffer_ + data_len_, pData, nLen);
	data_len_ += nLen;

#ifdef _USE_DBG_GBTCP
	printf("[GBSDK]: The begin data_len_: %d and nLen: %d\n",data_len_,nLen);
#endif

	if (data_len_<RTP_HEADER_LEN+4)
	{
#ifdef _USE_DBG_GBTCP
		printf("[GBSDK]: The data is little than 16\n");
#endif
		return ;
	}

	int nHandleLen=0;
	int nLeftLen=data_len_-nHandleLen;

	while(1)
	{
		if (nLeftLen<RTP_HEADER_LEN+4)
		{
#ifdef _USE_DBG_GBTCP
			printf("[GBSDK]: The left data is little than 16!\n");
#endif
			if (nLeftLen>0 && data_len_-nLeftLen>0)
			{
#ifdef _USE_DBG_GBTCP
				printf("[GBSDK]: The memmove data_len_ :%d and nLeftLen :%d\n",data_len_,nLeftLen);
#endif
				memmove(buffer_, buffer_+(data_len_-nLeftLen), nLeftLen);
				data_len_ = nLeftLen;
			}
			else
			{
				data_len_ = 0;
			}
			break;
		}
		if (buffer_[nHandleLen+0]=='$')
		{
			if (buffer_[nHandleLen+1]==0)//rtp
			{
				//usSize包括Rtp头
				unsigned short usSize=buffer_[nHandleLen+2];
				usSize=((usSize<<8) & 0xFF00|buffer_[nHandleLen+3]);
#ifdef _USE_DBG_GBTCP
				printf("[GBSDK]: The usSize is %d\n",usSize);
#endif

				if (nLeftLen<usSize+4)
				{
					//数据不够一帧
#ifdef _USE_DBG_GBTCP
					printf("[GBSDK]: The left data is not enough!\n");
#endif
					if (nLeftLen>0 && data_len_-nLeftLen>0)
					{
						printf("[GBSDK]: The memmove data_len_ :%d and nLeftLen :%d\n",data_len_,nLeftLen);
						memmove(buffer_, buffer_+(data_len_-nLeftLen), nLeftLen);
						
						data_len_ = nLeftLen;
					}
					else
					{
						data_len_ = 0;
					}
					break;
				}

				GBRtpHeader *pRtpHeader=(GBRtpHeader *)&buffer_[nHandleLen+4];

				if ((int)pRtpHeader->pt==96)
				{
					if (tcp_server_event_)
					{
						tcp_server_event_->OnRecvTcpData((unsigned char *)&buffer_[nHandleLen+RTP_HEADER_LEN+4],usSize-RTP_HEADER_LEN);
						nHandleLen+=4+usSize;//包括
						nLeftLen=data_len_-nHandleLen;
#ifdef _USE_DBG_GBTCP
						printf("[GBSDK]: data_len_ %d and nHandleLen %d and nLeftLen %d\n",data_len_,nHandleLen,nLeftLen);
#endif
					}
				}
				else
				{
					if (tcp_server_event_)
					{
						tcp_server_event_->OnRecvTcpData((unsigned char *)&buffer_[nHandleLen+4],usSize);
						nHandleLen+=4+usSize;
						nLeftLen=data_len_-nHandleLen;
					}
				}
			}
			else if (buffer_[1]==1)//rtcp
			{
				unsigned short usSize=buffer_[nHandleLen+2];
				usSize=((usSize<<8)|buffer_[nHandleLen+3]);

				if (nLeftLen<usSize+4)
				{
					//数据不够一帧
					if (nLeftLen>0 && data_len_-nLeftLen>0)
					{
						printf("[GBSDK]: The memmove data_len_ :%d and nLeftLen :%d\n",data_len_,nLeftLen);
						memmove(buffer_, buffer_+(data_len_-nLeftLen), nLeftLen);

						data_len_ = nLeftLen;
					}
					else
					{
						data_len_ = 0;
					}
					break;
				}

				GBRtpHeader * pRtpHeader=(GBRtpHeader*)&buffer_[nHandleLen+4];

				if ((int)pRtpHeader->pt==96)
				{
					nHandleLen+=4+usSize;
					nLeftLen=data_len_-nHandleLen;
				}
				else
				{
					nHandleLen+=4+usSize;
					nLeftLen=data_len_-nHandleLen;
				}
			}
			else
			{
				//error data
#ifdef _USE_DBG_GBTCP
				printf("[GBSDK]: The stream is not channel 0 or 1\n");
#endif
				data_len_=0;
				return ;
			}
		}
		else
		{
			//error data
#ifdef _USE_DBG_GBTCP
			printf("[GBSDK]: The stream is not start with $\n");
#endif
			data_len_=0;
			return ;
		}
	}
}


bool TcpServer::OnNETEC_TCPServerNotifyConnected(SOCKET hSocket,const char*cszLocalIP,const char*cszPeerIP)
{
	printf("[GBSDK]: OnNETEC_TCPServerNotifyConnected\n");
	if (tcp_server_stream_)
	{
		tcp_server_stream_->Close();
		delete tcp_server_stream_;
	}

	tcp_server_stream_=NETEC_TCPServerStream::Create(*this,hSocket, cszLocalIP, cszPeerIP);
	printf("<socket>[%d]\n", hSocket);
	printf("<PeerIP>[%s]\n", cszPeerIP);
	return true;
}


void TcpServer::OnNETEC_TCPServerStreamCallbackDisconnected()
{
	printf("[GBSDK]: OnNETEC_TCPServerStreamCallbackDisconnected\n");
	//if (tcp_server_stream_)
	//{
	//	tcp_server_stream_->Close();
	//	delete tcp_server_stream_;
	//	tcp_server_stream_=NULL;
	//}

	//if (m_pTCPServer)
	//{
	//	m_pTCPServer->ReleaseConnections();
	//	delete m_pTCPServer;
	//	m_pTCPServer=NULL;
	//}
}

