#include "stdafx.h"
#include "digcalc.h"

#include "osipparser2/osip_md5.h"
#include "osipparser2/osip_port.h"
#include <string.h>

void CvtHex(IN HASH Bin, OUT HASHHEX Hex)
{
	unsigned short i;
	unsigned char j;

	for (i = 0; i < HASHLEN; i++) {
		j = (Bin[i] >> 4) & 0xf;
		if (j <= 9)
			Hex[i * 2] = (j + '0');
		else
			Hex[i * 2] = (j + 'a' - 10);
		j = Bin[i] & 0xf;
		if (j <= 9)
			Hex[i * 2 + 1] = (j + '0');
		else
			Hex[i * 2 + 1] = (j + 'a' - 10);
	};
	Hex[HASHHEXLEN] = '\0';
}

/* calculate H(A1) as per spec */
void DigestCalcHA1(IN const char *pszAlg,
			  IN const char *pszUserName,
			  IN const char *pszRealm,
			  IN const char *pszPassword,
			  IN const char *pszNonce,
			  IN const char *pszCNonce, OUT HASHHEX SessionKey)
{
	osip_MD5_CTX Md5Ctx;
	HASH HA1;

	osip_MD5Init(&Md5Ctx);
	osip_MD5Update(&Md5Ctx, (unsigned char *) pszUserName, strlen(pszUserName));
	osip_MD5Update(&Md5Ctx, (unsigned char *) ":", 1);
	osip_MD5Update(&Md5Ctx, (unsigned char *) pszRealm, strlen(pszRealm));
	osip_MD5Update(&Md5Ctx, (unsigned char *) ":", 1);
	osip_MD5Update(&Md5Ctx, (unsigned char *) pszPassword, strlen(pszPassword));
	osip_MD5Final((unsigned char *) HA1, &Md5Ctx);
	if ((pszAlg != NULL) && osip_strcasecmp(pszAlg, "md5-sess") == 0) {
		osip_MD5Init(&Md5Ctx);
		osip_MD5Update(&Md5Ctx, (unsigned char *) HA1, HASHLEN);
		osip_MD5Update(&Md5Ctx, (unsigned char *) ":", 1);
		osip_MD5Update(&Md5Ctx, (unsigned char *) pszNonce, strlen(pszNonce));
		osip_MD5Update(&Md5Ctx, (unsigned char *) ":", 1);
		osip_MD5Update(&Md5Ctx, (unsigned char *) pszCNonce, strlen(pszCNonce));
		osip_MD5Final((unsigned char *) HA1, &Md5Ctx);
	}
	CvtHex(HA1, SessionKey);
}

/* calculate request-digest/response-digest as per HTTP Digest spec */
void DigestCalcResponse(IN HASHHEX HA1,	/* H(A1) */
							   IN const char *pszNonce,	/* nonce from server */
							   IN const char *pszNonceCount,	/* 8 hex digits */
							   IN const char *pszCNonce,	/* client nonce */
							   IN const char *pszQop,	/* qop-value: "", "auth", "auth-int" */
							   IN int Aka,	/* Calculating AKAv1-MD5 response */
							   IN const char *pszMethod,	/* method from the request */
							   IN const char *pszDigestUri,	/* requested URL */
							   IN HASHHEX HEntity,	/* H(entity body) if qop="auth-int" */
							   OUT HASHHEX Response
							   /* request-digest or response-digest */ )
{
	osip_MD5_CTX Md5Ctx;
	HASH HA2;
	HASH RespHash;
	HASHHEX HA2Hex;

	/* calculate H(A2) */
	osip_MD5Init(&Md5Ctx);
	osip_MD5Update(&Md5Ctx, (unsigned char *) pszMethod, strlen(pszMethod));
	osip_MD5Update(&Md5Ctx, (unsigned char *) ":", 1);
	osip_MD5Update(&Md5Ctx, (unsigned char *) pszDigestUri, strlen(pszDigestUri));

	if (pszQop == NULL) {
		goto auth_withoutqop;
	} else if (0 == osip_strcasecmp(pszQop, "auth-int")) {
		goto auth_withauth_int;
	} else if (0 == osip_strcasecmp(pszQop, "auth")) {
		goto auth_withauth;
	}

auth_withoutqop:
	osip_MD5Final((unsigned char *) HA2, &Md5Ctx);
	CvtHex(HA2, HA2Hex);

	/* calculate response */
	osip_MD5Init(&Md5Ctx);
	osip_MD5Update(&Md5Ctx, (unsigned char *) HA1, HASHHEXLEN);
	osip_MD5Update(&Md5Ctx, (unsigned char *) ":", 1);
	osip_MD5Update(&Md5Ctx, (unsigned char *) pszNonce, strlen(pszNonce));
	osip_MD5Update(&Md5Ctx, (unsigned char *) ":", 1);

	goto end;

auth_withauth_int:

	osip_MD5Update(&Md5Ctx, (unsigned char *) ":", 1);
	osip_MD5Update(&Md5Ctx, (unsigned char *) HEntity, HASHHEXLEN);

auth_withauth:
	osip_MD5Final((unsigned char *) HA2, &Md5Ctx);
	CvtHex(HA2, HA2Hex);

	/* calculate response */
	osip_MD5Init(&Md5Ctx);
	osip_MD5Update(&Md5Ctx, (unsigned char *) HA1, HASHHEXLEN);
	osip_MD5Update(&Md5Ctx, (unsigned char *) ":", 1);
	osip_MD5Update(&Md5Ctx, (unsigned char *) pszNonce, strlen(pszNonce));
	osip_MD5Update(&Md5Ctx, (unsigned char *) ":", 1);
	if (Aka == 0) {
		osip_MD5Update(&Md5Ctx, (unsigned char *) pszNonceCount,
			strlen(pszNonceCount));
		osip_MD5Update(&Md5Ctx, (unsigned char *) ":", 1);
		osip_MD5Update(&Md5Ctx, (unsigned char *) pszCNonce, strlen(pszCNonce));
		osip_MD5Update(&Md5Ctx, (unsigned char *) ":", 1);
		osip_MD5Update(&Md5Ctx, (unsigned char *) pszQop, strlen(pszQop));
		osip_MD5Update(&Md5Ctx, (unsigned char *) ":", 1);
	}
end:
	osip_MD5Update(&Md5Ctx, (unsigned char *) HA2Hex, HASHHEXLEN);
	osip_MD5Final((unsigned char *) RespHash, &Md5Ctx);
	CvtHex(RespHash, Response);
}

/*"
ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";*/
int base64_val(char x)
{
	switch (x) {
	case '=':
		return -1;
	case 'A':
		return OSIP_SUCCESS;
	case 'B':
		return 1;
	case 'C':
		return 2;
	case 'D':
		return 3;
	case 'E':
		return 4;
	case 'F':
		return 5;
	case 'G':
		return 6;
	case 'H':
		return 7;
	case 'I':
		return 8;
	case 'J':
		return 9;
	case 'K':
		return 10;
	case 'L':
		return 11;
	case 'M':
		return 12;
	case 'N':
		return 13;
	case 'O':
		return 14;
	case 'P':
		return 15;
	case 'Q':
		return 16;
	case 'R':
		return 17;
	case 'S':
		return 18;
	case 'T':
		return 19;
	case 'U':
		return 20;
	case 'V':
		return 21;
	case 'W':
		return 22;
	case 'X':
		return 23;
	case 'Y':
		return 24;
	case 'Z':
		return 25;
	case 'a':
		return 26;
	case 'b':
		return 27;
	case 'c':
		return 28;
	case 'd':
		return 29;
	case 'e':
		return 30;
	case 'f':
		return 31;
	case 'g':
		return 32;
	case 'h':
		return 33;
	case 'i':
		return 34;
	case 'j':
		return 35;
	case 'k':
		return 36;
	case 'l':
		return 37;
	case 'm':
		return 38;
	case 'n':
		return 39;
	case 'o':
		return 40;
	case 'p':
		return 41;
	case 'q':
		return 42;
	case 'r':
		return 43;
	case 's':
		return 44;
	case 't':
		return 45;
	case 'u':
		return 46;
	case 'v':
		return 47;
	case 'w':
		return 48;
	case 'x':
		return 49;
	case 'y':
		return 50;
	case 'z':
		return 51;
	case '0':
		return 52;
	case '1':
		return 53;
	case '2':
		return 54;
	case '3':
		return 55;
	case '4':
		return 56;
	case '5':
		return 57;
	case '6':
		return 58;
	case '7':
		return 59;
	case '8':
		return 60;
	case '9':
		return 61;
	case '+':
		return 62;
	case '/':
		return 63;
	}
	return OSIP_SUCCESS;
}


char *base64_decode_string(const char *buf, unsigned int len, int *newlen)
{
	unsigned int i, j;
	int x1, x2, x3, x4;
	char *out;
	out = (char *) osip_malloc((len * 3 / 4) + 8);
	if (out == NULL)
		return NULL;
	for (i = 0, j = 0; i + 3 < len; i += 4) {
		x1 = base64_val(buf[i]);
		x2 = base64_val(buf[i + 1]);
		x3 = base64_val(buf[i + 2]);
		x4 = base64_val(buf[i + 3]);
		out[j++] = (x1 << 2) | ((x2 & 0x30) >> 4);
		out[j++] = ((x2 & 0x0F) << 4) | ((x3 & 0x3C) >> 2);
		out[j++] = ((x3 & 0x03) << 6) | (x4 & 0x3F);
	}
	if (i < len) {
		x1 = base64_val(buf[i]);
		if (i + 1 < len)
			x2 = base64_val(buf[i + 1]);
		else
			x2 = -1;
		if (i + 2 < len)
			x3 = base64_val(buf[i + 2]);
		else
			x3 = -1;
		if (i + 3 < len)
			x4 = base64_val(buf[i + 3]);
		else
			x4 = -1;
		if (x2 != -1) {
			out[j++] = (x1 << 2) | ((x2 & 0x30) >> 4);
			if (x3 == -1) {
				out[j++] = ((x2 & 0x0F) << 4) | ((x3 & 0x3C) >> 2);
				if (x4 == -1) {
					out[j++] = ((x3 & 0x03) << 6) | (x4 & 0x3F);
				}
			}
		}

	}

	out[j++] = 0;
	*newlen = j;
	return out;
}

char base64[65] =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static char *base64_encode_string(const char *buf, unsigned int len, int *newlen)
{
	int i, k;
	int triplets, rest;
	char *out, *ptr;

	triplets = len / 3;
	rest = len % 3;
	out = (char *) osip_malloc((triplets * 4) + 8);
	if (out == NULL)
		return NULL;

	ptr = out;
	for (i = 0; i < triplets * 3; i += 3) {
		k = (((unsigned char) buf[i]) & 0xFC) >> 2;
		*ptr = base64[k];
		ptr++;

		k = (((unsigned char) buf[i]) & 0x03) << 4;
		k |= (((unsigned char) buf[i + 1]) & 0xF0) >> 4;
		*ptr = base64[k];
		ptr++;

		k = (((unsigned char) buf[i + 1]) & 0x0F) << 2;
		k |= (((unsigned char) buf[i + 2]) & 0xC0) >> 6;
		*ptr = base64[k];
		ptr++;

		k = (((unsigned char) buf[i + 2]) & 0x3F);
		*ptr = base64[k];
		ptr++;
	}
	i = triplets * 3;
	switch (rest) {
	case 0:
		break;
	case 1:
		k = (((unsigned char) buf[i]) & 0xFC) >> 2;
		*ptr = base64[k];
		ptr++;

		k = (((unsigned char) buf[i]) & 0x03) << 4;
		*ptr = base64[k];
		ptr++;

		*ptr = '=';
		ptr++;

		*ptr = '=';
		ptr++;
		break;
	case 2:
		k = (((unsigned char) buf[i]) & 0xFC) >> 2;
		*ptr = base64[k];
		ptr++;

		k = (((unsigned char) buf[i]) & 0x03) << 4;
		k |= (((unsigned char) buf[i + 1]) & 0xF0) >> 4;
		*ptr = base64[k];
		ptr++;

		k = (((unsigned char) buf[i + 1]) & 0x0F) << 2;
		*ptr = base64[k];
		ptr++;

		*ptr = '=';
		ptr++;
		break;
	}

	*newlen = ptr - out;
	return out;
}



char hexa[17] = "0123456789abcdef";