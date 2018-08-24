#include "StdAfx.h"
#include "GetHostDiskInfo.h"


GetHostDiskInfo::GetHostDiskInfo(void)
{
	m_ParentDevice=NULL;//��ʼ���������̾��
}


GetHostDiskInfo::~GetHostDiskInfo(void)
{
	if (NULL != m_ParentDevice)
	{
		(void)CloseHandle(m_ParentDevice);//�ͷ��������̾��
		m_ParentDevice = NULL;
	}
	
	


}

bool GetHostDiskInfo::UnicodeToZifu(UCHAR* Source_Unico, string& fileList, DWORD Size)
{
	wchar_t *toasiclls = new wchar_t[Size/2+1];
	if (NULL == toasiclls)
	{
		CFuncs::WriteLogInfo(SLT_ERROR, "UnicodeToZifu::new::����toasiclls�ڴ�ʧ��!");
		return false;
	}
	memset(toasiclls,0,Size+2);
	char *str = (char*)malloc(Size+2);
	if (NULL == str)
	{
		CFuncs::WriteLogInfo(SLT_ERROR, "UnicodeToZifu::malloc::����str�ڴ�ʧ��!");
		delete toasiclls;
		toasiclls = NULL;
		return false;
	}
	memset(str,0,Size+2);
	for (DWORD nl = 0; nl < Size; nl += 2)
	{
		toasiclls[nl/2] = Source_Unico[nl+1]<<8 | Source_Unico[nl];
	}

	int nRet=WideCharToMultiByte(CP_OEMCP, 0, toasiclls, -1, str, (Size+2), NULL, NULL); 
	if(nRet<=0)  
	{  
		CFuncs::WriteLogInfo(SLT_ERROR, "Unicode_To_Zifu::WideCharToMultiByte::ת��ʧ��ʧ��!");
		free(str);
		str = NULL;
		delete toasiclls;
		toasiclls = NULL;
		return false;
	}  
	else  
	{  
		bool strbool = true;
		for (DWORD i = 0;i < Size; i++)
		{
			if (str[i] == 0)
			{
				fileList.append(str, i);
				strbool = false;
				//printf("%s\n",str);
				break;
			}
		}
		if (strbool)
		{
			fileList.append(str, Size);
		}

	}   

	free(str);
	str = NULL;
	delete toasiclls;
	toasiclls = NULL;
	return true;
}

bool  GetHostDiskInfo::ReadOnce(HANDLE hDevice, UCHAR* Buffer, WORD SIZE, DWORD *BackBytesCount)
{
	BOOL bRet = FALSE;
	if (SIZE<=512)
	{
		bRet = ReadFile(hDevice, Buffer, SECTOR_SIZE, BackBytesCount, NULL);
		if(!bRet)
		{		
			CFuncs::WriteLogInfo(SLT_ERROR, "ReadFile::��ȡʧ��!");
			return false;	
		}
	}
	else
	{
		CFuncs::WriteLogInfo(SLT_ERROR, "ReadFile::�����ֽڴ�С,���ֻ����512");


		return false;
	}
	return true;
}
bool  GetHostDiskInfo::ReadSQData(HANDLE hDevice, UCHAR* Buffer, DWORD SIZE, DWORD64 addr, DWORD *BackBytesCount)
{
	LARGE_INTEGER LiAddr = {0};	
	LiAddr.QuadPart=addr;
	DWORD dwError = 0;

	BOOL bRet = SetFilePointerEx(hDevice, LiAddr, NULL,FILE_BEGIN);
	if(!bRet)
	{

		dwError = GetLastError();
		CFuncs::WriteLogInfo(SLT_ERROR, _T("ReadSQData::SetFilePointerExʧ��!,\
										   ���󷵻���: dwError = %d"), dwError);
		return false;	
	}
	bRet = ReadFile(hDevice, Buffer, SIZE, BackBytesCount, NULL);
	if(!bRet)
	{
		dwError = GetLastError();
		CFuncs::WriteLogInfo(SLT_ERROR, _T("ReadSQData::ReadFileʧ��!,\
										   ���󷵻���: dwError = %d"), dwError);					
		return false;	
	}

	return true;
}

bool  GetHostDiskInfo::GetMbrStartAddr(HANDLE hDevice, vector<DWORD64>& start_sq, UCHAR *ReadSectorBuffer, DWORD64 *LiAddr,DWORD *PtitionIdetifi)
{
	LMBR_Heads MBR = NULL;
	DWORD BackBytesCount = NULL;

	if(!ReadSQData(hDevice, ReadSectorBuffer, SECTOR_SIZE, (*LiAddr), &BackBytesCount))
	{
		CFuncs::WriteLogInfo(SLT_ERROR, "GetMbrStartAddr::ReadSQData��ȡʧ��!,��ַ��%llu",(*LiAddr));
		return false;
	}
	for (int i = 0; i < 64; i += 16)
	{
		MBR = (LMBR_Heads)&ReadSectorBuffer[446 + i];				
		if (MBR->_MBR_Partition_Type == 0x05 || MBR->_MBR_Partition_Type == 0x0f)
		{
			if (ReadSectorBuffer[0] == 0 && ReadSectorBuffer[1] == 0 && ReadSectorBuffer[2] == 0 && ReadSectorBuffer[3] == 0)
			{				
				(*LiAddr) = ((*LiAddr) + ((DWORD64)MBR->_MBR_Sec_pre_pa) * SECTOR_SIZE);				
				GetMbrStartAddr(hDevice, start_sq, &ReadSectorBuffer[0], LiAddr,PtitionIdetifi);
			} 
			else
			{							
				(*LiAddr) = ((DWORD64)(MBR->_MBR_Sec_pre_pa) * SECTOR_SIZE);							
				GetMbrStartAddr(hDevice, start_sq, &ReadSectorBuffer[0], LiAddr,PtitionIdetifi);
			}
		} 
		else if (MBR->_MBR_Partition_Type == 0x00)
		{
			return true;
		}
		else if (MBR->_MBR_Partition_Type == 0x07)
		{
			if (ReadSectorBuffer[0] == 0 && ReadSectorBuffer[1] == 0 && ReadSectorBuffer[2] == 0 && ReadSectorBuffer[3] == 0)
			{				
				(*PtitionIdetifi)++;
				start_sq.push_back(*PtitionIdetifi);
				start_sq.push_back((MBR->_MBR_Sec_pre_pa + (*LiAddr) / SECTOR_SIZE));  			
			}
			else
			{
				(*PtitionIdetifi)++;
				start_sq.push_back(*PtitionIdetifi);
				start_sq.push_back(MBR->_MBR_Sec_pre_pa); 		
			}
		}
	}
	return true;
}

bool  GetHostDiskInfo::GetHostStartNTFSAddr(HANDLE hDevice, vector<DWORD64>& start_sq, UCHAR *ReadSectorBuffer)
{
	BOOL  BRet = FALSE;
	bool bRet = false;
	DWORD dwFileSize = NULL;
	DWORD BackBytesCount = NULL;
	DWORD SectorNum = NULL;
	DWORD64 LiAddr = NULL;
	LGPT_Heads GptHead = NULL;
	bool bReads = true;
	LGPT_FB_TABLE GptTable = NULL;

	DWORD partitiontype = NULL;
	DWORD LayoutSize = sizeof(DRIVE_LAYOUT_INFORMATION_EX) + sizeof(DRIVE_LAYOUT_INFORMATION_EX) * 150;
	PDRIVE_LAYOUT_INFORMATION_EX  LpDlie = (PDRIVE_LAYOUT_INFORMATION_EX)malloc(LayoutSize);
	if(NULL != LpDlie)
	{
		BRet = DeviceIoControl(hDevice, IOCTL_DISK_GET_DRIVE_LAYOUT_EX, NULL, 0, LpDlie, LayoutSize, &BackBytesCount, NULL);
		if (BRet)
		{
			CFuncs::WriteLogInfo(SLT_INFORMATION, "��������%lu", LpDlie->PartitionCount);
			partitiontype = LpDlie->PartitionStyle;
			switch(partitiontype)
			{
			case 0:
				CFuncs::WriteLogInfo(SLT_INFORMATION, "����������MBR");

				break;
			case 1:
				CFuncs::WriteLogInfo(SLT_INFORMATION, "����������GPT");

				break;
			case 2:
				CFuncs::WriteLogInfo(SLT_INFORMATION, "Partition not formatted in either of the recognized formats��MBR or GPT");

				return true;
			}
		}
		else 
		{
			CFuncs::WriteLogInfo(SLT_ERROR, "GetHostNtfsStartAddr DeviceIoControl ��ȡ��������ʧ�ܣ� errorId = %d", GetLastError());	
			free(LpDlie);
			LpDlie = NULL;
			return false;
		}
	}
	else
	{
		CFuncs::WriteLogInfo(SLT_ERROR, "GetHostNtfsStartAddr LpDlie calloc�����ȡ������Ϣ�ڴ�ʧ�ܣ� errorId = %d", GetLastError());
		return false;
	}

	free(LpDlie);
	LpDlie = NULL;

	dwFileSize = GetFileSize(hDevice, NULL);
	CFuncs::WriteLogInfo(SLT_INFORMATION, "GetHostNtfsStartAddr GetFileSize �����ܴ�С��%lu", dwFileSize);

	bRet = ReadSQData(hDevice, ReadSectorBuffer, SECTOR_SIZE, 0, &BackBytesCount);	
	if(!bRet)
	{			
		CFuncs::WriteLogInfo(SLT_ERROR, "GetHostNtfsStartAddr ReadSQData::��ȡ��0��������ȡMBR��GPT��Ϣʧ��!");
		return false;	
	}

	while(SectorNum < 50)
	{
		if(ReadSectorBuffer[510] == 0x55 && ReadSectorBuffer[511] == 0xAA)
		{
			if(partitiontype == 0)
			{
				LiAddr = SectorNum * SECTOR_SIZE;
				CFuncs::WriteLogInfo(SLT_INFORMATION, "��ȡ��MBR���̵�MBR����");


			}else if (partitiontype == 1)
			{

				CFuncs::WriteLogInfo(SLT_INFORMATION, "��ȡ��GPT���̵ı���MBR����");
				SectorNum++;
				BRet = ReadFile(hDevice, ReadSectorBuffer, SECTOR_SIZE, &BackBytesCount, NULL);
				if(!BRet)
				{		
					
					CFuncs::WriteLogInfo(SLT_ERROR, "GetHostNtfsStartAddr : ReadFile::��ȡMBR����������һ����ʧ��!");
					return false;	
				}
			}
			break;
		}
		BRet = ReadFile(hDevice, ReadSectorBuffer, SECTOR_SIZE, &BackBytesCount, NULL);
		if(!BRet)
		{		
			
			CFuncs::WriteLogInfo(SLT_ERROR, "GetHostNtfsStartAddr : ReadFile::��ȡMBR����������һ����ʧ��!");
			return false;	
		}

		SectorNum++;
	}
	if(partitiontype == 0)
	{
		DWORD PititionIdetifi = NULL;
		if (!GetMbrStartAddr(hDevice, start_sq, &ReadSectorBuffer[0], &LiAddr, &PititionIdetifi))
		{
			
			CFuncs::WriteLogInfo(SLT_ERROR, "GetHostNtfsStartAddr GetMbrStartAddr Ѱ��MBR��ʼ����ʧ��!!");
			return false;
		}
		else
		{
			CFuncs::WriteLogInfo(SLT_INFORMATION, "�ɹ��ҵ�MBR����,һ��%d������", start_sq.size()/2);
		}

	}
	else if (partitiontype == 1)
	{
		DWORD GptIdentif = NULL;
		GptHead = (LGPT_Heads)&ReadSectorBuffer[0];
		if (GptHead->_Singed_name == 0x5452415020494645)
		{
			CFuncs::WriteLogInfo(SLT_INFORMATION, "����GPTͷ��");

		}
		else
		{
			
			CFuncs::WriteLogInfo(SLT_ERROR, "GetHostNtfsStartAddr �����������GPTͷ��!!");
			return false;
		}
		GptHead = NULL;

		while(bReads)
		{
			BRet = ReadFile(hDevice, ReadSectorBuffer, SECTOR_SIZE, &BackBytesCount, NULL);
			if(!BRet)
			{		
				
				CFuncs::WriteLogInfo(SLT_ERROR, "GetHostNtfsStartAddr :ReadFile::��ȡGPT��Ϣʧ��!");
				return false;	
			}

			SectorNum++;
			GptTable = (LGPT_FB_TABLE)&ReadSectorBuffer[0];
			for (int i = 0; (GptTable->_GUID_TYPE[0] != 0) && (i < 4); i++)
			{
				GptIdentif++;
				if (GptTable->_GUID_TYPE[0] == 0x4433b9e5ebd0a0a2)
				{
					start_sq.push_back(GptIdentif);
					start_sq.push_back(GptTable->_FB_Start_SQ);
				}
				if (i < 3)
				{
					GptTable++;
				}
			}
			if (GptTable->_FB_Start_SQ == 0)
			{
				
				CFuncs::WriteLogInfo(SLT_INFORMATION, "GPT�б����");
				bReads =  false;
			}
		}
		GptTable = NULL;
		CFuncs::WriteLogInfo(SLT_INFORMATION, "һ����ȡ��GPT����%d��",start_sq.size()/2);

	}

	

	return true;
}

bool GetHostDiskInfo::InitHostDisk(void)
{
	DWORD dwError=NULL;
	/************************************************************************/
	/* �����ڴ�                                                                     */
	/************************************************************************/
	UCHAR *PatitionBuffer = NULL;
	PatitionBuffer = (UCHAR*)malloc(FILE_SECTOR_SIZE + SECTOR_SIZE);
	if (NULL == PatitionBuffer)
	{
		dwError=GetLastError();
		CFuncs::WriteLogInfo(SLT_ERROR, _T("InitHostDisk��PatitionBuffer=(UCHAR*)malloc(4096)�����ڴ�ʧ��!,\
										   ���󷵻���: dwError = %d"), dwError);
		return false;
	}
	/*��ʼ���ڴ�*/

	memset(PatitionBuffer, 0, FILE_SECTOR_SIZE + SECTOR_SIZE);
	/************************************************************************/
	/* �õ���������0���                                                                     */
	/************************************************************************/
	m_ParentDevice = CreateFile(_T("\\\\.\\PhysicalDrive0"),//����ע�⣬���ֻ��һ�����̣�������Ҫ���ݸ������!!!!!
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);
	if (m_ParentDevice == INVALID_HANDLE_VALUE) 
	{
		dwError=GetLastError();
		CFuncs::WriteLogInfo(SLT_ERROR, _T("m_ParentDevice = CreateFile��ȡ\\\\.\\PhysicalDrive0���̾��ʧ��!,\
										   ���󷵻���: dwError = %d"), dwError);
		free(PatitionBuffer);
		PatitionBuffer=NULL;
		return false;
	}
	/************************************************************************/
	/* ��ø���MBR��GPT������ʼ��ַ                                                                     */
	/************************************************************************/
	if (!GetHostStartNTFSAddr(m_ParentDevice, m_vHostStarPartition, PatitionBuffer))
	{		
		CFuncs::WriteLogInfo(SLT_ERROR, _T("GET_MAIN_FB_START_SQ::��ȡ������Ϣʧ��!"));
		free(PatitionBuffer);
		PatitionBuffer=NULL;
		return false;
	} 

	free(PatitionBuffer);
	PatitionBuffer=NULL;
	return true;
}
