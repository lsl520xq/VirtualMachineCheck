#pragma once

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <string>

using namespace std;

typedef char                int8;
typedef short               int16;
typedef int                 int32;
typedef __int64             int64;

typedef unsigned char       uint8;
typedef unsigned short      uint16;
typedef unsigned int        uint32;
typedef unsigned __int64    uint64;

//�����ھ��ļ���󳤶�
#define MAX_MINING_FILE_CONTENT_LEN   (1024 * 1024 * 50)

#ifdef UNICODE
typedef std::wstring  vstring;
#else
typedef std::string   vstring;
#endif

//#ifndef TIXML_USE_STL
//    #define TIXML_USE_STL
//#endif

//��־����
typedef enum _SystemLogType_
{
	SLT_MIN = 0,

	SLT_UNKOWN = SLT_MIN,   //δ֪
	SLT_INFORMATION,        //��Ϣ
	SLT_WARNING,            //����
    SLT_ERROR,              //����

	SLT_MAX
}SystemLogType;

//����ֵ����
typedef enum _ReturnValueType_
{
	RVT_MIN = 0,

	RVT_TRUE = RVT_MIN,   //�ɹ�
	RVT_MAGIC,            //magic����
	RVT_FALSE,            //ʧ��
    RVT_BUFFER,           //json�����Buffer̫С

	RVT_MAX
}ReturnValueType;
//�ؼ���
#define DEFAULT_RULE_KEYWORD_LIST         ("keyword.rule")
