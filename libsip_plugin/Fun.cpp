#include "stdafx.h"
#include "fun.h"
#include "KBASE/Utils.h"
#include "inifile.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define ATL_BASE64_FLAG_NONE	0
#define ATL_BASE64_FLAG_NOPAD	1
#define ATL_BASE64_FLAG_NOCRLF  2


Fun::Fun(void)
{
}

Fun::~Fun(void)
{
}


inline int Base64EncodeGetRequiredLength(int nSrcLen, unsigned long dwFlags=ATL_BASE64_FLAG_NONE) throw()
{
	int nRet = nSrcLen*4/3;

	if ((dwFlags & ATL_BASE64_FLAG_NOPAD) == 0)
		nRet += nSrcLen % 3;

	int nCRLFs = nRet / 76 + 1;
	int nOnLastLine = nRet % 76;

	if (nOnLastLine)
	{
		if (nOnLastLine % 4)
			nRet += 4-(nOnLastLine % 4);
	}

	nCRLFs *= 2;

	if ((dwFlags & ATL_BASE64_FLAG_NOCRLF) == 0)
		nRet += nCRLFs;

	return nRet;
}

inline int Base64DecodeGetRequiredLength(int nSrcLen) throw()
{
	return nSrcLen;
}

inline bool Base64Encode(const unsigned char *pbSrcData,
						 int nSrcLen,
						 char* szDest,
						 int *pnDestLen,
						 unsigned long dwFlags=ATL_BASE64_FLAG_NONE) throw()
{
	static const char s_chBase64EncodingTable[64] = {
		'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q',
			'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g',	'h',
			'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y',
			'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/' };

		if (!pbSrcData || !szDest || !pnDestLen)
		{
			return false;
		}

		if(*pnDestLen < Base64EncodeGetRequiredLength(nSrcLen, dwFlags))
		{
			return false;
		}

		int nWritten( 0 );
		int nLen1( (nSrcLen/3)*4 );
		int nLen2( nLen1/76 );
		int nLen3( 19 );

		for (int i=0; i<=nLen2; i++)
		{
			if (i==nLen2)
				nLen3 = (nLen1%76)/4;

			for (int j=0; j<nLen3; j++)
			{
				unsigned long dwCurr(0);
				for (int n=0; n<3; n++)
				{
					dwCurr |= *pbSrcData++;
					dwCurr <<= 8;
				}
				for (int k=0; k<4; k++)
				{
					unsigned char b = (unsigned char)(dwCurr>>26);
					*szDest++ = s_chBase64EncodingTable[b];
					dwCurr <<= 6;
				}
			}
			nWritten+= nLen3*4;

			if ((dwFlags & ATL_BASE64_FLAG_NOCRLF)==0)
			{
				*szDest++ = '\r';
				*szDest++ = '\n';
				nWritten+= 2;
			}
		}

		if (nWritten && (dwFlags & ATL_BASE64_FLAG_NOCRLF)==0)
		{
			szDest-= 2;
			nWritten -= 2;
		}

		nLen2 = nSrcLen%3 ? nSrcLen%3 + 1 : 0;
		if (nLen2)
		{
			unsigned long dwCurr(0);
			for (int n=0; n<3; n++)
			{
				if (n<(nSrcLen%3))
					dwCurr |= *pbSrcData++;
				dwCurr <<= 8;
			}
			for (int k=0; k<nLen2; k++)
			{
				unsigned char b = (unsigned char)(dwCurr>>26);
				*szDest++ = s_chBase64EncodingTable[b];
				dwCurr <<= 6;
			}
			nWritten+= nLen2;
			if ((dwFlags & ATL_BASE64_FLAG_NOPAD)==0)
			{
				nLen3 = nLen2 ? 4-nLen2 : 0;
				for (int j=0; j<nLen3; j++)
				{
					*szDest++ = '=';
				}
				nWritten+= nLen3;
			}
		}

		*pnDestLen = nWritten;
		return true;
}

inline int DecodeBase64Char(unsigned int ch) throw()
{
	// returns -1 if the character is invalid
	// or should be skipped
	// otherwise, returns the 6-bit code for the character
	// from the encoding table
	if (ch >= 'A' && ch <= 'Z')
		return ch - 'A' + 0;	// 0 range starts at 'A'
	if (ch >= 'a' && ch <= 'z')
		return ch - 'a' + 26;	// 26 range starts at 'a'
	if (ch >= '0' && ch <= '9')
		return ch - '0' + 52;	// 52 range starts at '0'
	if (ch == '+')
		return 62;
	if (ch == '/')
		return 63;
	return -1;
}

inline bool Base64Decode(const char* szSrc, int nSrcLen, unsigned char *pbDest, int *pnDestLen) throw()
{
	// walk the source buffer
	// each four character sequence is converted to 3 bytes
	// CRLFs and =, and any characters not in the encoding table
	// are skiped

	if (szSrc == NULL || pnDestLen == NULL)
	{
		return false;
	}

	const char* szSrcEnd = szSrc + nSrcLen;
	int nWritten = 0;

	bool bOverflow = (pbDest == NULL) ? true : false;

	while (szSrc < szSrcEnd)
	{
		unsigned long dwCurr = 0;
		int i;
		int nBits = 0;
		for (i=0; i<4; i++)
		{
			if (szSrc >= szSrcEnd)
				break;
			int nCh = DecodeBase64Char(*szSrc);
			szSrc++;
			if (nCh == -1)
			{
				// skip this char
				i--;
				continue;
			}
			dwCurr <<= 6;
			dwCurr |= nCh;
			nBits += 6;
		}

		if(!bOverflow && nWritten + (nBits/8) > (*pnDestLen))
			bOverflow = true;

		// dwCurr has the 3 bytes to write to the output buffer
		// left to right
		dwCurr <<= 24-nBits;
		for (i=0; i<nBits/8; i++)
		{
			if(!bOverflow)
			{
				*pbDest = (unsigned char) ((dwCurr & 0x00ff0000) >> 16);
				pbDest++;
			}
			dwCurr <<= 8;
			nWritten++;
		}
	}

	*pnDestLen = nWritten;

	if(bOverflow)
	{
		if (pbDest!=NULL)
		{
		}
		return false;
	}

	return true;
}

string Fun::MemoryToString(unsigned char* pData, int nLen)
{
	char* pZipData = NULL;
	char* pBase64Data = NULL;
	string strEncodeText = "";

	do 
	{
		if (nLen<=0)
		{
			break;
		}

		//zlib_enc
		int nZipLen = nLen<1024?1024:nLen;
		pZipData = (char*)malloc(nZipLen);
		if (!Compress((char*)pData, nLen, pZipData, nZipLen))
		{
			break;
		}

		//base64_enc
		int nBase64Len = Base64EncodeGetRequiredLength(nZipLen, ATL_BASE64_FLAG_NOPAD);
		pBase64Data = (char*)malloc(nBase64Len);
		if (!Base64Encode((unsigned char*)pZipData, nZipLen, pBase64Data, &nBase64Len))
		{
			break;
		}

		pBase64Data[nBase64Len] = 0;
		strEncodeText = pBase64Data;

	} while(0);

	if (pZipData)
		free(pZipData);

	if (pBase64Data)
		free(pBase64Data);

	return strEncodeText;
}

bool Fun::StringToMemory(string& strZipText, unsigned char* pOutData, int&nLen)
{
	char* pUnzipData = NULL;
	unsigned char* pBase64Data = NULL;

	do 
	{
		//base64_dec
		int nBase64Len = Base64DecodeGetRequiredLength(strZipText.size());
		pBase64Data = (unsigned char*)malloc(nBase64Len);
		if (!Base64Decode(strZipText.c_str(), strZipText.length(), pBase64Data, &nBase64Len))
		{
			nLen = 0;
			break;
		}

		//zlib_dec
		int nUnzipLen=nLen<1024?1024:nLen;
		pUnzipData=(char*)malloc(nUnzipLen);
		if (!UnCompress((char*)pBase64Data, nBase64Len, pUnzipData, nUnzipLen))
		{
			nLen = 0;
			break;
		}

		nLen = nUnzipLen;
		memcpy(pOutData, pUnzipData, nUnzipLen);
	} while(0);

	if (pUnzipData)
		free(pUnzipData);

	if (pBase64Data)
		free(pBase64Data);

	return (nLen>0);
}

string Fun::GetIniPath(string file)
{
	string strPath ="";
#ifdef WIN32
	char szPath[MAX_PATH]={0};
	GetModuleFileNameA(NULL, szPath, MAX_PATH);
	strPath = szPath;
	int npos = strPath.find_last_of("\\");
	strPath=strPath.substr(0, npos+1)+file;

#else
	char szRunPath[1024] = {0};
	char link[64]={0};
	sprintf(link, "/proc/%d/exe", getpid());
	int ret = readlink(link, szRunPath, sizeof(szRunPath)); 
	if (ret<0) readlink("/proc/self/exe", szRunPath, sizeof(szRunPath)); 
	*(strrchr(szRunPath,'/')) = '\0';
	strPath=szRunPath;
	strPath=strPath+"/"+file;
#endif
	return strPath;
}

int Fun::ReadConfigInt(const char* sec,const char* key,int &value,int ndefault)
{
	string strPath = GetIniPath("config.ini");
	value=read_profile_int(sec,key ,ndefault,strPath.c_str()); 
	return value;
}

int Fun::WriteConfigInt(const char* sec,const char* key,int value)
{
	string strPath = GetIniPath("config.ini");
	return write_profile_string(sec,key,UINT2STR(value).c_str(),strPath.c_str());
}

int Fun::ReadConfigString(const char* sec,const char* key,string &strvalue,string strdefault)
{
	string strPath = GetIniPath("config.ini");
	char value[32] = {0};
	int ret = read_profile_string(sec, key , value, sizeof(value), strdefault.c_str(), strPath.c_str() );
	strvalue = value;

	return ret;
}

int Fun::ReadAlarmServerString(const char* sec,const char* key,string &strvalue,string strdefault)
{
	string strPath = GetIniPath("AlarmServer.ini");
	char value[32] = {0};
	int ret = read_profile_string(sec, key , value, sizeof(value), strdefault.c_str(), strPath.c_str() );
	strvalue = value;
	return ret;
}

int Fun::ReadAlarmServerInt(const char* sec,const char* key,int &value,int ndefault)
{
	string strPath = GetIniPath("AlarmServer.ini");
	value=read_profile_int(sec,key ,ndefault,strPath.c_str()); 
	return value;
}

int Fun::WriteConfigString(const char* sec,const char* key,string strvalue)
{
	string strPath = GetIniPath("config.ini");
	return write_profile_string(sec, key, strvalue.c_str(), strPath.c_str());
}

string Fun::TrimRight(string &pString)
{
	int strLength = 0;

	if (pString.empty())
	{
		return pString;
	}

	strLength = pString.size();

	for (int i=strLength-1; i>=0; --i)
	{
		if ( !isspace( pString.at(i) ))
		{
			pString = pString.substr(0, i+1);
			break;
		}
	}
	return pString;
}

string Fun::TrimLeft(string &pString)
{   
	if (pString.empty())
	{
		return pString;
	}

	unsigned long i = 0;
	while ( i<pString.size() )
	{
		if ( !isspace(pString.at(i)) )
		{
			pString = pString.substr(i, pString.size());
			break;
		}
		++i;
	}
	return pString;
}

string Fun::TrimString(string &pString)
{
	if (pString.empty())
	{
		return pString;
	}

	TrimLeft(pString);
	TrimRight(pString);

	return pString;
}

 #ifdef WIN32
 
 #else
 #include <iconv.h>
int Fun::code_convert(char *from_charset,char *to_charset,char *inbuf,int inlen,char *outbuf,int outlen)
 {
 	iconv_t cd;
 	int rc;
 	char **pin = &inbuf;
 	char **pout = &outbuf;
 	size_t in_len = (size_t)inlen;
 	size_t out_len = (size_t)outlen;
 
 	cd = iconv_open(to_charset,from_charset);
 	if (cd==0) 
 	{
		LOG::INF("iconv_open is  failed!\n");
 		iconv_close(cd);
 		return -1;
 	}
 	memset(outbuf,0,out_len);
 	if (iconv(cd, pin,&in_len,pout,&out_len)==-1) 
 	{
		LOG::INF("iconv is  failed! inbuf:%s, outbut:%s\n", *pin,pout );
 		iconv_close(cd);
 		return -1;
 	}
 	iconv_close(cd);
 	return 0;
 }

 #endif //_WIN32
 
 string Fun::UTF8toGB2312(const char*utf8)
 {
 #ifdef WIN32
  	USES_CONVERSION;
  	return W2A(A2U(utf8));
	 return "";
 #else
 	string gb2312 = utf8;
 	int utf_len = strlen(utf8);
 	int gb_len = utf_len*2 + 1;
 	char* gb_buf = new char[gb_len];
 	if (gb_buf)
 	{
 		code_convert("utf-8","gb2312",(char*)utf8, utf_len, gb_buf, gb_len);
 		gb2312 = gb_buf;
 		delete [] gb_buf;
 	}
 	return gb2312;
 #endif //WIN32
 }
 
 string Fun::GB2312toUTF8(const char*gb2312)
 {
 #ifdef WIN32
  	USES_CONVERSION;
  	return U2A(A2W(gb2312));
	  return "";
 #else
 	string utf = gb2312;
 	int gb_len = strlen(gb2312);
 	int utf_len = gb_len * 2 + 1;
 	char * utf_buf = new char[utf_len];
 	if (utf_buf)
 	{
 		code_convert("gb2312","utf-8",(char*)gb2312,gb_len,utf_buf,utf_len);
 		utf = utf_buf;
 		delete [] utf_buf;
 	}
 	return utf;
 #endif //WIN32
 }
