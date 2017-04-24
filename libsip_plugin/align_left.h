#pragma once

class align_left_samples;
class align_left_samples_callback
{
public:
	virtual void OnAlignLeftCallback_OneLeftSamples(align_left_samples* pLeft,short*pSamples,int nSamples) = 0;
};

class align_left_samples
{
	int m_nSamplesPerFrame;
	align_left_samples_callback&m_callback;
public:
	align_left_samples(align_left_samples_callback&cb,int nSamplesPerFrame);
	~align_left_samples(void);
public:
	void AlignSamples(short*pSamples,int nSamples);
protected:
	short*m_pLeftSamples;
	int	  m_nLeftSamples;
};

class align_left_bytes;
class align_left_bytes_callback
{
public:
	virtual void OnAlignLeftCallback_OneLeftBytes(align_left_bytes* pLeft,unsigned char*pBytes,int nBytes) = 0;
};

class align_left_bytes
{
	int m_nBytesPerFrame;
	align_left_bytes_callback&m_callback;
public:
	align_left_bytes(align_left_bytes_callback&cb,int nBytesPerFrame);
	~align_left_bytes(void);
public:
	void AlignBytes(unsigned char*pBytes,int nBytes);
protected:
	unsigned char* m_pLeftBytes;
	int m_nLeftBytes;
};
