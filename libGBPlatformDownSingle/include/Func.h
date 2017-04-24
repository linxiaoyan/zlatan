#ifndef _FUNC_H_
#define _FUNC_H_

#include <iostream>
#include <string>

class Func
{
public:
	Func(void);
	~Func(void);

	static std::string UTF8toGB2312(const char *utf8);
	static std::string GB2312toUTF8(const char *gb2312);

#ifndef WIN32
	static int code_convert(char *from_charset,char *to_charset,char *inbuf,int inlen,char *outbuf,int outlen);
#endif
};

#endif