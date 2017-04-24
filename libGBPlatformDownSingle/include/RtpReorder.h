#ifndef _RTP_REORDER_H_
#define _RTP_REORDER_H_

#include "On3PartyCallback.h"
#include "KBASE/CritSec.h"

#define RTP_BUFFER_SIZE 128

typedef struct
{
	char data[1500];
	int  len;
}RtpPacket;

class RtpReorder
{
public:
	RtpReorder(VideoCallback *cb);
	RtpReorder(AudioCallback *cb);
	void InputRtpPacket(char *packet, const int len);
	void Destroy();

private:
	inline void OutputRtpPacket();

private:
	RtpPacket rtp_buffer[RTP_BUFFER_SIZE];
	VideoCallback *video_cb_;
	AudioCallback *audio_cb_;
	
	unsigned short buff_size_;
	unsigned short input_sum_;
	unsigned short output_index_;

	KCritSec destroy_cs_;
	bool destroy_;

#ifdef RTP_REORDER_DEBUG
	FILE *fd_in;
	FILE *fd_out;
#endif
};

#endif