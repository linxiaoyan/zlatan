#pragma once
#ifndef FUN_DEF_H
#define FUN_DEF_H


#define MAXBUF 4096
#define MAXLEN(a) (sizeof(a)-1)
#define MINLEN(a,b) (((a)>(b)) ? (b) : (a))
#define SAFE_STRCPY(sz1,sz2) \
{\
	memset(sz1, 0, sizeof(sz1)); \
	strncpy(((char*)(sz1)), ((char*)(sz2)), MINLEN(MAXLEN(sz1), strlen((char*)(sz2))));\
}

class Fun
{
public:
	Fun(void);
	~Fun(void);
	static int ReadConfigInt(const char* sec,const char* key, int &value,int ndefault);
	static int WriteConfigInt(const char* sec,const char* key, int value);
	static int ReadConfigString(const char* sec,const char* key, string &strvalue,string strdefault);
	static int WriteConfigString(const char* sec,const char* key, string strvalue);
	static string GetIniPath(string file);

	static int ReadAlarmServerInt(const char* sec,const char* key, int &value,int ndefault);
	static int ReadAlarmServerString(const char* sec,const char* key, string &strvalue,string strdefault);

	static string TrimRight(string &pString);
	static string TrimLeft(string &pString);
	static string TrimString(string &pString);

	static string UTF8toGB2312(const char*utf8);
 	static string GB2312toUTF8(const char*gb2312);

	static string MemoryToString(unsigned char* pData, int nLen);
	static bool StringToMemory(string& strZipText, unsigned char* pOutData/*out*/, int&nLen/*in,out*/);
#ifndef WIN32
	static int code_convert(char *from_charset,char *to_charset,char *inbuf,int inlen,char *outbuf,int outlen);
#endif
};


#endif