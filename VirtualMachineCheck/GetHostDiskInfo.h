#pragma once

#include <windows.h>
#include<vector>
#include "VirtualFileStruct.h"
#include <iostream>
#include <WinIoCtl.h>
#include <map>

#include "../Common/Funcs.h"


using namespace std;
class GetHostDiskInfo
{
private:
	bool  GetHostStartNTFSAddr(HANDLE hDevice,vector<DWORD64>& start_sq, UCHAR *MbrBuffer);

	bool  GetMbrStartAddr(HANDLE hDevice, vector<DWORD64>& start_sq, UCHAR *ReadSectorBuffer, DWORD64 *LiAddr,DWORD *PtitionIdetifi);
public:
	GetHostDiskInfo(void);
	~GetHostDiskInfo(void);
public:

	HANDLE m_ParentDevice;				//����������
	vector<DWORD64> m_vHostStarPartition;	//����vwctor���洢����������ʼ��ַ

	bool  InitHostDisk(void);
	bool  ReadSQData(HANDLE hDevice, UCHAR* Buffer, DWORD SIZE, DWORD64 addr, DWORD *BackBytesCount);

	bool  ReadOnce(HANDLE hDevice,UCHAR* Buffer,WORD SIZE,DWORD *BackBytesCount);
	bool UnicodeToZifu(UCHAR* Source_Unico, string& fileList, DWORD Size);
};

