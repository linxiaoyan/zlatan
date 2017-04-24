#include "stdafx.h"
#include "func.h"
#include "KBASE.h"

Func::Func(void)
{
}

Func::~Func(void)
{
}

 #ifdef WIN32
 
 #else
 #include <iconv.h>
int Func::code_convert(char *from_charset,char *to_charset,char *inbuf,int inlen,char *outbuf,int outlen)
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
 
std::string Func::UTF8toGB2312(const char *utf8)
{
#ifdef WIN32
	std::string result;
	WCHAR *strSrc;
	char *szRes;

	//获得临时变量的大小
	int i = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
	strSrc = new WCHAR[i+1];
	MultiByteToWideChar(CP_UTF8, 0, utf8, -1, strSrc, i);

	//获得临时变量的大小
	i = WideCharToMultiByte(CP_ACP, 0, strSrc, -1, NULL, 0, NULL, NULL);
	szRes = new char[i+1];
	WideCharToMultiByte(CP_ACP, 0, strSrc, -1, szRes, i, NULL, NULL);

	result = szRes;
	delete []strSrc;
	delete []szRes;

	return result;
 #else
	std::string gb2312 = utf8;
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
 
std::string Func::GB2312toUTF8(const char *gb2312)
{
 #ifdef WIN32
	std::string result;
	WCHAR *strSrc;
	char *szRes;

	//获得临时变量的大小
	int i = MultiByteToWideChar(CP_ACP, 0, gb2312, -1, NULL, 0);
	strSrc = new WCHAR[i+1];
	MultiByteToWideChar(CP_ACP, 0, gb2312, -1, strSrc, i);

	//获得临时变量的大小
	i = WideCharToMultiByte(CP_UTF8, 0, strSrc, -1, NULL, 0, NULL, NULL);
	szRes = new char[i+1];
	int j=WideCharToMultiByte(CP_UTF8, 0, strSrc, -1, szRes, i, NULL, NULL);

	result = szRes;
	delete []strSrc;
	delete []szRes;

	return result;

 #else
	std::string utf = gb2312;
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