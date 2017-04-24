#ifndef DIGCALC_H
#define DIGCALC_H

#include <stdlib.h>

#define HASHLEN 16
typedef char HASH[HASHLEN];

#define HASHHEXLEN 32
typedef char HASHHEX[HASHHEXLEN + 1];

#define IN
#define OUT

/* AKA */
//#define MAX_HEADER_LEN  2049
//#define KLEN 16
//typedef u_char K[KLEN];
//#define RANDLEN 16
//typedef u_char RAND[RANDLEN];
//#define AUTNLEN 16
//typedef u_char AUTN[AUTNLEN];
//
//#define AKLEN 6
//typedef u_char AK[AKLEN];
//#define AMFLEN 2
//typedef u_char AMF[AMFLEN];
//#define MACLEN 8
//typedef u_char MAC[MACLEN];
//#define CKLEN 16
//typedef u_char CK[CKLEN];
//#define IKLEN 16
//typedef u_char IK[IKLEN];
//#define SQNLEN 6
//typedef u_char SQN[SQNLEN];
//#define AUTSLEN 14
//typedef char AUTS[AUTSLEN];
//#define AUTS64LEN 21
//typedef char AUTS64[AUTS64LEN];
//#define RESLEN 8
//typedef unsigned char RES[RESLEN];
//#define RESHEXLEN 17
//typedef char RESHEX[RESHEXLEN];
//typedef char RESHEXAKA2[RESHEXLEN + IKLEN * 2 + CKLEN * 2];

//AMF amf = "\0\0";
//AMF amfstar = "\0\0";

/* end AKA */


/* Private functions */
void CvtHex(IN HASH Bin, OUT HASHHEX Hex);
void DigestCalcHA1(IN const char *pszAlg,
						  IN const char *pszUserName,
						  IN const char *pszRealm,
						  IN const char *pszPassword,
						  IN const char *pszNonce,
						  IN const char *pszCNonce,
						  OUT HASHHEX SessionKey);
void DigestCalcResponse(IN HASHHEX HA1, IN const char *pszNonce,
							   IN const char *pszNonceCount,
							   IN const char *pszCNonce,
							   IN const char *pszQop,
							   IN int Aka,
							   IN const char *pszMethod,
							   IN const char *pszDigestUri,
							   IN HASHHEX HEntity, OUT HASHHEX Response);

#endif
