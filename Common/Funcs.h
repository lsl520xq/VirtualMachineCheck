#pragma once

#include <windows.h>
#include <time.h>
#include <io.h>
#include <tchar.h>
#include <process.h>
#include <iostream>
#include <shellapi.h>
#include <objbase.h>
#include <sys/stat.h>
#include <iostream>
#include <fstream>
//#include <VersionHelpers.h>
#include "Define.h"
#include "Exception.h"

extern vstring gAppPath;                // ����·��
extern vstring gLogPath;                // ��־�ļ���·��
extern vstring gRulePath;				// ����·��
extern vstring gConfigPath;				// �����ļ�·��
extern vstring gProcName;               // ������
extern vstring gDeviceId;               // �豸���
extern vstring gTemporaryPath;          // ��ʱ�ļ���·��
extern vstring gRuleKeywordListXML;     //����-�ؼ����б�

class CFuncs
{
public:
	CFuncs(void);
	~CFuncs(void);
public:

	static bool		Init(void);
	static bool		Init(const char* path);
	static void		Destroy(void);

	static vstring   ParseMac(const uint8* mac);
	static vstring   ParseMD5(const uint8* md5,const uint32 len);

	static vstring	GetTimeName(void);
	static vstring	GetTimeName(const time_t timestamp);
	static time_t	GetTimestamp(void);

	static vstring   GetTimeString(void);
	static vstring   GetTimeString(const time_t timestamp);

	static vstring	GetTimeFileName(void);
	static vstring	GetTimeFileName(const char* ext);
	
	static bool		FileExist(const char* path);
	static bool		FileExist(const vstring& path);

	static bool		FileExists(const char* path);
	static bool		DirectoryExists(const char* path);

	static int		GetSizeFromFile(const char* path);
	static int		GetSizeFromFile(const vstring& path);

	static vstring   ParseFileExt(const char* path);
	static vstring   ParseFileExt(const vstring& path);
	static vstring   ParseFileExtEx(const vstring& path);
	static vstring   ParseFileName(const vstring& path);
	static vstring   ParseFileNameExt(const vstring& path);
	static vstring   ParseFilePath(const vstring& path);

	static vstring   SvcFormatMessage(void);
	static vstring   SvcFormatMessage(const DWORD errorId);
	static vstring	SvcFormatMessage(const DWORD errorId,const char* format,...);
	static vstring	SvcFormatMessage(const DWORD errorId,const vstring& description);

	static bool		QueryRegistryValueInteger(const vstring& subKey,const vstring& name,int* outValue);

	static bool		QueryRegistryValueString(const vstring& subKey,const vstring& name,char* outValue,const int outValueLen);

	static bool		QueryRegistryValueString(const vstring& subKey, DWORD dwOption, const vstring& name,char* outValue,const int outValueLen);

	static bool     SetRegistryValueInteger(const char* subKey,const char* name,const int val);
    static bool     SetRegistryValueString(const char* subKey,const char* name,const char* val);

	static void		WriteLogInfo(const uint8 type,const vstring& content);
	static void		WriteLogInfo(const uint8 type,const char* format,...);

	static bool     CreateBinaryFile(const vstring& path,const char* buffer,const uint32 bufferLen);
	static FILE*    AppendBinaryFile(const string& path,const char* buffer,const size_t bufferLen);
	static bool     AppendBinaryFile(FILE* stream, const char* buffer,const size_t bufferLen);
	static void     AppendBinaryFile(FILE* stream);

	static bool     ParseLogFile(const vstring& path, vstring& fileCreateTime, vstring& fileModifyTime);

	//��������GUID
	static vstring   GetGUID(void);

	//����ִ���ⲿ����
	static bool     ExecuteExternalProgram(const char* path, const char* param, const uint32 timeout);
	//ɾ���ļ��м����ļ�
	static int      DeleteDirectory(const char* path);
	static int      DeleteDirectory(const vstring& path);

	//��ֲ���(��:a=b)
	static bool     SplitParameter(const vstring& param, vstring& key, vstring& value);

	//�ļ�����������LOG,�����LOG�ļ����������ļ�,��ɾ�����ļ���
	static bool     RollbackLog(const vstring& path,const uint16 saveDays);

	static char* SearchSubMemory(const char* data, const size_t data_len, const char* substr, const size_t substr_len, const size_t offset);
	static char* SearchSubMemory(const char* data, const size_t data_len, const char* substr, const size_t substr_len, const size_t offset, size_t& index);

	static vstring UpperToLower(const vstring& src);
	static vstring Trim(const vstring& str);

	//static bool GetSystemVersion(vstring& version);
	//Jan 25 17:07:11 2016 ת��Ϊ 2016-01-25 17:07:11
	static bool WeekOffConversion(const vstring& timeStr, vstring& outTime);

	//ɾ���ո�
	static vstring DeleteSpaceKey(const vstring& value);
	static vstring DeleteSpaceKey(const char* value, const size_t len);

};
