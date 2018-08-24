#pragma once

#include "Funcs.h"

#define  SYS_MUTEX_INSTANCE_FAILED     (0)  //����������ʧ��
#define  SYS_MUTEX_INSTANCE_SUCCESSED  (1)  //�����������ɹ�
#define  SYS_MUTEX_INSTANCE_EXISTS     (2)  //�������Ѿ�������(����)

class CPriviledge
{
private:
	static bool SetPrivilege(HANDLE hToken, LPCTSTR lpszPrivilege, BOOL bEnablePrivilege);
public:
	CPriviledge(void);
	~CPriviledge(void);
public:
	static bool RaisePriviledage(void);
	static int  CreateGlobalMutex(const char* name, const char* description, HANDLE& outHandle);
};
