#pragma once
#include <Windows.h>
#include <tchar.h>
#include <map>
#include <vector>
#include <iostream>
#include <Shlobj.h>
#include <Shlwapi.h>
#include "../Common/Funcs.h"
#include "../Mime/CharsetConver.h"

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "Advapi32.lib")

typedef std::vector<std::string> ValueVector;
typedef ValueVector::iterator ValueVectorIterator;

typedef std::map<std::string,std::string> ValueMap;
typedef ValueMap::iterator ValueMapIterator;

typedef std::map<vstring, size_t> SysDiskMap;
typedef SysDiskMap::iterator SysDiskMapIterator;


#define  VL_REG_SAM        _T("\\Windows\\System32\\config\\SAM")
#define  VL_REG_SYSTEM     _T("\\Windows\\System32\\config\\SYSTEM")
#define  VL_REG_DEFAULT    _T("\\Windows\\System32\\config\\DEFAULT")
#define  VL_REG_SECURITY   _T("\\Windows\\System32\\config\\SECURITY")
#define  VL_REG_SOFTWARE   _T("\\Windows\\System32\\config\\SOFTWARE")
#define  VL_REG_COMPONENTS _T("\\Windows\\System32\\config\\COMPONENTS") 

//ϵͳ��Ϣ
#define VL_REG_SOFTWARE_VERSION     _T("\\Microsoft\\Windows NT\\CurrentVersion")
//��¼��Ϣ
#define VL_REG_SOFTWARE_WINLOGON    _T("\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon")
///������Ϣ
#define VL_REG_SYSTEM_SHARES1       _T("\\ControlSet001\\Services\\LanmanServer\\Shares")
#define VL_REG_SYSTEM_SHARES2       _T("\\ControlSet002\\Services\\LanmanServer\\Shares")
//������
#define VL_REG_SYSTEM_COMPUTERNAME  _T("\\ControlSet001\\Control\\ComputerName\\ComputerName")
//�û���Ϣ
#define VL_REG_SAM_ACCOUNT          _T("\\SAM\\Domains\\Account\\Users\\Names")



class CSystemRegistry
{
private:
	static bool UnLoadRegistry(HKEY hKey, const char* subKey);
	static bool LoadRegistry(HKEY hKey, const char* subKey, const char* path);

	static bool SearchFileByDesktop(const string& path, const char* shorcut, string& outPath);

	static bool GetShareInfo(const string& subKey, string& outName);
public:
	static bool ISPESystem(void);
	static bool ISSystemDisk(const char* disk);
	static void ISSystemArchitecture(const char* disk, string& architecture);

	static bool GetDiskInfo(ValueVector& vValue);
	static bool GetSystemVersionInfo(const char* disk, string& version);

	static bool LoadAllLocalRegistry(void);
	static bool UnLoadAllLocalRegistry(void);
	static bool GetSystemDiskList(string& disklist);
	
	//��ȡ�������
	static bool GetComputerName(string& outName);
	//�����������:������=����·��
	static bool GetShareInfo(string& outName);
	//�����������:�û���\r\n
	static bool GetAllUserInfo(string& outUserName);
	static bool GetDefaultUserInfo(string& outUserName);
	//��ȡ�����ݷ�ʽ·��,֪����ݷ�ʽ�ľ���·��ʹ��
	static bool GetShortCutsFile(const char* shorcut, string& outPath);
	//��ȡ�����ݷ�ʽ·�� PE��ʹ��
	static bool GetPathForShortcuts(const char* shorcut, string& outPath);
	//����ļ��Ƿ�����ڴ����ֻ�滻�̷����
	static bool CheckFileExistsInOverallDisk(const string& path, string& newPath, const bool isSystemPartion);
public:
	CSystemRegistry(void);
	~CSystemRegistry(void);
};

