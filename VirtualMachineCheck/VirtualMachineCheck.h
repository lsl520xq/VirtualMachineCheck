// ���� ifdef ���Ǵ���ʹ�� DLL �������򵥵�
// ��ı�׼�������� DLL �е������ļ��������������϶���� VIRTUALMACHINECHECK_EXPORTS
// ���ű���ġ���ʹ�ô� DLL ��
// �κ�������Ŀ�ϲ�Ӧ����˷��š�������Դ�ļ��а������ļ����κ�������Ŀ���Ὣ
// VIRTUALMACHINECHECK_API ������Ϊ�Ǵ� DLL ����ģ����� DLL ���ô˺궨���
// ������Ϊ�Ǳ������ġ�
#ifdef VIRTUALMACHINECHECK_EXPORTS
#define VIRTUALMACHINECHECK_API EXTERN_C __declspec(dllexport)
#else
#define VIRTUALMACHINECHECK_API EXTERN_C __declspec(dllimport)
#endif

#include "GetHostDiskInfo.h"
#include "GetVirtualMachineInfo.h"
#include "../Common/Funcs.h"


#define VL_MAGIC_NUMBER (0x23E72DAC)

VIRTUALMACHINECHECK_API int VirtualCheck(const int magic, const char* checkExt,const char* virtualFileDir,const char* CheckvirtualFileDir, PFCallbackVirtualMachine VirtualFile);

VIRTUALMACHINECHECK_API int VirtualInternetRecordCheck(const int magic, const char* recordFilePath, const char* virtualFileDir,const char* CheckvirtualFileDir
	, PFCallbackVirtualInternetRecord VirtualRecord);
