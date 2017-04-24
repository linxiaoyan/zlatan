#include "stdafx.h"
#include "RtpReorder.h"
#include "GBDefines.h"

RtpReorder::RtpReorder(VideoCallback *cb)
	:video_cb_(cb)
	,audio_cb_(NULL)
	,buff_size_(RTP_BUFFER_SIZE)
	,input_sum_(0)
	,output_index_(0)
	,destroy_(false)
{
#ifdef RTP_REORDER_DEBUG
	fd_in = fopen("D:\\video_reorder_in", "w");
	fd_out = fopen("D:\\video_reorder_out", "w");
#endif

	for (int i = 0; i < buff_size_; ++i)
		rtp_buffer[i].len = 0;
}

RtpReorder::RtpReorder(AudioCallback *cb)
	:video_cb_(NULL)
	,audio_cb_(cb)
	,buff_size_(RTP_BUFFER_SIZE)
	,input_sum_(0)
	,output_index_(0)
	,destroy_(false)
{
#ifdef RTP_REORDER_DEBUG
	fd_in = fopen("D:\\audio_reorder_in", "w");
	fd_out = fopen("D:\\audio_reorder_out", "w");
#endif

	for (int i = 0; i < buff_size_; ++i)
		rtp_buffer[i].len = 0;
}

void RtpReorder::InputRtpPacket(char *pack, const int len)
{
	GBRtpHeader *rtp_header = (GBRtpHeader *)pack;
	
	if (input_sum_ == buff_size_/4 * 3)
	{
		OutputRtpPacket();
		output_index_ = (output_index_ + buff_size_/4) % buff_size_;
	}

	int seq = ntohs(rtp_header->seq);
	int index = seq % buff_size_;
	rtp_buffer[index].len = len;
	memcpy(&rtp_buffer[index].data, pack, len);

	++input_sum_;

#ifdef RTP_REORDER_DEBUG
	char tmp[1024];
	memset(tmp, 0, 1024);
	sprintf(tmp, "-------->input[%d]--->len[%d]\n", index, len);
	fputs(tmp, fd_in);
#endif
}

void RtpReorder::OutputRtpPacket()
{
	for (int i = output_index_; i < output_index_ + buff_size_/4; ++i)
	{
		if (rtp_buffer[i].len > 0)
		{
			destroy_cs_.Lock();
			if (destroy_)
			{
				destroy_cs_.Unlock();
				return;
			}

#ifdef RTP_REORDER_DEBUG
			char tmp[1024];
			memset(tmp, 0, 1024);
			sprintf(tmp, "-------->output[%d]--->len[%d]\n", i, rtp_buffer[i].len);
			fputs(tmp, fd_out);
#endif

			if (video_cb_)
			{
				video_cb_->On3PartyVideoCallback((unsigned char *)&rtp_buffer[i].data, rtp_buffer[i].len);
				rtp_buffer[i].len = 0;
			}
			else if (audio_cb_)
			{
				audio_cb_->On3PartyAudioCallback((unsigned char *)&rtp_buffer[i].data, rtp_buffer[i].len);
				rtp_buffer[i].len = 0;
			}

			--input_sum_;
			destroy_cs_.Unlock();
		}
	}
}

void RtpReorder::Destroy()
{
	destroy_cs_.Lock();
	destroy_ = true;
	destroy_cs_.Unlock();

#ifdef RTP_REORDER_DEBUG
	fclose(fd_in);
	fclose(fd_out);
#endif
}