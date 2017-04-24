#ifndef _ON_3PARTY_CALLBACK_H_
#define _ON_3PARTY_CALLBACK_H_

class VideoCallback
{
public:
	virtual void On3PartyVideoCallback(unsigned char *data, int data_len)=0;
};

class AudioCallback
{
public:
	virtual void On3PartyAudioCallback(unsigned char *data, int data_len)=0;
};

#endif