#pragma once

#include "../Common/Funcs.h"
#include <iostream>
#include <comutil.h>
#include "../Common/Exception.h"

#pragma comment(lib, "comsuppw.lib")

#define UC_FLAG ("\\u")

class CUrlConver
{
public:
	CUrlConver(void);
	~CUrlConver(void);
public:
	static void      GB2312ToUnicode(wchar_t* pOut,char* gbBuffer);
	static void      UTF8ToUnicode(wchar_t* pOut,char *pText);
	static void      UnicodeToUTF8(char* pOut,wchar_t* pText);
	static void      UnicodeToGB2312(char* pOut,wchar_t uData);

	static void      GB2312ToUTF8(string& pOut, char* pText, uint32 pLen); //gb2312 ת utf8
	static void      UTF8ToGB2312(string& pOut, char* pText, uint32 pLen, char* pBuffer, const uint32 bufLen); //utf8 ת gb2312

	static char      CharToInt(char ch);
	static char      StrToBin(char* str);
	static short     HexCharToShort(char c);
	static string    DecodeUnicodeNumber(const string& strNumber);
public:
	//ʮ�������ַ�ת��Ϊint
	static uint32 HexToInt(const uint8* data ,uint32 len);

	static string UrlGB2312(const char* str);                       //urlgb2312����
	static string UrlGB2312Decoder(const string& str);              //urlgb2312����

	static string UrlUTF8(const char* str);                         //urlutf8 ����
	static string UrlUTF8Decoder(const string& str, char* buf, const uint32 len); //urlutf8����

	static string UrlDecoder(string &str);                          //url����
	static bool   UrlDecoder(const char* src, const uint32 srclen, char* dst, const uint32 dstlen);
	
	static wstring StringToWstring(const string& str);              //ת��Ϊ���ַ�
	static string  WstringToString(const wstring &wstr);            //ת��Ϊխ�ַ�

	static void  DecodeHtmlUnicode( string& content);               //html_unicode,\uxxxx��ʽ

	static bool IsUrlCode(const string& str);
	static bool IsTextUTF8(char* str,size_t length);                
	static bool IsHtmlUnicode(string &strValue);

	//////////////////////////////////////////////////////////////////////////
	//ȫ��ת��Сд
	static string  UpperToLower(const string& src);
	static void    UpperToLower(char* src ,const unsigned int srcLen);

	//ȫ��ת����д
	static string  LowerToUpper(const string& src);
	static void    LowerToUpper(char* src ,const unsigned int srcLen);

	//�ַ�ת������
	static short HexCharToInteger(const char src);
	//URL����
	static bool  URLDecorder(const char* srcData, const unsigned int srcLen,char* desData,const unsigned int desLen);
	//utf-8ת��unicode
	static bool  UTF8ToUnicode(const char* srcData, const unsigned int srcLen,wchar_t* desData,const unsigned int desLen);
	//GB2312����
	static bool  GB2312Decorder(const char* srcData, const unsigned int srcLen,char* desData, unsigned int &desLen);
	//HtmlUnicode����(\uxxxx��ʽ)
	static bool  HtmlUnicodeDecorder(const char* srcData, const unsigned int srcLen, char* desData,const unsigned int desLen);


	//UTF-8����(����,�󲿷�Unicode,����ת��Ϊansi)
	static bool  UTF8Decorder(const char* srcData, const unsigned int srcLen,char* desData, unsigned int &desLen);
	//unicode����
	static bool  UnicodeDecorder(const char* srcData, const unsigned int srcLen,char* desData, unsigned int &desLen,const bool bNetFlag);

	//%uxxxxת��Ϊ\uxxxx
	static bool ReplaseCode(char* srcData, const unsigned int srcLen);
	//����+���ո�
	static bool ReplaseCode2(char* srcData, const unsigned int srcLen);

	//�����������ַ�����,\ / : * ? " < > | 
	static bool ClearSpecialChar( char* srcData, const unsigned int srcLen );

	//������ҳת���ַ�,\/,\"��,����\r,\n,\t����ȫ��ɾ��\���� 
	static bool ClearTransferredChar(char *srcData, const unsigned int srcLen);
};

