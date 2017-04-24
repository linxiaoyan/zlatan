#include "stdafx.h"
#include "align_left.h"

align_left_samples::align_left_samples(align_left_samples_callback&cb,int nSamplesPerFrame)
: m_callback(cb)
, m_nSamplesPerFrame(nSamplesPerFrame)
{
	m_pLeftSamples=(short*)malloc(nSamplesPerFrame*sizeof(short));
	m_nLeftSamples=0;
}

align_left_samples::~align_left_samples(void)
{
	if (m_pLeftSamples)
	{
		free(m_pLeftSamples);
		m_pLeftSamples=NULL;
	}
}

void align_left_samples::AlignSamples(short*pSamples,int nSamples)
{
	short*pTempSamples=pSamples;
	int nTempSamples=nSamples;
	if (m_nLeftSamples)
	{
		if (m_nLeftSamples+nSamples>=m_nSamplesPerFrame)
		{
			int nLeftSamples=m_nSamplesPerFrame-m_nLeftSamples;
			memcpy(m_pLeftSamples+m_nLeftSamples,pSamples,nLeftSamples<<1);
			m_callback.OnAlignLeftCallback_OneLeftSamples(this,m_pLeftSamples,m_nSamplesPerFrame);
			m_nLeftSamples=0;
			pTempSamples+=nLeftSamples;
			nTempSamples-=nLeftSamples;
		}
		else
		{
			memcpy(m_pLeftSamples+m_nLeftSamples,pSamples,nSamples<<1);
			m_nLeftSamples+=nSamples;
			return ;
		}
	}

	while (nTempSamples>=m_nSamplesPerFrame)
	{
		m_callback.OnAlignLeftCallback_OneLeftSamples(this,pTempSamples,m_nSamplesPerFrame);

		pTempSamples+=m_nSamplesPerFrame;
		nTempSamples-=m_nSamplesPerFrame;
	}

	if (nTempSamples>0)
	{
		memcpy(m_pLeftSamples,pTempSamples,nTempSamples<<1);
		m_nLeftSamples=nTempSamples;
	}
}



align_left_bytes::align_left_bytes(align_left_bytes_callback&cb,int nBytesPerFrame)
: m_callback(cb)
, m_nBytesPerFrame(nBytesPerFrame)
{
	m_pLeftBytes=(unsigned char*)malloc(nBytesPerFrame*sizeof(unsigned char));
	m_nLeftBytes=0;
}

align_left_bytes::~align_left_bytes(void)
{
	if (m_pLeftBytes)
	{
		free(m_pLeftBytes);
		m_pLeftBytes = NULL;
	}
}

void align_left_bytes::AlignBytes(unsigned char*pBytes,int nBytes)
{
	unsigned char*pTempBytes=pBytes;
	int nTempBytes=nBytes;
	if (m_nLeftBytes)
	{
		if (m_nLeftBytes+nBytes>=m_nBytesPerFrame)
		{
			int nLeftBytes=m_nBytesPerFrame-m_nLeftBytes;
			memcpy(m_pLeftBytes+m_nLeftBytes,pBytes,nLeftBytes);
			m_callback.OnAlignLeftCallback_OneLeftBytes(this,m_pLeftBytes,m_nBytesPerFrame);
			m_nLeftBytes=0;
			pTempBytes+=nLeftBytes;
			nTempBytes-=nLeftBytes;
		}
		else
		{
			memcpy(m_pLeftBytes+m_nLeftBytes,pBytes,nBytes);
			m_nLeftBytes+=nBytes;
			return ;
		}
	}

	while (nTempBytes>=m_nBytesPerFrame)
	{
		m_callback.OnAlignLeftCallback_OneLeftBytes(this,pTempBytes,m_nBytesPerFrame);

		pTempBytes+=m_nBytesPerFrame;
		nTempBytes-=m_nBytesPerFrame;
	}

	if (nTempBytes>0)
	{
		memcpy(m_pLeftBytes,pTempBytes,nTempBytes);
		m_nLeftBytes=nTempBytes;
	}
}