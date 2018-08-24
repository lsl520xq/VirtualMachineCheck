#pragma once

#include "../Common/Funcs.h"
#include "../Common/Exception.h"
#include "../Common/Common.h"
#include "../Sqlite3/dlsqlite3.h"
#include "../Mime/UrlConver.h"
//#include "BrowserRecordSave.h"

const vstring Firefox_Record			= "places.sqlite";
const vstring Maxthon_Record			= "\\History\\History.dat";

class CVirtualMachineBrowserRecord
{
public:
	CVirtualMachineBrowserRecord(void);
	~CVirtualMachineBrowserRecord(void);
public:
	//�ȸ������
	bool ChromeRecordFile( const char* virtualPath, const char* recordFilename, const vstring& strNetRecdFile, vstring& strSystemPath, PFCallbackVirtualInternetRecord InternetRecord);

	//ŷ��
	bool OperaRecordFile( const char* virtualPath, const char* recordFilename, const vstring& strNetRecdFile, vstring& strSystemPath, PFCallbackVirtualInternetRecord InternetRecord);

	//�ѹ������
	bool SogouRecordFile( const char* virtualPath, const char* recordFilename, const vstring& strNetRecdFile, vstring& strSystemPath, PFCallbackVirtualInternetRecord InternetRecord);

	//QQ�����
	bool QQRecordFile( const char* virtualPath, const char* recordFilename, const vstring& strNetRecdFile, vstring& strSystemPath, PFCallbackVirtualInternetRecord InternetRecord);

	//360��ȫ�����
	bool QihuRecordFile( const char* virtualPath, const char* recordFilename, const vstring& strNetRecdFile, vstring& strSystemPath, PFCallbackVirtualInternetRecord InternetRecord);

	//360���������
	bool QihuChromeRecordFile( const char* virtualPath, const char* recordFilename, const vstring& strNetRecdFile, vstring& strSystemPath, PFCallbackVirtualInternetRecord InternetRecord);

	//UC�����
	bool UCRecordFile( const char* virtualPath, const char* recordFilename, const vstring& strNetRecdFile, vstring& strSystemPath, PFCallbackVirtualInternetRecord InternetRecord);

	//�Ա������
	bool LiebaoRecordFile(const char* virtualPath, const char* recordFilename, const vstring& strNetRecdFile, vstring& strSystemPath, PFCallbackVirtualInternetRecord InternetRecord);

	//2345���������
	bool Browser2345RecordFile( const char* virtualPath, const char* recordFilename, const vstring& strNetRecdFile, vstring& strSystemPath, PFCallbackVirtualInternetRecord InternetRecord);

	//���������
	//bool MaxthonRecordDirectory(const char* browserType, const vstring& strNetRecdFile, vstring& strSystemPath, PFCallbackInternetRecord InternetRecord);

	bool ParseMaxthonRecordFile( const char* virtualPath, const char* recordFilename, const vstring& strNetRecdFile, vstring& strSystemPath, PFCallbackVirtualInternetRecord InternetRecord);

	//��������
	//bool FirefoxRecordDirectory(const char* localPath, const char* virtualPath, const char* recordFilename, const vstring& strNetRecdDir, vstring& strSystemPath, PFCallbackVirtualInternetRecord InternetRecord);

	bool ParseFirefoxRecordFile(const char* virtualPath, const char* recordFilename, const vstring& strNetRecdFile, vstring& strSystemPath, PFCallbackVirtualInternetRecord InternetRecord);

};

 