#include "StdAfx.h"
#include "GetVirtualMachineInfo.h"


GetVirtualMachineInfo::GetVirtualMachineInfo(void)
{
	m_BrowserRecord = NULL;
	m_BrowserRecord = new CVirtualMachineBrowserRecord();
}


GetVirtualMachineInfo::~GetVirtualMachineInfo(void)
{
	if(NULL != m_BrowserRecord)
	{
		delete m_BrowserRecord;
		m_BrowserRecord = NULL;
	}
}
bool  GetVirtualMachineInfo::FileTimeConver(UCHAR* szFileTime, string& strTime)
{
	if(NULL == szFileTime)
	{
		return false;
	}
	LPFILETIME pfileTime = (LPFILETIME)szFileTime;
	SYSTEMTIME systemTime = {0};
	BOOL bTime = FileTimeToSystemTime(pfileTime, &systemTime);
	if (bTime)
	{
		char szTime[32] = { 0 };
		sprintf_s(szTime, _countof(szTime), "%04d-%02d-%02d %02d:%02d:%02d", systemTime.wYear, systemTime.wMonth,
			systemTime.wDay, systemTime.wHour, systemTime.wMinute, systemTime.wSecond);
		strTime.assign(szTime);
		return true;
	}
	return false;
}
bool GetVirtualMachineInfo::GetPititionName(vector<char>&chkdsk, vector<DWORD64>&dwDiskNumber)
{
	char szVol[7] = { '\\', '\\', '.', '\\',0,':',0};
	for (char i = 'a'; i < 'z'; i++)
	{
		szVol[4]=i;
		HANDLE hDrv = CreateFile(
			szVol, 
			GENERIC_READ , 
			FILE_SHARE_READ | FILE_SHARE_WRITE, 
			NULL, 
			OPEN_EXISTING, 
			0, 
			NULL);
		if (hDrv != INVALID_HANDLE_VALUE)
		{
			DWORD dwBytes = 0;
			STORAGE_DEVICE_NUMBER pinfo;
			BOOL bRet = DeviceIoControl(
				hDrv, 
				IOCTL_STORAGE_GET_DEVICE_NUMBER, 
				NULL, 
				0,
				&pinfo, 
				sizeof(pinfo), 
				&dwBytes, 
				NULL);
			if (bRet)
			{
				chkdsk.push_back(i);
				dwDiskNumber.push_back(pinfo.PartitionNumber);

				CloseHandle(hDrv);  
				hDrv=NULL;
			}
			else
			{
				//CloseHandle(hDrv);
				//hDrv=NULL;
				CFuncs::WriteLogInfo(SLT_ERROR, "GetPititionName::DeviceIoControlʧ��, %d ������%c!", GetLastError(), i);
				//return false;
			}
			CloseHandle(hDrv);
			hDrv=NULL;
		}


	}
	return true;
}

bool  GetVirtualMachineInfo::GetMFTAddr(DWORD64 start_sq,vector<LONG64>& HVStarMFTAddr,vector<DWORD64>& HVStarMFTLen,UCHAR *HostPatitionCuNum
	, UCHAR *PatitionBuffer,bool HostOrVirtual)
{
	bool bRet = false;
	DWORD BackBytesCount = NULL;
	LFILE_Head_Recoding File_head_recod = NULL;
	LATTRIBUTE_HEADS ATTriBase = NULL;
	UCHAR *H80_data = NULL;
	DWORD AttributeSize = NULL;
	DWORD FirstAttriSize = NULL;

	if (HostOrVirtual && NULL != HostPatitionCuNum)
	{
		/************************************************************************/
		/* �ҵ���ʼ��MFT�ļ���¼��ַ                                                                     */
		/************************************************************************/

		memset(PatitionBuffer, 0, FILE_SECTOR_SIZE);
		DWORD64 StarMFTAddr = NULL;
		bRet = ReadSQData(m_ParentDevice, PatitionBuffer, SECTOR_SIZE, start_sq * SECTOR_SIZE, &BackBytesCount);		
		if(!bRet)
		{	
			CFuncs::WriteLogInfo(SLT_ERROR, "GetMFTAddr::ReadSQData:��ȡ����NTFS��һ����ʧ��!,��ȡMFT��ʼ��ַʧ��!");
			return false;	
		}

		LNTFS_TABLES first_ntfs_dbr = (LNTFS_TABLES)&PatitionBuffer[0];
		*HostPatitionCuNum = first_ntfs_dbr->_Single_Cu_Num;
		StarMFTAddr = first_ntfs_dbr->_MFT_Start_CU;
		if (NULL == StarMFTAddr)
		{
			CFuncs::WriteLogInfo(SLT_ERROR, "GetMFTAddr::StarMFTAddr:��õ���ʼMFT��ַΪ��!");

			return false;
		}
		/************************************************************************/
		/*     ������ʼ��MFT�ļ���¼��ַ���ҵ���һ��MFT�ļ���¼����ȡH80�����е�h80��ַ�ͳ���       */
		/************************************************************************/

		memset(PatitionBuffer, 0, FILE_SECTOR_SIZE);
		bRet=ReadSQData(m_ParentDevice, PatitionBuffer, FILE_SECTOR_SIZE, start_sq*SECTOR_SIZE + StarMFTAddr * (*HostPatitionCuNum) * SECTOR_SIZE, &BackBytesCount);		
		if(!bRet)
		{	
			CFuncs::WriteLogInfo(SLT_ERROR, "GetMFTAddr::ReadSQData:��ȡ����NTFS��һ����ʧ��!,��ȡMFT��ʼ��ַʧ��!");

			return false;	
		}
	}


	File_head_recod = (LFILE_Head_Recoding)&PatitionBuffer[0];

	if(File_head_recod->_FILE_Index == 0x454c4946)
	{
		RtlCopyMemory(&PatitionBuffer[510], &PatitionBuffer[File_head_recod->_Update_Sequence_Number[0]+2], 2);//������������	
		RtlCopyMemory(&PatitionBuffer[1022],&PatitionBuffer[File_head_recod->_Update_Sequence_Number[0]+4], 2);
		RtlCopyMemory(&FirstAttriSize, &File_head_recod->_First_Attribute_Dev[0], 2);
		if (FirstAttriSize > (FILE_SECTOR_SIZE - sizeof(ATTRIBUTE_HEADS)))
		{
			CFuncs::WriteLogInfo(SLT_ERROR, "GetMFTAddr::FirstAttriSize > FILE_SECTOR_SIZEʧ��!");
			return false;
		}
	
		while((FirstAttriSize + AttributeSize) < FILE_SECTOR_SIZE)
		{	
			ATTriBase = (LATTRIBUTE_HEADS)&PatitionBuffer[FirstAttriSize + AttributeSize];
			if(ATTriBase->_Attr_Type != 0xffffffff)
			{
				if (ATTriBase->_Attr_Type == 0x80)
				{
					
					if (ATTriBase->_PP_Attr == 0x01)
					{
						bool FirstIn = true;
						LONG64 H80_datarun = NULL;
						DWORD H80_datarun_len = NULL;
						H80_data = (UCHAR*)&ATTriBase[0];
						DWORD OFFSET = NULL;
						RtlCopyMemory(&OFFSET, &ATTriBase->TWOATTRIBUTEHEAD.NP_head._NPN_RunList_Offset[0], 2);

						if (OFFSET > (FILE_SECTOR_SIZE - FirstAttriSize - AttributeSize))
						{

							CFuncs::WriteLogInfo(SLT_ERROR, _T("GetMFTAddr::HA0OFFSET����Χ!"));
							return false;
						}
						if (H80_data[OFFSET] != 0 && H80_data[OFFSET] < 0x50)
						{

							while(OFFSET < ATTriBase->_Attr_Length)
							{
								H80_datarun = NULL;
								H80_datarun_len = NULL;
								if (H80_data[OFFSET] > 0 && H80_data[OFFSET] < 0x50)
								{
									UCHAR adres_fig = H80_data[OFFSET] >> 4;
									UCHAR len_fig = H80_data[OFFSET] & 0xf;
									for(int w = len_fig; w > 0; w--)
									{
										H80_datarun_len = H80_datarun_len | (H80_data[OFFSET+w] << (8*(w-1)));
									}
									if (H80_datarun_len > 0)
									{
										HVStarMFTLen.push_back(H80_datarun_len);
									}
									else
									{
										CFuncs::WriteLogInfo(SLT_ERROR, "GetMFTAddr:H80_datarun_len:��H80��ַ�������ݶ�Ϊ0!");
										return false;
									}

									for (int w = adres_fig; w > 0; w--)
									{
										H80_datarun = H80_datarun | (H80_data[OFFSET+w+len_fig] << (8*(w-1)));
									}
									if (H80_data[OFFSET + adres_fig + len_fig] > 127)
									{
										if (adres_fig == 3)
										{
											H80_datarun = ~(H80_datarun ^ 0xffffff);
										}
										if (adres_fig == 2)
										{												
											H80_datarun = ~(H80_datarun ^ 0xffff);

										}

									} 
									if (FirstIn)
									{
										if (H80_datarun > 0)
										{
											HVStarMFTAddr.push_back(H80_datarun);
										} 
										else
										{
											CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVHDVirtualFileAddr::H80_datarunΪ0��Ϊ��������!"));
											return false;
										}
										FirstIn = false;
									}
									else
									{
										if (HVStarMFTAddr.size() > 0)
										{
											H80_datarun = HVStarMFTAddr[HVStarMFTAddr.size() - 1] + H80_datarun;
											HVStarMFTAddr.push_back(H80_datarun);
										}
									}
									

									OFFSET = OFFSET + adres_fig + len_fig + 1;

								}
								else
								{

									break;
								}

							}


						}

					}
					else
					{
						CFuncs::WriteLogInfo(SLT_ERROR, "GetMFTAddr::��H80û���κε�ַ��ֻ������!");

						return false;
					}

				}


				if (ATTriBase->_Attr_Length < (FILE_SECTOR_SIZE - AttributeSize - FirstAttriSize) && ATTriBase->_Attr_Length > 0)
				{
					
						AttributeSize += ATTriBase->_Attr_Length;
						
				} 
				else
				{

					CFuncs::WriteLogInfo(SLT_ERROR, _T("GetMFTAddr::������:%lu,ATTriBase[i_attr]->_Attr_Length���ȹ����Ϊ0!")
						,ATTriBase->_Attr_Length);

					return false;
				}
			}
			else if (ATTriBase->_Attr_Type == 0xffffffff)
			{
				break;
			}
			else if(ATTriBase->_Attr_Type > 0xff && ATTriBase->_Attr_Type < 0xffffffff)
			{
				CFuncs::WriteLogInfo(SLT_ERROR, _T("GetMFTAddr:��ȡATTriBase[i_attr]->_Attr_Type����0xffffffff����"));

				return false;
			}

		}
	}else
	{
		CFuncs::WriteLogInfo(SLT_ERROR, "GetMFTAddr::File_head_recod->_FILE_Index:��MFT�ļ���¼ͷ����־����ʧ��!");

		return false;
	}
	if (HVStarMFTAddr.size() != HVStarMFTLen.size())
	{
		CFuncs::WriteLogInfo(SLT_ERROR, "GetMFTAddr::HVStarMFTAddr.size() != HVStarMFTLen.size()ʧ��!");
		return false;
	}

	return true;
}
bool GetVirtualMachineInfo::GetHostFileRecordAndDataRun(UCHAR *CacheBuff,DWORD64 hostpatition,LONG64 StartMftAddr,UCHAR HostCuNum,
	DWORD ReferNum, vector<string> lookforFileName,vector<string> &VmdkPathMftRefer, vector<string> &VhdPathMftRefer
	, vector<string> &VBoxPathMftRefer)
{
	bool bRet = false;

	DWORD BackBytesCount = 0;
	LFILE_Head_Recoding File_head_recod = NULL;
	LATTRIBUTE_HEADS ATTriBase = NULL;
	bool Found=false;	
	LAttr_30H H30 = NULL;
	UCHAR *H30_NAMES = NULL;
	UCHAR *H80_data = NULL;
	DWORD AttributeSize = NULL;
	DWORD FirstAttriSize = NULL;

	memset(CacheBuff,0,FILE_SECTOR_SIZE);
	bRet=ReadSQData(m_ParentDevice, CacheBuff, FILE_SECTOR_SIZE, (hostpatition * SECTOR_SIZE + StartMftAddr * HostCuNum * SECTOR_SIZE + FILE_SECTOR_SIZE * ReferNum),
		&BackBytesCount);		
	if(!bRet)
	{			
		CFuncs::WriteLogInfo(SLT_ERROR, _T("GetHostFileRecordAndDataRun::ReadSQData��ȡ����Start_CUʧ��!"));
		return false;	
	}	
	File_head_recod = (LFILE_Head_Recoding)&CacheBuff[0];

	if(File_head_recod->_FILE_Index == 0x454c4946 && File_head_recod->_Flags[0] != 0)
	{
		RtlCopyMemory(&CacheBuff[510], &CacheBuff[File_head_recod->_Update_Sequence_Number[0] + 2], 2);//������������	
		RtlCopyMemory(&CacheBuff[1022],&CacheBuff[File_head_recod->_Update_Sequence_Number[0] + 4], 2);
		RtlCopyMemory(&FirstAttriSize, &File_head_recod->_First_Attribute_Dev[0], 2);
		if (FirstAttriSize > (FILE_SECTOR_SIZE - sizeof(ATTRIBUTE_HEADS)))
		{
			CFuncs::WriteLogInfo(SLT_ERROR, "GetHostFileRecordAndDataRun::File_head_recod->_First_Attribute_Dev[0] > FILE_SECTOR_SIZEʧ��!");
			return false;
		}
		
		while ((FirstAttriSize + AttributeSize) < FILE_SECTOR_SIZE)
		{
			ATTriBase = (LATTRIBUTE_HEADS)&CacheBuff[FirstAttriSize + AttributeSize];
			if(ATTriBase->_Attr_Type != 0xffffffff)
			{

				if (!Found)
				{
					if (ATTriBase->_Attr_Type == 0x30)
					{
						DWORD H30Size = NULL;
						switch(ATTriBase->_PP_Attr)
						{
						case 0:
							if (ATTriBase->_AttrName_Length == 0)
							{
								H30 = (LAttr_30H)((UCHAR*)&ATTriBase[0] + 24);
								H30_NAMES = (UCHAR*)&ATTriBase[0] + 24 + 66;
								H30Size = 24 + 66 + AttributeSize + FirstAttriSize;
							} 
							else
							{
								H30 = (LAttr_30H)((UCHAR*)&ATTriBase[0] + 24 + 2 * ATTriBase->_AttrName_Length);
								H30_NAMES = (UCHAR*)&ATTriBase[0] + 24 + 2 * ATTriBase->_AttrName_Length + 66;
								H30Size = 24 + 2 * ATTriBase->_AttrName_Length + 66 + AttributeSize + FirstAttriSize;
							}
							break;
						case 0x01:
							if (ATTriBase->_AttrName_Length == 0)
							{
								H30 = (LAttr_30H)((UCHAR*)&ATTriBase[0] + 64);
								H30_NAMES = (UCHAR*)&ATTriBase[0] + 64 + 66;
								H30Size = 64 + 66 + AttributeSize + FirstAttriSize;
							} 
							else
							{
								H30 = (LAttr_30H)((UCHAR*)&ATTriBase[0] + 64 + 2 * ATTriBase->_AttrName_Length);
								H30_NAMES = (UCHAR*)&ATTriBase[0] + 64 + 2 * ATTriBase->_AttrName_Length + 66;
								H30Size = 64 + 2 * ATTriBase->_AttrName_Length + 66 + AttributeSize + FirstAttriSize;
							}
							break;
						}
						if ((FILE_SECTOR_SIZE - H30Size) < 0)
						{
							CFuncs::WriteLogInfo(SLT_ERROR, _T("GetHostFileRecordAndDataRun::(FILE_SECTOR_SIZE - H30Size)ʧ��!"));
							return false;
						}
						DWORD H30FileNameLen = H30->_H30_FILE_Name_Length * 2;
						if ( H30FileNameLen > (FILE_SECTOR_SIZE - H30Size))
						{
							CFuncs::WriteLogInfo(SLT_ERROR, _T("GetHostFileRecordAndDataRun::������Χʧ��!"));
							return false;
						}
						if (H30FileNameLen > 0)
						{
							string filename;
							if(!UnicodeToZifu(&H30_NAMES[0], filename, H30FileNameLen))
							{

								CFuncs::WriteLogInfo(SLT_ERROR, "GetHostFileRecordAndDataRun����Unicode_To_Zifu::ת��ʧ��!");
								return false;
							}
							vector<string>::iterator viter;
							for (viter = lookforFileName.begin(); viter != lookforFileName.end(); viter ++)
							{
								if (filename.rfind(*viter) != string::npos)
								{
									size_t posion = filename.rfind(*viter);
									size_t c_posion = NULL;
									c_posion = filename.length() - posion;
									if (viter->length() == c_posion)
									{
										Found = true;
										break;
									}
								}
							}
							if (Found)
							{
								string pathMft_tem;
								pathMft_tem.append((char*)&H30->_H30_Parent_FILE_Reference[0], 4);
								pathMft_tem.append(filename);
								if (pathMft_tem.find(".vmx") != string::npos)												
								{
									VmdkPathMftRefer.push_back(pathMft_tem);
								}
								else if(pathMft_tem.find(".vmsd") != string::npos)
								{
									VmdkPathMftRefer.push_back(pathMft_tem);
								}
								else if (pathMft_tem.find(".vhd") != string::npos)
								{
									VhdPathMftRefer.push_back(pathMft_tem);
								}
								else if (pathMft_tem.find(".vbox") != string::npos)
								{
									VBoxPathMftRefer.push_back(pathMft_tem);
								}																		

							}	

									
							
							}												
						}
					}
				


				if (ATTriBase->_Attr_Length < (FILE_SECTOR_SIZE - AttributeSize - FirstAttriSize) && ATTriBase->_Attr_Length > 0)
				{
					
					AttributeSize += ATTriBase->_Attr_Length;
								
				}  
				else
				{
					CFuncs::WriteLogInfo(SLT_ERROR, _T("GetHostFileRecordAndDataRun::������:%lu,ATTriBase[i_attr]->_Attr_Length���ȹ����Ϊ0!")
						,ATTriBase->_Attr_Length);
					return false;
				}

			}
			else if (ATTriBase->_Attr_Type == 0xffffffff)
			{
				break;
			}
			else if(ATTriBase->_Attr_Type>0xff && ATTriBase->_Attr_Type<0xffffffff)
			{
				CFuncs::WriteLogInfo(SLT_ERROR, _T("GetHostFileRecordAndDataRun:��ȡATTriBase[i_attr]->_Attr_Type����0xffffffff����"));
				return false;
			}
		}
	}


	return true;
}
bool GetVirtualMachineInfo::GetHostFileNameAndPath(DWORD64 HostStartNTFSaddr,vector<LONG64> StartMFTaddr,vector<DWORD64> StartMFTaddrLen,UCHAR HostCuNumber
	,string ParentMFT, string &NamePathBuffer, char chkdk)
{
	bool  bRet = false;
	DWORD BackBytesCount=NULL;
	LFILE_Head_Recoding File_head_recod = NULL;
	LATTRIBUTE_HEADS ATTriBase = NULL;
	LAttr_30H H30 = NULL;
	UCHAR *H30_NAMES = NULL;
	DWORD MFTnumber = NULL;
	DWORD lastnumber = NULL;
	string strTempPath;

	

	UCHAR *CacheBuffer = (UCHAR*) malloc(FILE_SECTOR_SIZE + SECTOR_SIZE);
	if (NULL == CacheBuffer)
	{

		CFuncs::WriteLogInfo(SLT_ERROR, _T("GetHostFileNameAndPath:CacheBuffer malloc�����ڴ�ʧ��!"));
		return false;
	}
	memset(CacheBuffer, 0, FILE_SECTOR_SIZE + SECTOR_SIZE);

	strTempPath.clear();
	MFTnumber=NULL;
	RtlCopyMemory(&MFTnumber,(UCHAR*)&ParentMFT[0],4);

	strTempPath.append("//");
	strTempPath.append((char*)&ParentMFT[4],(ParentMFT.length() - 4));


	DWORD numbers=NULL;
	while (MFTnumber !=5 && MFTnumber != 0)
	{
		DWORD AttributeSize = NULL;
		DWORD FirstAttriSize = NULL;
		if (numbers > 100)
		{
			break;
		}
		numbers++;

		DWORD64 MftLenAdd=NULL;
		LONG64 MftAddr=NULL;

		for (DWORD FMft = 0; FMft < StartMFTaddrLen.size(); FMft++)
		{
			if ((MFTnumber*2) <= (StartMFTaddrLen[FMft] * HostCuNumber + MftLenAdd))
			{
				MftAddr = (StartMFTaddr[FMft] * HostCuNumber + ((MFTnumber * 2) - MftLenAdd));
				break;
			} 
			else
			{
				MftLenAdd += (StartMFTaddrLen[FMft] * HostCuNumber);
			}
		}

		//Ѱ�ҵ���������·��
		//�ж�MFT���ĸ���ַ����

		//�ȶ�ȡ�����ļ���¼,����
		memset(CacheBuffer,0,FILE_SECTOR_SIZE);
		bRet = ReadSQData(m_ParentDevice,&CacheBuffer[0],FILE_SECTOR_SIZE,
			HostStartNTFSaddr * SECTOR_SIZE + MftAddr*SECTOR_SIZE,
			&BackBytesCount);		
		if(!bRet)
		{		

			free(CacheBuffer);
			CacheBuffer=NULL;
			CFuncs::WriteLogInfo(SLT_ERROR, _T("GetHostFileNameAndPath:ReadSQData::��ȡ����MFT�ο���ʧ��!"));
			return false;	
		}	
		File_head_recod=(LFILE_Head_Recoding)&CacheBuffer[0];

		if(File_head_recod->_FILE_Index == 0x454c4946)
		{
			RtlCopyMemory(&CacheBuffer[510], &CacheBuffer[File_head_recod->_Update_Sequence_Number[0]+2], 2);//������������	
			RtlCopyMemory(&CacheBuffer[1022], &CacheBuffer[File_head_recod->_Update_Sequence_Number[0]+4], 2);
			RtlCopyMemory(&FirstAttriSize, &File_head_recod->_First_Attribute_Dev[0], 2);
			if (FirstAttriSize > (FILE_SECTOR_SIZE - sizeof(ATTRIBUTE_HEADS)))
			{
				free(CacheBuffer);
				CacheBuffer = NULL;
				CFuncs::WriteLogInfo(SLT_ERROR, "GetHostFileNameAndPath::File_head_recod->_First_Attribute_Dev[0] > FILE_SECTOR_SIZEʧ��!");
				return false;
			}
			string H30temName;
			while ((FirstAttriSize + AttributeSize) < FILE_SECTOR_SIZE)
			{
				ATTriBase = (LATTRIBUTE_HEADS)&CacheBuffer[FirstAttriSize + AttributeSize];
				if(ATTriBase->_Attr_Type != 0xffffffff)
				{
					if (ATTriBase->_Attr_Type == 0x30)
					{
						DWORD H30Size = NULL;
						switch(ATTriBase->_PP_Attr)
						{
						case 0:
							if (ATTriBase->_AttrName_Length == 0)
							{
								H30 = (LAttr_30H)((UCHAR*)&ATTriBase[0]+24);
								H30_NAMES = (UCHAR*)&ATTriBase[0]+24+66;
								H30Size = 24 + 66 + AttributeSize + FirstAttriSize;
							} 
							else
							{
								H30 = (LAttr_30H)((UCHAR*)&ATTriBase[0]+ 24 + 2 * ATTriBase->_AttrName_Length);
								H30_NAMES = (UCHAR*)&ATTriBase[0] +24 + 2 * ATTriBase->_AttrName_Length+66;
								H30Size = 24 + 2 * ATTriBase->_AttrName_Length + 66 + AttributeSize + FirstAttriSize;
							}
							break;
						case 0x01:
							if (ATTriBase->_AttrName_Length == 0)
							{
								H30 = (LAttr_30H)((UCHAR*)&ATTriBase[0] + 64);
								H30_NAMES = (UCHAR*)&ATTriBase[0] + 64 + 66;
								H30Size = 64 + 66 + AttributeSize + FirstAttriSize;
							} 
							else
							{
								H30 = (LAttr_30H)((UCHAR*)&ATTriBase[0] + 64 + 2 * ATTriBase->_AttrName_Length);
								H30_NAMES = (UCHAR*)&ATTriBase[0] + 64 + 2 * ATTriBase->_AttrName_Length + 66;
								H30Size = 64 + 2 * ATTriBase->_AttrName_Length + 66 + AttributeSize + FirstAttriSize;
							}
							break;
						}
						if ((FILE_SECTOR_SIZE - H30Size) < 0)
						{
							CFuncs::WriteLogInfo(SLT_ERROR, _T("GetHostFileNameAndPath::(FILE_SECTOR_SIZE - H30Size)ʧ��!"));
							return false;
						}
						DWORD H30FileNameLen = H30->_H30_FILE_Name_Length * 2;
						if (H30FileNameLen > (FILE_SECTOR_SIZE - H30Size))
						{
							free(CacheBuffer);
							CacheBuffer=NULL;
							CFuncs::WriteLogInfo(SLT_ERROR, _T("GetHostFileNameAndPath::������Χʧ��!"));
							return false;
						}

						H30temName.clear();
						MFTnumber = NULL;
						RtlCopyMemory(&MFTnumber,&H30->_H30_Parent_FILE_Reference,4);
						H30temName.append("//");
						if (!UnicodeToZifu(&H30_NAMES[0], H30temName, H30FileNameLen))
						{

							free(CacheBuffer);
							CacheBuffer=NULL;
							CFuncs::WriteLogInfo(SLT_ERROR, _T("GetHostFileNameAndPath:Unicode_To_Zifu:ת��ʧ��!"));
							return false;
						}								
																			
					}
					if (ATTriBase->_Attr_Length < (FILE_SECTOR_SIZE - AttributeSize - FirstAttriSize) && ATTriBase->_Attr_Length > 0)
					{
						
							AttributeSize += ATTriBase->_Attr_Length; 
							
					
					} 
					else
					{		

						free(CacheBuffer);
						CacheBuffer=NULL;
						CFuncs::WriteLogInfo(SLT_ERROR, _T("GetHostFileNameAndPath:���Գ��ȹ���!,������:%lu"),ATTriBase->_Attr_Length);
						return false;
					}
				}
				else if (ATTriBase->_Attr_Type==0xffffffff)
				{
					break;
				}
				else if(ATTriBase->_Attr_Type > 0xff && ATTriBase->_Attr_Type < 0xffffffff)
				{

					free(CacheBuffer);
					CacheBuffer=NULL;
					CFuncs::WriteLogInfo(SLT_ERROR, _T("GetHostFileNameAndPath:��ȡATTriBase[i_attr]->_Attr_Type����0xffffffff����"));
					return false;
				}
			}
			strTempPath.append(H30temName);
		}
	}
	strTempPath.append("//");
	strTempPath.append(&chkdk,1);//���̷�
	int laststring = (strTempPath.length() - 1);
	for (int i = (strTempPath.length()-1); i > 0; i--)
	{
		if (strTempPath[i] == '/' && strTempPath[i-1] == '/')
		{
			if ((laststring-i) > 0)
			{
				NamePathBuffer.append(&strTempPath[i+1],(laststring-i));

				if (laststring == (strTempPath.length()-1))
				{
					NamePathBuffer.append(":\\");
				} 
				else if(i > 1)
				{
					NamePathBuffer.append("\\");
				}
				laststring = (i-2);
			}

		}
	}

	free(CacheBuffer);
	CacheBuffer=NULL;

	return true;
}
bool GetVirtualMachineInfo::GetVirtualNumber(vector<string> ParentMftBuff, vector<DWORD>&VirtualNumber)
{
	size_t posion = NULL;
	size_t last_posion = NULL;
	vector<string> Mft_Tem;

	Mft_Tem = ParentMftBuff;
	vector<string>::iterator iter;
	int i = NULL;
	while ((Mft_Tem.size()/2) != 0)
	{
		i++;
		if (i > 1000)
		{
			break;
		}
		string FileName;
		FileName = Mft_Tem[0];
		Mft_Tem.erase(Mft_Tem.begin());

		posion = FileName.find(".vm");
		bool Found = false;
		for (iter = Mft_Tem.begin(); iter != Mft_Tem.end(); iter ++)
		{
			last_posion = (*iter).find(".vm");
			if (posion == last_posion)
			{
				if (FileName[0] == (*iter)[0] && FileName[1] == (*iter)[1] && FileName[2] == (*iter)[2] && FileName[3] == (*iter)[3])
				{
					for (DWORD num = 0; num < ParentMftBuff.size(); num ++)
					{
						if (FileName == ParentMftBuff[num])
						{
							VirtualNumber.push_back(num);
							break;
						}
					}
					for (DWORD num = 0; num < ParentMftBuff.size(); num ++)
					{
						if ((*iter) == ParentMftBuff[num])
						{
							VirtualNumber.push_back(num);
							Mft_Tem.erase(iter);
							Found = true;
							break;

						}
					}
				}
				if (Found)
				{
					break;
				}
			}
		}
	}

	if (NULL == VirtualNumber.size())
	{
		CFuncs::WriteLogInfo(SLT_ERROR, "GetVirtualNumber::VirtualNumber����Ϊ��!");
		return false;
	}
	return true;
}
bool  GetVirtualMachineInfo::GetConfigInformation(map<DWORD, string> &DestBuff, UCHAR *SoursBuff
	, DWORD FileSize, DWORD *NameNum, string fPath)
{
	string VName;

	for (DWORD i = 0; i < FileSize; i++)
	{		
		if (SoursBuff[i]==0x66 && SoursBuff[i+1]==0x69 && SoursBuff[i+2]==0x6c && SoursBuff[i+3]==0x65
			&&SoursBuff[i+4]==0x4e && SoursBuff[i+5]==0x61 && SoursBuff[i+6]==0x6d&&SoursBuff[i+7]==0x65)//filename
		{
			for (DWORD y=(i+11);y<(i+100);y++)
			{

				if (SoursBuff[y-5]==0x2e&&SoursBuff[y-4]==0x76&&SoursBuff[y-3]==0x6d
					&&SoursBuff[y-2]==0x64&&SoursBuff[y-1]==0x6b)//.vmdk
				{
					if (SoursBuff[y] == 0x22)
					{
						VName.clear();
						VName.append(fPath);
						VName.append((char*)&SoursBuff[i+12],(y-i-12));	
						
						DestBuff[(*NameNum)] = VName;
						(*NameNum) ++;

						break;
					}

				}
			}
		}
	}
	return true;
}
bool  GetVirtualMachineInfo::GetVmdkInformation(vector<string> &DestData,  UCHAR *SoursBuff, DWORD DataTotalNum
	, DWORD FileNumber, string fPath, map<DWORD, string> VirtualConfigFileInfo)
{
	bool bParent = false;
	string FileName;
	for (DWORD i = 0; i< DataTotalNum; i++)
	{

		/*�ж��ǲ���ParentFileNameHint,����ǵĻ����������������̣���¼�����ĸ����̺���������*/
		if (SoursBuff[i] == 0x70 && SoursBuff[i+1] == 0x61 && SoursBuff[i+2] == 0x72 && SoursBuff[i+3] == 0x65 && SoursBuff[i+4] == 0x6e
			&& SoursBuff[i+5] == 0x74 && SoursBuff[i+6] == 0x46 && SoursBuff[i+7] == 0x69 && SoursBuff[i+8] == 0x6c && SoursBuff[i+9] == 0x65
			&& SoursBuff[i+10] == 0x4e && SoursBuff[i+11] == 0x61 && SoursBuff[i+12] ==0x6d && SoursBuff[i+13] == 0x65 && SoursBuff[i+14] == 0x48
			&& SoursBuff[i+15] == 0x69 && SoursBuff[i+16] == 0x6e && SoursBuff[i+17] == 0x74)
		{

			for (DWORD y = (i+18); y < (i+100); y++)
			{
				if (SoursBuff[y-5] == 0x2e && SoursBuff[y-4] == 0x76 && SoursBuff[y-3] == 0x6d
					&& SoursBuff[y-2] == 0x64 && SoursBuff[y-1] == 0x6b)
				{
					string ParentNum;
					FileName.clear();
					//FileName.append("DifferDisk:");
					FileName.append((char*)&SoursBuff[i+20],(y-i-20));
					//FileName.append("!");
					map<DWORD, string> ::iterator miter;
					for (miter = VirtualConfigFileInfo.begin(); miter != VirtualConfigFileInfo.end(); miter ++)
					{
						string Name_Tem;
						size_t posion = NULL;
						posion = miter->second.rfind("\\");
						if (posion != string::npos)
						{
							Name_Tem.append(&miter->second[posion + 1], (miter->second.length() - posion - 1));
						}
						if (FileName == Name_Tem)
						{
							ParentNum.append((char*)&miter->first, 1);
							break;
						}
						
					}
					DestData.push_back(ParentNum);
					
					

					break;
				}

			}
			bParent = true;
		}
		if (bParent)
		{
			if (SoursBuff[i] == 0x53 && SoursBuff[i+1] == 0x50 && SoursBuff[i+2] == 0x41 && SoursBuff[i+3] == 0x52 && SoursBuff[i+4] == 0x53
				&& SoursBuff[i+5] == 0x45)
			{
				for (DWORD y=(i+7);y<(i+100);y++)
				{
					if (SoursBuff[y-5] == 0x2e && SoursBuff[y-4] == 0x76 && SoursBuff[y-3] == 0x6d
						&& SoursBuff[y-2] == 0x64 && SoursBuff[y-1] == 0x6b)
					{
						FileName.clear();
						FileName.append(fPath);
						FileName.append((char*)&SoursBuff[i+8],(y-i-8));
						DestData.push_back(FileName);
					

						break;
					}
				}
			}
		}else
		{
			if (SoursBuff[i] == 0x53 && SoursBuff[i+1] == 0x50 && SoursBuff[i+2] == 0x41 && SoursBuff[i+3] == 0x52 && SoursBuff[i+4] == 0x53
				&& SoursBuff[i+5] == 0x45)
			{
				for (DWORD y = (i+7); y < (i+100); y++)
				{
					if (SoursBuff[y-5] == 0x2e && SoursBuff[y-4] == 0x76 && SoursBuff[y-3] == 0x6d
						&&SoursBuff[y-2] == 0x64 && SoursBuff[y-1] == 0x6b)
					{
						FileName.clear();
						FileName.append(fPath);
						FileName.append((char*)&SoursBuff[i+8], (y-i-8));
						DestData.push_back(FileName);
						
						break;
					}
				}
			}
		}
	}

	

	return true;
}
bool  GetVirtualMachineInfo::GetVirtualFileName(map<DWORD, vector<string>> &VirtualFileInfo, vector<string> VMwareMftFileName)
{
	
	bool bRet = false;
	DWORD BackBytesCount=0;
	DWORD dwError=NULL;

	DWORD filenumber = NULL;
	DWORD NameNumber = 1;
	string FilePatn;
	size_t Vposion = NULL;
	map<DWORD, string> VirtualConfigFileInfo;
	
	
	Vposion = VMwareMftFileName[0].rfind("\\");
	if (Vposion == string::npos)
	{
		CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualFileName:·������ʧ��!"));
		return false;
	}
	else
	{
		FilePatn.append(&VMwareMftFileName[0][0], (Vposion + 1));
	}

	for (DWORD VMnum = 0; VMnum < 2; VMnum++)
	{
		
		HANDLE VmDevice = CreateFile(VMwareMftFileName[VMnum].c_str(),
			GENERIC_READ,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			0,
			NULL);
		if (VmDevice == INVALID_HANDLE_VALUE) 
		{
			dwError=GetLastError();
			CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualFileName VmDevice = CreateFile��ȡVMware�����ļ����ʧ��!,\
											   ���󷵻���: dwError = %d"), dwError);
			return false;
		}
		DWORD FileSiz = GetFileSize(VmDevice, NULL);
		if (FileSiz > 0)
		{
			UCHAR *ReadInfoBuff = (UCHAR*) malloc(FileSiz + SECTOR_SIZE);
			if (NULL == ReadInfoBuff)
			{
				CloseHandle(VmDevice);
				VmDevice = NULL;
				CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualFileName:malloc:ReadInfoBuff �����ڴ�ʧ��!"));
				return false;	
			}
			memset(ReadInfoBuff, 0, (FileSiz + SECTOR_SIZE));


			bRet=ReadSQData(VmDevice, ReadInfoBuff, FileSiz,
				0,
				&BackBytesCount);		
			if(!bRet)
			{	
				CloseHandle(VmDevice);
				VmDevice = NULL;
				free(ReadInfoBuff);
				ReadInfoBuff = NULL;
				CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualFileName:ReadSQData:����vmx����ʧ��!"));
				return false;	
			}

			/*��������vmx��vmsd�ļ����������̺ͻ�����������������֣���һ�ּ���Ѱ��*/
			if (!GetConfigInformation(VirtualConfigFileInfo, ReadInfoBuff, FileSiz, &NameNumber, FilePatn))
			{
				CloseHandle(VmDevice);
				VmDevice = NULL;
				CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualFileName::GetConfigInformation:�����VMX�����ļ���ȡʧ��!"));
				free(ReadInfoBuff);
				ReadInfoBuff = NULL;
				return false;
			}

			free(ReadInfoBuff);
			ReadInfoBuff = NULL;
		}																			
		CloseHandle(VmDevice);
		VmDevice = NULL;		
	}	
	map<DWORD, string> ::iterator miter;
	for (miter = VirtualConfigFileInfo.begin(); miter != VirtualConfigFileInfo.end(); miter ++)
	{
		HANDLE ConfigDevice = CreateFile(miter->second.c_str(),
			GENERIC_READ,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			0,
			NULL);
		if (ConfigDevice == INVALID_HANDLE_VALUE) 
		{
			dwError=GetLastError();
			CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualFileName VmDevice = CreateFile��ȡVMwareVMDK�ļ����ʧ��!,\
											   ���󷵻���: dwError = %d"), dwError);
			return false;
		}
		UCHAR *VmdkHeadBuff = (UCHAR*)malloc(SECTOR_SIZE + SECTOR_SIZE);
		if (NULL == VmdkHeadBuff)
		{
			CloseHandle(ConfigDevice);
			ConfigDevice = NULL;
			CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualFileName:malloc:VmdkHeadBuff �����ڴ�ʧ��!"));
			return false;
		}
		memset(VmdkHeadBuff, 0, (SECTOR_SIZE + SECTOR_SIZE));
		bRet=ReadSQData(ConfigDevice, VmdkHeadBuff, SECTOR_SIZE,
			0,
			&BackBytesCount);		
		if(!bRet)
		{	
			CloseHandle(ConfigDevice);
			ConfigDevice = NULL;
			free(VmdkHeadBuff);
			VmdkHeadBuff = NULL;
			CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualFileName:ReadSQData:����vmx����ʧ��!"));
			return false;	
		}
		vector<string> ClassInfo;

		if (VmdkHeadBuff[0] == 0x4b && VmdkHeadBuff[1] == 0x44 && VmdkHeadBuff[2] == 0x4d && VmdkHeadBuff[3] == 0x56)
		{
			LVirtual_head vmdkhead = (LVirtual_head)&VmdkHeadBuff[0];
			DWORD64 Descripfileoff = vmdkhead->_Description_file_off;
			DWORD64 DescripfileSize = vmdkhead->_Description_file_size;

			free(VmdkHeadBuff);
			VmdkHeadBuff = NULL;

			if (NULL == Descripfileoff || NULL == DescripfileSize)
			{
				CloseHandle(ConfigDevice);
				ConfigDevice = NULL;
				CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualFileName:���ļ��������ļ�Ϊ��ʧ��!"));
				return false;
			}

			UCHAR *DescripBuff = (UCHAR*)malloc((size_t)DescripfileSize * SECTOR_SIZE + SECTOR_SIZE);
			if (NULL == DescripBuff)
			{
				CloseHandle(ConfigDevice);
				ConfigDevice = NULL;
				CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualFileName:malloc:VmdkHeadBuff �����ڴ�ʧ��!"));
				return false;
			}
			memset(DescripBuff, 0, ((size_t)DescripfileSize * SECTOR_SIZE + SECTOR_SIZE));
			bRet=ReadSQData(ConfigDevice, DescripBuff, ((DWORD)DescripfileSize * SECTOR_SIZE),
				0,
				&BackBytesCount);		
			if(!bRet)
			{	
				CloseHandle(ConfigDevice);
				ConfigDevice = NULL;
				free(DescripBuff);
				DescripBuff = NULL;
				CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualFileName:ReadSQData:����vmx����ʧ��!"));
				return false;	
			}
			if (!GetVmdkInformation(ClassInfo, DescripBuff, ((DWORD)DescripfileSize * SECTOR_SIZE), miter->first, FilePatn, VirtualConfigFileInfo))
			{
				CloseHandle(ConfigDevice);
				ConfigDevice = NULL;
				free(DescripBuff);
				DescripBuff = NULL;
				CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualFileName::GetVmdkInformation:�����VMDK�����ļ���ȡʧ��!"));
				return false;
			}
			VirtualFileInfo.insert(map<DWORD,vector<string>>::value_type(miter->first,ClassInfo));

			free(DescripBuff);
			DescripBuff = NULL;
			if (NULL == VirtualFileInfo[miter->first].size() )
			{
				CloseHandle(ConfigDevice);
				ConfigDevice = NULL;
				CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualFileName:VirtualFileInfoΪ��ʧ��!"));
				return false;
			}
		}
		else
		{
			free(VmdkHeadBuff);
			VmdkHeadBuff = NULL;

			DWORD VFileSiz = GetFileSize(ConfigDevice, NULL);
			if (VFileSiz > 0)
			{

				UCHAR *ReadConfigInfoBuff = (UCHAR*) malloc(VFileSiz + SECTOR_SIZE);
				if (NULL == ReadConfigInfoBuff)
				{
					CloseHandle(ConfigDevice);
					ConfigDevice = NULL;
					CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualFileName:malloc:ReadConfigInfoBuff �����ڴ�ʧ��!"));
					return false;	
				}
				memset(ReadConfigInfoBuff, 0, (VFileSiz + SECTOR_SIZE));


				bRet=ReadSQData(ConfigDevice, ReadConfigInfoBuff, VFileSiz,
					0,
					&BackBytesCount);		
				if(!bRet)
				{	
					CloseHandle(ConfigDevice);
					ConfigDevice = NULL;
					free(ReadConfigInfoBuff);
					ReadConfigInfoBuff = NULL;
					CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualFileName:ReadSQData:����vmx����ʧ��!"));
					return false;	
				}
				

				if (!GetVmdkInformation(ClassInfo, ReadConfigInfoBuff, VFileSiz, miter->first, FilePatn, VirtualConfigFileInfo))
				{
					CloseHandle(ConfigDevice);
					ConfigDevice = NULL;
					free(ReadConfigInfoBuff);
					ReadConfigInfoBuff = NULL;
					CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualFileName::GetVmdkInformation:�����VMDK�����ļ���ȡʧ��!"));
					return false;
				}
				VirtualFileInfo.insert(map<DWORD,vector<string>>::value_type(miter->first,ClassInfo));

				free(ReadConfigInfoBuff);
				ReadConfigInfoBuff = NULL;
				if (NULL == VirtualFileInfo[miter->first].size() )
				{
					CloseHandle(ConfigDevice);
					ConfigDevice = NULL;
					CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualFileName:VirtualFileInfoΪ��ʧ��!"));
					return false;
				}
			}
		}

		
		CloseHandle(ConfigDevice);
		ConfigDevice = NULL;
	}
		
			
	

	return true;
}
bool GetVirtualMachineInfo::VMwareReadData(string VirtualFileName, UCHAR *PatitionAddrBuffer, DWORD LeftSector, DWORD64 ReadAddr, DWORD ReadSize)
{
	DWORD dwError = NULL;
	bool Ret = false;
	DWORD BackBytesCount = NULL;
	if (ReadSize > (LeftSector * SECTOR_SIZE))
	{
		CFuncs::WriteLogInfo(SLT_ERROR, _T("VMwareReadData:����ʣ����Ч����ʧ��!"));
		return false;
	}
	HANDLE Fdrive = CreateFile(VirtualFileName.c_str(),
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);
	if (Fdrive == INVALID_HANDLE_VALUE) 
	{
		
		dwError=GetLastError();
		CFuncs::WriteLogInfo(SLT_ERROR, _T("VMwareReadData VmDevice = CreateFile��ȡVMware�����ļ����ʧ��!,\
			���󷵻���: dwError = %d"), dwError);
		return false;
	}

	memset(PatitionAddrBuffer, 0, (size_t)ReadSize);
	Ret = ReadSQData(Fdrive, &PatitionAddrBuffer[0], ReadSize, ReadAddr * SECTOR_SIZE
		, &BackBytesCount);		
	if(!Ret)
	{			
		CloseHandle(Fdrive);
		Fdrive = NULL;
		CFuncs::WriteLogInfo(SLT_ERROR, _T("VMwareReadData:ReadSQData: ��ȡ�����MBR�׵�ַͷ����Ϣʧ��!"));
		return false;	
	}

	CloseHandle(Fdrive);
	Fdrive = NULL;

	return true;
}
bool GetVirtualMachineInfo::VMwareAddressConversion(vector<string> VirtualName, DWORD64 *backAddr, DWORD64 changeAddr, DWORD64 VmdkFiletotalsize
	, DWORD *FileNum, DWORD64 Grain_size, DWORD64 GrainNumber, DWORD64 GrainListOff, DWORD *LeftSector, int VmdkfileType, DWORD Catalogoff)
{
	DWORD dwError = NULL;
	bool bRet = false;
	DWORD BackBytesCount = NULL;
	
	DWORD64 FileChangeAddrOff = NULL;
	if (VmdkfileType ==  1)
	{
		(*FileNum) = (DWORD)(changeAddr / VmdkFiletotalsize);
	}
	else if (VmdkfileType ==  2)
	{
		(*FileNum) = 0;
	}
	
	if ((*FileNum) >= VirtualName.size())
	{
		CFuncs::WriteLogInfo(SLT_ERROR, "VMwareAddressConversion:��ַ������Χʧ��!");
		return false;
	}
	if ((*FileNum) > 0)
	{
		FileChangeAddrOff = (changeAddr % VmdkFiletotalsize);
	} 
	else
	{
		FileChangeAddrOff = changeAddr;
	}
	HANDLE Fdrive = CreateFile(VirtualName[(*FileNum)].c_str(),
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);
	if (Fdrive == INVALID_HANDLE_VALUE) 
	{
		dwError=GetLastError();
		CFuncs::WriteLogInfo(SLT_ERROR, _T("VMwareAddressConversion VmDevice = CreateFile��ȡVMware�����ļ����ʧ��!,\
			���󷵻���: dwError = %d"), dwError);
		return false;
	}

	DWORD64 SerialNumber = NULL;
	UCHAR GrainAddr[4] = { NULL };

	SerialNumber = FileChangeAddrOff / Grain_size;
	if (SerialNumber >= GrainNumber)
	{
		CloseHandle(Fdrive);
		Fdrive = NULL;
		CFuncs::WriteLogInfo(SLT_ERROR, _T("VMwareAddressConversion:��Ŵ���������ʧ��!"));
		return false;
	}
	bRet=ReadSQData(Fdrive, &GrainAddr[0], 4, ((GrainListOff + Catalogoff) * SECTOR_SIZE + SerialNumber * 4),
		&BackBytesCount);		
	if(!bRet)
	{			
		CloseHandle(Fdrive);
		Fdrive = NULL;
		CFuncs::WriteLogInfo(SLT_ERROR, _T("VMwareAddressConversion:ReadSQData:��ȡvmdkͷ����Ϣʧ��!"));
		return false;	
	}
	CloseHandle(Fdrive);
	Fdrive = NULL;

	RtlCopyMemory(backAddr, &GrainAddr[0], 4);
	if (NULL == *backAddr)
	{
		//CFuncs::WriteLogInfo(SLT_ERROR, _T("VMwareAddressConversion:backAddrΪ��ʧ��!"));
		return false;
	}
	else
	{
		(*backAddr) += FileChangeAddrOff % Grain_size;
		*LeftSector = (DWORD)(Grain_size - FileChangeAddrOff % Grain_size);
	}

	return true;
}
bool GetVirtualMachineInfo::Find_virtu_GPT(vector<string> VirtualName, UCHAR *CacenBuff, vector<DWORD64>& VirtualStartaddr, DWORD64 VmdkFileTatolSize
	, DWORD64 GrainSize, DWORD64 GrainNumber, DWORD64 GrainListOff, int VmdkfileType, DWORD Catalogoff)
{

	DWORD64 VmdkChangeAddr = NULL;
	DWORD64 VmdkBackAddr = NULL;
	bool Ret = false;
	DWORD BackBytesCount = NULL;
	//��ȡGPT������ַ

	LGPT_FB_TABLE GTFB = NULL;
	bool Readsq = true;

	VmdkChangeAddr = 2;
	while(Readsq)
	{

		VmdkBackAddr = NULL;
		DWORD FileNumber = NULL;
		DWORD LeftSector = NULL;
		if(!VMwareAddressConversion(VirtualName, &VmdkBackAddr, VmdkChangeAddr, VmdkFileTatolSize, &FileNumber, GrainSize, GrainNumber, GrainListOff
			, &LeftSector, VmdkfileType, Catalogoff))
		{
			CFuncs::WriteLogInfo(SLT_ERROR, _T("Find_virtu_GPT:VMwareAddressConversion: ��ȡ�����GPTת����ַʧ��!"));
			return false;
		}
		
		if(!VMwareReadData(VirtualName[FileNumber], CacenBuff, LeftSector, VmdkBackAddr, SECTOR_SIZE))
		{			
			CFuncs::WriteLogInfo(SLT_ERROR, _T("Find_virtu_GPT:VMwareReadData: ��ȡ�����GPT��ַ��Ϣʧ��!"));
			return false;	
		}

		GTFB = (LGPT_FB_TABLE)&CacenBuff[0];
		for (int i = 0; (GTFB->_GUID_TYPE[0] != 0) && (i < 4); i++)
		{
			if (GTFB->_GUID_TYPE[0] == 0x4433b9e5ebd0a0a2)
			{
				VirtualStartaddr.push_back(GTFB->_FB_Start_SQ);
			}
			if (i < 3)
			{
				GTFB++;
			}
		}
		if (GTFB->_FB_Start_SQ == 0)
		{
			Readsq = false;
		}
	}
	if (VirtualStartaddr.size() == NULL)
	{
		CFuncs::WriteLogInfo(SLT_ERROR, _T("Find_virtu_GPT:GPT������ַΪ��,��Ѱʧ��!"));
		return false;
	}
	return true;

}
bool  GetVirtualMachineInfo::Find_virtu_Mbr(vector<string> VirtualName, UCHAR *CacenBuff, vector<DWORD64>& VirtualStartaddr, DWORD64 VmdkFileTatolSize
	, DWORD64 GrainSize, DWORD64 GrainNumber, DWORD64 GrainListOff, DWORD64 *VmdkChangeAddr, int VmdkfileType, DWORD Catalogoff)
{
	LMBR_Heads virmbr = NULL;
	
	bool bRet = false;
	DWORD BackBytesCount = NULL;

	DWORD64 VmdkBackAddr = NULL;
	DWORD FileNumber = NULL;
	DWORD LeftSector = NULL;
	if(!VMwareAddressConversion(VirtualName, &VmdkBackAddr, *VmdkChangeAddr, VmdkFileTatolSize, &FileNumber, GrainSize, GrainNumber, GrainListOff
		, &LeftSector, VmdkfileType, Catalogoff))
	{
		CFuncs::WriteLogInfo(SLT_ERROR, _T("Find_virtu_Mbr:VMwareAddressConversion: ��ȡ�����GPTת����ַʧ��!"));
		return false;
	}

	if(!VMwareReadData(VirtualName[FileNumber], CacenBuff, LeftSector, VmdkBackAddr, SECTOR_SIZE))
	{			
		CFuncs::WriteLogInfo(SLT_ERROR, _T("Find_virtu_Mbr:VMwareReadData: ��ȡ�����GPT��ַ��Ϣʧ��!"));
		return false;	
	}

	for (int i = 0; i<64; i += 16)
	{
		virmbr = (LMBR_Heads)&CacenBuff[446 + i];				
		if (virmbr->_MBR_Partition_Type == 0x05 || virmbr->_MBR_Partition_Type == 0x0f)
		{
			if (CacenBuff[0] == 0 && CacenBuff[1] == 0 && CacenBuff[2] == 0 && CacenBuff[3] == 0)
			{				
				*VmdkChangeAddr = (*VmdkChangeAddr + ((DWORD64)virmbr->_MBR_Sec_pre_pa));				
				Find_virtu_Mbr(VirtualName, CacenBuff, VirtualStartaddr, VmdkFileTatolSize, GrainSize, GrainNumber, GrainListOff, VmdkChangeAddr, VmdkfileType
					, Catalogoff);
			} 
			else
			{							
				*VmdkChangeAddr = ((DWORD64)(virmbr->_MBR_Sec_pre_pa));							
				Find_virtu_Mbr(VirtualName, CacenBuff, VirtualStartaddr, VmdkFileTatolSize, GrainSize, GrainNumber, GrainListOff, VmdkChangeAddr, VmdkfileType
					, Catalogoff);
			}
		} 
		else if (virmbr->_MBR_Partition_Type == 0x00)
		{
			CFuncs::WriteLogInfo(SLT_INFORMATION, _T("Find_virtu_Mbr ��ȡvirtualMBR���!"));
			return true;
		}
		else if (virmbr->_MBR_Partition_Type == 0x07)
		{
			if (CacenBuff[0] == 0x00 && CacenBuff[1] == 0x00 && CacenBuff[2] == 0x00 && CacenBuff[3] == 0x00)
			{			
				VirtualStartaddr.push_back((virmbr->_MBR_Sec_pre_pa + *VmdkChangeAddr));
			}
			else
			{
				VirtualStartaddr.push_back(virmbr->_MBR_Sec_pre_pa);			
			}
		}
	}
	return true;
}
bool GetVirtualMachineInfo::GetChildDiskNTFSAddr(vector<string> VirtualName, vector<DWORD64> &v_VirtualStartaddr, int VmdkfileType)
{
	DWORD dwError = NULL;

	HANDLE HeadDrive = CreateFile(VirtualName[0].c_str(),
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);
	if (HeadDrive == INVALID_HANDLE_VALUE) 
	{
		dwError=GetLastError();
		CFuncs::WriteLogInfo(SLT_ERROR, _T("GetChildDiskNTFSAddr VmDevice = CreateFile��ȡVMware�����ļ����ʧ��!,\
										   ���󷵻���: dwError = %d"), dwError);
		return false;
	}
	UCHAR *PatitionAddrBuffer = (UCHAR*) malloc(SECTOR_SIZE + SECTOR_SIZE);
	if (NULL == PatitionAddrBuffer)
	{
		CloseHandle(HeadDrive);
		HeadDrive = NULL;
		CFuncs::WriteLogInfo(SLT_ERROR, "GetChildDiskNTFSAddr:malloc PatitionAddrBufferʧ��!");
		return false;
	}
	memset(PatitionAddrBuffer, 0, SECTOR_SIZE + SECTOR_SIZE);


	bool Ret = false;
	DWORD BackBytesCount = NULL;
	Ret = ReadSQData(HeadDrive, &PatitionAddrBuffer[0], SECTOR_SIZE,
		0,
		&BackBytesCount);		
	if(!Ret)
	{		
		CloseHandle(HeadDrive);
		HeadDrive = NULL;
		free(PatitionAddrBuffer);
		PatitionAddrBuffer = NULL;
		CFuncs::WriteLogInfo(SLT_ERROR, _T("GetChildDiskNTFSAddr:ReadSQData: ��ȡvmdk�ļ�ͷ����Ϣʧ��!"));

		return false;	
	}
	
	LVirtual_head virtual_head = NULL;
	virtual_head = (LVirtual_head)&PatitionAddrBuffer[0];
	DWORD64 VmdkFiletotalsize = virtual_head->_File_capacity;
	DWORD64 Grain_size = virtual_head->_Grain_size;
	//DWORD64 GrainNumber = (virtual_head->_Grain_num * SECTOR_SIZE) / 4;
	DWORD64 GrainListOff = virtual_head->_Grain_list_off;

	UCHAR GrainAddr[4] = { NULL };
	DWORD Catalogoff = NULL;

	Ret=ReadSQData(HeadDrive, &GrainAddr[0], 4, (GrainListOff  * SECTOR_SIZE),
		&BackBytesCount);		
	if(!Ret)
	{			
		CloseHandle(HeadDrive);
		HeadDrive = NULL;
		CFuncs::WriteLogInfo(SLT_ERROR, _T("GetChildDiskNTFSAddr:ReadSQData:��ȡvmdkͷ����Ϣʧ��!"));
		return false;	
	}
	RtlCopyMemory(&Catalogoff, &GrainAddr[0], 4);
	Catalogoff = Catalogoff - (DWORD)GrainListOff;
	DWORD64 GrainNumber = (virtual_head->_Grain_num * SECTOR_SIZE * Catalogoff) / 4;

	CloseHandle(HeadDrive);
	HeadDrive = NULL;

	DWORD64 MbrGptBackAddr = NULL;
	DWORD VmdkFileNum = NULL;
	DWORD LeftSector = NULL;

	if(!VMwareAddressConversion(VirtualName, &MbrGptBackAddr, 1, VmdkFiletotalsize, &VmdkFileNum, Grain_size, GrainNumber, GrainListOff, &LeftSector
		, VmdkfileType, Catalogoff))
	{
		free(PatitionAddrBuffer);
		PatitionAddrBuffer = NULL;
		CFuncs::WriteLogInfo(SLT_ERROR, _T("GetChildDiskNTFSAddr:Virtual_to_Host_OneAddr: ��ȡ�����MBR�׵�ַͷ����Ϣʧ��!"));
		return true;
	}
	if (NULL == MbrGptBackAddr)
	{
		free(PatitionAddrBuffer);
		PatitionAddrBuffer = NULL;
		CFuncs::WriteLogInfo(SLT_INFORMATION, _T("GetChildDiskNTFSAddr::�˵�ַΪ��!"));
		return true;
	}
	
	if (!VMwareReadData(VirtualName[VmdkFileNum], PatitionAddrBuffer, LeftSector, MbrGptBackAddr, SECTOR_SIZE))
	{
		free(PatitionAddrBuffer);
		PatitionAddrBuffer = NULL;
		CFuncs::WriteLogInfo(SLT_INFORMATION, _T("GetChildDiskNTFSAddr::VMwareReadDataʧ��!"));
		return false;
	}

	LGPT_Heads GptHead = (LGPT_Heads)&PatitionAddrBuffer[0];
	if (GptHead->_Singed_name == 0x5452415020494645)
	{
		CFuncs::WriteLogInfo(SLT_INFORMATION, _T("����������GPT����"));
		if(!Find_virtu_GPT(VirtualName, PatitionAddrBuffer, v_VirtualStartaddr, VmdkFiletotalsize, Grain_size, GrainNumber, GrainListOff, VmdkfileType
			, Catalogoff))
		{
			
			free(PatitionAddrBuffer);
			PatitionAddrBuffer = NULL;
			CFuncs::WriteLogInfo(SLT_ERROR, _T("GetChildDiskNTFSAddr:Find_virtu_GPT: ��ȡ������ڲ�GPTʧ��!"));
			return true;
		}
	}
	else
	{
		CFuncs::WriteLogInfo(SLT_INFORMATION, _T("����������MBR����"));
		DWORD64 MbrChangeAddr = NULL;
		if (!Find_virtu_Mbr(VirtualName, PatitionAddrBuffer, v_VirtualStartaddr, VmdkFiletotalsize, Grain_size, GrainNumber, GrainListOff, &MbrChangeAddr
			, VmdkfileType, Catalogoff))
		{
			
			free(PatitionAddrBuffer);
			PatitionAddrBuffer = NULL;
			CFuncs::WriteLogInfo(SLT_ERROR, _T("GetChildDiskNTFSAddr:Find_virtu_Mbr: ��ȡ������ڲ�MBRʧ��!"));
			return true;	
		}
	}

	
	free(PatitionAddrBuffer);
	PatitionAddrBuffer = NULL;


	return true;
}
bool GetVirtualMachineInfo::GetVirtualNTSFAddr(map<DWORD, vector<string>> VirtualName,  DWORD VirtualNumber, vector<DWORD64> &v_VirtualNTFSStart
	, int VmdkfileType)
{
	map<DWORD, vector<string>>::iterator miter;
	for (miter = VirtualName.begin(); miter != VirtualName.end(); miter ++)
	{
		if (VirtualNumber == miter->first)
		{
			if (VirtualName[VirtualNumber][0].length() == 1)//�ж��ǲ��ǲ����
			{
				//���ǲ����
				vector<string> VirtualName_Tem;
				DWORD ParentNumber = NULL;
				for (DWORD num = 1; num < VirtualName[VirtualNumber].size(); num ++)
				{
					VirtualName_Tem.push_back(VirtualName[VirtualNumber][num]);
				}
				RtlCopyMemory(&ParentNumber, &VirtualName[VirtualNumber][0][0], 1);
				//ȡ�ò�����е�ntfs������ʼ��ַ,���Ϊ�գ���ȥ��������ȡ��nfts��ʼ��ַ
				if(!GetChildDiskNTFSAddr(VirtualName_Tem, v_VirtualNTFSStart, VmdkfileType))
				{
					CFuncs::WriteLogInfo(SLT_ERROR, "GetVirtualNTSFAddr:GetChildDiskNTFSAddrʧ��!");
					return false;
				}
				if (NULL == v_VirtualNTFSStart.size())
				{
					if (ParentNumber == VirtualNumber)
					{
						CFuncs::WriteLogInfo(SLT_ERROR, "GetVirtualNTSFAddr:ParentNumber�������������ظ���ʧ��!ʧ��!");
						return false;
					}
					//�ص��˺�����
					if(!GetVirtualNTSFAddr(VirtualName, ParentNumber, v_VirtualNTFSStart, VmdkfileType))
					{
						CFuncs::WriteLogInfo(SLT_ERROR, "GetVirtualNTSFAddr:�ص�GetVirtualNTSFAddr����ʧ��!");
						return false;
					}
				}
				else
				{
					CFuncs::WriteLogInfo(SLT_INFORMATION, "GetVirtualNTSFAddr,�ɹ��Ӳ�������ҵ�NTFS��ʼ��ַ!");
					return true;
				}

			}
			else
			{
				//���ǻ�����
				vector<string> VirtualName_Tem;
				for (DWORD num = 0; num < VirtualName[VirtualNumber].size(); num ++)
				{
					VirtualName_Tem.push_back(VirtualName[VirtualNumber][num]);
				}
				if(!GetChildDiskNTFSAddr(VirtualName_Tem, v_VirtualNTFSStart, VmdkfileType))
				{
					CFuncs::WriteLogInfo(SLT_ERROR, "GetVirtualNTSFAddr:GetBisicDiskNTFSAddrʧ��!");
					return false;
				}
				else
				{
					if (NULL != v_VirtualNTFSStart.size())
					{
						CFuncs::WriteLogInfo(SLT_INFORMATION, "GetVirtualNTSFAddr,�ɹ��ӻ��������ҵ�NTFS��ʼ��ַ!");
						return true;
					}else
					{
						CFuncs::WriteLogInfo(SLT_ERROR, "GetVirtualNTSFAddr:v_VirtualNTFSStartΪ��ʧ��!");
						return false;
					}

				}

			}
			break;
		}
	}
	

	return true;
}
bool GetVirtualMachineInfo::GetChildDiskMFTAddr( vector<string> VirtualName, DWORD64 *VirMFTStartAddr, UCHAR *m_VirtualCuNum, DWORD64 VirtualStartNTFSAddr
	, int VmdkfileType)
{

	DWORD dwError = NULL;

	HANDLE HeadDrive = CreateFile(VirtualName[0].c_str(),
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);
	if (HeadDrive == INVALID_HANDLE_VALUE) 
	{
		dwError=GetLastError();
		CFuncs::WriteLogInfo(SLT_ERROR, _T("GetChildDiskMFTAddr VmDevice = CreateFile��ȡVMware�����ļ����ʧ��!,\
										   ���󷵻���: dwError = %d"), dwError);
		return false;
	}
	UCHAR *PatitionAddrBuffer = (UCHAR*) malloc(SECTOR_SIZE + SECTOR_SIZE);
	if (NULL == PatitionAddrBuffer)
	{
		CloseHandle(HeadDrive);
		HeadDrive = NULL;
		CFuncs::WriteLogInfo(SLT_ERROR, "GetChildDiskMFTAddr:malloc PatitionAddrBufferʧ��!");
		return false;
	}
	memset(PatitionAddrBuffer, 0, SECTOR_SIZE + SECTOR_SIZE);


	bool Ret = false;
	DWORD BackBytesCount = NULL;
	Ret = ReadSQData(HeadDrive, &PatitionAddrBuffer[0], SECTOR_SIZE,
		0,
		&BackBytesCount);		
	if(!Ret)
	{		
		CloseHandle(HeadDrive);
		HeadDrive = NULL;
		free(PatitionAddrBuffer);
		PatitionAddrBuffer = NULL;
		CFuncs::WriteLogInfo(SLT_ERROR, _T("GetChildDiskMFTAddr:ReadSQData: ��ȡvmdk�ļ�ͷ����Ϣʧ��!"));

		return false;	
	}
	
	

	LVirtual_head virtual_head = NULL;
	virtual_head = (LVirtual_head)&PatitionAddrBuffer[0];
	DWORD64 VmdkFiletotalsize = virtual_head->_File_capacity;
	DWORD64 Grain_size = virtual_head->_Grain_size;
	
	DWORD64 GrainListOff = virtual_head->_Grain_list_off;

	UCHAR GrainAddr[4] = { NULL };
	DWORD Catalogoff = NULL;

	Ret=ReadSQData(HeadDrive, &GrainAddr[0], 4, (GrainListOff  * SECTOR_SIZE),
		&BackBytesCount);		
	if(!Ret)
	{			
		CloseHandle(HeadDrive);
		HeadDrive = NULL;
		CFuncs::WriteLogInfo(SLT_ERROR, _T("GetChildDiskMFTAddr:ReadSQData:��ȡvmdkͷ����Ϣʧ��!"));
		return false;	
	}
	RtlCopyMemory(&Catalogoff, &GrainAddr[0], 4);
	Catalogoff = Catalogoff - (DWORD)GrainListOff;
	DWORD64 GrainNumber = (virtual_head->_Grain_num * SECTOR_SIZE * Catalogoff) / 4;

	CloseHandle(HeadDrive);
	HeadDrive = NULL;

	DWORD64 StartMftBackAddr = NULL;
	DWORD VmdkFileNum = NULL;
	DWORD LeftSector = NULL;

	if(!VMwareAddressConversion(VirtualName, &StartMftBackAddr, VirtualStartNTFSAddr, VmdkFiletotalsize, &VmdkFileNum, Grain_size, GrainNumber
		, GrainListOff, &LeftSector, VmdkfileType, Catalogoff))
	{
		free(PatitionAddrBuffer);
		PatitionAddrBuffer = NULL;
		CFuncs::WriteLogInfo(SLT_ERROR, _T("GetChildDiskMFTAddr:Virtual_to_Host_OneAddr: ��ȡ�����MBR�׵�ַͷ����Ϣʧ��!"));
		return true;
	}
	if (NULL == StartMftBackAddr)
	{
		free(PatitionAddrBuffer);
		PatitionAddrBuffer = NULL;
		CFuncs::WriteLogInfo(SLT_INFORMATION, _T("GetChildDiskMFTAddr::�˵�ַΪ��!"));
		return true;
	}

	if (!VMwareReadData(VirtualName[VmdkFileNum], PatitionAddrBuffer, LeftSector, StartMftBackAddr, SECTOR_SIZE))
	{
		free(PatitionAddrBuffer);
		PatitionAddrBuffer = NULL;
		CFuncs::WriteLogInfo(SLT_INFORMATION, _T("GetChildDiskMFTAddr::VMwareReadDataʧ��!"));
		return false;
	}
	
	LNTFS_TABLES virtualNtfs = NULL;
	virtualNtfs = (LNTFS_TABLES)&PatitionAddrBuffer[0];
	(*m_VirtualCuNum) = virtualNtfs->_Single_Cu_Num;
	(*VirMFTStartAddr) = virtualNtfs->_MFT_Start_CU;
	if (NULL == (*VirMFTStartAddr))
	{
		free(PatitionAddrBuffer);
		PatitionAddrBuffer = NULL;
		CFuncs::WriteLogInfo(SLT_INFORMATION, _T("GetChildDiskMFTAddr:virstarMftcu: �����ת����ʼMFT��ַʧ��!"));
		return true;
	}

	free(PatitionAddrBuffer);
	PatitionAddrBuffer = NULL;

	return true;
}
bool GetVirtualMachineInfo::GetVirtualMFTAddr(map<DWORD, vector<string>> VirtualName,  DWORD VirtualNumber, DWORD64 *VirMFTStartAddr
	, DWORD64 VirtualStartNTFSAddr, UCHAR* m_VirtualCuNum, int VmdkfileType)
{
	map<DWORD, vector<string>>::iterator miter;
	for (miter = VirtualName.begin(); miter != VirtualName.end(); miter ++)
	{
		if (VirtualNumber == miter->first)
		{
			if (VirtualName[VirtualNumber][0].length() == 1)//�ж��ǲ��ǲ����
			{
				//���ǲ����
				vector<string> VirtualName_Tem;
				DWORD ParentNumber = NULL;
				for (DWORD num = 1; num < VirtualName[VirtualNumber].size(); num ++)
				{
					VirtualName_Tem.push_back(VirtualName[VirtualNumber][num]);
				}
				RtlCopyMemory(&ParentNumber, &VirtualName[VirtualNumber][0][0], 1);

				//ȡ�ò�����е�ntfs������ʼ��ַ,���Ϊ�գ���ȥ��������ȡ��nfts��ʼ��ַ
				if(!GetChildDiskMFTAddr(VirtualName_Tem, VirMFTStartAddr, m_VirtualCuNum, VirtualStartNTFSAddr, VmdkfileType))
				{
					CFuncs::WriteLogInfo(SLT_ERROR, "GetVirtualMFTAddr:GetChildDiskMFTAddrʧ��!");
					return false;
				}
				if (NULL == (*VirMFTStartAddr))
				{
					if (ParentNumber == VirtualNumber)
					{
						CFuncs::WriteLogInfo(SLT_ERROR, "GetVirtualMFTAddr::ParentNumber�������������ظ���ʧ��!ʧ��!!");
						return false;
					}
					//�ص��˺�����
					if(!GetVirtualMFTAddr(VirtualName, ParentNumber, VirMFTStartAddr, VirtualStartNTFSAddr, m_VirtualCuNum, VmdkfileType))
					{
						CFuncs::WriteLogInfo(SLT_ERROR, "GetVirtualMFTAddr:�ص�GetVirtualMFTAddr����ʧ��!");
						return false;
					}
				}
				else
				{
					CFuncs::WriteLogInfo(SLT_INFORMATION, "GetVirtualMFTAddr,�ɹ��Ӳ�������ҵ�MFT��ʼ��ַ!");
					return true;
				}
			}
			else
			{
				//���ǻ�����
				vector<string> VirtualName_Tem;
				for (DWORD num = 0; num < VirtualName[VirtualNumber].size(); num ++)
				{
					VirtualName_Tem.push_back(VirtualName[VirtualNumber][num]);
				}
				if(!GetChildDiskMFTAddr(VirtualName_Tem, VirMFTStartAddr, m_VirtualCuNum, VirtualStartNTFSAddr, VmdkfileType))
				{
					CFuncs::WriteLogInfo(SLT_ERROR, "GetVirtualMFTAddr:GetChildDiskMFTAddrʧ��!");
					return false;
				}
				else
				{
					if (NULL != (*VirMFTStartAddr))
					{
						CFuncs::WriteLogInfo(SLT_INFORMATION, "GetVirtualMFTAddr,�ɹ��ӻ��������ҵ�MFT��ʼ��ַ!");
						return true;
					}else
					{
						CFuncs::WriteLogInfo(SLT_ERROR, "GetVirtualMFTAddr:v_VirtualNTFSStartΪ��ʧ��!");
						return false;
					}

				}
			}
			break;
		}


	}

	return true;
}
bool GetVirtualMachineInfo::GetChildDiskAllMFTStartAddr(UCHAR m_VirtualCuNum, vector<LONG64> &v_VirtualStartMftAddr, vector<DWORD64> &v_VirtualStartMftLen
	, DWORD64 StartVirtualMFT, DWORD64 StartVirtualNTFS, vector<string> VirtualName, int VmdkfileType)
{
	DWORD dwError = NULL;

	HANDLE HeadDrive = CreateFile(VirtualName[0].c_str(),
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);
	if (HeadDrive == INVALID_HANDLE_VALUE) 
	{
		dwError=GetLastError();
		CFuncs::WriteLogInfo(SLT_ERROR, _T("GetChildDiskAllMFTStartAddr VmDevice = CreateFile��ȡVMware�����ļ����ʧ��!,\
										   ���󷵻���: dwError = %d"), dwError);
		return false;
	}
	UCHAR *PatitionAddrBuffer = (UCHAR*) malloc(FILE_SECTOR_SIZE + SECTOR_SIZE);
	if (NULL == PatitionAddrBuffer)
	{
		CloseHandle(HeadDrive);
		HeadDrive = NULL;
		CFuncs::WriteLogInfo(SLT_ERROR, "GetChildDiskAllMFTStartAddr:malloc PatitionAddrBufferʧ��!");
		return false;
	}
	memset(PatitionAddrBuffer, 0, FILE_SECTOR_SIZE + SECTOR_SIZE);


	bool Ret = false;
	DWORD BackBytesCount = NULL;
	Ret = ReadSQData(HeadDrive, &PatitionAddrBuffer[0], SECTOR_SIZE,
		0,
		&BackBytesCount);		
	if(!Ret)
	{		
		CloseHandle(HeadDrive);
		HeadDrive = NULL;
		free(PatitionAddrBuffer);
		PatitionAddrBuffer = NULL;
		CFuncs::WriteLogInfo(SLT_ERROR, _T("GetChildDiskAllMFTStartAddr:ReadSQData: ��ȡvmdk�ļ�ͷ����Ϣʧ��!"));

		return false;	
	}
	

	LVirtual_head virtual_head = NULL;
	virtual_head = (LVirtual_head)&PatitionAddrBuffer[0];
	DWORD64 VmdkFiletotalsize = virtual_head->_File_capacity;
	DWORD64 Grain_size = virtual_head->_Grain_size;
	//DWORD64 GrainNumber = (virtual_head->_Grain_num * SECTOR_SIZE) / 4;
	DWORD64 GrainListOff = virtual_head->_Grain_list_off;

	UCHAR GrainAddr[4] = { NULL };
	DWORD Catalogoff = NULL;

	Ret=ReadSQData(HeadDrive, &GrainAddr[0], 4, (GrainListOff  * SECTOR_SIZE),
		&BackBytesCount);		
	if(!Ret)
	{			
		CloseHandle(HeadDrive);
		HeadDrive = NULL;
		CFuncs::WriteLogInfo(SLT_ERROR, _T("GetChildDiskAllMFTStartAddr:ReadSQData:��ȡvmdkͷ����Ϣʧ��!"));
		return false;	
	}
	RtlCopyMemory(&Catalogoff, &GrainAddr[0], 4);
	Catalogoff = Catalogoff - (DWORD)GrainListOff;
	DWORD64 GrainNumber = (virtual_head->_Grain_num * SECTOR_SIZE * Catalogoff) / 4;

	CloseHandle(HeadDrive);
	HeadDrive = NULL;

	DWORD64 MftBackAddr = NULL;
	DWORD VmdkFileNum = NULL;
	DWORD LeftSector = NULL;

	if(!VMwareAddressConversion(VirtualName, &MftBackAddr, (StartVirtualNTFS + StartVirtualMFT * m_VirtualCuNum), VmdkFiletotalsize, &VmdkFileNum
		, Grain_size, GrainNumber, GrainListOff, &LeftSector, VmdkfileType, Catalogoff))
	{
		free(PatitionAddrBuffer);
		PatitionAddrBuffer = NULL;
		CFuncs::WriteLogInfo(SLT_ERROR, _T("GetChildDiskAllMFTStartAddr:Virtual_to_Host_OneAddr: ��ȡ�����MBR�׵�ַͷ����Ϣʧ��!"));
		return true;
	}
	if (NULL == MftBackAddr)
	{
		free(PatitionAddrBuffer);
		PatitionAddrBuffer = NULL;
		CFuncs::WriteLogInfo(SLT_INFORMATION, _T("GetChildDiskAllMFTStartAddr::�˵�ַΪ��!"));
		return true;
	}
	if (LeftSector >= 2)
	{
		if (!VMwareReadData(VirtualName[VmdkFileNum], PatitionAddrBuffer, LeftSector, MftBackAddr, SECTOR_SIZE * 2))
		{
			free(PatitionAddrBuffer);
			PatitionAddrBuffer = NULL;
			CFuncs::WriteLogInfo(SLT_INFORMATION, _T("GetChildDiskAllMFTStartAddr::VMwareReadDataʧ��!"));
			return false;
		}
	}
	else
	{
		if (!VMwareReadData(VirtualName[VmdkFileNum], PatitionAddrBuffer, LeftSector, MftBackAddr, SECTOR_SIZE))
		{
			free(PatitionAddrBuffer);
			PatitionAddrBuffer = NULL;
			CFuncs::WriteLogInfo(SLT_INFORMATION, _T("GetChildDiskAllMFTStartAddr::VMwareReadDataʧ��!"));
			return false;
		}
		MftBackAddr = NULL;
		if(!VMwareAddressConversion(VirtualName, &MftBackAddr, (StartVirtualNTFS + StartVirtualMFT * m_VirtualCuNum + 1), VmdkFiletotalsize, &VmdkFileNum
			, Grain_size, GrainNumber, GrainListOff, &LeftSector, VmdkfileType, Catalogoff))
		{
			free(PatitionAddrBuffer);
			PatitionAddrBuffer = NULL;
			CFuncs::WriteLogInfo(SLT_ERROR, _T("GetChildDiskAllMFTStartAddr:Virtual_to_Host_OneAddr: ��ȡ�����MBR�׵�ַͷ����Ϣʧ��!"));
			return true;
		}
		if (NULL == MftBackAddr)
		{
			free(PatitionAddrBuffer);
			PatitionAddrBuffer = NULL;
			CFuncs::WriteLogInfo(SLT_INFORMATION, _T("GetChildDiskAllMFTStartAddr::�˵�ַΪ��!"));
			return true;
		}
		if (!VMwareReadData(VirtualName[VmdkFileNum], &PatitionAddrBuffer[SECTOR_SIZE], LeftSector, MftBackAddr, SECTOR_SIZE))
		{
			free(PatitionAddrBuffer);
			PatitionAddrBuffer = NULL;
			CFuncs::WriteLogInfo(SLT_INFORMATION, _T("GetChildDiskAllMFTStartAddr::VMwareReadDataʧ��!"));
			return false;
		}
	}
	

	if (!GetMFTAddr(NULL, v_VirtualStartMftAddr, v_VirtualStartMftLen, NULL, PatitionAddrBuffer, false))//�����Ϊfalse
	{
		free(PatitionAddrBuffer);
		PatitionAddrBuffer = NULL;
		CFuncs::WriteLogInfo(SLT_ERROR, "GetChildDiskAllMFTStartAddr:GetMFTAddr:��ȡ���������MFT��ʼ��ַʧ��");
		return true;
	}

	free(PatitionAddrBuffer);
	PatitionAddrBuffer = NULL;
	return true;
}
bool GetVirtualMachineInfo::GetVirtualAllMFTStartAddr(map<DWORD, vector<string>> VirtualName, DWORD VirtualNumber,  DWORD64 VirMFTStartAddr
	, DWORD64 VirtualStartNTFSAddr, UCHAR m_VirtualCuNum, vector<LONG64> &v_VirtualStartMftAddr, vector<DWORD64> &v_VirtualStartMftLen, int VmdkfileType)
{
	map<DWORD, vector<string>>::iterator miter;
	for (miter = VirtualName.begin(); miter != VirtualName.end(); miter ++)
	{
		if (VirtualNumber == miter->first)
		{
			if (VirtualName[VirtualNumber][0].length() == 1)//�ж��ǲ��ǲ����
			{
				//���ǲ����
				vector<string> VirtualName_Tem;
				DWORD ParentNumber = NULL;
				for (DWORD num = 1; num < VirtualName[VirtualNumber].size(); num ++)
				{
					VirtualName_Tem.push_back(VirtualName[VirtualNumber][num]);
				}
				RtlCopyMemory(&ParentNumber, &VirtualName[VirtualNumber][0][0], 1);

				//ȡ�ò�����е�ntfs������ʼ��ַ,���Ϊ�գ���ȥ��������ȡ��nfts��ʼ��ַ
				if(!GetChildDiskAllMFTStartAddr(m_VirtualCuNum, v_VirtualStartMftAddr, v_VirtualStartMftLen, VirMFTStartAddr
					, VirtualStartNTFSAddr, VirtualName_Tem, VmdkfileType))
				{
					CFuncs::WriteLogInfo(SLT_ERROR, "GetVirtualAllMFTStartAddr:GetChildDiskNTFSAddrʧ��!");
					return false;
				}
				if (NULL == v_VirtualStartMftAddr.size() && NULL == v_VirtualStartMftLen.size())
				{
					if (ParentNumber == VirtualNumber)
					{
						CFuncs::WriteLogInfo(SLT_ERROR, "GetVirtualAllMFTStartAddr::ParentNumber�������������ظ���ʧ��!ʧ��!!");
						return false;
					}
					//�ص��˺�����
					if(!GetVirtualAllMFTStartAddr(VirtualName, ParentNumber, VirMFTStartAddr, VirtualStartNTFSAddr, m_VirtualCuNum, v_VirtualStartMftAddr
						, v_VirtualStartMftLen, VmdkfileType))
					{
						CFuncs::WriteLogInfo(SLT_ERROR, "GetVirtualAllMFTStartAddr:�ص�GetVirtualNTSFAddr����ʧ��!");
						return false;
					}
				}
				else
				{
					CFuncs::WriteLogInfo(SLT_INFORMATION, "GetVirtualAllMFTStartAddr,�ɹ��Ӳ�������ҵ�MFT��ʼ��ַ!");
					return true;
				}
			}
			else
			{
				//���ǻ�����
				vector<string> VirtualName_Tem;
				for (DWORD num = 0; num < VirtualName[VirtualNumber].size(); num ++)
				{
					VirtualName_Tem.push_back(VirtualName[VirtualNumber][num]);
				}
				if(!GetChildDiskAllMFTStartAddr(m_VirtualCuNum, v_VirtualStartMftAddr, v_VirtualStartMftLen, VirMFTStartAddr, VirtualStartNTFSAddr
					, VirtualName_Tem, VmdkfileType))
				{
					CFuncs::WriteLogInfo(SLT_ERROR, "GetVirtualAllMFTStartAddr:GetBisicDiskNTFSAddrʧ��!");
					return false;
				}
				else
				{
					if (NULL != v_VirtualStartMftAddr.size() && NULL != v_VirtualStartMftLen.size())
					{
						CFuncs::WriteLogInfo(SLT_INFORMATION, "GetVirtualAllMFTStartAddr,�ɹ��ӻ��������ҵ�MFT��ʼ��ַ!");
						return true;
					}else
					{
						CFuncs::WriteLogInfo(SLT_ERROR, "GetVirtualAllMFTStartAddr:v_VirtualNTFSStartΪ��ʧ��!");
						return false;
					}

				}
			}

		}

	}
	

	return true;
}
bool GetVirtualMachineInfo::GetVirtualFileAddr(DWORD64 VirtualStartNTFS, LONG64 VirStartMftRfAddr, UCHAR *CacheBuff,  vector<DWORD> &H20FileRefer,
	UCHAR VirtualCuNum, vector<string> checkfilename, DWORD *ParentMft, vector<LONG64> &Fileh80datarun, vector<DWORD> &Fileh80datalen, 
	string &Fileh80data, DWORD RereferNumber, string &FileName, DWORD64 *filerealsize, DWORD64 VmdkFiletotalsize, DWORD64 Grain_size, DWORD64 GrainNumber,
	DWORD64 GrainListOff, vector<string> VirtualName, int VmdkfileType, DWORD Catalogoff)
{
	*ParentMft = NULL;
	*filerealsize = NULL;
	FileName.clear();
	Fileh80datarun.clear();
	Fileh80datalen.clear();
	Fileh80data.clear();
	H20FileRefer.clear();
	bool Ret = false;
	DWORD64 VirtualBackAddr = NULL;
	DWORD BackBytesCount = NULL;
	LFILE_Head_Recoding File_head_recod = NULL;
	LATTRIBUTE_HEADS ATTriBase = NULL;
	bool Found = false;
	LAttr_30H H30 = NULL;
	LAttr_20H H20 = NULL;
	UCHAR *H30_NAMES = NULL;
	UCHAR *H80_data = NULL;


	memset(CacheBuff,0,FILE_SECTOR_SIZE);

	DWORD64 FileReferBackAddr = NULL;
	DWORD VmdkFileNum = NULL;
	DWORD LeftSector = NULL;
	DWORD AttributeSize = NULL;
	DWORD FirstAttriSize = NULL;

	if(!VMwareAddressConversion(VirtualName, &FileReferBackAddr, (VirtualStartNTFS + VirStartMftRfAddr * VirtualCuNum + RereferNumber), VmdkFiletotalsize
		, &VmdkFileNum, Grain_size, GrainNumber, GrainListOff, &LeftSector, VmdkfileType, Catalogoff))
	{

		//CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualFileAddr:VMwareAddressConversion��Ϣʧ��!"));
		return true;
	}
	if (NULL == FileReferBackAddr)
	{
		
		//CFuncs::WriteLogInfo(SLT_INFORMATION, _T("GetVirtualFileAddr::�˵�ַΪ��!"));
		return true;
	}
	if (LeftSector >= 2)
	{
		if (!VMwareReadData(VirtualName[VmdkFileNum], CacheBuff, LeftSector, FileReferBackAddr, SECTOR_SIZE * 2))
		{
		
			CFuncs::WriteLogInfo(SLT_INFORMATION, _T("GetVirtualFileAddr::VMwareReadDataʧ��!"));
			return false;
		}
	}
	else
	{
		if (!VMwareReadData(VirtualName[VmdkFileNum], CacheBuff, LeftSector, FileReferBackAddr, SECTOR_SIZE))
		{
		
			CFuncs::WriteLogInfo(SLT_INFORMATION, _T("GetVirtualFileAddr::VMwareReadDataʧ��!"));
			return false;
		}
		FileReferBackAddr = NULL;
		if(!VMwareAddressConversion(VirtualName, &FileReferBackAddr, (VirtualStartNTFS + VirStartMftRfAddr * VirtualCuNum + RereferNumber + 1)
			, VmdkFiletotalsize, &VmdkFileNum, Grain_size, GrainNumber, GrainListOff, &LeftSector, VmdkfileType, Catalogoff))
		{

			//CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualFileAddr:VMwareAddressConversionʧ��!"));
			return true;
		}
		if (NULL == FileReferBackAddr)
		{

			//CFuncs::WriteLogInfo(SLT_INFORMATION, _T("GetVirtualFileAddr::�˵�ַΪ��!"));
			return true;
		}
		if (!VMwareReadData(VirtualName[VmdkFileNum], &CacheBuff[SECTOR_SIZE], LeftSector, FileReferBackAddr, SECTOR_SIZE))
		{
			CFuncs::WriteLogInfo(SLT_INFORMATION, _T("GetVirtualFileAddr::VMwareReadDataʧ��!"));
			return false;
		}
	}


	File_head_recod = (LFILE_Head_Recoding)&CacheBuff[0];
	
	if(File_head_recod->_FILE_Index == 0x454c4946 && File_head_recod->_Flags[0] != 0)
	{
		RtlCopyMemory(&CacheBuff[510], &CacheBuff[File_head_recod->_Update_Sequence_Number[0]+2], 2);//������������	
		RtlCopyMemory(&CacheBuff[1022],&CacheBuff[File_head_recod->_Update_Sequence_Number[0]+4], 2);
		RtlCopyMemory(&FirstAttriSize, &File_head_recod->_First_Attribute_Dev[0], 2);
		if (FirstAttriSize > (FILE_SECTOR_SIZE - sizeof(ATTRIBUTE_HEADS)))
		{
			CFuncs::WriteLogInfo(SLT_ERROR, "GetVirtualFileAddr::File_head_recod->_First_Attribute_Dev[0] > FILE_SECTOR_SIZEʧ��!");
			return false;
		}
		
		while ((FirstAttriSize + AttributeSize) < FILE_SECTOR_SIZE)
		{
			ATTriBase = (LATTRIBUTE_HEADS)&CacheBuff[FirstAttriSize + AttributeSize];
			if(ATTriBase->_Attr_Type !=0xffffffff)
			{
				if (ATTriBase->_Attr_Type == 0x20)
				{
					DWORD h20Length=NULL;
					switch(ATTriBase->_PP_Attr)
					{
					case 0:
						if (ATTriBase->_AttrName_Length == 0)
						{
							h20Length = 24;
						} 
						else
						{
							h20Length = 24 + 2 * ATTriBase->_AttrName_Length;
						}
						break;
					case 0x01:
						if (ATTriBase->_AttrName_Length == 0)
						{
							h20Length = 64;
						} 
						else
						{
							h20Length = 64 + 2 * ATTriBase->_AttrName_Length;
						}
						break;
					}
					if (h20Length > (ATTriBase->_Attr_Length))
					{
						CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualFileAddr:h20Length > (ATTriBase->_Attr_Length)ʧ��!"));
						return false;
					}
					if (ATTriBase->_PP_Attr == 0)
					{
						H20 = (LAttr_20H)((UCHAR*)&ATTriBase[0] + h20Length);
						while (H20->_H20_TYPE != 0)
						{							
														
							if (H20->_H20_TYPE == 0x80)
							{
								H20FileRefer.push_back(H20->_H20_FILE_Reference_Num.LowPart);
								
							}
							else if (H20->_H20_TYPE == 0)
							{
								break;
							}
							else if (H20->_H20_TYPE > 0xFF)
							{
								break;
							}
							if(H20->_H20_Attr_Name_Length * 2 > 0)
							{
								if ((H20->_H20_Attr_Name_Length * 2 + 26) % 8 != 0)
								{
									h20Length += (((H20->_H20_Attr_Name_Length * 2 + 26) / 8) * 8 + 8);
								}
								else if ((H20->_H20_Attr_Name_Length * 2 + 26) % 8 == 0)
								{
									h20Length += (H20->_H20_Attr_Name_Length * 2 + 26);
								}
							}
							else
							{
								h20Length += 32;
							}
							if (h20Length > (ATTriBase->_Attr_Length))
							{
								break;
							}
							H20=(LAttr_20H)((UCHAR*)&ATTriBase[0] + h20Length);
						}
					} 
					else if (ATTriBase->_PP_Attr == 1)
					{
						UCHAR *H20Data = NULL;
						DWORD64 H20DataRun = NULL;
						H20Data = (UCHAR*)&ATTriBase[0];
						DWORD H20Offset = ATTriBase->TWOATTRIBUTEHEAD.NP_head._NPN_RunList_Offset[0];

						if (H20Offset > (FILE_SECTOR_SIZE - AttributeSize - FirstAttriSize))
						{
							CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualFileAddr:H20Offset������Χʧ��!"));
							return false;
						}

						if (H20Data[H20Offset] != 0 && H20Data[H20Offset] < 0x50)
						{
							UCHAR adres_fig = H20Data[H20Offset] >> 4;
							UCHAR len_fig = H20Data[H20Offset] & 0xf;
							for (int w = adres_fig; w > 0; w--)
							{
								H20DataRun = H20DataRun | (H20Data[H20Offset + w + len_fig] << (8 * (w - 1)));
							}
						}		
						UCHAR *H20CancheBuff = (UCHAR*)malloc(SECTOR_SIZE * VirtualCuNum + SECTOR_SIZE);
						if (NULL == H20CancheBuff)
						{
							CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualFileAddr:malloc: H20CancheBuffʧ��!"));
							return false;
						}
						memset(H20CancheBuff, 0, SECTOR_SIZE * VirtualCuNum + SECTOR_SIZE);
						for (int i=0; i < VirtualCuNum; i++)
						{					
							VirtualBackAddr=NULL;
							VmdkFileNum = NULL;
							LeftSector = NULL;
							if(!VMwareAddressConversion(VirtualName, &VirtualBackAddr, (VirtualStartNTFS + H20DataRun * VirtualCuNum + i), VmdkFiletotalsize
								, &VmdkFileNum, Grain_size, GrainNumber, GrainListOff, &LeftSector, VmdkfileType, Catalogoff))
							{

								CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualFileAddr:VMwareAddressConversion��Ϣʧ��!"));
								return true;
							}
							if (NULL == VirtualBackAddr)
							{

								CFuncs::WriteLogInfo(SLT_INFORMATION, _T("GetVirtualFileAddr::�˵�ַΪ��!"));
								break;
							}
							
							if (!VMwareReadData(VirtualName[VmdkFileNum], &H20CancheBuff[i * SECTOR_SIZE], LeftSector, VirtualBackAddr, SECTOR_SIZE))
							{

								CFuncs::WriteLogInfo(SLT_INFORMATION, _T("GetVirtualFileAddr::VMwareReadDataʧ��!"));
								return false;
							}
							
						}
						h20Length = 0;
						H20 = (LAttr_20H)&H20CancheBuff[h20Length];
						while (H20->_H20_TYPE != 0)
						{
							
							H20 = (LAttr_20H)&H20CancheBuff[h20Length];
							
							if (H20->_H20_TYPE == 0x80)
							{
								H20FileRefer.push_back(H20->_H20_FILE_Reference_Num.LowPart);

							}
							else if (H20->_H20_TYPE == 0)
							{
								break;
							}
							else if (H20->_H20_TYPE > 0xFF)
							{
								break;
							}
							if(H20->_H20_Attr_Name_Length * 2 > 0)
							{
								if ((H20->_H20_Attr_Name_Length * 2 + 26) % 8 != 0)
								{
									h20Length += (((H20->_H20_Attr_Name_Length * 2 + 26) / 8) * 8 + 8);
								}
								else if ((H20->_H20_Attr_Name_Length*2+26) % 8 == 0)
								{
									h20Length += (H20->_H20_Attr_Name_Length*2+26);
								}
							}
							else
							{
								h20Length += 32;
							}
							if (h20Length > (DWORD)(SECTOR_SIZE * VirtualCuNum))
							{
								break;
							}
							
						}

						free(H20CancheBuff);
						H20CancheBuff = NULL;
					}
				}
				if (!Found)
				{
					DWORD H30Size = NULL;
					if (ATTriBase->_Attr_Type == 0x30)
					{
						switch(ATTriBase->_PP_Attr)
						{
						case 0:
							if (ATTriBase->_AttrName_Length == 0)
							{
								H30 = (LAttr_30H)((UCHAR*)&ATTriBase[0] + 24);
								H30_NAMES = (UCHAR*)&ATTriBase[0] + 24 + 66;
								H30Size = 24 + 66 + AttributeSize + FirstAttriSize;
							} 
							else
							{
								H30 = (LAttr_30H)((UCHAR*)&ATTriBase[0] + 24 + 2 * ATTriBase->_AttrName_Length);
								H30_NAMES = (UCHAR*)&ATTriBase[0] + 24 + 2 * ATTriBase->_AttrName_Length+66;
								H30Size = 24 + 2 * ATTriBase->_AttrName_Length + 66 + AttributeSize + FirstAttriSize;
							}
							break;
						case 0x01:
							if (ATTriBase->_AttrName_Length == 0)
							{
								H30 = (LAttr_30H)((UCHAR*)&ATTriBase[0] + 64);
								H30_NAMES = (UCHAR*)&ATTriBase[0] + 64 + 66;
								H30Size = 64 + 66 + AttributeSize + FirstAttriSize;
							} 
							else
							{
								H30 = (LAttr_30H)((UCHAR*)&ATTriBase[0] + 64 + 2 * ATTriBase->_AttrName_Length);
								H30_NAMES = (UCHAR*)&ATTriBase[0] + 64 + 2 * ATTriBase->_AttrName_Length + 66;
								H30Size = 64 + 2 * ATTriBase->_AttrName_Length + 66 + AttributeSize + FirstAttriSize;
							}
							break;
						}
						if ((FILE_SECTOR_SIZE - H30Size) < 0)
						{
							CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualFileAddr::(FILE_SECTOR_SIZE - H30Size)ʧ��!"));
							return false;
						}
						DWORD H30FileNameLen = H30->_H30_FILE_Name_Length * 2;
						if (H30FileNameLen > (FILE_SECTOR_SIZE - H30Size))
						{
							CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualFileAddr::������Χʧ��!"));
							return false;
						}
						
						if (H30FileNameLen > 0)
						{
							string filename;
							if(!UnicodeToZifu(&H30_NAMES[0], filename, H30FileNameLen))
							{

								CFuncs::WriteLogInfo(SLT_ERROR, "GetVirtualFileAddr����Unicode_To_Zifu::ת��ʧ��!");
								return false;
							}
							vector<string>::iterator viter;
							for (viter = checkfilename.begin(); viter != checkfilename.end(); viter ++)
							{
								if (filename.rfind(*viter) != string::npos)
								{
									size_t posion = filename.rfind(*viter);
									size_t c_posion = NULL;
									c_posion = filename.length() - posion;
									if (viter->length() == c_posion)
									{
										Found = true;
										break;
									}
								}
							}							
							if (Found)
							{

								CFuncs::WriteLogInfo(SLT_INFORMATION, "GetVirtualFileAddr ���ļ���¼�ο�����:%lu",File_head_recod->_FR_Refer);
								
								RtlCopyMemory(ParentMft,&H30->_H30_Parent_FILE_Reference,4);

								FileName.append((char*)&H30_NAMES[0],(H30->_H30_FILE_Name_Length * 2));


								if (H20FileRefer.size() > 0)
								{
									vector<DWORD>::iterator vec;
									for (vec = H20FileRefer.begin(); vec < H20FileRefer.end(); vec ++)
									{
										if (*vec != File_head_recod->_FR_Refer)
										{
											CFuncs::WriteLogInfo(SLT_INFORMATION, "GetVirtualFileAddr ���ļ���¼H80�ض�λ��H20�У��ض�λ�ļ��ο�����:%lu", *vec);
										}
										else
										{
											H20FileRefer.erase(vec);//��ͬ�ľ�û�ض�λ������Ϊ��
										}
									}

								}																			
											
										
							}

									
						}	
								
																			
					}
				}
			
				if (Found)
				{
					bool FirstIn = true;
					DWORD H80_datarun_len = NULL;
					LONG64 H80_datarun = NULL;
					if (ATTriBase->_Attr_Type == 0x80)
					{
						(*filerealsize) = ((*filerealsize) + ATTriBase->TWOATTRIBUTEHEAD.NP_head._NPN_Attr_T_Size);
						if (ATTriBase->_PP_Attr == 0x01)
						{
							H80_data = (UCHAR*)&ATTriBase[0];
							DWORD OFFSET = NULL;
							RtlCopyMemory(&OFFSET, &ATTriBase->TWOATTRIBUTEHEAD.NP_head._NPN_RunList_Offset[0], 2);
							if (OFFSET > (FILE_SECTOR_SIZE - FirstAttriSize - AttributeSize))
							{
								CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualFileAddr::OFFSET������Χ!"));
								return false;
							}
							if (H80_data[OFFSET] != 0 && H80_data[OFFSET] < 0x50)
							{					
								while(OFFSET < ATTriBase->_Attr_Length)
								{
									H80_datarun_len = NULL;
									H80_datarun = NULL;
									if (H80_data[OFFSET] > 0 && H80_data[OFFSET] < 0x50)
									{
										UCHAR adres_fig = H80_data[OFFSET] >> 4;
										UCHAR len_fig = H80_data[OFFSET] & 0xf;
										for(int w = len_fig;w > 0; w--)
										{
											H80_datarun_len = H80_datarun_len | (H80_data[OFFSET+w] << (8 * (w - 1)));
										}
										if (H80_datarun_len > 0)
										{
											Fileh80datalen.push_back(H80_datarun_len);
										} 
										else
										{
											CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualFileAddr::H80_datarun_lenΪ0!"));
											return false;
										}

										for (int w = adres_fig; w > 0; w --)
										{
											H80_datarun = H80_datarun | (H80_data[OFFSET+w+len_fig] << (8 * (w - 1)));
										}
										if (H80_data[OFFSET + adres_fig + len_fig] > 127)
										{
											if (adres_fig == 3)
											{
												H80_datarun = ~(H80_datarun^0xffffff);
											}
											if (adres_fig == 2)
											{
												H80_datarun = ~(H80_datarun^0xffff);

											}

										} 
										if (FirstIn)
										{
											if (H80_datarun > 0)
											{
												Fileh80datarun.push_back(H80_datarun);
											} 
											else
											{
												CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualFileAddr::H80_datarunΪ0��Ϊ��������!"));
												return false;
											}
											FirstIn = false;
										}
										else
										{
											if (Fileh80datarun.size() > 0)
											{
												H80_datarun = Fileh80datarun[Fileh80datarun.size() - 1] + H80_datarun;
												Fileh80datarun.push_back(H80_datarun);
											}
										}
										
										OFFSET = OFFSET + adres_fig + len_fig + 1;
									}
									else
									{
										break;
									}

								}								
							}

						}
						else if(ATTriBase->_PP_Attr == 0)
						{
							H80_data = (UCHAR*)&ATTriBase[0];	
							if (ATTriBase->TWOATTRIBUTEHEAD.P_head._PN_AttrBody_Length < (FILE_SECTOR_SIZE - AttributeSize - FirstAttriSize - 24))
							{
								Fileh80data.append((char*)&H80_data[24],ATTriBase->TWOATTRIBUTEHEAD.P_head._PN_AttrBody_Length);
							}
							
						}

					}
				}
				if (ATTriBase->_Attr_Length < (FILE_SECTOR_SIZE - AttributeSize - FirstAttriSize) && ATTriBase->_Attr_Length > 0)
				{

					AttributeSize += ATTriBase->_Attr_Length;
							
				} 
				else
				{
					CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualFileAddr::������:%lu,ATTriBase[i_attr]->_Attr_Length���ȹ����Ϊ0!")
						,ATTriBase->_Attr_Length);
					return false;
				}
			}
			else if (ATTriBase->_Attr_Type == 0xffffffff)
			{
				if (!Found)
				{
					H20FileRefer.clear();
				}				
				memset(CacheBuff, 0, FILE_SECTOR_SIZE);
				break;
			}
			else if(ATTriBase->_Attr_Type > 0xff && ATTriBase->_Attr_Type < 0xffffffff)
			{
				CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualFileAddr:��ȡATTriBase[i_attr]->_Attr_Type����0xffffffff����"));
				return false;
			}

		}
	}
	return true;
}
bool  GetVirtualMachineInfo::GetVirtualH20FileReferH80Addr(UCHAR *CacenBuff, vector<LONG64> &H80datarun, vector<DWORD> &H80datarunlen
	, string &h80data, DWORD64 *FileRealSize)
{
	*FileRealSize = NULL;
	DWORD BackBytesCount=NULL;
	bool bRet=false;
	LFILE_Head_Recoding File_head_recod = NULL;
	LATTRIBUTE_HEADS ATTriBase = NULL;
	UCHAR *H80_data = NULL;
	DWORD AttributeSize = NULL;
	DWORD FirstAttriSize = NULL;

	File_head_recod = (LFILE_Head_Recoding)&CacenBuff[0];
	if(File_head_recod->_FILE_Index == 0x454c4946 && File_head_recod->_Flags[0] != 0)
	{
		RtlCopyMemory(&CacenBuff[510], &CacenBuff[File_head_recod->_Update_Sequence_Number[0]+2], 2);//������������	
		RtlCopyMemory(&CacenBuff[1022],&CacenBuff[File_head_recod->_Update_Sequence_Number[0]+4], 2);
		RtlCopyMemory(&FirstAttriSize, &File_head_recod->_First_Attribute_Dev[0], 2);
		if (FirstAttriSize > (FILE_SECTOR_SIZE - sizeof(ATTRIBUTE_HEADS)))
		{
			CFuncs::WriteLogInfo(SLT_ERROR, "GetVirtualH20FileReferH80Addr::File_head_recod->_First_Attribute_Dev[0] > FILE_SECTOR_SIZEʧ��!");
			return false;
		}

		
		while((AttributeSize + FirstAttriSize) < FILE_SECTOR_SIZE)
		{
			ATTriBase = (LATTRIBUTE_HEADS)&CacenBuff[FirstAttriSize + AttributeSize];
			if(ATTriBase->_Attr_Type != 0xffffffff)
			{
				if (ATTriBase->_Attr_Type == 0x80)
				{
					bool FirstIn = true;
					if (ATTriBase->_PP_Attr == 0x01)
					{
						*FileRealSize = (*FileRealSize + ATTriBase->TWOATTRIBUTEHEAD.NP_head._NPN_Attr_T_Size);
						DWORD H80_datarun_len = NULL;
						LONG64 H80_datarun = NULL;
						H80_data = (UCHAR*)&ATTriBase[0];
						DWORD OFFSET = NULL;
						RtlCopyMemory(&OFFSET, &ATTriBase->TWOATTRIBUTEHEAD.NP_head._NPN_RunList_Offset[0], 2);
						if (OFFSET > (FILE_SECTOR_SIZE - FirstAttriSize - AttributeSize))
						{
							CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualH20FileReferH80Addr::OFFSET������Χ!"));
							return false;
						}
						if (ATTriBase->_Attr_Length > (FILE_SECTOR_SIZE - AttributeSize - FirstAttriSize))
						{
							CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualH20FileReferH80Addr::ATTriBase->_Attr_Length������Χ!"));
							return false;
						}
						if (H80_data[OFFSET] != 0 && H80_data[OFFSET] < 0x50)
						{
							while(OFFSET < ATTriBase->_Attr_Length)
							{
								H80_datarun_len = NULL;
								H80_datarun = NULL;
								if (H80_data[OFFSET] > 0 && H80_data[OFFSET] < 0x50)
								{
									UCHAR adres_fig = H80_data[OFFSET] >> 4;
									UCHAR len_fig = H80_data[OFFSET] & 0xf;
									for(int w = len_fig; w > 0; w --)
									{
										H80_datarun_len = H80_datarun_len | (H80_data[OFFSET+w] << (8 * (w - 1)));
									}
									if (H80_datarun_len > 0)
									{
										H80datarunlen.push_back(H80_datarun_len);
									} 
									else
									{
										CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualH20FileReferH80Addr::H80_datarun_lenΪ0!"));
										return false;
									}

									for (int w = adres_fig; w > 0; w --)
									{
										H80_datarun = H80_datarun | (H80_data[OFFSET + w + len_fig] << (8*(w-1)));
									}
									if (H80_data[OFFSET + adres_fig + len_fig] > 127)
									{
										if (adres_fig == 3)
										{
											H80_datarun = ~(H80_datarun^0xffffff);
										}
										if (adres_fig == 2)
										{
											H80_datarun = ~(H80_datarun^0xffff);

										}

									} 
									if (FirstIn)
									{
										if (H80_datarun > 0)
										{
											H80datarun.push_back(H80_datarun);
										} 
										else
										{
											CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualH20FileReferH80Addr::H80_datarunΪ0��Ϊ��������!"));
											return false;
										}
										FirstIn = false;
									}
									else
									{
										if (H80datarun.size() > 0)
										{
											H80_datarun = H80datarun[H80datarun.size() - 1] + H80_datarun;
											H80datarun.push_back(H80_datarun);
										}
									}
									
									OFFSET = OFFSET + adres_fig + len_fig + 1;
								}
								else
								{
									break;
								}
							}							
						}
					}
					else if(ATTriBase->_PP_Attr == 0)
					{
						
						H80_data = (UCHAR*)&ATTriBase[0];	
						if (ATTriBase->TWOATTRIBUTEHEAD.P_head._PN_AttrBody_Length < (FILE_SECTOR_SIZE - AttributeSize - FirstAttriSize - 24))
						{
							h80data.append((char*)&H80_data[24],ATTriBase->TWOATTRIBUTEHEAD.P_head._PN_AttrBody_Length);
						}
						
					}
				}

				if (ATTriBase->_Attr_Length < (FILE_SECTOR_SIZE - AttributeSize - FirstAttriSize) && ATTriBase->_Attr_Length > 0)
				{
					
					AttributeSize += ATTriBase->_Attr_Length;
								
				} 
				else
				{

					CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualH20FileReferH80Addr::������:%lu,ATTriBase[i_attr]->_Attr_Length���ȹ����Ϊ0!")
						,ATTriBase->_Attr_Length);
					return false;
				}
			}else if (ATTriBase->_Attr_Type == 0xffffffff)
			{
				break;
			}
			else if(ATTriBase->_Attr_Type > 0xff && ATTriBase->_Attr_Type < 0xffffffff)
			{
				CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualH20FileReferH80Addr:��ȡATTriBase[i_attr]->_Attr_Type����0xffffffff����"));
				return false;
			}

		}

	}
	return true;
}
bool  GetVirtualMachineInfo::VirtualWriteLargeFile(UCHAR VirCuNum,vector<LONG64> FileH80Addr, vector <DWORD> FileH80Len, DWORD64 VirStartNTFSAddr
	, const wchar_t *PreserFileName, DWORD64 filerealSize, DWORD64 VmdkFiletotalsize, DWORD64 Grain_size, DWORD64 GrainNumber,
	DWORD64 GrainListOff, vector<string> VirtualName, int VmdkfileType, DWORD Catalogoff)
{
	BOOL bRet = FALSE;
	DWORD BackBytesCount = NULL;
	if (NULL == filerealSize)
	{
		CFuncs::WriteLogInfo(SLT_ERROR, _T("VirtualWriteLargeFile:filerealSizeΪ0!"));
		return false;
	}


	HANDLE VirtualFileDrive = ::CreateFileW(PreserFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, NULL, NULL);
	if (VirtualFileDrive == INVALID_HANDLE_VALUE)
	{

		CFuncs::WriteLogInfo(SLT_ERROR, _T("VirtualWriteLargeFile:CreateFile:��ȡ����������ļ��ں�ʧ��!"));
		return false;
	}

	LARGE_INTEGER RecoydwOffse={NULL};
	
	for (DWORD AddrNum = 0 ; AddrNum < FileH80Addr.size(); AddrNum ++)
	{
		
		DWORD64 DataBackAddr = NULL;
		DWORD VmdkFileNum = NULL;
		DWORD LeftSector = NULL;
		LONG64 DataAddr =  FileH80Addr[AddrNum];
				
		for (DWORD DataLen = NULL; DataLen < (FileH80Len[AddrNum] * VirCuNum * SECTOR_SIZE); DataLen += (LeftSector * SECTOR_SIZE))
		{
			DataBackAddr = NULL;
			VmdkFileNum = NULL;
			LeftSector = NULL;
			if(!VMwareAddressConversion(VirtualName, &DataBackAddr, (VirStartNTFSAddr + DataAddr * VirCuNum + (DataLen / SECTOR_SIZE)), VmdkFiletotalsize
				, &VmdkFileNum, Grain_size, GrainNumber, GrainListOff, &LeftSector, VmdkfileType, Catalogoff))
			{
				CloseHandle(VirtualFileDrive);
				VirtualFileDrive = NULL;
				CFuncs::WriteLogInfo(SLT_INFORMATION, _T("VirtualWriteLargeFile:VMwareAddressConversion: ʧ��!"));
				return true;
			}
			if (NULL == DataBackAddr || LeftSector > 4096)
			{

				CFuncs::WriteLogInfo(SLT_INFORMATION, _T("VirtualWriteLargeFile::�˵�ַΪ�� || LeftSector > 4096!"));
				break;
			}
			DWORD64 ReadDataSize = NULL;
			if ((LeftSector * SECTOR_SIZE) > ((FileH80Len[AddrNum] * VirCuNum * SECTOR_SIZE) - DataLen))
			{
				ReadDataSize = ((FileH80Len[AddrNum] * VirCuNum * SECTOR_SIZE) - DataLen);
			}
			else
			{
				ReadDataSize = (LeftSector * SECTOR_SIZE);
			}
			UCHAR *ReadBuff = (UCHAR*)malloc((size_t)ReadDataSize + SECTOR_SIZE);
			if (NULL == ReadBuff)
			{
				CloseHandle(VirtualFileDrive);
				VirtualFileDrive = NULL;
				CFuncs::WriteLogInfo(SLT_INFORMATION, _T("VirtualWriteLargeFile::malloc:ReadBuffʧ��!"));
				return false;
			}
			memset(ReadBuff, 0, ((size_t)ReadDataSize + SECTOR_SIZE));
			if (!VMwareReadData(VirtualName[VmdkFileNum], ReadBuff, LeftSector, DataBackAddr, (DWORD)ReadDataSize))
			{
				free(ReadBuff);
				ReadBuff = NULL;
				CloseHandle(VirtualFileDrive);
				VirtualFileDrive = NULL;
				CFuncs::WriteLogInfo(SLT_INFORMATION, _T("VirtualWriteLargeFile::VMwareReadDataʧ��!"));
				return false;
			}
			bRet = SetFilePointerEx(VirtualFileDrive, RecoydwOffse, NULL, FILE_BEGIN);
			if(!bRet)
			{
				free(ReadBuff);
				ReadBuff = NULL;
				CloseHandle(VirtualFileDrive);
				VirtualFileDrive = NULL;
				CFuncs::WriteLogInfo(SLT_ERROR, _T("VirtualWriteLargeFile:SetFilePointerEx:�ļ���λ����!"));
				return false;	
			}
			
			if ((RecoydwOffse.QuadPart + ReadDataSize) > filerealSize)
			{
				if(!::WriteFile(VirtualFileDrive, ReadBuff, (DWORD)(filerealSize - RecoydwOffse.QuadPart), &BackBytesCount, NULL))
				{
					free(ReadBuff);
					ReadBuff = NULL;
					CloseHandle(VirtualFileDrive);
					VirtualFileDrive = NULL;
					CFuncs::WriteLogInfo(SLT_ERROR, _T("VirtualWriteLargeFile:WriteFile:д�ļ�����ʧ��!"));
					return false;
				}
				free(ReadBuff);
				ReadBuff = NULL;
				break;
			}else
			{
				if(!::WriteFile(VirtualFileDrive, ReadBuff, (DWORD)ReadDataSize, &BackBytesCount, NULL))
				{
					free(ReadBuff);
					ReadBuff = NULL;
					CloseHandle(VirtualFileDrive);
					VirtualFileDrive = NULL;
					CFuncs::WriteLogInfo(SLT_ERROR, _T("VirtualWriteLargeFile:WriteFile:д�ļ�����ʧ��!"));
					return false;
				}
			}
			free(ReadBuff);
			ReadBuff = NULL;
			(RecoydwOffse.QuadPart) += ReadDataSize;
		}

	}
	
	
	CloseHandle(VirtualFileDrive);	
	VirtualFileDrive = NULL;
	return true;
}
bool GetVirtualMachineInfo::GetVirtualFilePath( DWORD64 VirtualNtfs, vector<LONG64> VirtualStartMFTaddr, vector<DWORD64> VirtualStartMFTaddrLen
	, UCHAR VirtualCuNum, DWORD ParentMFT, string& VirtualFilePath, string FileName, DWORD64 VmdkFiletotalsize, DWORD64 Grain_size, DWORD64 GrainNumber,
	DWORD64 GrainListOff, vector<string> VirtualName, int VmdkfileType, DWORD Catalogoff)
{
	DWORD MFTnumber = NULL;
	bool  bRet = false;
	DWORD BackBytesCount = NULL;
	LFILE_Head_Recoding File_head_recod = NULL;
	LATTRIBUTE_HEADS ATTriBase = NULL;
	LAttr_30H H30 = NULL;
	UCHAR *H30_NAMES = NULL;
	string StrTem;


	StrTem.append("//");
	StrTem.append(FileName);



	UCHAR *CacheBuffer = (UCHAR*) malloc(FILE_SECTOR_SIZE + SECTOR_SIZE);
	if (NULL == CacheBuffer)
	{
		CFuncs::WriteLogInfo(SLT_ERROR, "GetVirtualFilePath:malloc : CacheBufferʧ��!");
		return false;
	}
	memset(CacheBuffer, 0, FILE_SECTOR_SIZE + SECTOR_SIZE);
	File_head_recod = (LFILE_Head_Recoding)&CacheBuffer[0];	

	MFTnumber = ParentMFT;


	DWORD numbers = NULL;
	while (MFTnumber != 5 && MFTnumber != 0)
	{
		DWORD AttributeSize = NULL;
		DWORD FirstAttriSize = NULL;
		if (numbers>100)
		{
			free(CacheBuffer);
			CacheBuffer = NULL;
			CFuncs::WriteLogInfo(SLT_ERROR, "GetVirtualFilePath:numbers ·���ļ�����100��������!");
			return false;
		}
		DWORD64 MftLenAdd = NULL;
		LONG64 MftAddr = NULL;

		for (DWORD FMft = 0; FMft < VirtualStartMFTaddrLen.size(); FMft++)
		{
			if ((MFTnumber*2) <= (VirtualStartMFTaddrLen[FMft] * VirtualCuNum + MftLenAdd))
			{
				MftAddr = (VirtualStartMFTaddr[FMft] * VirtualCuNum + (MFTnumber * 2) - MftLenAdd);
				break;
			} 
			else
			{
				MftLenAdd += (VirtualStartMFTaddrLen[FMft] * VirtualCuNum);
			}
		}
	
		DWORD64 VirtualBackAddr = NULL;
		DWORD VmdkFileNum = NULL;
		DWORD LeftSector = NULL;
		memset(CacheBuffer, 0, FILE_SECTOR_SIZE);
		for (int i=0; i < 2; i++)
		{					
			VirtualBackAddr=NULL;
			VmdkFileNum = NULL;
			LeftSector = NULL;
			if(!VMwareAddressConversion(VirtualName, &VirtualBackAddr, (VirtualNtfs + MftAddr + i), VmdkFiletotalsize
				, &VmdkFileNum, Grain_size, GrainNumber, GrainListOff, &LeftSector, VmdkfileType, Catalogoff))
			{
				free(CacheBuffer);
				CacheBuffer = NULL;
				CFuncs::WriteLogInfo(SLT_INFORMATION, _T("GetVirtualFilePath:Virtual_to_Host_OneAddrʧ��!"));
				return true;
			}
			if (NULL == VirtualBackAddr)
			{
				free(CacheBuffer);
				CacheBuffer = NULL;
				CFuncs::WriteLogInfo(SLT_INFORMATION, _T("GetVirtualFilePath::�˵�ַΪ��!"));
				return true;
			}

			if (!VMwareReadData(VirtualName[VmdkFileNum], &CacheBuffer[i * SECTOR_SIZE], LeftSector, VirtualBackAddr, SECTOR_SIZE))
			{
				free(CacheBuffer);
				CacheBuffer = NULL;
				CFuncs::WriteLogInfo(SLT_INFORMATION, _T("GetVirtualFilePath::VMwareReadDataʧ��!"));
				return false;
			}

		}
		
		if (File_head_recod->_FILE_Index == 0)
		{
			free(CacheBuffer);
			CacheBuffer = NULL;
			return true;
		}
		if (File_head_recod->_FILE_Index != 0x454c4946 && File_head_recod->_FILE_Index > 0)
		{
			free(CacheBuffer);
			CacheBuffer = NULL;
			CFuncs::WriteLogInfo(SLT_ERROR, _T("�ҵ������ļ���¼����!"));
			return true;
		} 
		else if(File_head_recod->_FILE_Index == 0x454c4946)
		{
			RtlCopyMemory(&CacheBuffer[510], &CacheBuffer[File_head_recod->_Update_Sequence_Number[0]+2], 2);//������������	
			RtlCopyMemory(&CacheBuffer[1022],&CacheBuffer[File_head_recod->_Update_Sequence_Number[0]+4], 2);
			RtlCopyMemory(&FirstAttriSize, &File_head_recod->_First_Attribute_Dev[0], 2);
			if (FirstAttriSize > (FILE_SECTOR_SIZE - sizeof(ATTRIBUTE_HEADS)))
			{
				CFuncs::WriteLogInfo(SLT_ERROR, "GetVirtualFilePath::File_head_recod->_First_Attribute_Dev[0] > FILE_SECTOR_SIZEʧ��!");
				return false;
			}
			
			string H30temName;
			while ((FirstAttriSize + AttributeSize) < FILE_SECTOR_SIZE)
			{
				ATTriBase = (LATTRIBUTE_HEADS)&CacheBuffer[FirstAttriSize + AttributeSize];
				if(ATTriBase->_Attr_Type != 0xffffffff)
				{
					if (ATTriBase->_Attr_Type == 0x30)
					{
						DWORD H30Size = NULL;
						switch(ATTriBase->_PP_Attr)
						{
						case 0:
							if (ATTriBase->_AttrName_Length == 0)
							{
								H30 = (LAttr_30H)((UCHAR*)&ATTriBase[0] + 24);
								H30_NAMES = (UCHAR*)&ATTriBase[0] + 24 + 66;
								H30Size = 24 + 66 + AttributeSize + FirstAttriSize;
							} 
							else
							{
								H30 = (LAttr_30H)((UCHAR*)&ATTriBase[0] + 24 + 2 * ATTriBase->_AttrName_Length);
								H30_NAMES = (UCHAR*)&ATTriBase[0] + 24 + 2 * ATTriBase->_AttrName_Length+66;
								H30Size = 24 + 2 * ATTriBase->_AttrName_Length + 66 + AttributeSize + FirstAttriSize;
							}
							break;
						case 0x01:
							if (ATTriBase->_AttrName_Length == 0)
							{
								H30 = (LAttr_30H)((UCHAR*)&ATTriBase[0] + 64);
								H30_NAMES = (UCHAR*)&ATTriBase[0] + 64 + 66;
								H30Size = 64 + 66 + AttributeSize + FirstAttriSize;
							} 
							else
							{
								H30 = (LAttr_30H)((UCHAR*)&ATTriBase[0] + 64 + 2 * ATTriBase->_AttrName_Length);
								H30_NAMES = (UCHAR*)&ATTriBase[0] + 64 + 2 * ATTriBase->_AttrName_Length + 66;
								H30Size = 64 + 2 * ATTriBase->_AttrName_Length + 66 + AttributeSize + FirstAttriSize;
							}
							break;
						}
						if ((FILE_SECTOR_SIZE - H30Size) < 0)
						{
							free(CacheBuffer);
							CacheBuffer = NULL;
							CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualFilePath::(FILE_SECTOR_SIZE - H30Size)ʧ��!"));
							return false;
						}
						DWORD H30FileNameLen = H30->_H30_FILE_Name_Length * 2;
						if (H30FileNameLen > (FILE_SECTOR_SIZE - H30Size) || NULL == H30FileNameLen)
						{
							free(CacheBuffer);
							CacheBuffer = NULL;
							CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualFilePath::������Χ���ļ����ֳ���Ϊ��ʧ��!"));
							return false;
						}
						H30temName.clear();
						MFTnumber = NULL;
						RtlCopyMemory(&MFTnumber, &H30->_H30_Parent_FILE_Reference,4);
						H30temName.append("//");

						if (!UnicodeToZifu(&H30_NAMES[0], H30temName, H30FileNameLen))
						{
							free(CacheBuffer);
							CacheBuffer = NULL;
							CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualFilePath:Unicode_To_Zifu:ת��ʧ��!"));
							return false;
						}	
													
					}
					if (ATTriBase->_Attr_Length < (FILE_SECTOR_SIZE - AttributeSize - FirstAttriSize) && ATTriBase->_Attr_Length > 0)
					{

						AttributeSize += ATTriBase->_Attr_Length;

					} 
					else
					{		
						free(CacheBuffer);
						CacheBuffer = NULL;
						CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualFilePath:���Գ��ȹ���!,������:%lu"),ATTriBase->_Attr_Length);
						return false;
					}
				}
				else if (ATTriBase->_Attr_Type == 0xffffffff)
				{
					break;
				}
				else if(ATTriBase->_Attr_Type > 0xff && ATTriBase->_Attr_Type < 0xffffffff)
				{
					free(CacheBuffer);
					CacheBuffer = NULL;
					CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualFilePath:��ȡATTriBase[i_attr]->_Attr_Type����0xffffffff����"));
					return false;
				}
			}
			StrTem.append(H30temName);
		}
	}

	int laststring = (StrTem.length() - 1);
	for (int i = (StrTem.length()-1); i > 0; i--)
	{
		if (StrTem[i] == '/' && StrTem[i-1] == '/')
		{
			if ((laststring-i) > 0)
			{
				VirtualFilePath.append(&StrTem[i+1],(laststring-i));

				if (laststring == (StrTem.length()-1))
				{
					VirtualFilePath.append(":\\");
				} 
				else
				{
					VirtualFilePath.append("\\");
				}
				laststring = (i-2);
			}

		}
	}


	free(CacheBuffer);
	CacheBuffer = NULL;

	return true;
}
bool GetVirtualMachineInfo::VirtualWriteLitteFile(string &BuffH80, const wchar_t *FileDir)
{

	BOOL Ret = FALSE;
	DWORD nNumberOfBytesWritten = NULL;

	HANDLE hFile_recov = ::CreateFileW(FileDir, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, NULL, NULL);
	if (hFile_recov == INVALID_HANDLE_VALUE)
	{

		CFuncs::WriteLogInfo(SLT_ERROR, "WriteLitteFile:CreateFileWʧ��,������%d", GetLastError());
		return false;
	}

	Ret=::WriteFile(hFile_recov, BuffH80.c_str(),BuffH80.length() , &nNumberOfBytesWritten, NULL);
	if(!Ret)
	{	
		CloseHandle(hFile_recov);
		hFile_recov = NULL;
		CFuncs::WriteLogInfo(SLT_ERROR, "WriteLitteFile:WriteFileʧ��,������%d", GetLastError());

		return false;
	}


	CloseHandle(hFile_recov);
	hFile_recov = NULL;

	return true;

}
bool GetVirtualMachineInfo::AnalysisVmdkFile(map<DWORD, vector<string>> VMDKNameInfo, vector<string> checkfilename
	, const char* virtualFileDir, PFCallbackVirtualMachine VirtualFile)
{
	DWORD dwError = NULL;
	map<DWORD, vector<string> >::iterator  VirtualFileiter;  
	for (VirtualFileiter = VMDKNameInfo.begin(); VirtualFileiter != VMDKNameInfo.end(); VirtualFileiter++)
	{
		int  VmdkFileType = NULL;//������
		if (VirtualFileiter->second.size() > 2)
		{
			VmdkFileType = 1;//���ļ�����
		}
		else if (VirtualFileiter->second.size() > 0 && VirtualFileiter->second.size() < 3)
		{
			VmdkFileType = 2;//���ļ�����
		}
		else
		{
			break;
		}

	//	CFuncs::WriteLogInfo(SLT_INFORMATION, _T("AnalysisVmdkFile:%s:�����!"), VirtualFileiter->second[0]);
		vector <DWORD64> v_VirtualStartNTFSAddr;
		if(!GetVirtualNTSFAddr(VMDKNameInfo, VirtualFileiter->first, v_VirtualStartNTFSAddr, VmdkFileType))
		{
			CFuncs::WriteLogInfo(SLT_ERROR, "AnalysisVmdkFile:GetVirtualNTSFAddrʧ��!");
			//return false;
		}
		CFuncs::WriteLogInfo(SLT_INFORMATION, "AnalysisVmdkFile VMware�����һ��%d��NTFS��",v_VirtualStartNTFSAddr.size());
		DWORD64 VirNTFSStart = NULL;
		for (unsigned int virFenq = 0; virFenq < v_VirtualStartNTFSAddr.size(); virFenq++)
		{
			CFuncs::WriteLogInfo(SLT_INFORMATION, "AnalysisVmdkFile VMware�������ʼѰ�ҵ�%u��NTFS��",virFenq);
			VirNTFSStart = v_VirtualStartNTFSAddr[virFenq];
			DWORD64 StartMftAddr = NULL;
			UCHAR m_VirtualCuNum = NULL;
			if(!GetVirtualMFTAddr(VMDKNameInfo, VirtualFileiter->first, &StartMftAddr, VirNTFSStart, &m_VirtualCuNum, VmdkFileType))
			{
				if (NULL == StartMftAddr)
				{
					CFuncs::WriteLogInfo(SLT_ERROR, "AnalysisVmdkFile:GetVirtualMFTAddrʧ��!");
					break;
				}
				
			}

			vector <LONG64> v_VirtualMFTAddr;
			vector <DWORD64> v_VirtualMFTLen; 
			//�ҵ�MFT�ļ���¼��ȡ���е�MFT��ʼ��ַ���ļ���¼��С
			if (!GetVirtualAllMFTStartAddr(VMDKNameInfo, VirtualFileiter->first, StartMftAddr, VirNTFSStart, m_VirtualCuNum, v_VirtualMFTAddr
				, v_VirtualMFTLen, VmdkFileType))
			{
				CFuncs::WriteLogInfo(SLT_ERROR, "AnalysisVmdkFile:GetVirtualAllMFTStartAddrʧ��!");
				return false;
			}
			if (v_VirtualMFTAddr.size() != v_VirtualMFTLen.size())
			{
				CFuncs::WriteLogInfo(SLT_ERROR, "AnalysisVmdkFile:������ļ���¼��ַ�볤�ȸ�����ƥ�䣬ʧ��!");
				return false;
			}
			vector<string> VirtualName_Tem;
			if (VirtualFileiter->second[0].length() == 1)//�ж��ǲ��ǲ����
			{

				for (DWORD num = 1; num < VirtualFileiter->second.size(); num ++)
				{
					VirtualName_Tem.push_back(VirtualFileiter->second[num]);
				}

			}
			else
			{
				for (DWORD num = 0; num < VirtualFileiter->second.size(); num ++)
				{
					VirtualName_Tem.push_back(VirtualFileiter->second[num]);
				}
			}
			HANDLE HeadDrive = CreateFile(VirtualName_Tem[0].c_str(),
				GENERIC_READ,
				FILE_SHARE_READ | FILE_SHARE_WRITE,
				NULL,
				OPEN_EXISTING,
				0,
				NULL);
			if (HeadDrive == INVALID_HANDLE_VALUE) 
			{
				dwError=GetLastError();
				CFuncs::WriteLogInfo(SLT_ERROR, _T("AnalysisVmdkFile VmDevice = CreateFile��ȡVMware�����ļ����ʧ��!,\
												   ���󷵻���: dwError = %d"), dwError);
				return false;
			}
			UCHAR *PatitionAddrBuffer = (UCHAR*) malloc(FILE_SECTOR_SIZE + SECTOR_SIZE);
			if (NULL == PatitionAddrBuffer)
			{
				CloseHandle(HeadDrive);
				HeadDrive = NULL;
				CFuncs::WriteLogInfo(SLT_ERROR, "AnalysisVmdkFile:malloc PatitionAddrBufferʧ��!");
				return false;
			}
			memset(PatitionAddrBuffer, 0, FILE_SECTOR_SIZE + SECTOR_SIZE);


			bool Ret = false;
			DWORD BackBytesCount = NULL;
			Ret = ReadSQData(HeadDrive, &PatitionAddrBuffer[0], SECTOR_SIZE,
				0,
				&BackBytesCount);		
			if(!Ret)
			{		
				CloseHandle(HeadDrive);
				HeadDrive = NULL;
				free(PatitionAddrBuffer);
				PatitionAddrBuffer = NULL;
				CFuncs::WriteLogInfo(SLT_ERROR, _T("AnalysisVmdkFile:ReadSQData: ��ȡvmdk�ļ�ͷ����Ϣʧ��!"));

				return false;	
			}

			LVirtual_head virtual_head = NULL;
			virtual_head = (LVirtual_head)&PatitionAddrBuffer[0];
			DWORD64 VmdkFiletotalsize = virtual_head->_File_capacity;
			DWORD64 Grain_size = virtual_head->_Grain_size;

			DWORD64 GrainListOff = virtual_head->_Grain_list_off;

			UCHAR GrainAddr[4] = { NULL };
			DWORD Catalogoff = NULL;

			Ret=ReadSQData(HeadDrive, &GrainAddr[0], 4, (GrainListOff  * SECTOR_SIZE),
				&BackBytesCount);		
			if(!Ret)
			{			
				CloseHandle(HeadDrive);
				HeadDrive = NULL;
				CFuncs::WriteLogInfo(SLT_ERROR, _T("AnalysisVmdkFile:ReadSQData:��ȡvmdkͷ����Ϣʧ��!"));
				return false;	
			}
			RtlCopyMemory(&Catalogoff, &GrainAddr[0], 4);
			Catalogoff = Catalogoff - (DWORD)GrainListOff;
			DWORD64 GrainNumber = (virtual_head->_Grain_num * SECTOR_SIZE * Catalogoff) / 4;

			CloseHandle(HeadDrive);
			HeadDrive = NULL;

			vector<DWORD> VirH20FileRefer;
			DWORD VirParentMft = NULL;
			vector <LONG64> VirFileH80Addr;
			vector <DWORD> VirFileH80Len;
			string VirFileH80data;
			string VirFileName;
			DWORD64 FileRealSize = NULL;

			CFuncs::WriteLogInfo(SLT_INFORMATION, "AnalysisVmdkFile �����һ��%d��MFT����",v_VirtualMFTAddr.size());
			DWORD ReferNumber = 0;//�ļ���¼����
			for (DWORD MftFileNum = NULL; MftFileNum < v_VirtualMFTAddr.size(); MftFileNum++)
			{
				CFuncs::WriteLogInfo(SLT_INFORMATION, "AnalysisVmdkFile ��MFTһ��%lu��!",v_VirtualMFTLen[MftFileNum]);
				CFuncs::WriteLogInfo(SLT_INFORMATION, "AnalysisVmdkFile ��ʼѰ�ҵ�%u��MFT��",MftFileNum);
				ReferNumber = 0;
				while(GetVirtualFileAddr(VirNTFSStart, v_VirtualMFTAddr[MftFileNum], PatitionAddrBuffer,  VirH20FileRefer, m_VirtualCuNum,
					checkfilename, &VirParentMft, VirFileH80Addr, VirFileH80Len, VirFileH80data, ReferNumber, VirFileName, &FileRealSize,
					VmdkFiletotalsize, Grain_size, GrainNumber, GrainListOff, VirtualName_Tem, VmdkFileType, Catalogoff))
				{

					if (VirH20FileRefer.size() > 0)
					{
						UCHAR *VirH20CacheBuff = (UCHAR*)malloc(FILE_SECTOR_SIZE + SECTOR_SIZE);
						if ( NULL == VirH20CacheBuff)
						{
							free(PatitionAddrBuffer);
							PatitionAddrBuffer = NULL;

							CFuncs::WriteLogInfo(SLT_ERROR, "AnalysisVmdkFile:malloc VirH20CacheBuffʧ��!");
							return false;
						}
						vector<DWORD>::iterator h20vec;
						for (h20vec = VirH20FileRefer.begin(); h20vec < VirH20FileRefer.end(); h20vec++)
						{

							memset(VirH20CacheBuff, 0, FILE_SECTOR_SIZE);
							DWORD64 VirMftLen=NULL;
							DWORD64 VirStartMftRfAddr=NULL;
							for (DWORD FRN = 0; FRN < v_VirtualMFTAddr.size();FRN++)
							{
								if (((*h20vec) * 2) < (VirMftLen + v_VirtualMFTLen[FRN] * m_VirtualCuNum))
								{
									VirStartMftRfAddr = v_VirtualMFTAddr[FRN] * m_VirtualCuNum + ((*h20vec) * 2 - VirMftLen);
									break;
								} 
								else
								{
									VirMftLen+=(v_VirtualMFTAddr[FRN] * m_VirtualCuNum);
								}
							}
							DWORD64 H20VirtualBackAddr = NULL;

							DWORD VmdkFileNum = NULL;
							DWORD LeftSector = NULL;

							if(!VMwareAddressConversion(VirtualName_Tem, &H20VirtualBackAddr, (VirNTFSStart + VirStartMftRfAddr * m_VirtualCuNum), VmdkFiletotalsize
								, &VmdkFileNum, Grain_size, GrainNumber, GrainListOff, &LeftSector, VmdkFileType, Catalogoff))
							{

				
								CFuncs::WriteLogInfo(SLT_ERROR, _T("AnalysisVmdkFile:Virtual_to_Host_OneAddr: ��ȡ�����MBR�׵�ַͷ����Ϣʧ��!"));
								break;
							}

							if (LeftSector >= 2)
							{
								if (!VMwareReadData(VirtualName_Tem[VmdkFileNum], VirH20CacheBuff, LeftSector, H20VirtualBackAddr, SECTOR_SIZE * 2))
								{
									free(PatitionAddrBuffer);
									PatitionAddrBuffer = NULL;
									free(VirH20CacheBuff);
									VirH20CacheBuff = NULL;
									CFuncs::WriteLogInfo(SLT_INFORMATION, _T("AnalysisVmdkFile::VMwareReadDataʧ��!"));
									return false;
								}
							}
							else
							{
								if (!VMwareReadData(VirtualName_Tem[VmdkFileNum], VirH20CacheBuff, LeftSector, H20VirtualBackAddr, SECTOR_SIZE))
								{
									free(PatitionAddrBuffer);
									PatitionAddrBuffer = NULL;
									free(VirH20CacheBuff);
									VirH20CacheBuff = NULL;
									CFuncs::WriteLogInfo(SLT_INFORMATION, _T("AnalysisVmdkFile::VMwareReadDataʧ��!"));
									return false;
								}
								H20VirtualBackAddr = NULL;
								if(!VMwareAddressConversion(VirtualName_Tem, &H20VirtualBackAddr, (VirNTFSStart + VirStartMftRfAddr * m_VirtualCuNum + 1)
									, VmdkFiletotalsize, &VmdkFileNum, Grain_size, GrainNumber, GrainListOff, &LeftSector, VmdkFileType, Catalogoff))
								{

							
									CFuncs::WriteLogInfo(SLT_ERROR, _T("AnalysisVmdkFile:Virtual_to_Host_OneAddr: ��ȡ�����MBR�׵�ַͷ����Ϣʧ��!"));
									break;
								}

								if (!VMwareReadData(VirtualName_Tem[VmdkFileNum], &VirH20CacheBuff[SECTOR_SIZE], LeftSector, H20VirtualBackAddr, SECTOR_SIZE))
								{
									free(PatitionAddrBuffer);
									PatitionAddrBuffer = NULL;
									free(VirH20CacheBuff);
									VirH20CacheBuff = NULL;
									CFuncs::WriteLogInfo(SLT_INFORMATION, _T("AnalysisVmdkFile::VMwareReadDataʧ��!"));
									return false;
								}
							}

							if(!GetVirtualH20FileReferH80Addr(VirH20CacheBuff, VirFileH80Addr, VirFileH80Len, VirFileH80data, &FileRealSize))
							{
								free(PatitionAddrBuffer);
								PatitionAddrBuffer = NULL;
								free(VirH20CacheBuff);
								VirH20CacheBuff = NULL;
								CFuncs::WriteLogInfo(SLT_ERROR, "AnalysisVmdkFile:GetH20FileReferH80Addrʧ��!");
								return false;
							}

						}
						free(VirH20CacheBuff);
						VirH20CacheBuff = NULL;


					}
					if (VirFileH80Addr.size() > 0)//����Ϊ��ַ����ȡ���ļ�
					{

						string VirtualFilePath;
						string StrTemName;
						if (VirFileName.length() > 0)
						{
							if(!UnicodeToZifu((UCHAR*)&VirFileName[0], StrTemName, VirFileName.length()))
							{
								free(PatitionAddrBuffer);
								PatitionAddrBuffer = NULL;
								CFuncs::WriteLogInfo(SLT_ERROR, "AnalysisVmdkFile:UnicodeToZifu : FileNameʧ��!");
								return false;
							}
						
						

						DWORD NameSize = VirFileName.length() + strlen(virtualFileDir);
						wchar_t * WirteName = new wchar_t[NameSize+1];
						if (NULL == WirteName)
						{
							CFuncs::WriteLogInfo(SLT_ERROR, _T("AnalysisVmdkFile:new:WirteName ���������ڴ�ʧ��!"));
						}
						memset(WirteName,0,(NameSize+1)*2);
						MultiByteToWideChar(CP_ACP, 0, virtualFileDir, strlen(virtualFileDir), WirteName, NameSize);

						for (DWORD NameIndex = 0; NameIndex < VirFileName.length(); NameIndex += 2)
						{

							RtlCopyMemory(&WirteName[strlen(virtualFileDir) + NameIndex / 2], (UCHAR*)&VirFileName[NameIndex],2);
						}


						if(!VirtualWriteLargeFile(m_VirtualCuNum, VirFileH80Addr, VirFileH80Len, VirNTFSStart, WirteName, FileRealSize, VmdkFiletotalsize
							, Grain_size, GrainNumber, GrainListOff, VirtualName_Tem, VmdkFileType, Catalogoff))
						{

							CFuncs::WriteLogInfo(SLT_ERROR, "AnalysisVmdkFile:VirtualWriteLargeFile:д���ļ�ʧ��");

						}else
						{
							if(!GetVirtualFilePath(VirNTFSStart, v_VirtualMFTAddr, v_VirtualMFTLen, m_VirtualCuNum, VirParentMft, VirtualFilePath
								, VirFileName, VmdkFiletotalsize, Grain_size, GrainNumber, GrainListOff, VirtualName_Tem, VmdkFileType, Catalogoff))
							{

								CFuncs::WriteLogInfo(SLT_ERROR, "AnalysisVmdkFile:GetVirtualFileNameAndPath:��ȡ·��ʧ��");

							}
						}
						if(!VirtualFilePath.empty())
						{

							VirtualFile(VirtualFilePath.c_str(), StrTemName.c_str());
						}
						delete WirteName;
						WirteName=NULL;
						}
					} 
					else if (VirFileH80data.length() > 0)
					{
						string StrTemName;
						string VirtualFilePath;
						//string VirtualFileName;
						if (VirFileName.length() > 0)
						{
							if(!UnicodeToZifu((UCHAR*)&VirFileName[0], StrTemName, VirFileName.length()))
							{
								free(PatitionAddrBuffer);
								PatitionAddrBuffer = NULL;
								CFuncs::WriteLogInfo(SLT_ERROR, "AnalysisVmdkFile:UnicodeToZifu : FileNameʧ��!");
								return false;
							}
						
						//VirtualFileName.append(StrTemName);

						DWORD NameSize = VirFileName.length() + strlen(virtualFileDir) + 1;
						wchar_t * WirteName = new wchar_t[NameSize+1];
						if (NULL == WirteName)
						{
							CFuncs::WriteLogInfo(SLT_ERROR, _T("AnalysisVmdkFile:new:WirteName ���������ڴ�ʧ��!"));
						}
						memset(WirteName,0,(NameSize+1)*2);
						MultiByteToWideChar(CP_ACP, 0, virtualFileDir, strlen(virtualFileDir), WirteName, NameSize);
						for (DWORD NameIndex = 0; NameIndex < VirFileName.length(); NameIndex+=2)
						{

							RtlCopyMemory(&WirteName[strlen(virtualFileDir) + NameIndex / 2], &VirFileName[NameIndex],2);
						}
						if(!VirtualWriteLitteFile(VirFileH80data, WirteName))
						{

							CFuncs::WriteLogInfo(SLT_ERROR, "AnalysisVmdkFile:WriteLitteFile:дС�ļ�ʧ��ʧ��");

						}
						else
						{
							if(!GetVirtualFilePath(VirNTFSStart, v_VirtualMFTAddr, v_VirtualMFTLen, m_VirtualCuNum, VirParentMft, VirtualFilePath
								, VirFileName, VmdkFiletotalsize, Grain_size, GrainNumber, GrainListOff, VirtualName_Tem, VmdkFileType, Catalogoff))
							{

								CFuncs::WriteLogInfo(SLT_ERROR, "AnalysisVmdkFile:GetVirtualFilePath:��ȡ·��ʧ��");

							}
						}
						if(!VirtualFilePath.empty())
						{
															
							VirtualFile(VirtualFilePath.c_str(), StrTemName.c_str());
						}

						delete WirteName;
						WirteName=NULL;
						}
					}

					memset(PatitionAddrBuffer,0,FILE_SECTOR_SIZE);
					ReferNumber += 2;

					if ((ReferNumber) > v_VirtualMFTLen[MftFileNum] * m_VirtualCuNum)
					{
						CFuncs::WriteLogInfo(SLT_INFORMATION, "AnalysisVmdkFile:ReferNum:�������MFT���ļ���¼����%lu",ReferNumber);
					
						break;
					}


				}

				CFuncs::WriteLogInfo(SLT_INFORMATION, "AnalysisVmdkFile:ReferNum:�ļ���¼����%lu",ReferNumber);
			}
			free(PatitionAddrBuffer);
			PatitionAddrBuffer = NULL;
		}
	}
	return true;
}
bool GetVirtualMachineInfo::VMwareFileCheck(vector<string> VMwareMftFileName, vector<string> checkfilename, const char* virtualFileDir, PFCallbackVirtualMachine VirtualFile)
{

	map<DWORD, vector<string>> VMDKNameInfo;//��Ŷ�Ӧ��Ӧ�� vmdk�����ļ�

	if(!GetVirtualFileName(VMDKNameInfo, VMwareMftFileName))
	{
		CFuncs::WriteLogInfo(SLT_ERROR, _T("VMwareFileCheck:GetVirtualFileName:��ȡvmdk����ʧ��!"));			
		return false;
	}
	if (!AnalysisVmdkFile(VMDKNameInfo, checkfilename, virtualFileDir
		, VirtualFile))
	{
		CFuncs::WriteLogInfo(SLT_ERROR, _T("VMwareFileCheck:AnalysisVmdkFile:ʧ��!"));			
		return false;
	}
		
	
	return true;
}
bool GetVirtualMachineInfo::JudgeVHDFile(string VhdName, UCHAR *vhdtype)
{
	bool Ret=false;
	DWORD BackBytesCount=NULL;
	DWORD dwError = NULL;

	HANDLE VhdDrive = CreateFile(VhdName.c_str(),
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);
	if (VhdDrive == INVALID_HANDLE_VALUE) 
	{
		dwError=GetLastError();
		CFuncs::WriteLogInfo(SLT_ERROR, _T("JudgeVHDFile VmDevice = CreateFile��ȡVMware�����ļ����ʧ��!,\
										   ���󷵻���: dwError = %d"), dwError);
		return false;
	}
	LARGE_INTEGER filesize={NULL};
	BOOL Sret = GetFileSizeEx(VhdDrive, &filesize);
	if (!Sret)
	{
		CloseHandle(VhdDrive);
		VhdDrive = NULL;
		CFuncs::WriteLogInfo(SLT_ERROR, _T("JudgeVHDFile::GetFileSizeExʧ��!"));
		return false;
	}
	UCHAR *CacheBuff = (UCHAR*) malloc(FILE_SECTOR_SIZE);
	if (NULL == CacheBuff)
	{
		CloseHandle(VhdDrive);
		VhdDrive = NULL;
		CFuncs::WriteLogInfo(SLT_ERROR, _T("JudgeVHDFile::malloc:CacheBuffʧ��!"));
		return false;
	}
	memset(CacheBuff, 0,FILE_SECTOR_SIZE);
	printf("%llu\n", filesize.QuadPart);
	Ret=ReadSQData(VhdDrive, CacheBuff, SECTOR_SIZE, filesize.QuadPart - SECTOR_SIZE,&BackBytesCount);		
	if(!Ret)
	{		
		CloseHandle(VhdDrive);
		VhdDrive = NULL;
		free(CacheBuff);
		CacheBuff = NULL;
		CFuncs::WriteLogInfo(SLT_ERROR, _T("JudgeVHDFile::ReadSQData��ȡ����ʧ��!"));
		return false;	
	}
	CloseHandle(VhdDrive);
	VhdDrive = NULL;
	LVHD_footer vhdfoot = (LVHD_footer)&CacheBuff[0];
	if (vhdfoot->_MAGIC.LowPart != 0x656e6f63)
	{
		free(CacheBuff);
		CacheBuff = NULL;
		CFuncs::WriteLogInfo(SLT_ERROR, _T("JudgeVHDFile::Ѱַ���󣬴��ļ�����VHD���̣����ߴ�VHD�ǹ̶����̣�������������!"));
		return false;
	}
	/*if (vhdfoot->_Disktype[3] == 2)4Ϊ��֣�3Ϊ����
	{
		free(CacheBuff);
		CacheBuff = NULL;
		CFuncs::WriteLogInfo(SLT_ERROR, _T("JudgeVHDFile::�˴������Ͳ��Ƕ�̬����,�ǹ̶���!"));
		return false;
	}*/
	*vhdtype = vhdfoot->_Disktype[3];
	free(CacheBuff);
	CacheBuff = NULL;
	return true;
}
void  GetVirtualMachineInfo::GetDwodSize(UCHAR *source,DWORD *dest)
{
	for (int i=0;i<4;i++)
	{
		(*dest)=(*dest) | source[i]<<8*(3-i);
	}
}
void  GetVirtualMachineInfo::GetDwodtoDwod(DWORD *source,DWORD *dest)
{

	(*dest)=(*dest) | ((*source)&0xff000000)>>3*8;
	(*dest)=(*dest) | ((*source)&0xff0000)>>8;
	(*dest)=(*dest) | ((*source)&0xff00)<<8;
	(*dest)=(*dest) | ((*source)&0xff)<<3*8;

}
bool GetVirtualMachineInfo::VhdNameChange(LONGLONG VHDUUID, UCHAR *VhdHeadBuff, wchar_t *NtfsIncreVhdPathName)
{
	if ((VHDUUID) > 0)
	{
		UCHAR *IncrementalVhdPath = & VhdHeadBuff[64];

		for (DWORD i = 0 ; i < (SECTOR_SIZE - 64) ; i++)
		{
			if (IncrementalVhdPath[i] == 0 && IncrementalVhdPath[i+1] == 0 && IncrementalVhdPath[i+2] == 0 && IncrementalVhdPath[i+3] == 0 )
			{
				if (i < 400)
				{
					for (DWORD nl = 0; nl < i; nl += 2)
					{
						NtfsIncreVhdPathName[nl/2] = IncrementalVhdPath[nl] << 8 | IncrementalVhdPath[nl + 1];
					}
				}
				else
				{

					CFuncs::WriteLogInfo(SLT_ERROR, _T("VhdNameChange:IncreVhdPathName ����400 ʧ��!"));
					return false;
				}
				break;
			}
		}
	}
	return true;
}
bool GetVirtualMachineInfo::CacheParentVhdTable(HANDLE h_drive,UCHAR *CacheBuff,DWORD BatOffset,DWORD BatSize)
{
	bool bRet = false;
	DWORD BackBytesCount = NULL;
	DWORD ReadBatSize = (BatSize / SECTOR_SIZE + 1) * SECTOR_SIZE;
	bRet=ReadSQData(h_drive, &CacheBuff[0], ReadBatSize,
		BatOffset,
		&BackBytesCount);		
	if(!bRet)
	{			

		CFuncs::WriteLogInfo(SLT_ERROR, _T("CacheParentVhdTable:ReadSQData:��ȡ��ַ������Ŀ¼��ַʧ��!"));
		return false;	
	}

	return true;
}
bool GetVirtualMachineInfo::ParentVhdOneAddrChange(DWORD64 ChangeAddr,DWORD *VhdTable,DWORD BatBlockSize,DWORD64 *BackAddr,DWORD BatEntryMaxNumber)
{
	//ȫ�����ֽ����Ƚ�
	DWORD64 EntryNumber = NULL;
	DWORD64 BatOffset = NULL;
	*BackAddr = NULL;

	EntryNumber = ChangeAddr / BatBlockSize;
	if (EntryNumber > BatEntryMaxNumber)
	{
		CFuncs::WriteLogInfo(SLT_ERROR, _T("ParentVhdOneAddrChange:EntryNumber: ��ȡ�ı��ų���������Ŀ!"));
		return false;
	}
	BatOffset = ChangeAddr%BatBlockSize;

	if (VhdTable[EntryNumber] == 0xffffffff)
	{
		//CFuncs::WriteLogInfo(SLT_ERROR, _T("VhdOneAddrChange:EntryNumber: ���ַΪ��!"));
		return false;
	}
	DWORD EntryChangeAddr = NULL;
	GetDwodtoDwod(&VhdTable[EntryNumber], &EntryChangeAddr);//��С������ת���ɴ������,ȡ��һ������ַ	
	BatOffset += (EntryChangeAddr * SECTOR_SIZE + SECTOR_SIZE);
	*BackAddr = BatOffset;

	return true;
}
bool GetVirtualMachineInfo::FindParentVHDVirtual_GPT(HANDLE h_drive,DWORD *BatEntry,DWORD BatEntryTotalNum,DWORD BatBlockSize
	,UCHAR *CacheBuff,vector<DWORD64>& VirtualStartaddr, UCHAR vhdtype)
{
	memset(CacheBuff, 0, SECTOR_SIZE);
	DWORD64 VhdChangeAddr = NULL;
	DWORD64 VhdBackAddr = NULL;
	bool Readsq = true;
	bool Ret = false;
	DWORD BackBytesCount = NULL;
	LGPT_FB_TABLE GTFB = NULL;
	VhdChangeAddr = 2;

	while(Readsq)
	{
		VhdBackAddr = VhdChangeAddr * SECTOR_SIZE;
		if (vhdtype != 2)
		{
			if(!ParentVhdOneAddrChange(VhdChangeAddr * SECTOR_SIZE, BatEntry, BatBlockSize, &VhdBackAddr, BatEntryTotalNum))
			{
				CFuncs::WriteLogInfo(SLT_ERROR, _T("FindParentVHDVirtual_GPT:Virtual_to_Host_OneAddr: ��ȡ�����GPTת����ַʧ��!"));
				return false;
			}
		}
		
		VhdChangeAddr++;
		Ret = ReadSQData(h_drive, &CacheBuff[0], SECTOR_SIZE, VhdBackAddr, &BackBytesCount);		
		if(!Ret)
		{			
			CFuncs::WriteLogInfo(SLT_ERROR, _T("FindParentVHDVirtual_GPT:ReadSQData: ��ȡ�����GPT��ַ��Ϣʧ��!"));
			return false;	
		}
		GTFB = (LGPT_FB_TABLE)&CacheBuff[0];
		for (int i = 0;(GTFB->_GUID_TYPE[0] != 0) && (i < 4); i++)
		{
			if (GTFB->_GUID_TYPE[0] == 0x4433b9e5ebd0a0a2)
			{
				VirtualStartaddr.push_back(GTFB->_FB_Start_SQ);
			}
			if (i < 3)
			{
				GTFB++;
			}
		}
		if (GTFB->_FB_Start_SQ == 0)
		{
			Readsq = false;
		}
	}
	if (VirtualStartaddr.size() == NULL)
	{
		CFuncs::WriteLogInfo(SLT_ERROR, _T("FindParentVHDVirtual_GPT:GPT������ַΪ��,��Ѱʧ��!"));
		return false;
	}
	return true;
}
bool GetVirtualMachineInfo::FindParentVHDVirtual_Mbr(HANDLE h_drive,DWORD64 *VhdChangeAddr,DWORD *BatEntry,DWORD BatEntryTotalNum,DWORD BatBlockSize,
	UCHAR *CacheBuff,vector<DWORD64>& VirtualStartaddr, UCHAR vhdtype)
{
	memset(CacheBuff, 0, SECTOR_SIZE);
	DWORD64 VhdBackAddr = NULL;
	DWORD BackBytesCount = NULL;
	bool Ret = false;
	LMBR_Heads virmbr = NULL;
	VhdBackAddr = *VhdChangeAddr;
	if (vhdtype != 2)
	{
		if(!ParentVhdOneAddrChange((*VhdChangeAddr), BatEntry, BatBlockSize, &VhdBackAddr, BatEntryTotalNum))
		{
			CFuncs::WriteLogInfo(SLT_ERROR, _T("FindParentVHDVirtual_Mbr:Virtual_to_Host_OneAddr: ��ȡ�����MBR�׵�ַͷ����Ϣʧ��!"));
			return false;
		}
	}
	
	//	printf("*hostaddr %0.8x\n",hostaddr);
	Ret = ReadSQData(h_drive, &CacheBuff[0], SECTOR_SIZE, VhdBackAddr, &BackBytesCount);		
	if(!Ret)
	{			

		CFuncs::WriteLogInfo(SLT_ERROR, _T("FindParentVHDVirtual_Mbr:ReadSQData: ��ȡ�����MBR�׵�ַͷ����Ϣʧ��!"));
		return false;	
	}
	for (int i = 0; i < 64; i += 16)
	{
		virmbr = (LMBR_Heads)&CacheBuff[446+i];				
		if (virmbr->_MBR_Partition_Type == 0x05 || virmbr->_MBR_Partition_Type == 0x0f)
		{
			if (CacheBuff[0] == 0 && CacheBuff[1] == 0 && CacheBuff[2] == 0 && CacheBuff[3] == 0)
			{				
				*VhdChangeAddr = (*VhdChangeAddr + ((DWORD64)virmbr->_MBR_Sec_pre_pa));				
				FindParentVHDVirtual_Mbr(h_drive, VhdChangeAddr, BatEntry, BatEntryTotalNum, BatBlockSize,
					CacheBuff, VirtualStartaddr, vhdtype);
			} 
			else
			{							
				*VhdChangeAddr = ((DWORD64)(virmbr->_MBR_Sec_pre_pa));							
				FindParentVHDVirtual_Mbr(h_drive, VhdChangeAddr, BatEntry, BatEntryTotalNum, BatBlockSize,
					CacheBuff, VirtualStartaddr, vhdtype);
			}
		} 
		else if (virmbr->_MBR_Partition_Type == 0x00)
		{
			CFuncs::WriteLogInfo(SLT_INFORMATION, _T("FindParentVHDVirtual_Mbr ��ȡvirtualMBR���!"));
			return true;
		}
		else if (virmbr->_MBR_Partition_Type == 0x07)
		{
			if (CacheBuff[0] == 0x00 && CacheBuff[1] == 0x00 && CacheBuff[2] == 0x00 && CacheBuff[3] == 0x00)
			{			
				VirtualStartaddr.push_back((virmbr->_MBR_Sec_pre_pa + (*VhdChangeAddr)));
			}
			else
			{
				VirtualStartaddr.push_back(virmbr->_MBR_Sec_pre_pa);			
			}
		}
	}
	return true;
}
bool GetVirtualMachineInfo::GetParentVhdPatitionAddr(HANDLE h_VhdDrive,UCHAR *CacheFileRecoBuffer,vector<DWORD64>&VirtualStartaddr, UCHAR vhdtype
	, LONGLONG *VhdUUID,  wchar_t *NtfsIncreVhdPathName)
{
	DWORD dwError=NULL;
	bool Ret=false;
	DWORD BackBytesCount=NULL;
	DWORD batentrynum=NULL;//�洢�����ַ��Ŀ�ı���
	DWORD BatOffset = NULL;
	DWORD BatBlockSize = NULL;

	//��ȡͷ����Ϣ��ȡ��������Ϣ
	if (vhdtype != 2)
	{
		if (!GetVhdHeadInfor(h_VhdDrive, &batentrynum, &BatOffset, &BatBlockSize, VhdUUID, NtfsIncreVhdPathName))
		{
			CFuncs::WriteLogInfo(SLT_ERROR, "GetParentVhdPatitionAddr:GetVhdHeadInfor!");
			return false;
		}

		if (NULL == batentrynum)
		{
			CFuncs::WriteLogInfo(SLT_ERROR, "GetParentVhdPatitionAddr:batentrynum�����Ϊ��!");
			return false;
		}
	}
	
	//���仺������ַ�ڴ�
	UCHAR *EntryAddrBuff = NULL;
	DWORD *EntryAddr = NULL;//����DWOD����ָ�룬ȡһ��������ַ
	DWORD64 VhdBackAddr = SECTOR_SIZE;
	if (vhdtype != 2)
	{
		EntryAddrBuff = (UCHAR*)malloc(batentrynum * 4 + SECTOR_SIZE * 2);
		if (NULL == EntryAddrBuff)
		{
			CFuncs::WriteLogInfo(SLT_ERROR, "GetParentVhdPatitionAddr:malloc����EntryAddrBuff�ڴ�ʧ��!");
			return false;
		}
		memset(EntryAddrBuff, 0, (batentrynum * 4 + SECTOR_SIZE * 2));

		if(!CacheParentVhdTable(h_VhdDrive, EntryAddrBuff, BatOffset, batentrynum * 4))
		{

			free(EntryAddrBuff);
			EntryAddrBuff = NULL;
			CFuncs::WriteLogInfo(SLT_ERROR, "GetParentVhdPatitionAddr:CacheParentVhdTableʧ��!");
			return false;
		}

		EntryAddr = (DWORD*)&EntryAddrBuff[0];

		if(!ParentVhdOneAddrChange(1 * SECTOR_SIZE, EntryAddr, BatBlockSize, &VhdBackAddr, batentrynum))
		{

			free(EntryAddrBuff);
			EntryAddrBuff = NULL;
			CFuncs::WriteLogInfo(SLT_ERROR, _T("GetParentVhdPatitionAddr:ParentVhdOneAddrChange: ת�����������Ϣʧ��!"));
			return true;
		}
	}
	

	memset(CacheFileRecoBuffer, 0, SECTOR_SIZE);
	Ret = ReadSQData(h_VhdDrive, &CacheFileRecoBuffer[0], SECTOR_SIZE, VhdBackAddr
		, &BackBytesCount);		
	if(!Ret)
	{			
		if (vhdtype != 2)
		{
			free(EntryAddrBuff);
			EntryAddrBuff = NULL;
		}
		CFuncs::WriteLogInfo(SLT_ERROR, _T("GetParentVhdPatitionAddr:ReadSQData: ��ȡ������ڶ�������Ϣʧ��!"));
		return false;	
	}
	LGPT_Heads GptHead = (LGPT_Heads)&CacheFileRecoBuffer[0];
	if (GptHead->_Singed_name == 0x5452415020494645)
	{
		CFuncs::WriteLogInfo(SLT_INFORMATION, _T("����������GPT����"));
		if(!FindParentVHDVirtual_GPT(h_VhdDrive, EntryAddr, batentrynum, BatBlockSize, CacheFileRecoBuffer, VirtualStartaddr, vhdtype))
		{
			if (vhdtype != 2)
			{
				free(EntryAddrBuff);
				EntryAddrBuff = NULL;
			}
			CFuncs::WriteLogInfo(SLT_ERROR, _T("GetParentVhdPatitionAddr:vFindParentVHDVirtual_GPT: ��ȡ��������ڲ�GPTʧ��!"));
			return true;
		}
	}
	else
	{
		CFuncs::WriteLogInfo(SLT_INFORMATION, _T("����������MBR����"));
		DWORD64 ChangeAddr = NULL;
		if (!FindParentVHDVirtual_Mbr(h_VhdDrive, &ChangeAddr, EntryAddr, batentrynum, BatBlockSize, CacheFileRecoBuffer, VirtualStartaddr, vhdtype))
		{
			if (vhdtype != 2)
			{
				free(EntryAddrBuff);
				EntryAddrBuff = NULL;
			}
			CFuncs::WriteLogInfo(SLT_ERROR, _T("GetParentVhdPatitionAddr:FindParentVHDVirtual_Mbr: ��ȡ��������ڲ�MBRʧ��!"));
			return true;	
		}
	}
	

	if (vhdtype != 2)
	{
		free(EntryAddrBuff);
		EntryAddrBuff = NULL;
	}

	return true;
}
bool GetVirtualMachineInfo::GetDifferNTFSStartAddr(HANDLE VhdDrive, vector<DWORD64> &VirNTFSStartAddr, LONGLONG *VhdUUID,  wchar_t * IncreVhdPathName, UCHAR vhdtype)
{
	bool Ret  = false;
	DWORD BackBytesCount = NULL;
	*VhdUUID = NULL;
	DWORD batentrynum = NULL;//�洢�����ַ��Ŀ�ı���
	DWORD BatOffset = NULL;
	DWORD BatBlockSize = NULL;

	if (vhdtype != 2)
	{
		if (!GetVhdHeadInfor(VhdDrive, &batentrynum, &BatOffset, &BatBlockSize, VhdUUID, IncreVhdPathName))
		{
			CFuncs::WriteLogInfo(SLT_ERROR, "GetDifferNTFSStartAddr:GetVhdHeadInforʧ��!");
			return false;
		}
		if (NULL == batentrynum)
		{
			CFuncs::WriteLogInfo(SLT_ERROR, "GetDifferNTFSStartAddr:batentrynum�����Ϊ��!");
			return false;
		}
	}
	
	DWORD64 BackAddr = SECTOR_SIZE;
	UCHAR *EntryAddrBuff = NULL;
	DWORD *EntryAddr = NULL;
	if (vhdtype != 2)
	{
		//���仺������ַ�ڴ�
		EntryAddrBuff = (UCHAR*)malloc(batentrynum * 4 + SECTOR_SIZE * 2);
		if (NULL == EntryAddrBuff)
		{
			CFuncs::WriteLogInfo(SLT_ERROR, "GetDifferNTFSStartAddr:malloc����EntryAddrBuff�ڴ�ʧ��!");
			return false;
		}
		memset(EntryAddrBuff, 0, batentrynum * 4 + SECTOR_SIZE * 2);

		Ret=CacheParentVhdTable(VhdDrive, EntryAddrBuff, BatOffset, batentrynum * 4);
		if (!Ret)
		{
			free(EntryAddrBuff);
			EntryAddrBuff = NULL;
			CFuncs::WriteLogInfo(SLT_ERROR, "GetDifferNTFSStartAddr:CacheVhdTableDataʧ��!");
			return false;
		}
		
		EntryAddr = (DWORD*)&EntryAddrBuff[0];
		//��ȡ�ڶ��������жϷ�������


		if(!ParentVhdOneAddrChange(1 * SECTOR_SIZE,EntryAddr, BatBlockSize, &BackAddr,  batentrynum))
		{
			free(EntryAddrBuff);
			EntryAddrBuff = NULL;
			CFuncs::WriteLogInfo(SLT_ERROR, _T("GetDifferNTFSStartAddr:VhdOneAddrChange: ��ȡ�������Ϣʧ��!"));
			return true;
		}
	}
	

	UCHAR *FileRecoBuff = (UCHAR*) malloc(FILE_SECTOR_SIZE);
	if (NULL == FileRecoBuff)
	{
		if (vhdtype != 2)
		{
			free(EntryAddrBuff);
			EntryAddrBuff = NULL;
		}
		CFuncs::WriteLogInfo(SLT_ERROR, _T("GetDifferNTFSStartAddr:malloc: FileRecoBuffʧ��!"));
		return false;
	}
	memset(FileRecoBuff, 0,SECTOR_SIZE);

	Ret = ReadSQData(VhdDrive, &FileRecoBuff[0], SECTOR_SIZE, BackAddr
		, &BackBytesCount);		
	if(!Ret)
	{		
		free(FileRecoBuff);
		FileRecoBuff = NULL;
		if (vhdtype != 2)
		{
			free(EntryAddrBuff);
			EntryAddrBuff = NULL;
		}
		CFuncs::WriteLogInfo(SLT_ERROR, _T("GetDifferNTFSStartAddr:ReadSQData: ��ȡ������ڶ�������Ϣʧ��!"));
		return false;	
	}
	LGPT_Heads GptHead = (LGPT_Heads)&FileRecoBuff[0];
	//vector<DWORD64>VirtualStartaddr;//�洢������ڲ�������ַ
	if (GptHead->_Singed_name == 0x5452415020494645)
	{
		CFuncs::WriteLogInfo(SLT_INFORMATION, _T("����������GPT����"));
		if(!FindParentVHDVirtual_GPT(VhdDrive, EntryAddr, batentrynum, BatBlockSize, FileRecoBuff, VirNTFSStartAddr, vhdtype))
		{
			free(FileRecoBuff);
			FileRecoBuff = NULL;
			if (vhdtype != 2)
			{
				free(EntryAddrBuff);
				EntryAddrBuff = NULL;
			}
			CFuncs::WriteLogInfo(SLT_ERROR, _T("GetDifferNTFSStartAddr:Find_VHDVirtual_GPT: ��ȡ������ڲ�GPTʧ��!"));
			return true;
		}
	}
	else
	{
		CFuncs::WriteLogInfo(SLT_INFORMATION, _T("����������MBR����"));
		DWORD64 ChangeAddr=NULL;
		if (!FindParentVHDVirtual_Mbr(VhdDrive, &ChangeAddr, EntryAddr, batentrynum, BatBlockSize,FileRecoBuff, VirNTFSStartAddr, vhdtype))
		{
			free(FileRecoBuff);
			FileRecoBuff = NULL;
			if (vhdtype != 2)
			{
				free(EntryAddrBuff);
				EntryAddrBuff = NULL;
			}
			CFuncs::WriteLogInfo(SLT_ERROR, _T("GetDifferNTFSStartAddr:Find_VHDVirtual_Mbr: ��ȡ������ڲ�MBRʧ��!"));
			return true;	
		}
	}

	free(FileRecoBuff);
	FileRecoBuff = NULL;
	if (vhdtype != 2)
	{
		free(EntryAddrBuff);
		EntryAddrBuff = NULL;
	}

	return true;
}
bool GetVirtualMachineInfo::GetVHDVirtualNTFSStartAddr(LONGLONG *VhdUUID, vector<DWORD64> &VirtualNTFSStartaddr, wchar_t * IncreVhdPathName
	, HANDLE VhdDrive, string VHDSalfPath, UCHAR *vhdtype)
{
	bool Ret = false;
	DWORD BackBytesCount = NULL;
	DWORD dwError = NULL;

	if ((*VhdUUID) > 0)
	{
		//���ǲ����
		if (NULL == IncreVhdPathName)
		{
			CFuncs::WriteLogInfo(SLT_ERROR, "GetVHDVirtualNTFSStartAddr:IncreVhdPathName: ��ַΪ��ʧ��!");
			return false;
		}
		
		string strFileName;
		wstring wpathname = wstring(IncreVhdPathName);
		if (wpathname.find(L":\\") == string::npos)
		{		
			strFileName.append(VHDSalfPath);
			 strFileName.append(CUrlConver::WstringToString(IncreVhdPathName));
		}
		else
		{
			strFileName.append(CUrlConver::WstringToString(IncreVhdPathName));
		}
		if (!JudgeVHDFile(strFileName.c_str(), vhdtype))
		{
			CFuncs::WriteLogInfo(SLT_ERROR, "GetVHDVirtualNTFSStartAddr:JudgeVHDFileʧ��!");
			return false;
		}
		HANDLE h_VhdDrive=CreateFile(strFileName.c_str(),
			GENERIC_READ,
			FILE_SHARE_READ |FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			0,
			NULL);
		if (h_VhdDrive == INVALID_HANDLE_VALUE) 
		{

			dwError=GetLastError();
			CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVHDVirtualNTFSStartAddr h_VhdDrive = CreateFile��ȡIncreVhdPathName���ʧ��!,\
											   ���󷵻���: dwError = %d"), dwError);
			return false;
		}


		UCHAR *ParentVhdBuff = (UCHAR*)malloc(FILE_SECTOR_SIZE);
		if (NULL == ParentVhdBuff)
		{
			CloseHandle(h_VhdDrive);
			h_VhdDrive = NULL;
			CFuncs::WriteLogInfo(SLT_ERROR, "GetVHDVirtualNTFSStartAddr:malloc: ParentVhdBuffʧ��!");
			return false;
		}
		memset(ParentVhdBuff, 0, FILE_SECTOR_SIZE);
		if(!GetParentVhdPatitionAddr(h_VhdDrive, ParentVhdBuff, VirtualNTFSStartaddr, *vhdtype, VhdUUID, IncreVhdPathName))
		{
			CloseHandle(h_VhdDrive);
			h_VhdDrive = NULL;
			free(ParentVhdBuff);
			ParentVhdBuff = NULL;
			CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVHDVirtualNTFSStartAddr:GetParentVhdPatitionAddr��ȡ�������������ʼ��ַʧ��!"));
			return false;
		}

		free(ParentVhdBuff);
		ParentVhdBuff = NULL;

		if (NULL == VirtualNTFSStartaddr.size())
		{
			
			if (GetVHDVirtualNTFSStartAddr(VhdUUID, VirtualNTFSStartaddr, IncreVhdPathName, VhdDrive, VHDSalfPath, vhdtype))
			{
				CloseHandle(h_VhdDrive);
				h_VhdDrive = NULL;
				CFuncs::WriteLogInfo(SLT_INFORMATION, "GetVHDVirtualNTFSStartAddr:GetVHDVirtualNTFSStartAddr���سɹ�!");
				return true;
			}
			else
			{
				CloseHandle(h_VhdDrive);
				h_VhdDrive = NULL;
				CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVHDVirtualNTFSStartAddr:GetVHDVirtualNTFSStartAddr����ʧ��!"));
				return false;
			}		

		}
		else
		{
			CloseHandle(h_VhdDrive);
			h_VhdDrive = NULL;
			CFuncs::WriteLogInfo(SLT_INFORMATION, "GetVHDVirtualNTFSStartAddr:�ڸ������ҵ�NTFS��ʼ��ַ!");
			return true;
		}


	} 
	else if((*VhdUUID) == 0)
	{
		//���ǻ�����
		if(!GetDifferNTFSStartAddr(VhdDrive, VirtualNTFSStartaddr, VhdUUID, IncreVhdPathName, *vhdtype))
		{

			CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVHDVirtualNTFSStartAddr:GetDifferNTFSStartAddr ʧ��!"));
			return false;
		}

		if (NULL == VirtualNTFSStartaddr.size())
		{
			if ((*VhdUUID) > 0)
			{
				if (GetVHDVirtualNTFSStartAddr(VhdUUID, VirtualNTFSStartaddr, IncreVhdPathName, VhdDrive, VHDSalfPath, vhdtype))
				{
					CFuncs::WriteLogInfo(SLT_INFORMATION, "GetVHDVirtualNTFSStartAddr:GetVHDVirtualNTFSStartAddr���سɹ�!");
					return true;
				}
				else
				{
					CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVHDVirtualNTFSStartAddr:GetVHDVirtualNTFSStartAddr����ʧ��!"));
					return false;
				}
			}
			else
			{
				CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVHDVirtualNTFSStartAddr:VirtualNTFSStartaddr����������Ϊ�� ʧ��!"));
				return false;
			}

		}
	}

	return true;
}
bool GetVirtualMachineInfo::GetIncrementMFTStartAddr(HANDLE p_drive,UCHAR *CacheBuff,DWORD64 *VirtualStartMftAddr,UCHAR *VirtualCuNum
	,DWORD64 VirtualStartNTFS, UCHAR vhdtype, LONGLONG *VhdUUID, wchar_t * IncreVhdPathName)
{
	DWORD dwError = NULL;
	bool Ret = false;
	DWORD BackBytesCount = NULL;
	DWORD Parentbatentrynum = NULL;//�洢�����ַ��Ŀ�ı���
	DWORD ParentBatOffset = NULL;
	DWORD ParentBatBlockSize = NULL;

	if (vhdtype != 2)
	{
		if (!GetVhdHeadInfor(p_drive, &Parentbatentrynum, &ParentBatOffset, &ParentBatBlockSize, VhdUUID, IncreVhdPathName))
		{
			CFuncs::WriteLogInfo(SLT_ERROR, "GetIncrementMFTStartAddr:batentrynum�����Ϊ��!");
			return false;
		}
		if (NULL == Parentbatentrynum)
		{
			CFuncs::WriteLogInfo(SLT_ERROR, "GetIncrementMFTStartAddr:batentrynum�����Ϊ��!");
			return false;
		}
	}
	UCHAR *ParentEntryAddrBuff = NULL;
	DWORD *ParentEntryAddr = NULL;//����DWOD����ָ�룬ȡһ��������ַ
	DWORD64 ParentVhdBackAddr = VirtualStartNTFS * SECTOR_SIZE;
	//���仺������ַ�ڴ�
	if (vhdtype != 2)
	{
		ParentEntryAddrBuff = (UCHAR*)malloc(Parentbatentrynum * 4 + SECTOR_SIZE * 2);
		if (NULL == ParentEntryAddrBuff)
		{
			CFuncs::WriteLogInfo(SLT_ERROR, "GetIncrementMFTStartAddr:malloc����EntryAddrBuff�ڴ�ʧ��!");
			return false;
		}
		memset(ParentEntryAddrBuff, 0, Parentbatentrynum * 4 + SECTOR_SIZE * 2);

		if(!CacheParentVhdTable(p_drive, ParentEntryAddrBuff, ParentBatOffset, Parentbatentrynum * 4))
		{
			CFuncs::WriteLogInfo(SLT_ERROR, "GetIncrementMFTStartAddr:CacheParentVhdTableʧ��!");
			return false;
		}

		ParentEntryAddr = (DWORD*)&ParentEntryAddrBuff[0];

		if(!ParentVhdOneAddrChange(VirtualStartNTFS * SECTOR_SIZE, ParentEntryAddr, ParentBatBlockSize, &ParentVhdBackAddr, Parentbatentrynum))
		{
			CFuncs::WriteLogInfo(SLT_ERROR, _T("GetIncrementMFTStartAddr:ParentVhdOneAddrChange: ת����������ڶ�������Ϣʧ��!"));
			return false;
		}
	}
	

	memset(CacheBuff, 0, SECTOR_SIZE);
	Ret = ReadSQData(p_drive, &CacheBuff[0], SECTOR_SIZE, ParentVhdBackAddr
		, &BackBytesCount);		
	if(!Ret)
	{			

		CFuncs::WriteLogInfo(SLT_ERROR, _T("GetIncrementMFTStartAddr:ReadSQData: ��ȡ������ڶ�������Ϣʧ��!"));
		return false;	
	}
	LNTFS_TABLES virtualNtfs = NULL;
	virtualNtfs = (LNTFS_TABLES)&CacheBuff[0];
	(*VirtualCuNum) = virtualNtfs->_Single_Cu_Num;
	(*VirtualStartMftAddr) = virtualNtfs->_MFT_Start_CU;

	free(ParentEntryAddrBuff);
	ParentEntryAddrBuff = NULL;
	return true;

}
bool GetVirtualMachineInfo::GetBasicMftAddr(HANDLE VhdDrive ,LONGLONG* VhdUUID, wchar_t *IncreVhdPathName, DWORD64 VirtualStartNTFS, UCHAR *VirtualCuNum
	, DWORD64 *StartMftAddr, UCHAR vhdtype)
{
	
	bool Ret=false;
	DWORD BackBytesCount = NULL;
	*VhdUUID = NULL;
	DWORD batentrynum = NULL;//�洢�����ַ��Ŀ�ı���
	DWORD BatOffset = NULL;
	DWORD BatBlockSize = NULL;
	if (vhdtype != 2)
	{
		if (!GetVhdHeadInfor(VhdDrive, &batentrynum, &BatOffset, &BatBlockSize, VhdUUID, IncreVhdPathName))
		{
			CFuncs::WriteLogInfo(SLT_ERROR, "GetBasicMftAddr:GetVhdHeadInforʧ��!");
			return false;
		}
		if (NULL == batentrynum)
		{
			CFuncs::WriteLogInfo(SLT_ERROR, "GetBasicMftAddr:batentrynum�����Ϊ��!");
			return false;
		}
	}

	UCHAR *EntryAddrBuff = NULL;
	DWORD *EntryAddr = NULL;//����DWOD����ָ�룬ȡһ��������ַ
	DWORD64 VirtualBackAddr = VirtualStartNTFS * SECTOR_SIZE;
	//���仺������ַ�ڴ�
	if (vhdtype != 2)
	{
		EntryAddrBuff= (UCHAR*)malloc(batentrynum * 4 + SECTOR_SIZE * 2);
		if (NULL == EntryAddrBuff)
		{
			CFuncs::WriteLogInfo(SLT_ERROR, "GetBasicMftAddr:malloc����EntryAddrBuff�ڴ�ʧ��!");
			return false;
		}
		memset(EntryAddrBuff, 0, batentrynum * 4 + SECTOR_SIZE * 2);

		Ret=CacheParentVhdTable(VhdDrive,EntryAddrBuff, BatOffset, batentrynum * 4);
		if (!Ret)
		{
			free(EntryAddrBuff);
			EntryAddrBuff = NULL;
			CFuncs::WriteLogInfo(SLT_ERROR, "GetBasicMftAddr:CacheVhdTableDataʧ��!");
			return false;
		}

		EntryAddr = (DWORD*)&EntryAddrBuff[0];

		if (!ParentVhdOneAddrChange(VirtualStartNTFS * SECTOR_SIZE, EntryAddr, BatBlockSize, &VirtualBackAddr, batentrynum))
		{
			free(EntryAddrBuff);
			EntryAddrBuff = NULL;
			CFuncs::WriteLogInfo(SLT_ERROR, _T("GetBasicMftAddr:VhdOneAddrChange: �����ת����ʼntfs��ַʧ��!"));
			return false;
		}
		free(EntryAddrBuff);
		EntryAddrBuff = NULL;
	}
	

	UCHAR *CacheBuff = (UCHAR*) malloc(SECTOR_SIZE + 1);
	if (NULL == CacheBuff)
	{
		CFuncs::WriteLogInfo(SLT_ERROR, _T("GetBasicMftAddr:malloc: CacheBuffʧ��!"));
		return false;
	}
	memset(CacheBuff, 0 ,SECTOR_SIZE + 1);
	Ret = ReadSQData(VhdDrive, &CacheBuff[0], SECTOR_SIZE, VirtualBackAddr,
		&BackBytesCount);		
	if(!Ret)
	{			
		free(CacheBuff);
		CacheBuff = NULL;
		CFuncs::WriteLogInfo(SLT_ERROR, _T("GetBasicMftAddr:ReadSQData: ��ȡ�����ת�������ʼntfs��ַ��һ����ʧ��!"));
		return false;	
	}
	LNTFS_TABLES virtualNtfs = NULL;
	virtualNtfs = (LNTFS_TABLES)&CacheBuff[0];
	(*VirtualCuNum) = virtualNtfs->_Single_Cu_Num;
	(*StartMftAddr) = virtualNtfs->_MFT_Start_CU;

	free(CacheBuff);
	CacheBuff = NULL;

	return true;
}
bool GetVirtualMachineInfo::GetVirtualStartMftAddr(HANDLE VhdDrive, LONGLONG *VhdUUID, DWORD64 *VirtualMftStartaddr, wchar_t * IncreVhdPathName, UCHAR* m_VirtualCuNum
	, DWORD64 StarNTFSAddr, string VHDSalfPath, UCHAR *vhdtype)
{
	bool Ret = false;
	DWORD BackBytesCount = NULL;
	DWORD dwError = NULL;

	if ((*VhdUUID) > 0)
	{
		//���ǲ����
		if (NULL == IncreVhdPathName)
		{
			CFuncs::WriteLogInfo(SLT_ERROR, "GetVirtualStartMftAddr:IncreVhdPathName: ��ַΪ��ʧ��!");
			return false;
		}
		string strFileName;
		wstring wpathname = wstring(IncreVhdPathName);
		if (wpathname.find(L":\\") == string::npos)
		{		
			strFileName.append(VHDSalfPath);
			strFileName.append(CUrlConver::WstringToString(IncreVhdPathName));
		}
		else
		{
			strFileName.append(CUrlConver::WstringToString(IncreVhdPathName));
		}
		if (!JudgeVHDFile(strFileName.c_str(), vhdtype))
		{
			CFuncs::WriteLogInfo(SLT_ERROR, "GetVirtualStartMftAddr:JudgeVHDFileʧ��!");
			return false;
		}
		HANDLE h_VhdDrive=CreateFile(strFileName.c_str(),
			GENERIC_READ,
			FILE_SHARE_READ |FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			0,
			NULL);
		if (h_VhdDrive == INVALID_HANDLE_VALUE) 
		{

			dwError=GetLastError();
			CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualStartMftAddr h_VhdDrive = CreateFile��ȡIncreVhdPathName���ʧ��!,\
											   ���󷵻���: dwError = %d"), dwError);
			return false;
		}


		UCHAR *ParentVhdBuff = (UCHAR*)malloc(FILE_SECTOR_SIZE);
		if (NULL == ParentVhdBuff)
		{
			CloseHandle(h_VhdDrive);
			h_VhdDrive = NULL;
			CFuncs::WriteLogInfo(SLT_ERROR, "GetVirtualStartMftAddr:malloc: ParentVhdBuffʧ��!");
			return false;
		}
		memset(ParentVhdBuff, 0, FILE_SECTOR_SIZE);
		if(!GetIncrementMFTStartAddr(h_VhdDrive, ParentVhdBuff, VirtualMftStartaddr, m_VirtualCuNum, StarNTFSAddr, *vhdtype, VhdUUID, IncreVhdPathName))
		{
			CloseHandle(h_VhdDrive);
			h_VhdDrive = NULL;
			free(ParentVhdBuff);
			ParentVhdBuff = NULL;
			CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualStartMftAddr:GetParentVhdMftAddr��ȡ�������������ʼ��ַʧ��!"));
			return false;
		}

		free(ParentVhdBuff);
		ParentVhdBuff = NULL;

		if (NULL == (*VirtualMftStartaddr))
		{			
			
			if (GetVirtualStartMftAddr(VhdDrive, VhdUUID, VirtualMftStartaddr, IncreVhdPathName, m_VirtualCuNum, StarNTFSAddr, VHDSalfPath, vhdtype))
			{
				CloseHandle(h_VhdDrive);
				h_VhdDrive = NULL;
				CFuncs::WriteLogInfo(SLT_INFORMATION, "GetVirtualStartMftAddr:GetVirtualStartMftAddr���سɹ�!");
				return true;
			}
			else
			{
				CloseHandle(h_VhdDrive);
				h_VhdDrive = NULL;
				CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualStartMftAddr:GetVirtualStartMftAddr����ʧ��!"));
				return false;
			}
						
		}
		else
		{
			CloseHandle(h_VhdDrive);
			h_VhdDrive = NULL;
			CFuncs::WriteLogInfo(SLT_INFORMATION, "GetVirtualStartMftAddr:�ڸ������ҵ�Mft��ʼ��ַ!");
			return true;
		}


	} 
	else if((*VhdUUID) == 0)
	{
		//���ǻ�����
		if(!GetBasicMftAddr(VhdDrive, VhdUUID, IncreVhdPathName, StarNTFSAddr, m_VirtualCuNum
			, VirtualMftStartaddr, *vhdtype))
		{

			CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualStartMftAddr:GetDifferMftStartAddr ʧ��!"));
			return false;
		}
		if (NULL == (*VirtualMftStartaddr))
		{
			if ((*VhdUUID) > 0)
			{
				if (GetVirtualStartMftAddr(VhdDrive, VhdUUID, VirtualMftStartaddr, IncreVhdPathName
					,m_VirtualCuNum, StarNTFSAddr, VHDSalfPath, vhdtype))
				{
					CFuncs::WriteLogInfo(SLT_INFORMATION, "GetVirtualStartMftAddr:GetVirtualStartMftAddr���سɹ�!");
					return true;
				}
				else
				{
					CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualStartMftAddr:GetVirtualStartMftAddr����ʧ��!"));
					return false;
				}
			}
			else
			{
				CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualStartMftAddr:VirtualNTFSStartaddr����������Ϊ�� ʧ��!"));
				return false;
			}

		}
	}

	return true;
}
bool GetVirtualMachineInfo::GetIncrementAllMFTStartAddr(HANDLE p_drive, UCHAR *CacheBuff,DWORD64 StartNTFSAddr,DWORD64 StratMFTAddr,UCHAR VirtualCuNum
	,vector<LONG64> &v_VirtualStartMftAddr,vector<DWORD64> &v_VirtualStartMftLen, UCHAR VHDtype, LONGLONG *VhdUUID, wchar_t *IncreVhdPathName)
{
	DWORD dwError=NULL;
	bool Ret=false;
	DWORD BackBytesCount=NULL;
	DWORD Parentbatentrynum=NULL;//�洢�����ַ��Ŀ�ı���
	DWORD ParentBatOffset=NULL;
	DWORD ParentBatBlockSize=NULL;

	if (VHDtype != 2)
	{
		if (!GetVhdHeadInfor(p_drive, &Parentbatentrynum, &ParentBatOffset, &ParentBatBlockSize, VhdUUID, IncreVhdPathName))
		{
			CFuncs::WriteLogInfo(SLT_ERROR, "GetIncrementAllMFTStartAddr:GetVhdHeadInforʧ��!");
			return false;
		}
		if (NULL == Parentbatentrynum)
		{
			CFuncs::WriteLogInfo(SLT_ERROR, "GetIncrementAllMFTStartAddr:batentrynum�����Ϊ��!");
			return false;
		}
	}

	UCHAR *ParentEntryAddrBuff = NULL;
	DWORD *ParentEntryAddr=NULL;//����DWOD����ָ�룬ȡһ��������ַ
	DWORD64 ParentVhdBackAddr = (StartNTFSAddr*SECTOR_SIZE+StratMFTAddr*VirtualCuNum*SECTOR_SIZE);
	//���仺������ַ�ڴ�
	if (VHDtype != 2)
	{
		ParentEntryAddrBuff = (UCHAR*)malloc(Parentbatentrynum*4+SECTOR_SIZE*2);
		if (NULL == ParentEntryAddrBuff)
		{
			CFuncs::WriteLogInfo(SLT_ERROR, "GetIncrementAllMFTStartAddr:malloc����EntryAddrBuff�ڴ�ʧ��!");
			return false;
		}
		memset(ParentEntryAddrBuff,0,Parentbatentrynum*4+SECTOR_SIZE*2);

		if(!CacheParentVhdTable(p_drive,ParentEntryAddrBuff,ParentBatOffset,Parentbatentrynum*4))
		{
			free(ParentEntryAddrBuff);
			ParentEntryAddrBuff=NULL;
			CFuncs::WriteLogInfo(SLT_ERROR, "GetIncrementAllMFTStartAddr:CacheParentVhdTableʧ��!");
			return false;
		}

		ParentEntryAddr=(DWORD*)&ParentEntryAddrBuff[0];


		if(!ParentVhdOneAddrChange((StartNTFSAddr*SECTOR_SIZE+StratMFTAddr*VirtualCuNum*SECTOR_SIZE), ParentEntryAddr,ParentBatBlockSize
			,&ParentVhdBackAddr,Parentbatentrynum))
		{
			free(ParentEntryAddrBuff);
			ParentEntryAddrBuff=NULL;
			CFuncs::WriteLogInfo(SLT_ERROR, _T("GetIncrementAllMFTStartAddr:ParentVhdOneAddrChange: ת����������ڶ�������Ϣʧ��!"));
			return false;
		}

		memset(CacheBuff, 0,FILE_SECTOR_SIZE);
		Ret = ReadSQData(p_drive,&CacheBuff[0],SECTOR_SIZE,ParentVhdBackAddr
			, &BackBytesCount);		
		if(!Ret)
		{			
			free(ParentEntryAddrBuff);
			ParentEntryAddrBuff=NULL;
			CFuncs::WriteLogInfo(SLT_ERROR, _T("GetIncrementAllMFTStartAddr:ReadSQData: ��ȡ������ڶ�������Ϣʧ��!"));
			return false;	
		}
		ParentVhdBackAddr=NULL;
		if(!ParentVhdOneAddrChange((StartNTFSAddr*SECTOR_SIZE+StratMFTAddr*VirtualCuNum*SECTOR_SIZE+SECTOR_SIZE),ParentEntryAddr,ParentBatBlockSize
			,&ParentVhdBackAddr,Parentbatentrynum))
		{
			free(ParentEntryAddrBuff);
			ParentEntryAddrBuff=NULL;
			CFuncs::WriteLogInfo(SLT_ERROR, _T("GetIncrementAllMFTStartAddr:ParentVhdOneAddrChange: ת����������ڶ�������Ϣʧ��!"));
			return false;
		}
		free(ParentEntryAddrBuff);
		ParentEntryAddrBuff=NULL;
		Ret = ReadSQData(p_drive,&CacheBuff[SECTOR_SIZE],SECTOR_SIZE,ParentVhdBackAddr
			, &BackBytesCount);		
		if(!Ret)
		{			

			CFuncs::WriteLogInfo(SLT_ERROR, _T("GetIncrementAllMFTStartAddr:ReadSQData: ��ȡ������ڶ�������Ϣʧ��!"));
			return false;	
		}
	}
	else
	{
		Ret = ReadSQData(p_drive,&CacheBuff[0],FILE_SECTOR_SIZE,ParentVhdBackAddr
			, &BackBytesCount);		
		if(!Ret)
		{			

			CFuncs::WriteLogInfo(SLT_ERROR, _T("GetIncrementAllMFTStartAddr:ReadSQData: ��ȡ������ڶ�������Ϣʧ��!"));
			return false;	
		}

	}
	

	

	if(!GetMFTAddr(NULL, v_VirtualStartMftAddr, v_VirtualStartMftLen, NULL, CacheBuff, false))
	{
		
		CFuncs::WriteLogInfo(SLT_ERROR, "GetIncrementAllMFTStartAddr:GetMFTAddr:��ȡ���������MFT��ʼ��ַʧ��");
		return true;
	}



	return true;

}
bool GetVirtualMachineInfo::GetBasicAllMftAddr(HANDLE VhdDrive ,LONGLONG *VhdUUID, wchar_t* IncreVhdPathName, DWORD64 VirtualStartNTFS, DWORD64 StartMftAddr
	, UCHAR VirtualCuNum, UCHAR *CacheBuff, vector<LONG64> &v_VirtualStartMftAddr,vector<DWORD64> &v_VirtualStartMftLen, UCHAR VHDtype)
{
	
	bool Ret=false;
	DWORD BackBytesCount = NULL;
	(*VhdUUID) = NULL;
	DWORD batentrynum = NULL;//�洢�����ַ��Ŀ�ı���
	DWORD BatOffset = NULL;
	DWORD BatBlockSize = NULL;

	if (VHDtype != 2)
	{
		if (!GetVhdHeadInfor(VhdDrive, &batentrynum, &BatOffset, &BatBlockSize, VhdUUID, IncreVhdPathName))
		{
			CFuncs::WriteLogInfo(SLT_ERROR, "GetBasicAllMftAddr:GetVhdHeadInforʧ��!");
			return false;
		}
		if (NULL == batentrynum)
		{
			CFuncs::WriteLogInfo(SLT_ERROR, "GetBasicAllMftAddr:batentrynum�����Ϊ��!");
			return false;
		}
	}
	UCHAR *EntryAddrBuff = NULL;
	DWORD *EntryAddr = NULL;//����DWOD����ָ�룬ȡһ��������ַ
	DWORD64 VirtualBackAddr = (VirtualStartNTFS * SECTOR_SIZE + StartMftAddr * VirtualCuNum * SECTOR_SIZE);
	//���仺������ַ�ڴ�
	if (VHDtype != 2)
	{
		EntryAddrBuff = (UCHAR*)malloc(batentrynum * 4 + SECTOR_SIZE * 2);
		if (NULL == EntryAddrBuff)
		{
			CFuncs::WriteLogInfo(SLT_ERROR, "GetBasicAllMftAddr:malloc����EntryAddrBuff�ڴ�ʧ��!");
			return false;
		}
		memset(EntryAddrBuff, 0, batentrynum * 4 + SECTOR_SIZE * 2);

		Ret=CacheParentVhdTable(VhdDrive, EntryAddrBuff, BatOffset, batentrynum * 4);
		if (!Ret)
		{
			free(EntryAddrBuff);
			EntryAddrBuff = NULL;
			CFuncs::WriteLogInfo(SLT_ERROR, "GetBasicAllMftAddr:CacheVhdTableDataʧ��!");
			return false;
		}

		EntryAddr = (DWORD*)&EntryAddrBuff[0];
		VirtualBackAddr = NULL;
		memset(CacheBuff,0,FILE_SECTOR_SIZE);
		if(!ParentVhdOneAddrChange((VirtualStartNTFS * SECTOR_SIZE + StartMftAddr * VirtualCuNum * SECTOR_SIZE), EntryAddr,BatBlockSize 
			, &VirtualBackAddr, batentrynum))
		{
			free(EntryAddrBuff);
			EntryAddrBuff = NULL;
			CFuncs::WriteLogInfo(SLT_ERROR, _T("GetBasicAllMftAddr:VhdOneAddrChange: ת����ʼMft�ļ���¼��ַʧ��!"));
			return false;
		}
		Ret = ReadSQData(VhdDrive, &CacheBuff[0], SECTOR_SIZE, VirtualBackAddr,
			&BackBytesCount);		
		if(!Ret)
		{			
			free(EntryAddrBuff);
			EntryAddrBuff = NULL;
			CFuncs::WriteLogInfo(SLT_ERROR, _T("GetBasicAllMftAddr:ReadSQData: ��ȡ��ʼMft�ļ���¼��ַʧ��!"));
			return false;	
		}
		VirtualBackAddr = NULL;
		if(!ParentVhdOneAddrChange((VirtualStartNTFS * SECTOR_SIZE + StartMftAddr * VirtualCuNum * SECTOR_SIZE + SECTOR_SIZE),
			EntryAddr, BatBlockSize, &VirtualBackAddr, batentrynum))
		{
			free(EntryAddrBuff);
			EntryAddrBuff = NULL;
			CFuncs::WriteLogInfo(SLT_ERROR, _T("GetBasicAllMftAddr:VhdOneAddrChange: ת����ʼMft�ļ���¼��ַʧ��!"));
			return false;
		}
		Ret = ReadSQData(VhdDrive, &CacheBuff[SECTOR_SIZE], SECTOR_SIZE, VirtualBackAddr,
			&BackBytesCount);		
		if(!Ret)
		{			
			free(EntryAddrBuff);
			EntryAddrBuff = NULL;
			CFuncs::WriteLogInfo(SLT_ERROR, _T("GetBasicAllMftAddr:ReadSQData: ��ȡ��ʼMft�ļ���¼��ַʧ��!"));
			return false;	
		}
		free(EntryAddrBuff);
		EntryAddrBuff = NULL;
	}
	else
	{
		Ret = ReadSQData(VhdDrive, &CacheBuff[0], FILE_SECTOR_SIZE, VirtualBackAddr,
			&BackBytesCount);		
		if(!Ret)
		{			
			CFuncs::WriteLogInfo(SLT_ERROR, _T("GetBasicAllMftAddr:ReadSQData: ��ȡ��ʼMft�ļ���¼��ַʧ��!"));
			return false;	
		}
	}
	


	if(!GetMFTAddr(NULL, v_VirtualStartMftAddr, v_VirtualStartMftLen, NULL, CacheBuff, false))
	{
		CFuncs::WriteLogInfo(SLT_ERROR, "GetBasicAllMftAddr:GetMFTAddr:��ȡ��������������MFT��ʼ��ַʧ��");
	}
	if (v_VirtualStartMftLen.size() != v_VirtualStartMftAddr.size())
	{
		CFuncs::WriteLogInfo(SLT_ERROR, "GetBasicAllMftAddr:v_VirtualStartMftLen:��v_VirtualStartMftAddr��ͬ����ʧ��");
		return false;
	}

	return true;
}
bool GetVirtualMachineInfo::VirGetAllMftAddr(HANDLE VhdDrive, LONGLONG *VhdUUID, wchar_t *IncreVhdPathName, DWORD64 StartNtfsAddr, DWORD64 StartMftAddr
	, UCHAR VirtualCuNum, vector<LONG64> &v_VirtualStartMftAddr, vector<DWORD64> &v_VirtualStartMftLen, string VHDSalfPath, UCHAR *vhdtype)
{
	bool Ret = false;
	DWORD BackBytesCount = NULL;
	DWORD dwError = NULL;

	if ((*VhdUUID) > 0)
	{
		//���ǲ����
		if (NULL == IncreVhdPathName)
		{
			CFuncs::WriteLogInfo(SLT_ERROR, "VirGetAllMftAddr:IncreVhdPathName: ��ַΪ��ʧ��!");
			return false;
		}
		string strFileName;
		wstring wpathname = wstring(IncreVhdPathName);
		if (wpathname.find(L":\\") == string::npos)
		{		
			strFileName.append(VHDSalfPath);
			strFileName.append(CUrlConver::WstringToString(IncreVhdPathName));
		}
		else
		{
			strFileName.append(CUrlConver::WstringToString(IncreVhdPathName));
		}
		if (!JudgeVHDFile(strFileName.c_str(), vhdtype))
		{
			CFuncs::WriteLogInfo(SLT_ERROR, "VirGetAllMftAddr:JudgeVHDFileʧ��!");
			return false;
		}
		HANDLE h_VhdDrive=CreateFile(strFileName.c_str(),
			GENERIC_READ,
			FILE_SHARE_READ |FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			0,
			NULL);
		if (h_VhdDrive == INVALID_HANDLE_VALUE) 
		{

			dwError=GetLastError();
			CFuncs::WriteLogInfo(SLT_ERROR, _T("VirGetAllMftAddr h_VhdDrive = CreateFile��ȡIncreVhdPathName���ʧ��!,\
											   ���󷵻���: dwError = %d"), dwError);
			return false;
		}


		UCHAR *ParentVhdBuff = (UCHAR*)malloc(FILE_SECTOR_SIZE);
		if (NULL == ParentVhdBuff)
		{
			CloseHandle(h_VhdDrive);
			h_VhdDrive = NULL;
			CFuncs::WriteLogInfo(SLT_ERROR, "VirGetAllMftAddr:malloc: ParentVhdBuffʧ��!");
			return false;
		}
		memset(ParentVhdBuff, 0, FILE_SECTOR_SIZE);
		if(!GetIncrementAllMFTStartAddr(h_VhdDrive, ParentVhdBuff, StartNtfsAddr, StartMftAddr, VirtualCuNum, v_VirtualStartMftAddr, v_VirtualStartMftLen
			, *vhdtype, VhdUUID, IncreVhdPathName))
		{
			CloseHandle(h_VhdDrive);
			h_VhdDrive = NULL;
			free(ParentVhdBuff);
			ParentVhdBuff = NULL;
			CFuncs::WriteLogInfo(SLT_ERROR, _T("VirGetAllMftAddr:GetParentVhdMftAddr��ȡ�������������ʼ��ַʧ��!"));
			return false;
		}

		free(ParentVhdBuff);
		ParentVhdBuff = NULL;

		if (NULL == v_VirtualStartMftAddr.size() || NULL == v_VirtualStartMftLen.size())
		{
			
			if (VirGetAllMftAddr(VhdDrive, VhdUUID, IncreVhdPathName, StartNtfsAddr, StartMftAddr, VirtualCuNum, v_VirtualStartMftAddr
				, v_VirtualStartMftLen, VHDSalfPath, vhdtype))
			{
				CloseHandle(h_VhdDrive);
				h_VhdDrive = NULL;
				CFuncs::WriteLogInfo(SLT_INFORMATION, "VirGetAllMftAddr:VirGetAllMftAddr���سɹ�!");
				return true;
			}
			else
			{
				CloseHandle(h_VhdDrive);
				h_VhdDrive = NULL;
				CFuncs::WriteLogInfo(SLT_ERROR, _T("VirGetAllMftAddr:VirGetAllMftAddr����ʧ��!"));
				return false;
			}	

		}
		else
		{
			CloseHandle(h_VhdDrive);
			h_VhdDrive = NULL;
			CFuncs::WriteLogInfo(SLT_INFORMATION, "VirGetAllMftAddr:�ڸ������ҵ�Mft��ʼ��ַ!");
			return true;
		}


	} 
	else if((*VhdUUID) == 0)
	{
		UCHAR *CacheBuff = (UCHAR*) malloc(FILE_SECTOR_SIZE);
		if (NULL == CacheBuff)
		{
			CFuncs::WriteLogInfo(SLT_ERROR, "VirGetAllMftAddr:malloc: CacheBuffʧ��!");
			return false;
		}
		memset(CacheBuff, 0, FILE_SECTOR_SIZE);
		//���ǻ�����
		if(!GetBasicAllMftAddr(VhdDrive, VhdUUID, IncreVhdPathName, StartNtfsAddr, StartMftAddr, VirtualCuNum
			, CacheBuff, v_VirtualStartMftAddr, v_VirtualStartMftLen, *vhdtype))
		{
			free(CacheBuff);
			CacheBuff = NULL;
			CFuncs::WriteLogInfo(SLT_ERROR, _T("VirGetAllMftAddr:GetBasicAllMftAddr ʧ��!"));
			return false;
		}
		free(CacheBuff);
		CacheBuff = NULL;
		if (NULL == v_VirtualStartMftAddr.size() || NULL == v_VirtualStartMftLen.size())
		{
			if ((*VhdUUID) > 0)
			{
				if (VirGetAllMftAddr(VhdDrive, VhdUUID, IncreVhdPathName, StartNtfsAddr, StartMftAddr, VirtualCuNum, v_VirtualStartMftAddr
					, v_VirtualStartMftLen, VHDSalfPath, vhdtype))
				{
					CFuncs::WriteLogInfo(SLT_INFORMATION, "VirGetAllMftAddr:VirGetAllMftAddr���سɹ�!");
					return true;
				}
				else
				{
					CFuncs::WriteLogInfo(SLT_ERROR, _T("VirGetAllMftAddr:VirGetAllMftAddr����ʧ��!"));
					return false;
				}
			}
			else
			{
				CFuncs::WriteLogInfo(SLT_ERROR, _T("VirGetAllMftAddr:v_VirtualStartMftAddr v_VirtualStartMftLen����������Ϊ�� ʧ��!"));
				return false;
			}

		}else
		{
			CFuncs::WriteLogInfo(SLT_INFORMATION, "VirGetAllMftAddr:�ڻ��������ҵ�Mft���е�ַ!");
			return true;
		}
	}

	return true;
}
bool GetVirtualMachineInfo::GetVHDVirtualFileAddr(HANDLE VhdDrive ,DWORD64 VirtualStartNTFS, LONG64 VirStartMftRfAddr, DWORD *BatEntry
	, DWORD BatEntryMaxNumber, DWORD BatBlockSize, UCHAR *CacheBuff, vector<DWORD> &H20FileRefer, UCHAR VirtualCuNum, vector<string> checkfilename
	, DWORD *ParentMft, vector<LONG64>&fileh80datarun, vector<DWORD>&fileh80datalen, string &fileh80data, DWORD Rerefer, string &FileName
	, DWORD64 *FileRealSize, UCHAR VHDtype)
{
	*ParentMft = NULL;
	*FileRealSize = NULL;
	FileName.clear();
	fileh80datarun.clear();
	fileh80datalen.clear();
	fileh80data.clear();
	H20FileRefer.clear();
	bool Ret=false;
	DWORD64 VirtualBackAddr = NULL;
	DWORD BackBytesCount = NULL;
	LFILE_Head_Recoding File_head_recod = NULL;
	LATTRIBUTE_HEADS ATTriBase = NULL;
	bool Found = false;
	LAttr_30H H30 = NULL;
	LAttr_20H H20 = NULL;
	UCHAR *H30_NAMES = NULL;
	UCHAR *H80_data = NULL;
	DWORD AttributeSize = NULL;
	DWORD FirstAttriSize = NULL;

	memset(CacheBuff, 0, FILE_SECTOR_SIZE);
	if (VHDtype != 2)
	{
		for (int i = 0;i < 2; i++)
		{
			VirtualBackAddr = NULL;
			if(!ParentVhdOneAddrChange((VirtualStartNTFS * SECTOR_SIZE + VirStartMftRfAddr * VirtualCuNum * SECTOR_SIZE + Rerefer * FILE_SECTOR_SIZE + SECTOR_SIZE * i)
				, BatEntry, BatBlockSize, &VirtualBackAddr, BatEntryMaxNumber))
			{
				//CFuncs::WriteLogInfo(SLT_ERROR, _T("VHDVirtualDiskAnaly:VhdOneAddrChange: ת����ʼMft�ļ���¼��ַʧ��!"));
				return true;
			}
			Ret = ReadSQData(VhdDrive, &CacheBuff[i*SECTOR_SIZE], SECTOR_SIZE, VirtualBackAddr,
				&BackBytesCount);		
			if(!Ret)
			{			
				CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVHDVirtualFileAddr:ReadSQData: ��ȡ��ʼMft�ļ���¼��ַʧ��!"));
				return false;	
			}
		}
	}
	else
	{
		Ret = ReadSQData(VhdDrive, &CacheBuff[0], FILE_SECTOR_SIZE, (VirtualStartNTFS * SECTOR_SIZE + VirStartMftRfAddr 
			* VirtualCuNum * SECTOR_SIZE + Rerefer * FILE_SECTOR_SIZE),
			&BackBytesCount);		
		if(!Ret)
		{			
			CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVHDVirtualFileAddr:ReadSQData: ��ȡ��ʼMft�ļ���¼��ַʧ��!"));
			return false;	
		}
	}
	

	File_head_recod = (LFILE_Head_Recoding)&CacheBuff[0];

	if(File_head_recod->_FILE_Index == 0x454c4946 && File_head_recod->_Flags[0] != 0)
	{
		RtlCopyMemory(&CacheBuff[510], &CacheBuff[File_head_recod->_Update_Sequence_Number[0]+2], 2);//������������	
		RtlCopyMemory(&CacheBuff[1022],&CacheBuff[File_head_recod->_Update_Sequence_Number[0]+4], 2);
		RtlCopyMemory(&FirstAttriSize, &File_head_recod->_First_Attribute_Dev[0], 2);
		if (FirstAttriSize > (FILE_SECTOR_SIZE - sizeof(ATTRIBUTE_HEADS)))
		{
			CFuncs::WriteLogInfo(SLT_ERROR, "GetVHDVirtualFileAddr::File_head_recod->_First_Attribute_Dev[0] > FILE_SECTOR_SIZEʧ��!");
			return false;
		}
		while ((FirstAttriSize + AttributeSize) < FILE_SECTOR_SIZE)
		{
			ATTriBase = (LATTRIBUTE_HEADS)&CacheBuff[FirstAttriSize + AttributeSize];
			if(ATTriBase->_Attr_Type != 0xffffffff)
			{
				if (ATTriBase->_Attr_Type == 0x20)
				{
					DWORD h20Length = NULL;
					switch(ATTriBase->_PP_Attr)
					{
					case 0:
						if (ATTriBase->_AttrName_Length == 0)
						{
							h20Length = 24;
						} 
						else
						{
							h20Length = 24 + 2 * ATTriBase->_AttrName_Length;
						}
						break;
					case 0x01:
						if (ATTriBase->_AttrName_Length == 0)
						{
							h20Length = 64;
						} 
						else
						{
							h20Length = 64 + 2 * ATTriBase->_AttrName_Length;
						}
						break;
					}
					if (h20Length > (ATTriBase->_Attr_Length))
					{
						CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualFileAddr:h20Length > (ATTriBase->_Attr_Length)ʧ��!"));
						return false;
					}
					if (ATTriBase->_PP_Attr == 0)
					{
						H20 = (LAttr_20H)((UCHAR*)&ATTriBase[0] + h20Length);
						while (H20->_H20_TYPE != 0)
						{						
						
							if (H20->_H20_TYPE == 0x80)
							{
								H20FileRefer.push_back(H20->_H20_FILE_Reference_Num.LowPart);

							}else if (H20->_H20_TYPE == 0)
							{
								break;
							}
							else if (H20->_H20_TYPE > 0xFF)
							{
								break;
							}
							if(H20->_H20_Attr_Name_Length * 2 > 0)
							{
								if ((H20->_H20_Attr_Name_Length * 2 + 26) % 8 != 0)
								{
									h20Length += (((H20->_H20_Attr_Name_Length * 2 + 26) / 8) * 8 + 8);
								}
								else if ((H20->_H20_Attr_Name_Length * 2 + 26) % 8 == 0)
								{
									h20Length += (H20->_H20_Attr_Name_Length * 2 + 26);
								}
							}
							else
							{
								h20Length += 32;
							}
							if (h20Length > (ATTriBase->_Attr_Length))
							{
								break;
							}
								H20 = (LAttr_20H)((UCHAR*)&ATTriBase[0] + h20Length);

						}
					} 
					else if (ATTriBase->_PP_Attr == 1)
					{
						UCHAR *H20Data = NULL;
						DWORD64 H20DataRun = NULL;
						H20Data = (UCHAR*)&ATTriBase[0];
						DWORD H20Offset = ATTriBase->TWOATTRIBUTEHEAD.NP_head._NPN_RunList_Offset[0];

						if (H20Offset > (FILE_SECTOR_SIZE - AttributeSize - FirstAttriSize))
						{
							CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVHDVirtualFileAddr:H20Offset������Χʧ��!"));
							return false;
						}

						if (H20Data[H20Offset] != 0 && H20Data[H20Offset] < 0x50)
						{
							UCHAR adres_fig = H20Data[H20Offset] >> 4;
							UCHAR len_fig = H20Data[H20Offset] & 0xf;
							for (int w = adres_fig; w > 0; w--)
							{
								H20DataRun = H20DataRun | (H20Data[H20Offset + w + len_fig] << (8 * (w - 1)));
							}
						}					
						UCHAR *H20CancheBuff = (UCHAR*)malloc(SECTOR_SIZE * VirtualCuNum + SECTOR_SIZE);
						if (NULL == H20CancheBuff)
						{
							CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVHDVirtualFileAddr:malloc: H20CancheBuffʧ��!"));
							return false;
						}
						memset(H20CancheBuff, 0, SECTOR_SIZE * VirtualCuNum);
						if (VHDtype != 2)
						{
							for (int i = 0; i < VirtualCuNum; i++)
							{					
								VirtualBackAddr = NULL;
								if(!ParentVhdOneAddrChange((VirtualStartNTFS * SECTOR_SIZE + H20DataRun * VirtualCuNum * SECTOR_SIZE + SECTOR_SIZE * i)
									, BatEntry, BatBlockSize, &VirtualBackAddr, BatEntryMaxNumber))
								{
									free(H20CancheBuff);
									H20CancheBuff = NULL;
									//CFuncs::WriteLogInfo(SLT_ERROR, _T("VHDVirtualDiskAnaly:VhdOneAddrChange: ת����ʼMft�ļ���¼��ַʧ��!"));
									return true;
								}
								Ret = ReadSQData(VhdDrive, &H20CancheBuff[i * SECTOR_SIZE], SECTOR_SIZE, VirtualBackAddr,
									&BackBytesCount);		
								if(!Ret)
								{	
									free(H20CancheBuff);
									H20CancheBuff = NULL;
									CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVHDVirtualFileAddr:ReadSQData: ��ȡ��ʼMft�ļ���¼��ַʧ��!"));
									return false;	
								}
							}
						}
						else
						{
							Ret = ReadSQData(VhdDrive, &H20CancheBuff[0], SECTOR_SIZE * VirtualCuNum, VirtualStartNTFS * SECTOR_SIZE + H20DataRun * VirtualCuNum * SECTOR_SIZE,
								&BackBytesCount);		
							if(!Ret)
							{	
								free(H20CancheBuff);
								H20CancheBuff = NULL;
								CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVHDVirtualFileAddr:ReadSQData: ��ȡ��ʼMft�ļ���¼��ַʧ��!"));
								return false;	
							}
						}
						
						h20Length = 0;
						H20 = (LAttr_20H)&H20CancheBuff[h20Length];
						while (H20->_H20_TYPE != 0)
						{
							
							H20 = (LAttr_20H)&H20CancheBuff[h20Length];
							if (H20->_H20_TYPE == 0x80)
							{
								H20FileRefer.push_back(H20->_H20_FILE_Reference_Num.LowPart);

							}
							else if (H20->_H20_TYPE == 0)
							{
								break;
							}
							else if (H20->_H20_TYPE > 0xFF)
							{
								break;
							}
							

							if(H20->_H20_Attr_Name_Length * 2 > 0)
							{
								if ((H20->_H20_Attr_Name_Length * 2 + 26) % 8 != 0)
								{
									h20Length += (((H20->_H20_Attr_Name_Length * 2 + 26) / 8) * 8 + 8);
								}
								else if ((H20->_H20_Attr_Name_Length * 2 + 26) % 8 == 0)
								{
									h20Length += (H20->_H20_Attr_Name_Length * 2 + 26);
								}
							}
							else
							{
								h20Length += 32;
							}
							if (h20Length > (DWORD)(SECTOR_SIZE * VirtualCuNum))
							{
								break;
							}
						}	
						free(H20CancheBuff);
						H20CancheBuff = NULL;
					}
				}
				if (!Found)
				{
					if (ATTriBase->_Attr_Type == 0x30)
					{
						DWORD H30Size = NULL;
						switch(ATTriBase->_PP_Attr)
						{
						case 0:
							if (ATTriBase->_AttrName_Length == 0)
							{
								H30 = (LAttr_30H)((UCHAR*)&ATTriBase[0] + 24);
								H30_NAMES = (UCHAR*)&ATTriBase[0] + 24 + 66;
								H30Size = 24 + 66 + AttributeSize + FirstAttriSize;
							} 
							else
							{
								H30 = (LAttr_30H)((UCHAR*)&ATTriBase[0] + 24 + 2 * ATTriBase->_AttrName_Length);
								H30_NAMES = (UCHAR*)&ATTriBase[0] + 24 + 2 * ATTriBase->_AttrName_Length+66;
								H30Size = 24 + 2 * ATTriBase->_AttrName_Length + 66 + AttributeSize + FirstAttriSize;
							}
							break;
						case 0x01:
							if (ATTriBase->_AttrName_Length == 0)
							{
								H30 = (LAttr_30H)((UCHAR*)&ATTriBase[0] + 64);
								H30_NAMES = (UCHAR*)&ATTriBase[0] + 64 + 66;
								H30Size = 64 + 66 + AttributeSize + FirstAttriSize;
							} 
							else
							{
								H30 = (LAttr_30H)((UCHAR*)&ATTriBase[0] + 64 + 2 * ATTriBase->_AttrName_Length);
								H30_NAMES = (UCHAR*)&ATTriBase[0] + 64 + 2 * ATTriBase->_AttrName_Length + 66;
								H30Size = 64 + 2 * ATTriBase->_AttrName_Length + 66 + AttributeSize + FirstAttriSize;
							}
							break;
						}
						if ((FILE_SECTOR_SIZE - H30Size) < 0)
						{
							CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVHDVirtualFileAddr::(FILE_SECTOR_SIZE - H30Size)ʧ��!"));
							return false;
						}
						DWORD H30FileNameLen = H30->_H30_FILE_Name_Length * 2;
						if (H30FileNameLen > (FILE_SECTOR_SIZE - H30Size) || NULL == H30FileNameLen)
						{
							CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVHDVirtualFileAddr::������Χʧ��!"));
							return false;
						}
						string filename;
						if(!UnicodeToZifu(&H30_NAMES[0],filename,(H30->_H30_FILE_Name_Length * 2)))
						{

							CFuncs::WriteLogInfo(SLT_ERROR, "GetVHDVirtualFileAddr����Unicode_To_Zifu::ת��ʧ��!");
							return false;
						}
						vector<string>::iterator viter;
						for (viter = checkfilename.begin(); viter != checkfilename.end(); viter ++)
						{
							if (filename.rfind(*viter) != string::npos)
							{
								size_t posion = filename.rfind(*viter);
								size_t c_posion = NULL;
								c_posion = filename.length() - posion;
								if (viter->length() == c_posion)
								{
									Found = true;
									break;
								}
							}
						}				
						if (Found)
						{
							CFuncs::WriteLogInfo(SLT_INFORMATION, "GetVHDVirtualFileAddr ���ļ���¼�ο�����:%lu",File_head_recod->_FR_Refer);
							*ParentMft = NULL;
							RtlCopyMemory(ParentMft,&H30->_H30_Parent_FILE_Reference[0],4);																		
								
							FileName.append((char*)&H30_NAMES[0],(H30->_H30_FILE_Name_Length*2));

							if (H20FileRefer.size() > 0)
							{
								vector<DWORD>::iterator vec;
								for (vec = H20FileRefer.begin(); vec < H20FileRefer.end(); vec ++)
								{
									if (*vec != File_head_recod->_FR_Refer)
									{
										CFuncs::WriteLogInfo(SLT_INFORMATION, "GetVHDVirtualFileAddr ���ļ���¼H80�ض�λ��H20�У��ض�λ�ļ��ο�����:%lu", *vec);
									}
									else
									{
										H20FileRefer.erase(vec);//��ͬ�ľ�û�ض�λ������Ϊ��
									}
								}

							}

							
						}			

																																												
						
					}
				}
				if (Found)
				{
					DWORD H80_datarun_len = NULL;
					LONG64 H80_datarun = NULL;
					if (ATTriBase->_Attr_Type == 0x80)
					{
						bool FirstIn = true;
						if (ATTriBase->_PP_Attr == 0x01)
						{
							
							(*FileRealSize) = ((*FileRealSize) + ATTriBase->TWOATTRIBUTEHEAD.NP_head._NPN_Attr_T_Size);//ȡ�ô��ļ�����ʵ��С
							H80_data = (UCHAR*)&ATTriBase[0];
							DWORD OFFSET = NULL;
							RtlCopyMemory(&OFFSET, &ATTriBase->TWOATTRIBUTEHEAD.NP_head._NPN_RunList_Offset[0], 2);
							if (OFFSET > (FILE_SECTOR_SIZE - FirstAttriSize - AttributeSize))
							{
								CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVHDVirtualFileAddr::OFFSET������Χ!"));
								return false;
							}
							if (H80_data[OFFSET] != 0 && H80_data[OFFSET] < 0x50)
							{					
								while(OFFSET < ATTriBase->_Attr_Length)
								{
									H80_datarun_len = NULL;
									H80_datarun = NULL;
									if (H80_data[OFFSET] > 0 && H80_data[OFFSET] < 0x50)
									{
										UCHAR adres_fig = H80_data[OFFSET] >> 4;
										UCHAR len_fig = H80_data[OFFSET] & 0xf;
										for(int w = len_fig; w > 0; w--)
										{
											H80_datarun_len = H80_datarun_len | (H80_data[OFFSET + w] << (8 * (w - 1)));
										}
										if (H80_datarun_len > 0)
										{
											fileh80datalen.push_back(H80_datarun_len);
										} 
										else
										{
											CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVHDVirtualFileAddr::H80_datarun_lenΪ0!"));
											return false;
										}

										for (int w = adres_fig; w > 0; w--)
										{
											H80_datarun = H80_datarun | (H80_data[OFFSET + w + len_fig] << (8 * (w - 1)));
										}
										if (H80_data[OFFSET + adres_fig + len_fig] > 127)
										{
											if (adres_fig == 3)
											{
												H80_datarun = ~(H80_datarun ^ 0xffffff);
											}
											if (adres_fig == 2)
											{
												H80_datarun = ~(H80_datarun ^ 0xffff);

											}

										} 
										if (FirstIn)
										{
											if (H80_datarun > 0)
											{
												fileh80datarun.push_back(H80_datarun);
											} 
											else
											{
												CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVHDVirtualFileAddr::H80_datarunΪ0��Ϊ��������!"));
												return false;
											}
											FirstIn = false;
										}
										else
										{
											if (fileh80datarun.size() > 0)
											{
												H80_datarun = fileh80datarun[fileh80datarun.size() - 1] + H80_datarun;
												fileh80datarun.push_back(H80_datarun);
											}
										}
										
										OFFSET = OFFSET + adres_fig + len_fig + 1;
									}
									else
									{
										break;
									}

								}								
							}

						}
						else if(ATTriBase->_PP_Attr == 0)
						{
							H80_data = (UCHAR*)&ATTriBase[0];	
							if (ATTriBase->TWOATTRIBUTEHEAD.P_head._PN_AttrBody_Length < (FILE_SECTOR_SIZE - AttributeSize - FirstAttriSize - 24))
							{
								fileh80data.append((char*)&H80_data[24],ATTriBase->TWOATTRIBUTEHEAD.P_head._PN_AttrBody_Length);
							}
							

						}

					}
				}
				if (ATTriBase->_Attr_Length < (FILE_SECTOR_SIZE - AttributeSize - FirstAttriSize) && ATTriBase->_Attr_Length > 0)
				{

					AttributeSize += ATTriBase->_Attr_Length;

				}  
				else
				{
					CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVHDVirtualFileAddr::������:%lu,ATTriBase[i_attr]->_Attr_Length���ȹ����Ϊ0!")
						,ATTriBase->_Attr_Length);
					return false;
				}
			}
			else if (ATTriBase->_Attr_Type == 0xffffffff)
			{
				if (!Found)
				{
					H20FileRefer.clear();
				}				
				memset(CacheBuff, 0, FILE_SECTOR_SIZE);
				break;
			}
			else if(ATTriBase->_Attr_Type > 0xff && ATTriBase->_Attr_Type < 0xffffffff)
			{
				CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVHDVirtualFileAddr:��ȡATTriBase[i_attr]->_Attr_Type����0xffffffff����"));
				return false;
			}

		}
	}
	return true;
}
bool GetVirtualMachineInfo::VHDWriteLargeFile(HANDLE VhdhDrive, vector<LONG64> FileH80Addr, vector<DWORD> FileH80Len, UCHAR VirtualCuNum, DWORD *BatEntry
	, DWORD BatEntryMaxNumber, DWORD BatBlockSize, UCHAR *WriteBuff , const wchar_t *FileDir, DWORD64 VirPatition
	, DWORD64 fileRealSize, UCHAR Vhdtype)
{
	if (FileH80Addr.size() != FileH80Len.size())
	{
		CFuncs::WriteLogInfo(SLT_ERROR, _T("VHDWriteLargeFile :FileH80Addr.size() != FileH80Len.size()ʧ��!"));
		return false;
	}

	BOOL Ret = false;
	DWORD BackBytesCount = NULL;
	DWORD64 VirtualBackAddr = NULL;

	HANDLE hFile_recov = ::CreateFileW(FileDir, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, NULL, NULL);
	if (hFile_recov == INVALID_HANDLE_VALUE)
	{
		CFuncs::WriteLogInfo(SLT_ERROR, _T("VHDWriteLargeFile :CreateFileWʧ��!"));
		return false;
	}
	LARGE_INTEGER WriteIndex = {NULL};
	DWORD nNumberOfBytesWritten = NULL;
	for (DWORD H80Num = NULL; H80Num < FileH80Addr.size(); H80Num++)
	{
		DWORD FileAddrAddNumber = NULL;
		for (DWORD SectorCuNum = NULL; SectorCuNum < FileH80Len[H80Num]; SectorCuNum ++)
		{
			memset(WriteBuff, 0, VirtualCuNum * SECTOR_SIZE);
			if (Vhdtype != 2)
			{
				for (DWORD Sector = NULL; Sector < VirtualCuNum; Sector ++)
				{
					VirtualBackAddr = NULL;
					if (!ParentVhdOneAddrChange((VirPatition * SECTOR_SIZE + FileH80Addr[H80Num] * VirtualCuNum * SECTOR_SIZE + FileAddrAddNumber * SECTOR_SIZE)
						, BatEntry, BatBlockSize, &VirtualBackAddr, BatEntryMaxNumber))
					{
						CloseHandle(hFile_recov);
						hFile_recov = NULL;
						CFuncs::WriteLogInfo(SLT_ERROR, _T("VHDWriteLargeFile :VdiOneAddrChangeʧ��!"));
						return true;
					}

					Ret=ReadSQData(VhdhDrive, &WriteBuff[Sector * SECTOR_SIZE], SECTOR_SIZE,
						VirtualBackAddr,
						&BackBytesCount);		
					if(!Ret)
					{		
						CloseHandle(hFile_recov);
						hFile_recov = NULL;
						CFuncs::WriteLogInfo(SLT_ERROR, "VHDWriteLargeFile:ReadSQDataʧ��,������%d", GetLastError());
						return false;	
					}
					FileAddrAddNumber ++;
				}
			}
			else
			{
				Ret=ReadSQData(VhdhDrive, &WriteBuff[0], SECTOR_SIZE * VirtualCuNum,
					VirPatition * SECTOR_SIZE + FileH80Addr[H80Num] * VirtualCuNum * SECTOR_SIZE + FileAddrAddNumber * SECTOR_SIZE,
					&BackBytesCount);		
				if(!Ret)
				{		
					CloseHandle(hFile_recov);
					hFile_recov = NULL;
					CFuncs::WriteLogInfo(SLT_ERROR, "VHDWriteLargeFile:ReadSQDataʧ��,������%d", GetLastError());
					return false;	
				}
				FileAddrAddNumber += VirtualCuNum;
			}
			
			Ret=SetFilePointerEx(hFile_recov,
				WriteIndex,
				NULL,
				FILE_BEGIN);
			if(!Ret)
			{
				CloseHandle(hFile_recov);
				hFile_recov = NULL;
				CFuncs::WriteLogInfo(SLT_ERROR, "VirtualVdiWriteLargeFile:SetFilePointerExʧ��,������%d", GetLastError());
				return false;	
			}
			if (((DWORD64)WriteIndex.QuadPart + (VirtualCuNum * SECTOR_SIZE)) > fileRealSize)
			{
				Ret = ::WriteFile(hFile_recov, WriteBuff, (DWORD)(fileRealSize - WriteIndex.QuadPart), &nNumberOfBytesWritten, NULL);
				if(!Ret)
				{	
					CFuncs::WriteLogInfo(SLT_ERROR, "VHDWriteLargeFile:WriteFileʧ��,������%d", GetLastError());
					(void)CloseHandle(hFile_recov);
					hFile_recov = NULL;
					return false;
				}
				CloseHandle(hFile_recov);
				hFile_recov = NULL;
				return true;

			} 
			else
			{
				Ret = ::WriteFile(hFile_recov, WriteBuff, (DWORD)(VirtualCuNum * SECTOR_SIZE), &nNumberOfBytesWritten, NULL);
				if(!Ret)
				{	
					CFuncs::WriteLogInfo(SLT_ERROR, "VHDWriteLargeFile:WriteFileʧ��,������%d", GetLastError());
					(void)CloseHandle(hFile_recov);
					hFile_recov = NULL;
					return false;
				}
			}
			
			WriteIndex.QuadPart += (VirtualCuNum * SECTOR_SIZE);
		}

	}
	CloseHandle(hFile_recov);
	hFile_recov = NULL;
	return true;
}
bool GetVirtualMachineInfo::GetVHDFileNameAndPath(DWORD64 VirtualNtfs, vector<LONG64> VirtualStartMFTaddr, vector<DWORD64> VirtualStartMFTaddrLen
	, UCHAR VirtualCuNum, DWORD ParentMFT, UCHAR *CacheBuffer, string& VirtualFilePath, DWORD *BatEntry, DWORD BatEntryMaxNumber, DWORD BatBlockSize
	, string FileName, HANDLE VhdDrive, UCHAR Vhdtype)
{
	DWORD MFTnumber = NULL;
	bool  bRet = false;
	DWORD BackBytesCount = NULL;
	LFILE_Head_Recoding File_head_recod = NULL;
	LATTRIBUTE_HEADS ATTriBase = NULL;
	LAttr_30H H30 = NULL;
	UCHAR *H30_NAMES = NULL;


	string StrTem;
		
	StrTem.append("//");
	StrTem.append(FileName);

	File_head_recod = (LFILE_Head_Recoding)&CacheBuffer[0];
	

	MFTnumber = ParentMFT;


	DWORD numbers = NULL;
	while (MFTnumber != 5 && MFTnumber != 0)
	{
		DWORD AttributeSize = NULL;
		DWORD FirstAttriSize = NULL;
		if (numbers > 100)
		{
			CFuncs::WriteLogInfo(SLT_ERROR, "GetVHDFileNameAndPath:numbers ·���ļ�����30��������!");
			return false;
		}
		DWORD64 MftLenAdd = NULL;
		LONG64 MftAddr = NULL;

		for (DWORD FMft = 0; FMft < VirtualStartMFTaddrLen.size(); FMft++)
		{
			if ((MFTnumber * 2) <= (VirtualStartMFTaddrLen[FMft] * VirtualCuNum + MftLenAdd))
			{
				MftAddr = (VirtualStartMFTaddr[FMft] * VirtualCuNum + ((MFTnumber * 2) - MftLenAdd));
				break;
			} 
			else
			{
				MftLenAdd += (VirtualStartMFTaddrLen[FMft] * VirtualCuNum);
			}
		}
	
		DWORD64 VirtualBackAddr = NULL;
		memset(CacheBuffer, 0, FILE_SECTOR_SIZE);
		if (Vhdtype != 2)
		{
			for (int i = 0; i < 2; i++)
			{
				VirtualBackAddr = NULL;
				if(!ParentVhdOneAddrChange((VirtualNtfs * SECTOR_SIZE + MftAddr * SECTOR_SIZE + SECTOR_SIZE * i)
					, BatEntry, BatBlockSize, &VirtualBackAddr, BatEntryMaxNumber))
				{
					CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVHDFileNameAndPath:VhdOneAddrChange: ת����ʼMft�ļ���¼��ַʧ��!"));
					return true;
				}
				bRet = ReadSQData(VhdDrive, &CacheBuffer[i * SECTOR_SIZE], SECTOR_SIZE, VirtualBackAddr,
					&BackBytesCount);		
				if(!bRet)
				{			
					CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVHDFileNameAndPath:ReadSQData: ��ȡ��ʼMft�ļ���¼��ַʧ��!"));
					return false;	
				}
			}
		}
		else
		{
			bRet = ReadSQData(VhdDrive, &CacheBuffer[0], FILE_SECTOR_SIZE, VirtualNtfs * SECTOR_SIZE + MftAddr * SECTOR_SIZE,
				&BackBytesCount);		
			if(!bRet)
			{			
				CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVHDFileNameAndPath:ReadSQData: ��ȡ��ʼMft�ļ���¼��ַʧ��!"));
				return false;	
			}
		}
		
		if (File_head_recod->_FILE_Index == 0)
		{
			CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVHDFileNameAndPath �ҵ������ļ���¼����!"));
			return true;
		} 
		else if (File_head_recod->_FILE_Index != 0x454c4946 && File_head_recod->_FILE_Index > 0)
		{
			CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVHDFileNameAndPath �ҵ������ļ���¼����!"));
			return true;
		} 
		else if(File_head_recod->_FILE_Index == 0x454c4946)
		{
			RtlCopyMemory(&CacheBuffer[510], &CacheBuffer[File_head_recod->_Update_Sequence_Number[0]+2], 2);//������������	
			RtlCopyMemory(&CacheBuffer[1022],&CacheBuffer[File_head_recod->_Update_Sequence_Number[0]+4], 2);
			RtlCopyMemory(&FirstAttriSize, &File_head_recod->_First_Attribute_Dev[0], 2);
			if (FirstAttriSize > (FILE_SECTOR_SIZE - sizeof(ATTRIBUTE_HEADS)))
			{
				CFuncs::WriteLogInfo(SLT_ERROR, "GetVHDFileNameAndPath::File_head_recod->_First_Attribute_Dev[0] > FILE_SECTOR_SIZEʧ��!");
				return false;
			}

			string H30temName;
			while ((FirstAttriSize + AttributeSize) < FILE_SECTOR_SIZE)
			{
				ATTriBase = (LATTRIBUTE_HEADS)&CacheBuffer[FirstAttriSize + AttributeSize];
				if(ATTriBase->_Attr_Type != 0xffffffff)
				{
					if (ATTriBase->_Attr_Type == 0x30)
					{
						DWORD H30Size = NULL;
						switch(ATTriBase->_PP_Attr)
						{
						case 0:
							if (ATTriBase->_AttrName_Length == 0)
							{
								H30 = (LAttr_30H)((UCHAR*)&ATTriBase[0] + 24);
								H30_NAMES = (UCHAR*)&ATTriBase[0] + 24 + 66;
								H30Size = 24 + 66 + AttributeSize + FirstAttriSize;
							} 
							else
							{
								H30 = (LAttr_30H)((UCHAR*)&ATTriBase[0] + 24 + 2 * ATTriBase->_AttrName_Length);
								H30_NAMES = (UCHAR*)&ATTriBase[0] + 24 + 2 * ATTriBase->_AttrName_Length+66;
								H30Size = 24 + 2 * ATTriBase->_AttrName_Length + 66 + AttributeSize + FirstAttriSize;
							}
							break;
						case 0x01:
							if (ATTriBase->_AttrName_Length == 0)
							{
								H30 = (LAttr_30H)((UCHAR*)&ATTriBase[0] + 64);
								H30_NAMES = (UCHAR*)&ATTriBase[0] + 64 + 66;
								H30Size = 64 + 66 + AttributeSize + FirstAttriSize;
							} 
							else
							{
								H30 = (LAttr_30H)((UCHAR*)&ATTriBase[0] + 64 + 2 * ATTriBase->_AttrName_Length);
								H30_NAMES = (UCHAR*)&ATTriBase[0] + 64 + 2 * ATTriBase->_AttrName_Length + 66;
								H30Size = 64 + 2 * ATTriBase->_AttrName_Length + 66 + AttributeSize + FirstAttriSize;
							}
							break;
						}
						if ((FILE_SECTOR_SIZE - H30Size) < 0)
						{
							CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVHDFileNameAndPath::(FILE_SECTOR_SIZE - H30Size)ʧ��!"));
							return false;
						}
						DWORD H30FileNameLen = H30->_H30_FILE_Name_Length * 2;
						if (H30FileNameLen > (FILE_SECTOR_SIZE - H30Size) || NULL == H30FileNameLen)
						{
							CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVHDFileNameAndPath::������Χʧ��!"));
							return false;
						}
						H30temName.clear();
						MFTnumber = NULL;
						RtlCopyMemory(&MFTnumber, &H30->_H30_Parent_FILE_Reference, 4);
						H30temName.append("//");
						if (!UnicodeToZifu(&H30_NAMES[0], H30temName, H30FileNameLen))
						{

							CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVHDFileNameAndPath:Unicode_To_Zifu:ת��ʧ��!"));
							return false;
						}	

																		
					}
					if (ATTriBase->_Attr_Length < (FILE_SECTOR_SIZE - AttributeSize - FirstAttriSize) && ATTriBase->_Attr_Length > 0)
					{

						AttributeSize += ATTriBase->_Attr_Length;

					} 
					else
					{								
						CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVHDFileNameAndPath:���Գ��ȹ���!,������:%lu"),ATTriBase->_Attr_Length);
						return false;
					}
				}
				else if (ATTriBase->_Attr_Type == 0xffffffff)
				{
					break;
				}
				else if(ATTriBase->_Attr_Type > 0xff && ATTriBase->_Attr_Type < 0xffffffff)
				{
					CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVHDFileNameAndPath:��ȡATTriBase[i_attr]->_Attr_Type����0xffffffff����"));
					return false;
				}
			}
			StrTem.append(H30temName);
		}
	}
	int laststring = (StrTem.length() - 1);
	for (int i = (StrTem.length()-1); i > 0; i--)
	{
		if (StrTem[i] == '/' && StrTem[i-1] == '/')
		{
			if ((laststring-i) > 0)
			{
				VirtualFilePath.append(&StrTem[i+1],(laststring-i));				
				VirtualFilePath.append("\\");

				laststring = (i-2);
			}

		}
	}

	return true;
}
bool GetVirtualMachineInfo::GetVhdHeadInfor(HANDLE VhdDrive, DWORD *batentrynum, DWORD *BatOffset, DWORD *BatBlockSize, LONGLONG *VHDUUID, wchar_t *NtfsIncreVhdPathName)
{
	bool Ret = false;
	DWORD BackBytesCount = NULL;
	*VHDUUID = NULL;
	UCHAR *VhdHeadBuff = (UCHAR*)malloc(FILE_SECTOR_SIZE);
	if (NULL == VhdHeadBuff)
	{
		CloseHandle(VhdDrive);
		VhdDrive = NULL;
		CFuncs::WriteLogInfo(SLT_ERROR, "VhdFileCheck:malloc: VhdHeadBuffʧ��!");
		return false;
	}
	memset(VhdHeadBuff, 0,SECTOR_SIZE);

	Ret=ReadSQData(VhdDrive, VhdHeadBuff, SECTOR_SIZE,
		SECTOR_SIZE, &BackBytesCount);		
	if(!Ret)
	{				
		free(VhdHeadBuff);
		VhdHeadBuff = NULL;
		CloseHandle(VhdDrive);
		VhdDrive = NULL;
		CFuncs::WriteLogInfo(SLT_ERROR, "VhdFileCheck:ReadSQData��ȡͷ����Ϣʧ��!");
		return false;	
	}


	LVHD_Head vhdhead = (LVHD_Head)&VhdHeadBuff[0];
	*VHDUUID = vhdhead->_Parent_UUID[0].QuadPart;


	GetDwodSize(&vhdhead->_Bat_entry_number[0], batentrynum);//ȡ��������


	GetDwodtoDwod(&vhdhead->_Bat_offsetLowPart, BatOffset);	//����ƫ�Ƶ�ַ	


	GetDwodtoDwod(&vhdhead->_block_size, BatBlockSize);
	memset(NtfsIncreVhdPathName, 0, 402);
	if (!VhdNameChange(*VHDUUID, VhdHeadBuff, NtfsIncreVhdPathName))
	{
		free(VhdHeadBuff);
		VhdHeadBuff = NULL;

		CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVhdHeadInfor:VhdNameChange����!"));
		return false;
	}

	free(VhdHeadBuff);
	VhdHeadBuff = NULL;

	return true;
}
bool GetVirtualMachineInfo::VhdFileCheck(string VhdMftFileName, vector<string> checkfilename, const char* virtualFileDir, PFCallbackVirtualMachine VirtualFile)
{
	DWORD dwError = NULL;
	
	bool Ret = false;
	DWORD BackBytesCount = NULL;
	UCHAR VHDType = 0;
	if (!JudgeVHDFile(VhdMftFileName, &VHDType))
	{
		CFuncs::WriteLogInfo(SLT_ERROR, _T("VhdFileCheck:GetFileNameAndPath::VHD�ж�ʧ��!"));
		return false;
	}
	HANDLE VhdDrive = CreateFile(VhdMftFileName.c_str(),
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);
	if (VhdDrive == INVALID_HANDLE_VALUE) 
	{
		dwError=GetLastError();
		CFuncs::WriteLogInfo(SLT_ERROR, _T("VhdFileCheck VmDevice = CreateFile��ȡVMware�����ļ����ʧ��!,\
										   ���󷵻���: dwError = %d"), dwError);
		return false;
	}
	string VHDSalfPath;
	size_t s_posion = NULL;
	size_t e_posion = NULL;
	if (VhdMftFileName.rfind("\\") != string::npos)
	{
		s_posion = VhdMftFileName.rfind("\\");
		if (VhdMftFileName.rfind("\\", s_posion - 1) != string::npos)
		{
			e_posion = VhdMftFileName.rfind("\\", s_posion - 1);
			VHDSalfPath.append(&VhdMftFileName[0], e_posion + 1);
		}
	}
	//��ȡͷ����Ϣ�ж��ǻ����̻���������
	wchar_t *NtfsIncreVhdPathName=new wchar_t[201];
	if (NULL == NtfsIncreVhdPathName)
	{

		CloseHandle(VhdDrive);
		VhdDrive = NULL;
		CFuncs::WriteLogInfo(SLT_ERROR, _T("VhdFileCheck:new:IncreVhdPathName�����ڴ����!"));
		return false;
	}
	memset(NtfsIncreVhdPathName, 0, 402);
	DWORD batentrynum = NULL;//�洢�����ַ��Ŀ�ı���
	DWORD BatOffset = NULL;
	DWORD BatBlockSize = NULL;
	LONGLONG VHDUUID = NULL;
	if (VHDType != 2)
	{
		if (!GetVhdHeadInfor(VhdDrive, &batentrynum, &BatOffset, &BatBlockSize, &VHDUUID, NtfsIncreVhdPathName))
		{
			delete NtfsIncreVhdPathName;
			NtfsIncreVhdPathName = NULL;
			CloseHandle(VhdDrive);
			VhdDrive = NULL;
			CFuncs::WriteLogInfo(SLT_ERROR, _T("VhdFileCheck:GetVhdHeadInfor����!"));
			return false;
		}
	}
	
	

	
	memset(NtfsIncreVhdPathName, 0, 402);

	

	vector<DWORD64> v_VirtualNTFSStartAddr;
	UCHAR NTFSvhdtype = VHDType;
	LONGLONG NtfsVHDUUID = NULL;
	if(!GetVHDVirtualNTFSStartAddr(&NtfsVHDUUID, v_VirtualNTFSStartAddr, NtfsIncreVhdPathName, VhdDrive, VHDSalfPath, &NTFSvhdtype))
	{
		delete NtfsIncreVhdPathName;
		NtfsIncreVhdPathName = NULL;
		CloseHandle(VhdDrive);
		VhdDrive = NULL;
		CFuncs::WriteLogInfo(SLT_ERROR, _T("VhdFileCheck:GetVHDVirtualNTFSStartAddrʧ��!"));
		return false;
	}
	delete NtfsIncreVhdPathName;
	NtfsIncreVhdPathName = NULL;

	for (DWORD ntfsidnex = NULL; ntfsidnex < v_VirtualNTFSStartAddr.size(); ntfsidnex++)
	{
		DWORD64 VirtualPatition = v_VirtualNTFSStartAddr[ntfsidnex];
		LONGLONG MftVHDUUID = NULL;
		DWORD64 VirStartMftAddr = NULL;
		UCHAR m_VirtualCuNum = NULL;

		UCHAR *pVhdHeadBuff = (UCHAR*)malloc(FILE_SECTOR_SIZE);
		if (NULL == pVhdHeadBuff)
		{
			CloseHandle(VhdDrive);
			VhdDrive = NULL;
			CFuncs::WriteLogInfo(SLT_ERROR, "VhdFileCheck:malloc: VhdHeadBuffʧ��!");
			return false;
		}
		memset(pVhdHeadBuff, 0,SECTOR_SIZE);

		Ret=ReadSQData(VhdDrive, pVhdHeadBuff, SECTOR_SIZE,
			SECTOR_SIZE, &BackBytesCount);		
		if(!Ret)
		{		
			CloseHandle(VhdDrive);
			VhdDrive = NULL;
			free(pVhdHeadBuff);
			pVhdHeadBuff = NULL;
			CFuncs::WriteLogInfo(SLT_ERROR, "VhdFileCheck:ReadSQData��ȡͷ����Ϣʧ��!");
			return false;	
		}

		wchar_t *IncreVhdPathName=new wchar_t[201];
		if (NULL == IncreVhdPathName)
		{
			free(pVhdHeadBuff);
			pVhdHeadBuff = NULL;
			CloseHandle(VhdDrive);
			VhdDrive = NULL;
			CFuncs::WriteLogInfo(SLT_ERROR, _T("VhdFileCheck:new:IncreVhdPathName�����ڴ����!"));
			return false;
		}
		memset(IncreVhdPathName, 0, 402);
		UCHAR MftVhdtype = VHDType;
		if(!GetVirtualStartMftAddr(VhdDrive, &MftVHDUUID,&VirStartMftAddr, IncreVhdPathName, &m_VirtualCuNum, VirtualPatition
			, VHDSalfPath, &MftVhdtype))
		{
			free(pVhdHeadBuff);
			pVhdHeadBuff = NULL;
			delete IncreVhdPathName;
			IncreVhdPathName = NULL;
			CloseHandle(VhdDrive);
			VhdDrive = NULL;
			CFuncs::WriteLogInfo(SLT_ERROR, _T("VhdFileCheck:GetVirtualStartMftAddrʧ��!"));
			return false;
		}
		if (NULL == VirStartMftAddr)
		{
			CloseHandle(VhdDrive);
			VhdDrive = NULL;
			free(pVhdHeadBuff);
			pVhdHeadBuff = NULL;
			delete IncreVhdPathName;
			IncreVhdPathName = NULL;
			CFuncs::WriteLogInfo(SLT_ERROR, _T("VhdFileCheck:GetVirtualStartMftAddrΪ��ʧ��!"));
			break;
		}
		memset(IncreVhdPathName, 0, 402);

		free(pVhdHeadBuff);
		pVhdHeadBuff = NULL;
		LONGLONG AllMftVHDUUID = NULL;
		vector<LONG64> v_VirtualStartMftAddr;
		vector<DWORD64> v_VirtualStartMftLen;
		UCHAR Allmftvhdtype = VHDType;
		if (!VirGetAllMftAddr(VhdDrive, &AllMftVHDUUID, IncreVhdPathName, VirtualPatition, VirStartMftAddr, m_VirtualCuNum, v_VirtualStartMftAddr
			, v_VirtualStartMftLen, VHDSalfPath, &Allmftvhdtype))
		{
			CloseHandle(VhdDrive);
			VhdDrive = NULL;
			delete IncreVhdPathName;
			IncreVhdPathName = NULL;
			CFuncs::WriteLogInfo(SLT_ERROR, _T("VhdFileCheck:VirGetAllMftAddrʧ��!"));
			return false;
		}
		delete IncreVhdPathName;
		IncreVhdPathName = NULL;

		/************************************************************************/
		/* �����ļ���¼����ʼ��ַ��ʼѰ����Ӧ���ļ�                                                                     */
		/************************************************************************/
		UCHAR *EntryAddrBuff = NULL;
		DWORD *EntryAddr = NULL;//����DWOD����ָ�룬ȡһ��������ַ
		if (VHDType != 2)
		{
			if (NULL == batentrynum)
			{
				CloseHandle(VhdDrive);
				VhdDrive = NULL;
				CFuncs::WriteLogInfo(SLT_ERROR, "VhdFileCheck:batentrynum�����Ϊ��!");
				return false;
			}
			//���仺������ַ�ڴ�
			EntryAddrBuff = (UCHAR*)malloc(batentrynum * 4 + SECTOR_SIZE * 2);
			if (NULL == EntryAddrBuff)
			{
				CloseHandle(VhdDrive);
				VhdDrive = NULL;
				CFuncs::WriteLogInfo(SLT_ERROR, "VhdFileCheck:malloc����EntryAddrBuff�ڴ�ʧ��!");
				return false;
			}
			memset(EntryAddrBuff, 0, batentrynum * 4 + SECTOR_SIZE * 2);

			Ret=CacheParentVhdTable(VhdDrive, EntryAddrBuff, BatOffset, batentrynum * 4);
			if (!Ret)
			{
				free(EntryAddrBuff);
				EntryAddrBuff = NULL;
				CloseHandle(VhdDrive);
				VhdDrive = NULL;
				CFuncs::WriteLogInfo(SLT_ERROR, "VhdFileCheck:CacheVhdTableDataʧ��!");
				return false;
			}

			EntryAddr = (DWORD*)&EntryAddrBuff[0];
		}
		

		DWORD ReferNum = 0;//�ļ���¼����
		vector<LONG64> VirtualFileH80Addr;
		vector<DWORD> VirtualFileH80AddrLen;
		string VirtualFileBuffH80;
		string VirtualFileName;

		DWORD VirtualParentMft = NULL;
		vector<DWORD> VirtualH20Refer;
		DWORD64 VirtualH20DataRun = NULL;
		DWORD64 VirtualBackAddr = NULL;

		UCHAR* RecodeCacheBuff = (UCHAR*) malloc(FILE_SECTOR_SIZE + SECTOR_SIZE);
		if (NULL == RecodeCacheBuff)
		{
			if (VHDType != 2)
			{
				free(EntryAddrBuff);
				EntryAddrBuff = NULL;
			}			
			CloseHandle(VhdDrive);
			VhdDrive = NULL;
			CFuncs::WriteLogInfo(SLT_ERROR, "VhdFileCheck:malloc:RecodeCacheBuffʧ��!");
			return false;
		}
		memset(RecodeCacheBuff, 0, FILE_SECTOR_SIZE + SECTOR_SIZE);

		DWORD64 FileRealSize = NULL;
		for (DWORD FileRecNum = 0; FileRecNum < v_VirtualStartMftAddr.size(); FileRecNum++)
		{
			ReferNum=0;

			while(GetVHDVirtualFileAddr(VhdDrive ,VirtualPatition, v_VirtualStartMftAddr[FileRecNum], EntryAddr, batentrynum, BatBlockSize,
				RecodeCacheBuff, VirtualH20Refer, m_VirtualCuNum, checkfilename, &VirtualParentMft, VirtualFileH80Addr, VirtualFileH80AddrLen
				,VirtualFileBuffH80, ReferNum, VirtualFileName, &FileRealSize, VHDType))
			{		
				if (VirtualH20Refer.size() > 0)
				{
					UCHAR *H20CacheBuff = (UCHAR*)malloc(FILE_SECTOR_SIZE);
					if (NULL == H20CacheBuff)
					{
						if (VHDType != 2)
						{
							free(EntryAddrBuff);
							EntryAddrBuff = NULL;
						}
						CloseHandle(VhdDrive);
						VhdDrive = NULL;
						free(RecodeCacheBuff);
						RecodeCacheBuff = NULL;
						CFuncs::WriteLogInfo(SLT_ERROR, _T("VhdFileCheck:malloc: H20CacheBuffʧ��!"));
						return false;
					}
					vector<DWORD>::iterator h20vec;
					for (h20vec = VirtualH20Refer.begin(); h20vec < VirtualH20Refer.end(); h20vec++)
					{
						memset(H20CacheBuff, 0, FILE_SECTOR_SIZE);
						DWORD64 VirMftLen = NULL;
						DWORD64 VirStartMftRfAddr = NULL;
						for (DWORD FRN = 0; FRN < v_VirtualStartMftLen.size(); FRN++)
						{
							if (((*h20vec) * 2) < (VirMftLen + v_VirtualStartMftLen[FRN] * m_VirtualCuNum))
							{
								VirStartMftRfAddr = v_VirtualStartMftAddr[FRN] * m_VirtualCuNum + ((*h20vec) * 2 - VirMftLen);
								break;
							} 
							else
							{
								VirMftLen += (v_VirtualStartMftLen[FRN] * m_VirtualCuNum);
							}
						}
						if (VHDType != 2)
						{
							for (int i = 0; i < 2; i++)
							{
								VirtualBackAddr = NULL;
								if(!ParentVhdOneAddrChange((VirtualPatition * SECTOR_SIZE + VirStartMftRfAddr * SECTOR_SIZE + SECTOR_SIZE * i)
									, EntryAddr, BatBlockSize, &VirtualBackAddr, batentrynum))
								{
									if (VHDType != 2)
									{
										free(EntryAddrBuff);
										EntryAddrBuff = NULL;
									}
									free(RecodeCacheBuff);
									RecodeCacheBuff = NULL;
									free(H20CacheBuff);
									H20CacheBuff = NULL;
									CloseHandle(VhdDrive);
									VhdDrive = NULL;
									CFuncs::WriteLogInfo(SLT_ERROR, _T("VhdFileCheck:VhdOneAddrChange: ת����ʼMft�ļ���¼��ַʧ��!"));
									return false;
								}
								Ret = ReadSQData(VhdDrive, &H20CacheBuff[i*SECTOR_SIZE], SECTOR_SIZE, VirtualBackAddr,
									&BackBytesCount);		
								if(!Ret)
								{		
									if (VHDType != 2)
									{
										free(EntryAddrBuff);
										EntryAddrBuff = NULL;
									}
									free(RecodeCacheBuff);
									RecodeCacheBuff = NULL;
									free(H20CacheBuff);
									H20CacheBuff = NULL;
									CloseHandle(VhdDrive);
									VhdDrive = NULL;
									CFuncs::WriteLogInfo(SLT_ERROR, _T("VhdFileCheck:ReadSQData: ��ȡ��ʼMft�ļ���¼��ַʧ��!"));
									return false;	
								}
							}
						}
						else
						{
							Ret = ReadSQData(VhdDrive, &H20CacheBuff[0], FILE_SECTOR_SIZE, VirtualPatition * SECTOR_SIZE + VirStartMftRfAddr * SECTOR_SIZE,
								&BackBytesCount);		
							if(!Ret)
							{		
								
								free(RecodeCacheBuff);
								RecodeCacheBuff = NULL;
								free(H20CacheBuff);
								H20CacheBuff = NULL;
								CloseHandle(VhdDrive);
								VhdDrive = NULL;
								CFuncs::WriteLogInfo(SLT_ERROR, _T("VhdFileCheck:ReadSQData: ��ȡ��ʼMft�ļ���¼��ַʧ��!"));
								return false;	
							}
						}
						
						if(!GetVirtualH20FileReferH80Addr(H20CacheBuff, VirtualFileH80Addr, VirtualFileH80AddrLen, VirtualFileBuffH80, &FileRealSize))
						{
							if (VHDType != 2)
							{
								free(EntryAddrBuff);
								EntryAddrBuff = NULL;
							}
							free(RecodeCacheBuff);
							RecodeCacheBuff = NULL;
							free(H20CacheBuff);
							H20CacheBuff = NULL;
							CloseHandle(VhdDrive);
							VhdDrive = NULL;
							CFuncs::WriteLogInfo(SLT_ERROR, _T("VhdFileCheck:GetH20FileReferH80Addr: ʧ��!"));
							return false;
						}
					}
					free(H20CacheBuff);
					H20CacheBuff = NULL;

				}
				if (VirtualFileH80Addr.size() > 0)//����Ϊ��ַ����ȡ���ļ�
				{
					string VirtualPath;
					string StrTemName;
					if (VirtualFileName.length() > 0)
					{
						if(!UnicodeToZifu((UCHAR*)&VirtualFileName[0], StrTemName, VirtualFileName.length()))
						{
							if (VHDType != 2)
							{
								free(EntryAddrBuff);
								EntryAddrBuff = NULL;
							}
							free(RecodeCacheBuff);
							RecodeCacheBuff = NULL;
							CloseHandle(VhdDrive);
							VhdDrive = NULL;
							CFuncs::WriteLogInfo(SLT_ERROR, "VhdFileCheck:UnicodeToZifu : FileNameʧ��!");
							return false;
						}




						DWORD NameSize = VirtualFileName.length() + strlen(virtualFileDir);
						wchar_t * WirteName = new wchar_t[NameSize + 1];
						if (NULL == WirteName)
						{
							CFuncs::WriteLogInfo(SLT_ERROR, _T("VhdFileCheck:new:WirteName ���������ڴ�ʧ��!"));
						}
						memset(WirteName, 0, (NameSize + 1) * 2);
						MultiByteToWideChar(CP_ACP, 0, virtualFileDir, strlen(virtualFileDir), WirteName, NameSize);

						for (DWORD NameIndex = 0; NameIndex < VirtualFileName.length(); NameIndex += 2)
						{
							RtlCopyMemory(&WirteName[strlen(virtualFileDir) + NameIndex / 2],(UCHAR*) &VirtualFileName[NameIndex],2);
						}

						UCHAR *WriteFileBuffer=(UCHAR*)malloc(m_VirtualCuNum * SECTOR_SIZE + SECTOR_SIZE);
						if (NULL == WriteFileBuffer)
						{
							if (VHDType != 2)
							{
								free(EntryAddrBuff);
								EntryAddrBuff = NULL;
							}
							free(RecodeCacheBuff);
							RecodeCacheBuff = NULL;
							CloseHandle(VhdDrive);
							VhdDrive = NULL;
							delete WirteName;
							WirteName = NULL;
							CFuncs::WriteLogInfo(SLT_ERROR, "VhdFileCheck:malloc����WriteFileBuffer�ڴ�ʧ��!");
							return false;
						}
						memset(WriteFileBuffer, 0, (m_VirtualCuNum * SECTOR_SIZE + SECTOR_SIZE));
						if(!VHDWriteLargeFile(VhdDrive, VirtualFileH80Addr, VirtualFileH80AddrLen, m_VirtualCuNum, EntryAddr, batentrynum, BatBlockSize
							, WriteFileBuffer, WirteName, VirtualPatition, FileRealSize, VHDType))
						{

							delete WirteName;
							WirteName = NULL;
							free(WriteFileBuffer);
							WriteFileBuffer = NULL;


							CFuncs::WriteLogInfo(SLT_ERROR, "VhdFileCheck:WriteLargeFile:��ȡH20H80��ַʧ��ʧ��");
							break;
						}
						delete WirteName;
						WirteName = NULL;
						free(WriteFileBuffer);
						WriteFileBuffer = NULL;

						if(!GetVHDFileNameAndPath(VirtualPatition, v_VirtualStartMftAddr, v_VirtualStartMftLen, m_VirtualCuNum,
							VirtualParentMft, RecodeCacheBuff, VirtualPath, EntryAddr, batentrynum, BatBlockSize, StrTemName, VhdDrive, VHDType))
						{
							CFuncs::WriteLogInfo(SLT_ERROR, "VhdFileCheck:GetVirtualFileNameAndPath:��ȡ·��ʧ��");
							break;
						}
						VirtualFile( VirtualPath.c_str(), StrTemName.c_str());
					}

				} 
				else if (VirtualFileBuffH80.length() > 0)//������h80���ȡС�ļ�
				{
					string VirtualPath;
					string StrTemName;

					if (VirtualFileName.length() > 0)
					{
						if(!UnicodeToZifu((UCHAR*)&VirtualFileName[0], StrTemName, VirtualFileName.length()))
						{
							if (VHDType != 2)
							{
								free(EntryAddrBuff);
								EntryAddrBuff = NULL;
							}
							free(RecodeCacheBuff);
							RecodeCacheBuff = NULL;
							CloseHandle(VhdDrive);
							VhdDrive = NULL;
							CFuncs::WriteLogInfo(SLT_ERROR, "VhdFileCheck:UnicodeToZifu : FileNameʧ��!");
							return false;
						}




						DWORD NameSize = VirtualFileName.length() + strlen(virtualFileDir) + 1;
						wchar_t * WirteName = new wchar_t[NameSize + 1];
						if (NULL == WirteName)
						{
							CFuncs::WriteLogInfo(SLT_ERROR, _T("VhdFileCheck:new:WirteName ���������ڴ�ʧ��!"));
						}
						memset(WirteName, 0, (NameSize + 1) * 2);
						MultiByteToWideChar(CP_ACP, 0, virtualFileDir, strlen(virtualFileDir), WirteName, NameSize);
						for (DWORD NameIndex = 0; NameIndex < VirtualFileName.length(); NameIndex += 2)
						{

							RtlCopyMemory(&WirteName[strlen(virtualFileDir) + NameIndex / 2], &VirtualFileName[NameIndex], 2);
						}
						if(!VirtualWriteLitteFile(VirtualFileBuffH80, WirteName))
						{
							delete WirteName;
							WirteName=NULL;
							CFuncs::WriteLogInfo(SLT_ERROR, "VhdFileCheck:WriteLitteFile:ʧ��ʧ��");
							break;
						}
						delete WirteName;
						WirteName=NULL;
						if(!GetVHDFileNameAndPath(VirtualPatition, v_VirtualStartMftAddr, v_VirtualStartMftLen, m_VirtualCuNum,
							VirtualParentMft, RecodeCacheBuff, VirtualPath, EntryAddr, batentrynum, BatBlockSize, StrTemName, VhdDrive, VHDType))
						{
							CFuncs::WriteLogInfo(SLT_ERROR, "VhdFileCheck:GetVirtualFileNameAndPath:��ȡ·��ʧ��");
							break;
						}

						VirtualFile( VirtualPath.c_str(), StrTemName.c_str());
					}

				}


				ReferNum++;
				if ((ReferNum * 2) > v_VirtualStartMftLen[FileRecNum] * m_VirtualCuNum)
				{
					break;
				}
			}

		}
		if (VHDType != 2)
		{
			free(EntryAddrBuff);
			EntryAddrBuff = NULL;
		}
		free(RecodeCacheBuff);
		RecodeCacheBuff = NULL;

	}
	CloseHandle(VhdDrive);
	VhdDrive = NULL;	

	
	return true;
}
bool  GetVirtualMachineInfo::ReadSQCharData(HANDLE hDevice, char* Buffer, DWORD SIZE, DWORD64 addr, DWORD *BackBytesCount)
{
	LARGE_INTEGER LiAddr = {0};	
	LiAddr.QuadPart=addr;
	DWORD dwError = 0;

	BOOL bRet = SetFilePointerEx(hDevice, LiAddr, NULL,FILE_BEGIN);
	if(!bRet)
	{

		dwError = GetLastError();
		CFuncs::WriteLogInfo(SLT_ERROR, _T("ReadSQCharData::SetFilePointerExʧ��!,\
										   ���󷵻���: dwError = %d"), dwError);
		return false;	
	}
	bRet = ReadFile(hDevice, Buffer, SIZE, BackBytesCount, NULL);
	if(!bRet)
	{
		dwError = GetLastError();
		CFuncs::WriteLogInfo(SLT_ERROR, _T("ReadSQCharData::ReadFileʧ��!,\
										   ���󷵻���: dwError = %d"), dwError);					
		return false;	
	}

	return true;
}
bool GetVirtualMachineInfo::DwordStringToHex(DWORD *outdword, string Instring)
{
	(*outdword) = NULL;
	if (Instring.length() > 8)
	{
		CFuncs::WriteLogInfo(SLT_ERROR, _T("DwordStringToHex :Instring.length()̫��ʧ��!"));
		return false;
	}
	DWORD HexBuff =NULL;
	DWORD _HexBuff =NULL;
	map<UCHAR, char> hexmap;
	hexmap[0] = '0';
	hexmap[1] = '1';
	hexmap[2] = '2';
	hexmap[3] = '3';
	hexmap[4] = '4';
	hexmap[5] = '5';
	hexmap[6] = '6';
	hexmap[7] = '7';
	hexmap[8] = '8';
	hexmap[9] = '9';
	hexmap[10] = 'a';
	hexmap[11] = 'b';
	hexmap[12] = 'c';
	hexmap[13] = 'd';
	hexmap[14] = 'e';
	hexmap[15] = 'f';

	for (int i = (Instring.length() - 1); i >= 0; i -= 2)
	{
		HexBuff = NULL;
		_HexBuff = NULL;

		map<UCHAR, char>::iterator mapindex;
		for (int a = 0; a < 2; a++)
		{
			for (mapindex = hexmap.begin(); mapindex != hexmap.end(); mapindex ++)
			{
				if (mapindex->second == Instring[i - a])
				{
					if(a == 0)
					{
						HexBuff = mapindex->first;
					}else
					{
						_HexBuff = mapindex->first;
					}

					break;
				}
			}
		}


		(*outdword) = (*outdword) | (HexBuff << (4 * (Instring.length() - i - 1) ));
		(*outdword) = (*outdword) | (_HexBuff << (4 * (Instring.length() - i)));
		//printf("%x\n",(*outdword));
	}

	return true;
}
bool GetVirtualMachineInfo::GetVdiInformation(map<DWORD, string> &VdiFileInfo, string VdiBuff, string vdipath, int *VBoxFileType)
{
	size_t VirPosition = NULL;
	size_t _VirPosition = NULL;
	size_t End_VirPosition = NULL;
	while (VirPosition != string::npos)
	{

		VirPosition = VdiBuff.find("<HardDisk uuid=\"{", VirPosition + 8);
		if (string::npos == VirPosition)
		{
			break;;
		}
		string VdiUUID;
		DWORD UUIDS = NULL;
		VdiUUID.append(&VdiBuff[VirPosition + 17], 8);
		if (!DwordStringToHex(&UUIDS, VdiUUID))
		{
			CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVdiInformation :DwordStringToHexʧ��!"));
			return false;
		}

		_VirPosition = NULL;
		_VirPosition = VdiBuff.find("location=\"", VirPosition);
		if (string::npos == _VirPosition)
		{
			break;
		}
		End_VirPosition = NULL;
		End_VirPosition = VdiBuff.find(".vdi\"", (_VirPosition + 10));
		*VBoxFileType = 1;//
		if (string::npos == End_VirPosition)
		{
			End_VirPosition = NULL;
			End_VirPosition = VdiBuff.find(".vmdk\"", (_VirPosition + 10));
			if (string::npos == End_VirPosition)
			{
				
				break;					
			}
			else
			{
				*VBoxFileType = 2;//vmdk�ṹ
			}
		}
		string pathSrtem;
		if (*VBoxFileType == 1)
		{
			pathSrtem.append(&VdiBuff[_VirPosition + 10], End_VirPosition - _VirPosition - 6);
		} 
		else if(*VBoxFileType == 2)
		{
			pathSrtem.append(&VdiBuff[_VirPosition + 10], End_VirPosition - _VirPosition - 5);
		}
		

		if (pathSrtem.find(":/") != string::npos)
		{
			VdiFileInfo[UUIDS].append(pathSrtem);
		} 
		else
		{
			VdiFileInfo[UUIDS].append(vdipath);
			VdiFileInfo[UUIDS].append(pathSrtem);
		}

	}

	return true;
}
bool GetVirtualMachineInfo::GetVboxInformation(map<DWORD, string> &VdiFileInfo, string VboxPath, string vdipath, int *VBoxFileType)
{
	DWORD TotalLen = NULL;
	bool Ret = false;
	DWORD BackBytesCount = NULL;
	DWORD dwError = NULL;

	HANDLE VboxDrive = CreateFile(VboxPath.c_str(),
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);
	if (VboxDrive == INVALID_HANDLE_VALUE) 
	{
		dwError=GetLastError();
		CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVboxInformation VmDevice = CreateFile��ȡVMware�����ļ����ʧ��!,\
										   ���󷵻���: dwError = %d"), dwError);
		return false;
	}
	TotalLen = GetFileSize(VboxDrive, NULL);
	if (NULL == TotalLen || TotalLen > (SECTOR_SIZE * SECTOR_SIZE))
	{
		CloseHandle(VboxDrive);
		VboxDrive = NULL;
		CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVboxInformation :��VBOX��СΪ��ʧ��!"));
		return true;
	}
	char *VboxBuffer = (char*)malloc(TotalLen + SECTOR_SIZE);
	if (NULL == VboxBuffer)
	{
		CloseHandle(VboxDrive);
		VboxDrive = NULL;
		CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVboxInformation :malloc:VboxBufferʧ��!"));
		return false;
	}
	memset(VboxBuffer, 0, (TotalLen + SECTOR_SIZE));

	Ret = ReadSQCharData(VboxDrive, VboxBuffer, TotalLen,
			0,
			&BackBytesCount);		
		if(!Ret)
		{		
			free(VboxBuffer);
			VboxBuffer = NULL;
			CloseHandle(VboxDrive);
			VboxDrive = NULL;
			CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVboxInformation :ReadSQData::��ȡ����ʧ��!"));
			return false;	
		}
		
	CloseHandle(VboxDrive);
	VboxDrive = NULL;

	if(!GetVdiInformation(VdiFileInfo, VboxBuffer, vdipath, VBoxFileType))
	{
		free(VboxBuffer);
		VboxBuffer = NULL;
		CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVboxInformation :GetVdiInformation::��ȡ����ʧ��!"));
		return false;
	}

	free(VboxBuffer);
	VboxBuffer = NULL;
	return true;
}
bool GetVirtualMachineInfo::GetVdiHeadInformation(HANDLE hdrive,DWORD *VdiBatAddr,DWORD *VdiDataAddr,DWORD *BatSingleSize, DWORD *ParentUUID)
{
	bool Ret=false;
	DWORD BackBytesCount=NULL;

	UCHAR *Buffer = (UCHAR*) malloc(FILE_SECTOR_SIZE);
	if (NULL == Buffer)
	{
		CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVdiHeadInformation :malloc::Bufferʧ��!"));
		return false;
	}
	memset(Buffer, 0, FILE_SECTOR_SIZE);
	Ret=ReadSQData(hdrive, Buffer, SECTOR_SIZE,
		0,
		&BackBytesCount);		
	if(!Ret)
	{			
		free(Buffer);
		Buffer = NULL;
		CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVdiHeadInformation :ReadSQDataʧ��!"));
		return false;	
	}	

	LVDIHead VdiHead = NULL;

	VdiHead = (LVDIHead)&Buffer[0];
	if (VdiHead->_VboxSignature[0] != 0x203c3c3c && VdiHead->_VboxSignature[1] != 0x6361724f)//�ж�ͷ����־λ�ǲ���vdi�ļ�
	{
		free(Buffer);
		Buffer = NULL;
		CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVdiHeadInformation :ͷ����־λ����vdi�ļ�!"));
		return false;
	}

	(*VdiBatAddr) = VdiHead->_VdiBatStartAddr;
	(*VdiDataAddr) =VdiHead->_VdiDataStartAddr;
	(*BatSingleSize) = VdiHead->_VdiBatSingleSize;
	(*ParentUUID) = VdiHead->_VdiParentUUID->LowPart;

	if ( (*VdiBatAddr)==NULL || (*VdiDataAddr) == NULL || (*BatSingleSize) == NULL)
	{
		free(Buffer);
		Buffer = NULL;
		CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVdiHeadInformation :����ȫΪ��ʧ��!"));
		return false;
	}

	free(Buffer);
	Buffer = NULL;
	return true;

}
bool GetVirtualMachineInfo::GetVdiBatListInformation(HANDLE hDrive, UCHAR *VdiBatBuff, DWORD VdiBatStartAddr, DWORD BatListSize)
{
	bool bRet = false;
	DWORD BackBytesCount = NULL;
	DWORD ReadBatSize = (BatListSize / SECTOR_SIZE + 1) * SECTOR_SIZE;

	bRet=ReadSQData(hDrive, &VdiBatBuff[0], ReadBatSize,
		VdiBatStartAddr,
		&BackBytesCount);		
	if(!bRet)
	{			
		CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVdiBatListInformation:ReadSQData:��ȡ��ַ������Ŀ¼��ַʧ��!"));
		return false;	
	}

	return true;
}
bool GetVirtualMachineInfo::VdiOneAddrChange(DWORD64 VdiChangeAddr, UCHAR *VdiBatBuff, DWORD VdiBatSingleSize, DWORD64 *VdiBackAddr
	, DWORD BatListSize, DWORD VdiDataStartAddr)
{
	DWORD *VdiSingleBat = NULL;
	DWORD64 BatIdentifi = NULL;
	DWORD64 BatOffset = NULL;

	VdiSingleBat = (DWORD*)&VdiBatBuff[0];
	BatIdentifi = VdiChangeAddr / VdiBatSingleSize;
	if (BatIdentifi > BatListSize/4)
	{
		CFuncs::WriteLogInfo(SLT_ERROR, _T("VdiOneAddrChange :BatIdentifi�������ܵ�BAT����ʧ��!"));
		return false;
	}
	if (VdiSingleBat[BatIdentifi] == 0xffffffff)
	{
		//	CFuncs::WriteLogInfo(SLT_ERROR, _T("VdiOneAddrChange :BATλ��Ϊ0xffffffff,�˵�ַΪ��!ʧ��!"));
		return false;
	}
	BatOffset = VdiChangeAddr % VdiBatSingleSize;

	(*VdiBackAddr) = VdiSingleBat[BatIdentifi];
	(*VdiBackAddr) = (*VdiBackAddr) * VdiBatSingleSize;

	(*VdiBackAddr) = BatOffset + (*VdiBackAddr) + VdiDataStartAddr;

	return true;
}
bool GetVirtualMachineInfo::FindVirtualVdiGPTInfo(HANDLE h_drive, UCHAR *CacheBuff, UCHAR *VdiBatBuff, DWORD VdiBatSingleSize, DWORD BatListSize,
	vector<DWORD64> &VirtualStartaddr, DWORD VdiDataStartAddr)
{
	memset(CacheBuff, 0, SECTOR_SIZE);
	DWORD64 VdiChangeAddr = NULL;
	DWORD64 VdiBackAddr = NULL;
	bool Readsq = true;
	bool Ret = false;
	DWORD BackBytesCount = NULL;
	LGPT_FB_TABLE GTFB = NULL;
	VdiChangeAddr = 2 * SECTOR_SIZE;

	while(Readsq)
	{
		VdiBackAddr = NULL;

		if(!VdiOneAddrChange(VdiChangeAddr, VdiBatBuff, VdiBatSingleSize, &VdiBackAddr, BatListSize, VdiDataStartAddr))
		{
			CFuncs::WriteLogInfo(SLT_ERROR, _T("FindVirtualVdiGPTInfo:Virtual_to_Host_OneAddr: ��ȡ�����GPTת����ַʧ��!"));
			return false;
		}
		VdiChangeAddr++;
		Ret = ReadSQData(h_drive, &CacheBuff[0], SECTOR_SIZE, VdiBackAddr, &BackBytesCount);		
		if(!Ret)
		{			
			CFuncs::WriteLogInfo(SLT_ERROR, _T("FindVirtualVdiGPTInfo:ReadSQData: ��ȡ�����GPT��ַ��Ϣʧ��!"));
			return false;	
		}
		GTFB = (LGPT_FB_TABLE)&CacheBuff[0];
		for (int i = 0;(GTFB->_GUID_TYPE[0] != 0) && (i < 4); i++)
		{
			if (GTFB->_GUID_TYPE[0] == 0x4433b9e5ebd0a0a2)
			{
				VirtualStartaddr.push_back(GTFB->_FB_Start_SQ);
			}
			if (i < 3)
			{
				GTFB++;
			}
		}
		if (GTFB->_FB_Start_SQ == 0)
		{
			Readsq = false;
		}
	}
	if (VirtualStartaddr.size() == NULL)
	{
		CFuncs::WriteLogInfo(SLT_ERROR, _T("FindVirtualVdiGPTInfo:GPT������ַΪ��,��Ѱʧ��!"));
		return false;
	}
	return true;
}
bool GetVirtualMachineInfo::FindVirtualVdiMBRInfo(HANDLE h_drive, UCHAR *CacheBuff, DWORD64 *VdiChangeAddr, UCHAR *VdiBatBuff, DWORD VdiBatSingleSize,
	DWORD BatListSize, vector<DWORD64> &VirtualStartaddr, DWORD VdiDataStartAddr)
{
	memset(CacheBuff, 0, SECTOR_SIZE);
	DWORD64 VhdBackAddr = NULL;
	DWORD BackBytesCount = NULL;
	bool Ret = false;
	LMBR_Heads virmbr = NULL;

	if(!VdiOneAddrChange((*VdiChangeAddr), VdiBatBuff, VdiBatSingleSize, &VhdBackAddr, BatListSize, VdiDataStartAddr))
	{
		CFuncs::WriteLogInfo(SLT_ERROR, _T("FindVirtualVdiMBRInfo:VdiOneAddrChange: ��ȡ�����MBR�׵�ַͷ����Ϣʧ��!"));
		return false;
	}
	//	printf("*hostaddr %0.8x\n",hostaddr);
	Ret = ReadSQData(h_drive, &CacheBuff[0], SECTOR_SIZE, VhdBackAddr, &BackBytesCount);		
	if(!Ret)
	{			

		CFuncs::WriteLogInfo(SLT_ERROR, _T("FindVirtualVdiMBRInfo:ReadSQData: ��ȡ�����MBR�׵�ַͷ����Ϣʧ��!"));
		return false;	
	}
	for (int i = 0; i < 64; i += 16)
	{
		virmbr = (LMBR_Heads)&CacheBuff[446+i];				
		if (virmbr->_MBR_Partition_Type == 0x05 || virmbr->_MBR_Partition_Type == 0x0f)
		{
			if (CacheBuff[0] == 0 && CacheBuff[1] == 0 && CacheBuff[2] == 0 && CacheBuff[3] == 0)
			{				
				(*VdiChangeAddr) = ((*VdiChangeAddr) + ((DWORD64)virmbr->_MBR_Sec_pre_pa));				
				if(!FindVirtualVdiMBRInfo(h_drive, CacheBuff, VdiChangeAddr, VdiBatBuff, VdiBatSingleSize, BatListSize, VirtualStartaddr, VdiDataStartAddr))
				{
					CFuncs::WriteLogInfo(SLT_ERROR, _T("FindVirtualVdiMBRInfo:FindVirtualMBRInfo: �ص�ʧ��!"));
					return false;
				}
			} 
			else
			{							
				(*VdiChangeAddr) = ((DWORD64)(virmbr->_MBR_Sec_pre_pa));							
				if(!FindVirtualVdiMBRInfo(h_drive, CacheBuff, VdiChangeAddr, VdiBatBuff, VdiBatSingleSize, BatListSize, VirtualStartaddr, VdiDataStartAddr))
				{
					CFuncs::WriteLogInfo(SLT_ERROR, _T("FindVirtualVdiMBRInfo:FindVirtualMBRInfo: �ص�ʧ��!"));
					return false;
				}
			}
		} 
		else if (virmbr->_MBR_Partition_Type == 0x00)
		{
			CFuncs::WriteLogInfo(SLT_INFORMATION, _T("FindVirtualVdiMBRInfo ��ȡvirtualMBR���!"));
			return true;
		}
		else if (virmbr->_MBR_Partition_Type == 0x07)
		{
			if (CacheBuff[0] == 0x00 && CacheBuff[1] == 0x00 && CacheBuff[2] == 0x00 && CacheBuff[3] == 0x00)
			{			
				VirtualStartaddr.push_back((virmbr->_MBR_Sec_pre_pa + (*VdiChangeAddr)));
			}
			else
			{
				VirtualStartaddr.push_back(virmbr->_MBR_Sec_pre_pa);			
			}
		}
	}
	return true;
}
bool GetVirtualMachineInfo::GetVdiChildNTFSInfomation(HANDLE hDrive, DWORD BatListSize, DWORD VdiBatStartAddr, DWORD VdiBatSingleSize, 
	vector<DWORD64> &VirtualStartaddr, DWORD VdiDataStartAddr)
{
	bool Ret = false;
	DWORD BackBytesCount = NULL;

	if (BatListSize > 4096 * 5000)
	{
		CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVdiChildNTFSInfomation :BatListSize����,ʧ��!"));
		return false;
	}
	UCHAR *VdiBatBuff = (UCHAR*) malloc(BatListSize + FILE_SECTOR_SIZE);
	if (NULL == VdiBatBuff)
	{
		CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVdiChildNTFSInfomation :malloc:VdiBatBuffʧ��!"));
		return false;
	}
	memset(VdiBatBuff, 0, (BatListSize + FILE_SECTOR_SIZE));
	if(!GetVdiBatListInformation(hDrive, VdiBatBuff, VdiBatStartAddr, BatListSize))
	{
		free(VdiBatBuff);
		VdiBatBuff = NULL;
		CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVdiChildNTFSInfomation :GetVdiBatListInformationʧ��!"));
		return false;
	}
	DWORD64 VdiBackAddr = NULL;
	if(!VdiOneAddrChange((1 * SECTOR_SIZE), VdiBatBuff, VdiBatSingleSize, &VdiBackAddr, BatListSize, VdiDataStartAddr))
	{
		free(VdiBatBuff);
		VdiBatBuff = NULL;
		CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVdiChildNTFSInfomation :VdiOneAddrChangeʧ��!"));
		return true;
	}
	UCHAR *VdiCancheBuff = (UCHAR*)malloc(FILE_SECTOR_SIZE + SECTOR_SIZE);
	if (NULL == VdiCancheBuff)
	{
		free(VdiBatBuff);
		VdiBatBuff = NULL;
		CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVdiChildNTFSInfomation :malloc:VdiCancheBuffʧ��!"));
		return false;
	}
	memset(VdiCancheBuff, 0, FILE_SECTOR_SIZE + SECTOR_SIZE);
	Ret = ReadSQData(hDrive, &VdiCancheBuff[0], SECTOR_SIZE, VdiBackAddr
		, &BackBytesCount);		
	if(!Ret)
	{			
		free(VdiCancheBuff);
		VdiCancheBuff = NULL;
		free(VdiBatBuff);
		VdiBatBuff = NULL;
		CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVdiChildNTFSInfomation:ReadSQData: ��ȡ������ڶ�������Ϣʧ��!"));
		return false;	
	}
	LGPT_Heads GptHead = (LGPT_Heads)&VdiCancheBuff[0];
	if (GptHead->_Singed_name == 0x5452415020494645)
	{
		CFuncs::WriteLogInfo(SLT_INFORMATION, _T("GetVdiChildNTFSInfomation ����������GPT����"));
		if (!FindVirtualVdiGPTInfo(hDrive, VdiCancheBuff, VdiBatBuff, VdiBatSingleSize, BatListSize, VirtualStartaddr, VdiDataStartAddr))
		{
			free(VdiCancheBuff);
			VdiCancheBuff = NULL;
			free(VdiBatBuff);
			VdiBatBuff = NULL;
			CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVdiChildNTFSInfomation :FindVirtualGPTInfoʧ��!"));
			return true;
		}
	}
	else
	{
		CFuncs::WriteLogInfo(SLT_INFORMATION, _T("GetVdiChildNTFSInfomation ��������������MBR������Ҳ�����ڲ������"));
		DWORD64 ChangeAddr = NULL;
		if (!FindVirtualVdiMBRInfo(hDrive, VdiCancheBuff, &ChangeAddr, VdiBatBuff, VdiBatSingleSize, BatListSize, VirtualStartaddr, VdiDataStartAddr))
		{
			free(VdiCancheBuff);
			VdiCancheBuff = NULL;
			free(VdiBatBuff);
			VdiBatBuff = NULL;
			CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVdiChildNTFSInfomation :FindVirtualMBRInfoʧ��!"));
			return true;
		}

	}


	free(VdiCancheBuff);
	VdiCancheBuff = NULL;
	free(VdiBatBuff);
	VdiBatBuff = NULL;
	return true;
}
bool GetVirtualMachineInfo::GetVdiNTFSStartAddr(DWORD *VdiUUID, map<DWORD, string> VdiFileInfo, vector<DWORD64> &VdiNTFSStartAddr)
{
	DWORD dwError = NULL;
	HANDLE VdiDevice = NULL;
	map<DWORD, string>::iterator Vdiindex;
	for (Vdiindex = VdiFileInfo.begin(); Vdiindex != VdiFileInfo.end(); Vdiindex ++)
	{
		if ((*VdiUUID) == Vdiindex->first)
		{
			//��һ�ν���˺���ʱ��UUID��Ϊ0��ȡ�þ������ȡͷ����Ϣ
			VdiDevice = CreateFile(Vdiindex->second.c_str(),//����ע�⣬���ֻ��һ�����̣�������Ҫ���ݸ������!!!!!
				GENERIC_READ,
				FILE_SHARE_READ,
				NULL,
				OPEN_EXISTING,
				0,
				NULL);
			if (VdiDevice == INVALID_HANDLE_VALUE) 
			{
				dwError=GetLastError();
				CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVdiNTFSStartAddr VdiDevice = CreateFile��ȡvdi�ļ����ʧ��!,\
												   ���󷵻���: dwError = %d"), dwError);
				return false;
			}
			break;
		}

	}
	if (NULL == VdiDevice)
	{
		CFuncs::WriteLogInfo(SLT_ERROR, "GetVdiNTFSStartAddr:VdiDevice,λ�ҵ�uuid��Ӧ��vdi�ļ�����ȡ���ʧ��!");
		return false;
	}
	//��ȡͷ����Ϣ
	DWORD VdiBatAddr = NULL;
	DWORD VdiDataAddr = NULL;
	DWORD BatSingleSize = NULL;
	DWORD BatBuffSize=NULL;//����Bat����ܴ�С

	if(!GetVdiHeadInformation(VdiDevice, &VdiBatAddr, &VdiDataAddr, &BatSingleSize, VdiUUID))
	{
		CloseHandle(VdiDevice);
		VdiDevice = NULL;
		CFuncs::WriteLogInfo(SLT_ERROR, "GetVdiNTFSStartAddr:GetVdiHeadInformationʧ��!");
		return false;
	}
	BatBuffSize = VdiDataAddr - VdiBatAddr;

	//�õ�ͷ����Ϣ��,��ȡ���̵�NTFS��ʼ��ַ�����ҵ����򷵻�true,��û�ҵ���UUID����0����Ѱ�Ҹ���NTFS����û�ҵ���UUIDΪ0��
	if(!GetVdiChildNTFSInfomation(VdiDevice, BatBuffSize, VdiBatAddr, BatSingleSize, VdiNTFSStartAddr, VdiDataAddr))
	{
		CloseHandle(VdiDevice);
		VdiDevice = NULL;
		CFuncs::WriteLogInfo(SLT_ERROR, "GetVdiNTFSStartAddr:GetChildNTFSInfomationʧ��!");
		return false;
	}
	CloseHandle(VdiDevice);
	VdiDevice = NULL;
	if(NULL == VdiNTFSStartAddr.size())
	{
		if ((*VdiUUID) != 0)
		{
			if(!GetVdiNTFSStartAddr(VdiUUID, VdiFileInfo, VdiNTFSStartAddr))
			{
				CFuncs::WriteLogInfo(SLT_ERROR, "GetVdiNTFSStartAddr:GetVdiNTFSStartAddr :�ص�ʧ��!");
				return false;
			}
		} 
		else if(NULL == (*VdiUUID))
		{
			CFuncs::WriteLogInfo(SLT_ERROR, "GetVdiNTFSStartAddr:VdiUUID���ڿգ���û�ҵ�NTFS��ʼ��ַʧ��!");
			return false;
		}
	}
	else if(VdiNTFSStartAddr.size() > 0)
	{
		CFuncs::WriteLogInfo(SLT_INFORMATION, _T("GetVdiNTFSStartAddr:�ҵ���NTFS��ʼ��ַ"));
		return true;
	}


	return true;

}
bool GetVirtualMachineInfo::GetVdiChildMftStartAddr(HANDLE hDrive, DWORD BatListSize, DWORD VdiBatStartAddr, DWORD VdiBatSingleSize, DWORD64 VirtulPatition
	, DWORD64 *VirtualStartMft, UCHAR *m_VirtualCuNum, DWORD VdiDataStartAddr)
{
	bool Ret = false;
	DWORD BackBytesCount = NULL;

	if (BatListSize > 4096 * 5000)
	{
		CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVdiChildMftStartAddr :BatListSize����,ʧ��!"));
		return false;
	}
	UCHAR *VdiBatBuff = (UCHAR*) malloc(BatListSize + FILE_SECTOR_SIZE);
	if (NULL == VdiBatBuff)
	{
		CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVdiChildMftStartAddr :malloc:VdiBatBuffʧ��!"));
		return false;
	}
	memset(VdiBatBuff, 0, (BatListSize + FILE_SECTOR_SIZE));
	if(!GetVdiBatListInformation(hDrive, VdiBatBuff, VdiBatStartAddr, BatListSize))
	{
		free(VdiBatBuff);
		VdiBatBuff = NULL;
		CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVdiChildMftStartAddr :GetVdiBatListInformationʧ��!"));
		return false;
	}
	DWORD64 VdiBackAddr = NULL;
	if(!VdiOneAddrChange((VirtulPatition * SECTOR_SIZE), VdiBatBuff, VdiBatSingleSize, &VdiBackAddr, BatListSize, VdiDataStartAddr))
	{
		free(VdiBatBuff);
		VdiBatBuff = NULL;
		CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVdiChildMftStartAddr :VdiOneAddrChangeʧ��!"));
		return true;
	}

	UCHAR *VdiCancheBuff = (UCHAR*)malloc(FILE_SECTOR_SIZE + SECTOR_SIZE);
	if (NULL == VdiCancheBuff)
	{
		free(VdiBatBuff);
		VdiBatBuff = NULL;
		CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVdiChildMftStartAddr :malloc:VdiCancheBuffʧ��!"));
		return false;
	}
	memset(VdiCancheBuff, 0, FILE_SECTOR_SIZE + SECTOR_SIZE);
	Ret = ReadSQData(hDrive, &VdiCancheBuff[0], SECTOR_SIZE, VdiBackAddr
		, &BackBytesCount);		
	if(!Ret)
	{			
		free(VdiCancheBuff);
		VdiCancheBuff = NULL;
		free(VdiBatBuff);
		VdiBatBuff = NULL;
		CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVdiChildMftStartAddr:ReadSQData: ��ȡ������ڶ�������Ϣʧ��!"));
		return false;	
	}
	LNTFS_TABLES virtualNtfs = NULL;
	virtualNtfs = (LNTFS_TABLES)&VdiCancheBuff[0];
	(*m_VirtualCuNum) = virtualNtfs->_Single_Cu_Num;
	(*VirtualStartMft) = virtualNtfs->_MFT_Start_CU;

	free(VdiCancheBuff);
	VdiCancheBuff = NULL;
	free(VdiBatBuff);
	VdiBatBuff = NULL;

	return true;
}
bool GetVirtualMachineInfo::GetVdiStartMftAddr(DWORD *VdiUUID, map<DWORD, string> VdiFileInfo, DWORD64 VirtulPatition, DWORD64 *VirtualStartMft
	, UCHAR *m_VirtualCuNum)
{
	DWORD dwError = NULL;
	HANDLE VdiDevice = NULL;
	map<DWORD, string>::iterator Vdiindex;
	for (Vdiindex = VdiFileInfo.begin(); Vdiindex != VdiFileInfo.end(); Vdiindex ++)
	{
		if ((*VdiUUID) == Vdiindex->first)
		{
			//��һ�ν���˺���ʱ��UUID��Ϊ0��ȡ�þ������ȡͷ����Ϣ
			VdiDevice = CreateFile(Vdiindex->second.c_str(),//����ע�⣬���ֻ��һ�����̣�������Ҫ���ݸ������!!!!!
				GENERIC_READ,
				FILE_SHARE_READ,
				NULL,
				OPEN_EXISTING,
				0,
				NULL);
			if (VdiDevice == INVALID_HANDLE_VALUE) 
			{
				dwError=GetLastError();
				CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVdiStartMftAddr VdiDevice = CreateFile��ȡvdi�ļ����ʧ��!,\
												   ���󷵻���: dwError = %d"), dwError);
				return false;
			}
			break;
		}

	}
	if (NULL == VdiDevice)
	{
		CFuncs::WriteLogInfo(SLT_ERROR, "GetVdiStartMftAddr:VdiDevice,λ�ҵ�uuid��Ӧ��vdi�ļ�����ȡ���ʧ��!");
		return false;
	}

	//��ȡͷ����Ϣ
	DWORD VdiBatAddr = NULL;
	DWORD VdiDataAddr = NULL;
	DWORD BatSingleSize = NULL;
	DWORD BatBuffSize=NULL;//����Bat����ܴ�С

	if(!GetVdiHeadInformation(VdiDevice, &VdiBatAddr, &VdiDataAddr, &BatSingleSize, VdiUUID))
	{
		CloseHandle(VdiDevice);
		VdiDevice = NULL;
		CFuncs::WriteLogInfo(SLT_ERROR, "GetVdiNTFSStartAddr:GetVdiHeadInformationʧ��!");
		return false;
	}
	BatBuffSize = VdiDataAddr - VdiBatAddr;

	if(!GetVdiChildMftStartAddr(VdiDevice, BatBuffSize, VdiBatAddr, BatSingleSize, VirtulPatition, VirtualStartMft, m_VirtualCuNum, VdiDataAddr))
	{
		CloseHandle(VdiDevice);
		VdiDevice = NULL;
		CFuncs::WriteLogInfo(SLT_ERROR, "GetVdiStartMftAddr:GetChildMftStartAddrʧ��!");
		return false;
	}
	CloseHandle(VdiDevice);
	VdiDevice = NULL;

	if (NULL == (*VirtualStartMft) || NULL == (*m_VirtualCuNum))
	{
		if ((*VdiUUID) != 0)
		{
			if (!GetVdiStartMftAddr(VdiUUID, VdiFileInfo, VirtulPatition, VirtualStartMft, m_VirtualCuNum))
			{
				CFuncs::WriteLogInfo(SLT_ERROR, "GetVdiStartMftAddr:GetVdiStartMftAddr :�ص�ʧ��!");
				return false;
			}
		} 
		else if(NULL == (*VdiUUID))
		{
			CFuncs::WriteLogInfo(SLT_ERROR, "GetVdiStartMftAddr:VdiUUID���ڿգ���û�ҵ�Mft��ʼ��ַʧ��!");
			return false;
		}
	} 


	return true;
}
bool GetVirtualMachineInfo::GetVdiChildAllMftAddr(HANDLE hDrive, DWORD BatListSize, DWORD VdiBatStartAddr, DWORD VdiBatSingleSize, DWORD64 VirtulPatition
	, DWORD64 VirtualStartMft, UCHAR m_VirtualCuNum, vector<LONG64> &v_VirtualStartMftAddr, vector<DWORD64> &v_VirtualStartMftLen, DWORD VdiDataStartAddr)
{
	bool Ret = false;
	DWORD BackBytesCount = NULL;

	if (BatListSize > 4096 * 5000)
	{
		CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVdiChildAllMftAddr :BatListSize����,ʧ��!"));
		return false;
	}
	UCHAR *VdiBatBuff = (UCHAR*) malloc(BatListSize + FILE_SECTOR_SIZE);
	if (NULL == VdiBatBuff)
	{
		CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVdiChildAllMftAddr :malloc:VdiBatBuffʧ��!"));
		return false;
	}
	memset(VdiBatBuff, 0, (BatListSize + FILE_SECTOR_SIZE));
	if(!GetVdiBatListInformation(hDrive, VdiBatBuff, VdiBatStartAddr, BatListSize))
	{
		free(VdiBatBuff);
		VdiBatBuff = NULL;
		CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVdiChildAllMftAddr :GetVdiBatListInformationʧ��!"));
		return false;
	}
	DWORD64 VdiBackAddr = NULL;
	if(!VdiOneAddrChange((VirtulPatition * SECTOR_SIZE + VirtualStartMft * m_VirtualCuNum * SECTOR_SIZE), VdiBatBuff, VdiBatSingleSize
		, &VdiBackAddr, BatListSize, VdiDataStartAddr))
	{
		free(VdiBatBuff);
		VdiBatBuff = NULL;
		CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVdiChildAllMftAddr :VdiOneAddrChangeʧ��!"));
		return true;
	}

	UCHAR *VdiCancheBuff = (UCHAR*)malloc(FILE_SECTOR_SIZE + SECTOR_SIZE);
	if (NULL == VdiCancheBuff)
	{
		free(VdiBatBuff);
		VdiBatBuff = NULL;
		CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVdiChildAllMftAddr :malloc:VdiCancheBuffʧ��!"));
		return false;
	}
	memset(VdiCancheBuff, 0, FILE_SECTOR_SIZE + SECTOR_SIZE);

	Ret = ReadSQData(hDrive, &VdiCancheBuff[0], SECTOR_SIZE, VdiBackAddr
		, &BackBytesCount);		
	if(!Ret)
	{			
		free(VdiBatBuff);
		VdiBatBuff = NULL;
		free(VdiCancheBuff);
		VdiCancheBuff = NULL;
		CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVdiChildAllMftAddr:ReadSQData: ��ȡ������ڶ�������Ϣʧ��!"));
		return false;	
	}
	VdiBackAddr = NULL;

	if(!VdiOneAddrChange((VirtulPatition * SECTOR_SIZE + VirtualStartMft * m_VirtualCuNum * SECTOR_SIZE + SECTOR_SIZE), VdiBatBuff, VdiBatSingleSize
		, &VdiBackAddr, BatListSize, VdiDataStartAddr))
	{
		free(VdiBatBuff);
		VdiBatBuff = NULL;
		free(VdiCancheBuff);
		VdiCancheBuff = NULL;
		CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVdiChildAllMftAddr :VdiOneAddrChangeʧ��!"));
		return false;
	}
	free(VdiBatBuff);
	VdiBatBuff = NULL;

	Ret = ReadSQData(hDrive, &VdiCancheBuff[SECTOR_SIZE], SECTOR_SIZE, VdiBackAddr
		, &BackBytesCount);		
	if(!Ret)
	{			
		free(VdiCancheBuff);
		VdiCancheBuff = NULL;
		CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVdiChildAllMftAddr:ReadSQData: ��ȡ������ڶ�������Ϣʧ��!"));
		return false;	
	}
	if(!GetMFTAddr(NULL, v_VirtualStartMftAddr, v_VirtualStartMftLen, NULL, VdiCancheBuff, false))
	{
		CFuncs::WriteLogInfo(SLT_ERROR, "GetVdiChildAllMftAddr:GetMFTAddr:��ȡ���������MFT��ʼ��ַʧ��");
		return false;
	}

	return true;

}
bool GetVirtualMachineInfo::GetVdiAllMftAddr(DWORD *VdiUUID, map<DWORD, string> VdiFileInfo, DWORD64 VirtulPatition, DWORD64 VirtualStartMft
	, UCHAR m_VirtualCuNum,  vector<LONG64> &v_VirtualStartMftAddr, vector<DWORD64> &v_VirtualStartMftLen)
{
	DWORD dwError = NULL;
	HANDLE VdiDevice = NULL;
	map<DWORD, string>::iterator Vdiindex;
	for (Vdiindex = VdiFileInfo.begin(); Vdiindex != VdiFileInfo.end(); Vdiindex ++)
	{
		if ((*VdiUUID) == Vdiindex->first)
		{
			//��һ�ν���˺���ʱ��UUID��Ϊ0��ȡ�þ������ȡͷ����Ϣ
			VdiDevice = CreateFile(Vdiindex->second.c_str(),//����ע�⣬���ֻ��һ�����̣�������Ҫ���ݸ������!!!!!
				GENERIC_READ,
				FILE_SHARE_READ,
				NULL,
				OPEN_EXISTING,
				0,
				NULL);
			if (VdiDevice == INVALID_HANDLE_VALUE) 
			{
				dwError=GetLastError();

				CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVdiAllMftAddr VdiDevice = CreateFile��ȡvdi�ļ����ʧ��!,\
												   ���󷵻���: dwError = %d"), dwError);
				return false;
			}
			break;
		}

	}
	if (NULL == VdiDevice)
	{
		CFuncs::WriteLogInfo(SLT_ERROR, "GetVdiAllMftAddr:VdiDevice,λ�ҵ�uuid��Ӧ��vdi�ļ�����ȡ���ʧ��!");
		return false;
	}

	//��ȡͷ����Ϣ
	DWORD VdiBatAddr = NULL;
	DWORD VdiDataAddr = NULL;
	DWORD BatSingleSize = NULL;
	DWORD BatBuffSize=NULL;//����Bat����ܴ�С

	if(!GetVdiHeadInformation(VdiDevice, &VdiBatAddr, &VdiDataAddr, &BatSingleSize, VdiUUID))
	{
		CloseHandle(VdiDevice);
		VdiDevice = NULL;
		CFuncs::WriteLogInfo(SLT_ERROR, "GetVdiAllMftAddr:GetVdiHeadInformationʧ��!");
		return false;
	}
	BatBuffSize = VdiDataAddr - VdiBatAddr;
	if (!GetVdiChildAllMftAddr(VdiDevice, BatBuffSize, VdiBatAddr, BatSingleSize, VirtulPatition, VirtualStartMft, m_VirtualCuNum,
		v_VirtualStartMftAddr, v_VirtualStartMftLen, VdiDataAddr))
	{
		CFuncs::WriteLogInfo(SLT_ERROR, "GetVdiAllMftAddr:GetChildAllMftAddrʧ��!");
		return false;
	}
	if (v_VirtualStartMftAddr.size() != v_VirtualStartMftLen.size())
	{
		CFuncs::WriteLogInfo(SLT_ERROR, "GetVdiAllMftAddr:v_VirtualStartMftAddr.size() != v_VirtualStartMftLen.size()ʧ��!");
		return false;
	}
	if (NULL == v_VirtualStartMftAddr.size())
	{
		if ((*VdiUUID) != 0)
		{
			if (!GetVdiAllMftAddr(VdiUUID, VdiFileInfo, VirtulPatition, VirtualStartMft, m_VirtualCuNum, v_VirtualStartMftAddr, v_VirtualStartMftLen))
			{
				CFuncs::WriteLogInfo(SLT_ERROR, "GetVdiAllMftAddr:GetVdiAllMftAddr:�ص�ʧ��!");
				return false;
			}
		} 
		else if(NULL == (*VdiUUID))
		{
			CFuncs::WriteLogInfo(SLT_ERROR, "GetVdiAllMftAddr:(*VdiUUID)λ�գ���Ѱ�ҵ�mft��ַλ��ʧ��!");
			return false;
		}
	} 


	return true;
}
bool GetVirtualMachineInfo::GetVboxVirtualFileAddr(HANDLE hDrive, DWORD64 VirtualStartPatition, DWORD64 VirStartMftRfAddr, UCHAR VirtualCuNum, DWORD Rerefer, UCHAR *VdiBatBuff
	, DWORD VdiBatSingleSize, DWORD BatListSize, UCHAR *CacheBuff, DWORD *ParentMft, string &FileName, vector<LONG64> &fileh80datarun, vector<DWORD> &fileh80datalen
	, string &fileh80data, vector<DWORD> &H20FileRefer, vector<string> checkfilename, DWORD VdiDataStartAddr, DWORD64 *fileRealSize)
{
	*ParentMft = NULL;
	*fileRealSize = NULL;
	FileName.clear();
	fileh80datarun.clear();
	fileh80datalen.clear();
	fileh80data.clear();
	H20FileRefer.clear();
	bool Ret=false;
	DWORD64 VirtualBackAddr = NULL;
	DWORD BackBytesCount = NULL;
	LFILE_Head_Recoding File_head_recod = NULL;
	LATTRIBUTE_HEADS ATTriBase = NULL;
	bool Found = false;
	LAttr_30H H30 = NULL;
	LAttr_20H H20 = NULL;
	UCHAR *H30_NAMES = NULL;
	UCHAR *H80_data = NULL;
	DWORD AttributeSize = NULL;
	DWORD FirstAttriSize = NULL;

	memset(CacheBuff, 0, FILE_SECTOR_SIZE);

	for (int i = 0; i < 2; i++)
	{
		VirtualBackAddr = NULL;
		if(!VdiOneAddrChange((VirtualStartPatition * SECTOR_SIZE + VirStartMftRfAddr * VirtualCuNum * SECTOR_SIZE + Rerefer * FILE_SECTOR_SIZE + SECTOR_SIZE * i)
			, VdiBatBuff, VdiBatSingleSize, &VirtualBackAddr, BatListSize, VdiDataStartAddr))
		{
			//CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualFileAddr:VhdOneAddrChange: ת����ʼMft�ļ���¼��ַʧ��!"));
			return true;
		}
		Ret = ReadSQData(hDrive, &CacheBuff[i * SECTOR_SIZE], SECTOR_SIZE, VirtualBackAddr,
			&BackBytesCount);		
		if(!Ret)
		{			
			CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVboxVirtualFileAddr:ReadSQData: ��ȡ��ʼMft�ļ���¼��ַʧ��!"));
			return false;	
		}
	}

	File_head_recod = (LFILE_Head_Recoding)&CacheBuff[0];
	if(File_head_recod->_FILE_Index == 0x454c4946 && File_head_recod->_Flags[0] != 0)
	{
		RtlCopyMemory(&CacheBuff[510], &CacheBuff[File_head_recod->_Update_Sequence_Number[0]+2], 2);//������������	
		RtlCopyMemory(&CacheBuff[1022],&CacheBuff[File_head_recod->_Update_Sequence_Number[0]+4], 2);
		RtlCopyMemory(&FirstAttriSize, &File_head_recod->_First_Attribute_Dev[0], 2);
		if (FirstAttriSize > (FILE_SECTOR_SIZE - sizeof(ATTRIBUTE_HEADS)))
		{
			CFuncs::WriteLogInfo(SLT_ERROR, "GetVboxVirtualFileAddr::File_head_recod->_First_Attribute_Dev[0] > FILE_SECTOR_SIZEʧ��!");
			return false;
		}
		while ((FirstAttriSize + AttributeSize) < FILE_SECTOR_SIZE)
		{
			ATTriBase = (LATTRIBUTE_HEADS)&CacheBuff[FirstAttriSize + AttributeSize];
			if(ATTriBase->_Attr_Type != 0xffffffff)
			{
				if (ATTriBase->_Attr_Type == 0x20)
				{
					DWORD h20Length = NULL;
					switch(ATTriBase->_PP_Attr)
					{
					case 0:
						if (ATTriBase->_AttrName_Length == 0)
						{
							h20Length = 24;
						} 
						else
						{
							h20Length = 24 + 2 * ATTriBase->_AttrName_Length;
						}
						break;
					case 0x01:
						if (ATTriBase->_AttrName_Length == 0)
						{
							h20Length = 64;
						} 
						else
						{
							h20Length = 64 + 2 * ATTriBase->_AttrName_Length;
						}
						break;
					}
					if (h20Length > (ATTriBase->_Attr_Length))
					{
						CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVboxVirtualFileAddr:h20Length > (ATTriBase->_Attr_Length)ʧ��!"));
						return false;
					}
					if (ATTriBase->_PP_Attr == 0)
					{
						H20 = (LAttr_20H)((UCHAR*)&ATTriBase[0] + h20Length);
						while (H20->_H20_TYPE != 0)
						{
												
							
							if (H20->_H20_TYPE == 0x80)
							{
								H20FileRefer.push_back(H20->_H20_FILE_Reference_Num.LowPart);
						
							}else if (H20->_H20_TYPE == 0)
							{
								break;
							}
							else if (H20->_H20_TYPE > 0xFF)
							{
								break;
							}
							if(H20->_H20_Attr_Name_Length * 2 > 0)
							{
								if ((H20->_H20_Attr_Name_Length * 2 + 26) % 8 != 0)
								{
									h20Length += (((H20->_H20_Attr_Name_Length * 2 + 26) / 8) * 8 + 8);
								}
								else if ((H20->_H20_Attr_Name_Length * 2 + 26) % 8 == 0)
								{
									h20Length += (H20->_H20_Attr_Name_Length * 2 + 26);
								}
							}
							else
							{
								h20Length += 32;
							}
							if (h20Length > (ATTriBase->_Attr_Length))
							{
								break;
							}
							H20 = (LAttr_20H)((UCHAR*)&ATTriBase[0] + h20Length);

						}
					} 
					else if (ATTriBase->_PP_Attr == 1)
					{
						UCHAR *H20Data = NULL;
						DWORD64 H20DataRun = NULL;

						H20Data = (UCHAR*)&ATTriBase[0];
						DWORD H20Offset = ATTriBase->TWOATTRIBUTEHEAD.NP_head._NPN_RunList_Offset[0];

						if (H20Offset > (FILE_SECTOR_SIZE - AttributeSize - FirstAttriSize))
						{
							CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVboxVirtualFileAddr:H20Offset������Χʧ��!"));
							return false;
						}

						if (H20Data[H20Offset] != 0 && H20Data[H20Offset] < 0x50)
						{
							UCHAR adres_fig = H20Data[H20Offset] >> 4;
							UCHAR len_fig = H20Data[H20Offset] & 0xf;
							for (int w = adres_fig; w > 0; w--)
							{
								H20DataRun = H20DataRun | (H20Data[H20Offset + w + len_fig] << (8 * (w - 1)));
							}
						}					
						UCHAR *H20CancheBuff = (UCHAR*)malloc(SECTOR_SIZE * VirtualCuNum + SECTOR_SIZE);
						if (NULL == H20CancheBuff)
						{
							CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVboxVirtualFileAddr:malloc: H20CancheBuffʧ��!"));
							return false;
						}
						memset(H20CancheBuff, 0, SECTOR_SIZE * VirtualCuNum);

						for (int i = 0; i < VirtualCuNum; i++)
						{					
							VirtualBackAddr = NULL;
							if(!VdiOneAddrChange((VirtualStartPatition * SECTOR_SIZE + H20DataRun * VirtualCuNum * SECTOR_SIZE + SECTOR_SIZE * i)
								, VdiBatBuff, VdiBatSingleSize, &VirtualBackAddr, BatListSize, VdiDataStartAddr))
							{
								free(H20CancheBuff);
								H20CancheBuff = NULL;
								CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVboxVirtualFileAddr:VhdOneAddrChange: ת����ʼMft�ļ���¼��ַʧ��!"));
								return false;
							}
							Ret = ReadSQData(hDrive, &H20CancheBuff[i*SECTOR_SIZE], SECTOR_SIZE, VirtualBackAddr,
								&BackBytesCount);		
							if(!Ret)
							{			
								free(H20CancheBuff);
								H20CancheBuff = NULL;
								CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVboxVirtualFileAddr:ReadSQData: ��ȡ��ʼMft�ļ���¼��ַʧ��!"));
								return false;	
							}
						}
						h20Length = 0;
						H20 = (LAttr_20H)&H20CancheBuff[h20Length];
						while (H20->_H20_TYPE != 0)
						{
							
							H20 = (LAttr_20H)&H20CancheBuff[h20Length];
							if (H20->_H20_TYPE == 0x80)
							{
								H20FileRefer.push_back(H20->_H20_FILE_Reference_Num.LowPart);

							}
							else if(H20->_H20_TYPE == 0)
							{
								break;
							}
							else if (H20->_H20_TYPE > 0xFF)
							{
								break;
							}
							if(H20->_H20_Attr_Name_Length * 2 > 0)
							{
								if ((H20->_H20_Attr_Name_Length * 2 + 26) % 8 != 0)
								{
									h20Length += (((H20->_H20_Attr_Name_Length * 2 + 26) / 8) * 8 + 8);
								}
								else if ((H20->_H20_Attr_Name_Length * 2 + 26) % 8 == 0)
								{
									h20Length += (H20->_H20_Attr_Name_Length * 2 + 26);
								}
							}
							else
							{
								h20Length += 32;
							}
							if (h20Length > (DWORD)(SECTOR_SIZE * VirtualCuNum))
							{
								break;
							}
						}	
						free(H20CancheBuff);
						H20CancheBuff = NULL;
					}
				}
				if (!Found)
				{
					if (ATTriBase->_Attr_Type == 0x30)
					{
						DWORD H30Size = NULL;
						switch(ATTriBase->_PP_Attr)
						{
						case 0:
							if (ATTriBase->_AttrName_Length == 0)
							{
								H30 = (LAttr_30H)((UCHAR*)&ATTriBase[0] + 24);
								H30_NAMES = (UCHAR*)&ATTriBase[0] + 24 + 66;
								H30Size = 24 + 66 + AttributeSize + FirstAttriSize;
							} 
							else
							{
								H30 = (LAttr_30H)((UCHAR*)&ATTriBase[0] + 24 + 2 * ATTriBase->_AttrName_Length);
								H30_NAMES = (UCHAR*)&ATTriBase[0] + 24 + 2 * ATTriBase->_AttrName_Length+66;
								H30Size = 24 + 2 * ATTriBase->_AttrName_Length + 66 + AttributeSize + FirstAttriSize;
							}
							break;
						case 0x01:
							if (ATTriBase->_AttrName_Length == 0)
							{
								H30 = (LAttr_30H)((UCHAR*)&ATTriBase[0] + 64);
								H30_NAMES = (UCHAR*)&ATTriBase[0] + 64 + 66;
								H30Size = 64 + 66 + AttributeSize + FirstAttriSize;
							} 
							else
							{
								H30 = (LAttr_30H)((UCHAR*)&ATTriBase[0] + 64 + 2 * ATTriBase->_AttrName_Length);
								H30_NAMES = (UCHAR*)&ATTriBase[0] + 64 + 2 * ATTriBase->_AttrName_Length + 66;
								H30Size = 64 + 2 * ATTriBase->_AttrName_Length + 66 + AttributeSize + FirstAttriSize;
							}
							break;
						}
						if ((FILE_SECTOR_SIZE - H30Size) < 0)
						{
							CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVboxVirtualFileAddr::(FILE_SECTOR_SIZE - H30Size)ʧ��!"));
							return false;
						}
						DWORD H30FileNameLen = H30->_H30_FILE_Name_Length * 2;
						if (H30FileNameLen > (FILE_SECTOR_SIZE - H30Size) || NULL == H30FileNameLen)
						{
							CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVboxVirtualFileAddr::������Χʧ��!"));
							return false;
						}
						string filename;
						if(!UnicodeToZifu(&H30_NAMES[0],filename, H30FileNameLen))
						{

							CFuncs::WriteLogInfo(SLT_ERROR, "GetVboxVirtualFileAddr����Unicode_To_Zifu::ת��ʧ��!");
							return false;
						}
						vector<string>::iterator viter;
						for (viter = checkfilename.begin(); viter != checkfilename.end(); viter ++)
						{
							if (filename.rfind(*viter) != string::npos)
							{
								size_t posion = filename.rfind(*viter);
								size_t c_posion = NULL;
								c_posion = filename.length() - posion;
								if (viter->length() == c_posion)
								{
									Found = true;
									break;
								}
							}
						}	
						if (Found)
						{

							CFuncs::WriteLogInfo(SLT_INFORMATION, "GetVboxVirtualFileAddr ���ļ���¼�ο�����:%lu",File_head_recod->_FR_Refer);
							*ParentMft = NULL;
							RtlCopyMemory(ParentMft,&H30->_H30_Parent_FILE_Reference,4);																		

							FileName.append((char*)&H30_NAMES[0],H30FileNameLen);

							if (H20FileRefer.size() > 0)
							{
								vector<DWORD>::iterator vec;
								for (vec = H20FileRefer.begin(); vec < H20FileRefer.end(); vec ++)
								{
									if (*vec != File_head_recod->_FR_Refer)
									{
										CFuncs::WriteLogInfo(SLT_INFORMATION, "GetVboxVirtualFileAddr ���ļ���¼H80�ض�λ��H20�У��ض�λ�ļ��ο�����:%lu", *vec);
									}
									else
									{
										H20FileRefer.erase(vec);//��ͬ�ľ�û�ض�λ������Ϊ��
									}
								}

							}


						}
																																															
						
					}
				}
				if (Found)
				{
					DWORD H80_datarun_len = NULL;
					LONG64 H80_datarun = NULL;
					if (ATTriBase->_Attr_Type == 0x80)
					{
						bool FirstIn = true;

						if (ATTriBase->_PP_Attr == 0x01)
						{
							(*fileRealSize) = ((*fileRealSize) + ATTriBase->TWOATTRIBUTEHEAD.NP_head._NPN_Attr_T_Size);//ȡ�ô��ļ�����ʵ��С
							H80_data = (UCHAR*)&ATTriBase[0];
							DWORD OFFSET = NULL;
							RtlCopyMemory(&OFFSET, &ATTriBase->TWOATTRIBUTEHEAD.NP_head._NPN_RunList_Offset[0], 2);
							if (OFFSET > (FILE_SECTOR_SIZE - FirstAttriSize - AttributeSize))
							{
								CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVboxVirtualFileAddr::OFFSET������Χ!"));
								return false;
							}
							if (H80_data[OFFSET] != 0 && H80_data[OFFSET] < 0x50)
							{					
								while(OFFSET < ATTriBase->_Attr_Length)
								{
									H80_datarun_len = NULL;
									H80_datarun = NULL;
									if (H80_data[OFFSET] > 0 && H80_data[OFFSET] < 0x50)
									{
										UCHAR adres_fig = H80_data[OFFSET] >> 4;
										UCHAR len_fig = H80_data[OFFSET] & 0xf;
										for(int w = len_fig; w > 0; w--)
										{
											H80_datarun_len = H80_datarun_len | (H80_data[OFFSET + w] << (8 * (w - 1)));
										}
										if (H80_datarun_len > 0)
										{
											fileh80datalen.push_back(H80_datarun_len);
										} 
										else
										{
											CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVboxVirtualFileAddr::H80_datarun_lenΪ0!"));
											return false;
										}

										for (int w = adres_fig; w > 0; w--)
										{
											H80_datarun = H80_datarun | (H80_data[OFFSET + w + len_fig] << (8 * (w - 1)));
										}
										if (H80_data[OFFSET + adres_fig + len_fig] > 127)
										{
											if (adres_fig == 3)
											{
												H80_datarun = ~(H80_datarun ^ 0xffffff);
											}
											if (adres_fig == 2)
											{
												H80_datarun = ~(H80_datarun ^ 0xffff);

											}

										} 
										if (FirstIn)
										{
											if (H80_datarun > 0)
											{
												fileh80datarun.push_back(H80_datarun);
											} 
											else
											{
												CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVboxVirtualFileAddr::H80_datarunΪ0��Ϊ��������!"));
												return false;
											}
											FirstIn = false;
										}
										else
										{
											if (fileh80datarun.size() > 0)
											{
												H80_datarun = fileh80datarun[fileh80datarun.size() - 1] + H80_datarun;
												fileh80datarun.push_back(H80_datarun);
											}
										}
										
										OFFSET = OFFSET + adres_fig + len_fig + 1;
									}
									else
									{
										break;
									}

								}								
							}

						}
						else if(ATTriBase->_PP_Attr == 0)
						{
							H80_data = (UCHAR*)&ATTriBase[0];	
							if (ATTriBase->TWOATTRIBUTEHEAD.P_head._PN_AttrBody_Length < (FILE_SECTOR_SIZE - AttributeSize - FirstAttriSize - 24))
							{
								fileh80data.append((char*)&H80_data[24],ATTriBase->TWOATTRIBUTEHEAD.P_head._PN_AttrBody_Length);
							}
						

						}

					}
				}
				if (ATTriBase->_Attr_Length < (FILE_SECTOR_SIZE - AttributeSize - FirstAttriSize) && ATTriBase->_Attr_Length > 0)
				{

					AttributeSize += ATTriBase->_Attr_Length;

				}  
				else
				{
					CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVboxVirtualFileAddr::������:%lu,ATTriBase[i_attr]->_Attr_Length���ȹ����Ϊ0!")
						,ATTriBase->_Attr_Length);
					return false;
				}
			}
			else if (ATTriBase->_Attr_Type == 0xffffffff)
			{
				if (!Found)
				{
					H20FileRefer.clear();
				}				
				memset(CacheBuff, 0, FILE_SECTOR_SIZE);
				break;
			}
			else if(ATTriBase->_Attr_Type > 0xff && ATTriBase->_Attr_Type < 0xffffffff)
			{
				CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVboxVirtualFileAddr:��ȡATTriBase[i_attr]->_Attr_Type����0xffffffff����"));
				return false;
			}

		}
	}
	return true;
}
bool GetVirtualMachineInfo::GetVirtualVdiFileNameAndPath(HANDLE hDrive, string FileName, UCHAR *CacheBuffer, DWORD ParentMFT, vector<LONG64> VirtualStartMFTaddr
	, vector<DWORD64> VirtualStartMFTaddrLen, UCHAR VirtualCuNum, DWORD64 VirtualNtfs, UCHAR *VdiBatBuff, DWORD VdiBatSingleSize, DWORD BatListSize
	, string& VirtualFilePath, DWORD VdiDataStartAddr)
{
	DWORD MFTnumber = NULL;
	bool  bRet = false;
	DWORD BackBytesCount = NULL;
	LFILE_Head_Recoding File_head_recod = NULL;
	LATTRIBUTE_HEADS ATTriBase = NULL;
	LAttr_30H H30 = NULL;
	UCHAR *H30_NAMES = NULL;
	string StrTem;

	StrTem.append("//");
	StrTem.append(FileName);

	File_head_recod = (LFILE_Head_Recoding)&CacheBuffer[0];


	MFTnumber = ParentMFT;


	DWORD numbers = NULL;
	while (MFTnumber != 5 && MFTnumber != 0)
	{
		DWORD AttributeSize = NULL;
		DWORD FirstAttriSize = NULL;
		if (numbers > 100)
		{
			CFuncs::WriteLogInfo(SLT_ERROR, "GetVirtualVdiFileNameAndPath:numbers ·���ļ�����30��������!");
			return false;
		}
		DWORD64 MftLenAdd = NULL;
		LONG64 MftAddr = NULL;

		for (DWORD FMft = 0; FMft < VirtualStartMFTaddrLen.size(); FMft++)
		{
			if ((MFTnumber * 2) <= (VirtualStartMFTaddrLen[FMft] * VirtualCuNum + MftLenAdd))
			{
				MftAddr = (VirtualStartMFTaddr[FMft] * VirtualCuNum + ((MFTnumber * 2) - MftLenAdd));
				break;
			} 
			else
			{
				MftLenAdd += (VirtualStartMFTaddrLen[FMft] * VirtualCuNum);
			}
		}
		DWORD64 VirtualBackAddr = NULL;
		memset(CacheBuffer, 0, FILE_SECTOR_SIZE);
		for (int i = 0; i < 2; i++)
		{
			VirtualBackAddr = NULL;
			if(!VdiOneAddrChange((VirtualNtfs * SECTOR_SIZE + MftAddr * SECTOR_SIZE + SECTOR_SIZE * i)
				, VdiBatBuff, VdiBatSingleSize, &VirtualBackAddr, BatListSize, VdiDataStartAddr))
			{
				CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualVdiFileNameAndPath:VhdOneAddrChange: ת����ʼMft�ļ���¼��ַʧ��!"));
				return true;
			}
			bRet = ReadSQData(hDrive, &CacheBuffer[i * SECTOR_SIZE], SECTOR_SIZE, VirtualBackAddr,
				&BackBytesCount);		
			if(!bRet)
			{			
				CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualVdiFileNameAndPath:ReadSQData: ��ȡ��ʼMft�ļ���¼��ַʧ��!"));
				return false;	
			}
		}

		if (File_head_recod->_FILE_Index != 0x454c4946 && File_head_recod->_FILE_Index > 0)
		{
			CFuncs::WriteLogInfo(SLT_ERROR, _T("�ҵ������ļ���¼����!"));
			return true;
		} 
		else if(File_head_recod->_FILE_Index == 0x454c4946)
		{
			RtlCopyMemory(&CacheBuffer[510], &CacheBuffer[File_head_recod->_Update_Sequence_Number[0]+2], 2);//������������	
			RtlCopyMemory(&CacheBuffer[1022],&CacheBuffer[File_head_recod->_Update_Sequence_Number[0]+4], 2);
			RtlCopyMemory(&FirstAttriSize, &File_head_recod->_First_Attribute_Dev[0], 2);
			if (FirstAttriSize > (FILE_SECTOR_SIZE - sizeof(ATTRIBUTE_HEADS)))
			{
				CFuncs::WriteLogInfo(SLT_ERROR, "GetVirtualVdiFileNameAndPath::File_head_recod->_First_Attribute_Dev[0] > FILE_SECTOR_SIZEʧ��!");
				return false;
			}
			string H30temName;
			while ((FirstAttriSize + AttributeSize) < FILE_SECTOR_SIZE)
			{
				ATTriBase = (LATTRIBUTE_HEADS)&CacheBuffer[FirstAttriSize + AttributeSize];
				if(ATTriBase->_Attr_Type != 0xffffffff)
				{
					if (ATTriBase->_Attr_Type == 0x30)
					{
						DWORD H30Size = NULL;
						switch(ATTriBase->_PP_Attr)
						{
						case 0:
							if (ATTriBase->_AttrName_Length == 0)
							{
								H30 = (LAttr_30H)((UCHAR*)&ATTriBase[0] + 24);
								H30_NAMES = (UCHAR*)&ATTriBase[0] + 24 + 66;
								H30Size = 24 + 66 + AttributeSize + FirstAttriSize;
							} 
							else
							{
								H30 = (LAttr_30H)((UCHAR*)&ATTriBase[0] + 24 + 2 * ATTriBase->_AttrName_Length);
								H30_NAMES = (UCHAR*)&ATTriBase[0] + 24 + 2 * ATTriBase->_AttrName_Length+66;
								H30Size = 24 + 2 * ATTriBase->_AttrName_Length + 66 + AttributeSize + FirstAttriSize;
							}
							break;
						case 0x01:
							if (ATTriBase->_AttrName_Length == 0)
							{
								H30 = (LAttr_30H)((UCHAR*)&ATTriBase[0] + 64);
								H30_NAMES = (UCHAR*)&ATTriBase[0] + 64 + 66;
								H30Size = 64 + 66 + AttributeSize + FirstAttriSize;
							} 
							else
							{
								H30 = (LAttr_30H)((UCHAR*)&ATTriBase[0] + 64 + 2 * ATTriBase->_AttrName_Length);
								H30_NAMES = (UCHAR*)&ATTriBase[0] + 64 + 2 * ATTriBase->_AttrName_Length + 66;
								H30Size = 64 + 2 * ATTriBase->_AttrName_Length + 66 + AttributeSize + FirstAttriSize;
							}
							break;
						}
						if ((FILE_SECTOR_SIZE - H30Size) < 0)
						{
							CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualVdiFileNameAndPath::(FILE_SECTOR_SIZE - H30Size)ʧ��!"));
							return false;
						}
						DWORD H30FileNameLen = H30->_H30_FILE_Name_Length * 2;
						if (H30FileNameLen > (FILE_SECTOR_SIZE - H30Size) || NULL == H30FileNameLen)
						{
							CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualVdiFileNameAndPath::������Χʧ��!"));
							return false;
						}
						H30temName.clear();
						MFTnumber = NULL;
						RtlCopyMemory(&MFTnumber, &H30->_H30_Parent_FILE_Reference, 4);
						H30temName.append("//");
						if (!UnicodeToZifu(&H30_NAMES[0], H30temName, H30FileNameLen))
						{

							CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualVdiFileNameAndPath:Unicode_To_Zifu:ת��ʧ��!"));
							return false;
						}	

						//	break;													
					}
					if (ATTriBase->_Attr_Length < (FILE_SECTOR_SIZE - AttributeSize - FirstAttriSize) && ATTriBase->_Attr_Length > 0)
					{

						AttributeSize += ATTriBase->_Attr_Length;
					}  
					else
					{								
						CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualVdiFileNameAndPath:���Գ��ȹ���!,������:%lu"),ATTriBase->_Attr_Length);
						return false;
					}
				}
				else if (ATTriBase->_Attr_Type == 0xffffffff)
				{
					break;
				}
				else if(ATTriBase->_Attr_Type > 0xff && ATTriBase->_Attr_Type < 0xffffffff)
				{
					CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualVdiFileNameAndPath:��ȡATTriBase[i_attr]->_Attr_Type����0xffffffff����"));
					return false;
				}
			}
			StrTem.append(H30temName);
		}
	}
	int laststring = (StrTem.length() - 1);
	for (int i = (StrTem.length()-1); i > 0; i--)
	{
		if (StrTem[i] == '/' && StrTem[i-1] == '/')
		{
			if ((laststring-i) > 0)
			{
				VirtualFilePath.append(&StrTem[i+1],(laststring-i));				
				VirtualFilePath.append("\\");

				laststring = (i-2);
			}

		}
	}

	return true;
}
bool GetVirtualMachineInfo::VirtualVdiWriteLargeFile(HANDLE hDrive, vector<LONG64> FileH80Addr, vector<DWORD> FileH80Len, UCHAR VirtualCuNum, UCHAR *VdiBatBuff
	, DWORD VdiBatSingleSize, DWORD BatListSize, char *WriteBuff ,const wchar_t *FileDir, DWORD VdiDataStartAddr, DWORD64 VirPatition
	, DWORD64 fileRealSize)
{
	if (FileH80Addr.size() != FileH80Len.size())
	{
		CFuncs::WriteLogInfo(SLT_ERROR, _T("VirtualWriteLargeFile :FileH80Addr.size() != FileH80Len.size()ʧ��!"));
		return false;
	}

	
	BOOL Ret = false;
	DWORD BackBytesCount = NULL;
	DWORD64 VirtualBackAddr = NULL;
	LARGE_INTEGER WriteIndex = {NULL};
	DWORD nNumberOfBytesWritten = NULL;


	HANDLE hFile_recov = ::CreateFileW(FileDir, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, NULL, NULL);
	if (hFile_recov == INVALID_HANDLE_VALUE)
	{
		CFuncs::WriteLogInfo(SLT_ERROR, "VirtualVdiWriteLargeFile:CreateFileWʧ��,������%d", GetLastError());
		return false;
	}

	for (DWORD H80Num = NULL; H80Num < FileH80Addr.size(); H80Num++)
	{

		DWORD FileAddrAddNumber = NULL;
		for (DWORD SectorCuNum = NULL; SectorCuNum < FileH80Len[H80Num]; SectorCuNum ++)
		{
			memset(WriteBuff, 0, VirtualCuNum * SECTOR_SIZE);

			for (DWORD Sector = NULL; Sector < VirtualCuNum; Sector ++)
			{
				VirtualBackAddr = NULL;
				if (!VdiOneAddrChange((VirPatition * SECTOR_SIZE + FileH80Addr[H80Num] * VirtualCuNum * SECTOR_SIZE + FileAddrAddNumber * SECTOR_SIZE)
					, VdiBatBuff, VdiBatSingleSize, &VirtualBackAddr, BatListSize
					, VdiDataStartAddr))
				{
					CFuncs::WriteLogInfo(SLT_ERROR, _T("VirtualVdiWriteLargeFile :VdiOneAddrChangeʧ��!"));
					return true;
				}
				Ret=ReadSQCharData(hDrive, &WriteBuff[Sector * SECTOR_SIZE], SECTOR_SIZE,
					VirtualBackAddr,
					&BackBytesCount);		
				if(!Ret)
				{		
					CloseHandle(hFile_recov);
					hFile_recov = NULL;
					CFuncs::WriteLogInfo(SLT_ERROR, "VHDWriteLargeFile:ReadSQDataʧ��,������%d", GetLastError());
					return false;	
				}
				FileAddrAddNumber ++;
			}
			Ret=SetFilePointerEx(hFile_recov,
				WriteIndex,
				NULL,
				FILE_BEGIN);
			if(!Ret)
			{
				CloseHandle(hFile_recov);
				hFile_recov = NULL;
				CFuncs::WriteLogInfo(SLT_ERROR, "VirtualVdiWriteLargeFile:SetFilePointerExʧ��,������%d", GetLastError());
				return false;	
			}
			if (((DWORD64)WriteIndex.QuadPart + (VirtualCuNum * SECTOR_SIZE)) > fileRealSize)
			{
				Ret = ::WriteFile(hFile_recov, WriteBuff, (DWORD)(fileRealSize - WriteIndex.QuadPart), &nNumberOfBytesWritten, NULL);
				if(!Ret)
				{	
					CFuncs::WriteLogInfo(SLT_ERROR, "VHDWriteLargeFile:WriteFileʧ��,������%d", GetLastError());
					(void)CloseHandle(hFile_recov);
					hFile_recov = NULL;
					return false;
				}
				(void)CloseHandle(hFile_recov);
				hFile_recov = NULL;
				return true;
			} 
			else
			{
				Ret = ::WriteFile(hFile_recov, WriteBuff, (DWORD)(VirtualCuNum * SECTOR_SIZE), &nNumberOfBytesWritten, NULL);
				if(!Ret)
				{	
					CFuncs::WriteLogInfo(SLT_ERROR, "VHDWriteLargeFile:WriteFileʧ��,������%d", GetLastError());
					(void)CloseHandle(hFile_recov);
					hFile_recov = NULL;
					return false;
				}
			}

			WriteIndex.QuadPart += (VirtualCuNum * SECTOR_SIZE);
		}

	}
	

	
	(void)CloseHandle(hFile_recov);
	hFile_recov = NULL;

	return true;
}
bool GetVirtualMachineInfo::GetVboxVmdkDescrip(char *descripBuff, vector<string> &vmdkname, DWORD *ParenUUID, string Vmdkpath)
{
	string descrip_str;
	size_t s_posion = NULL;
	size_t e_posion = NULL;

	descrip_str = string(descripBuff);
	while(descrip_str.find("SPARSE \"", e_posion) != string::npos)
	{
		s_posion = descrip_str.find("SPARSE \"", e_posion);
		e_posion = descrip_str.find(".vmdk\"", s_posion);
		if (e_posion != string::npos)
		{
			string vname;
			vname.append(Vmdkpath);
			vname.append(&descrip_str[s_posion + 8], (e_posion - s_posion - 3));
			vmdkname.push_back(vname);
		}
		else
		{
			break;
		}
	}
	s_posion = descrip_str.find("ddb.uuid.parent=\"");
	if (s_posion != string::npos)
	{
		string VdiUUID;
		if (descrip_str.length() > (s_posion + 25))
		{
			VdiUUID.append(&descrip_str[s_posion + 17], 8);
		}		
		if (!DwordStringToHex(ParenUUID, VdiUUID))
		{
			CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVboxVmdkDescrip :DwordStringToHexʧ��!"));
			return false;
		}
	}

	return true;
}
bool GetVirtualMachineInfo::GetVboxVmdkInfomation(map<DWORD, vector<string>> &VboxVmdkInfo, map<DWORD, string>VdiFileInfo)
{
	map<DWORD, string>::iterator vdiiter;
	DWORD dwError = NULL;
	bool Ret = false;
	DWORD BackBytesCount = NULL;

	DWORD mainNum = 1;
	for (vdiiter = VdiFileInfo.begin(); vdiiter != VdiFileInfo.end(); vdiiter ++)
	{
		size_t pathposion = NULL;
		string Vmdkpath;
		if (vdiiter->second.rfind("/") != string::npos)
		{
			pathposion = vdiiter->second.rfind("/");
			Vmdkpath.append(&vdiiter->second[0], pathposion + 1);
		}
		else if (vdiiter->second.rfind("\\") != string::npos)
		{
			pathposion = vdiiter->second.rfind("\\");
			Vmdkpath.append(&vdiiter->second[0], pathposion + 1);
		}
		DWORD ParenUUID = NULL;
		vector<string> vmdkName;
		HANDLE	VdiDevice = CreateFile(vdiiter->second.c_str(),//����ע�⣬���ֻ��һ�����̣�������Ҫ���ݸ������!!!!!
			GENERIC_READ,
			FILE_SHARE_READ,
			NULL,
			OPEN_EXISTING,
			0,
			NULL);
		if (VdiDevice == INVALID_HANDLE_VALUE) 
		{
			dwError=GetLastError();
			CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVboxVmdkInfomation VdiDevice = CreateFile��ȡvdi�ļ����ʧ��!,\
											   ���󷵻���: dwError = %d"), dwError);
			return false;
		}
		UCHAR *HeadBuff = (UCHAR*)malloc(FILE_SECTOR_SIZE);
		if (NULL == HeadBuff)
		{
			CloseHandle(VdiDevice);
			VdiDevice = NULL;
			CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVboxVmdkInfomation:malloc: HeadBuffʧ��!"));
			return false;
		}
		memset(HeadBuff, 0, FILE_SECTOR_SIZE);
		Ret = ReadSQData(VdiDevice, HeadBuff, SECTOR_SIZE, 0
			, &BackBytesCount);		
		if(!Ret)
		{			
			free(HeadBuff);
			HeadBuff = NULL;
			CloseHandle(VdiDevice);
			VdiDevice = NULL;
			CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVboxVmdkInfomation:ReadSQData: ��ȡ������ڶ�������Ϣʧ��!"));
			return false;	
		}
		if (HeadBuff[0] == 0x4b && HeadBuff[1] == 0x44 && HeadBuff[2] == 0x4d && HeadBuff[3] == 0x56)
		{
			LVirtual_head vmdkhead = (LVirtual_head)&HeadBuff[0];
			DWORD64 Descripfileoff = vmdkhead->_Description_file_off;
			DWORD64 DescripfileSize = vmdkhead->_Description_file_size;

			free(HeadBuff);
			HeadBuff = NULL;

			if (NULL == Descripfileoff || NULL == DescripfileSize)
			{
				CloseHandle(VdiDevice);
				VdiDevice = NULL;
				CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVboxVmdkInfomation:���ļ��������ļ�Ϊ��ʧ��!"));
				return false;
			}

			char *DescripBuff = (char*)malloc((size_t)DescripfileSize * SECTOR_SIZE + SECTOR_SIZE);
			if (NULL == DescripBuff)
			{
				CloseHandle(VdiDevice);
				VdiDevice = NULL;
				CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVboxVmdkInfomation:malloc:VmdkHeadBuff �����ڴ�ʧ��!"));
				return false;
			}
			memset(DescripBuff, 0, ((size_t)DescripfileSize * SECTOR_SIZE + SECTOR_SIZE));
			Ret=ReadSQCharData(VdiDevice, DescripBuff, ((DWORD)DescripfileSize * SECTOR_SIZE),
				Descripfileoff * SECTOR_SIZE,
				&BackBytesCount);		
			if(!Ret)
			{	
				CloseHandle(VdiDevice);
				VdiDevice = NULL;
				free(DescripBuff);
				DescripBuff = NULL;
				CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVboxVmdkInfomation:ReadSQData:����vmx����ʧ��!"));
				return false;	
			}
			CloseHandle(VdiDevice);
			VdiDevice = NULL;
			if (!GetVboxVmdkDescrip(DescripBuff, vmdkName, &ParenUUID, Vmdkpath))
			{
				free(DescripBuff);
				DescripBuff = NULL;
				CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVboxVmdkInfomation:GetVboxVmdkDescripʧ��!"));	
				return false;
			}
			free(DescripBuff);
			DescripBuff = NULL;
		} 
		else
		{
			free(HeadBuff);
			HeadBuff = NULL;

			DWORD VFileSiz = GetFileSize(VdiDevice, NULL);
			if (VFileSiz > 0)
			{

				char *ReadConfigInfoBuff = (char*) malloc(VFileSiz + SECTOR_SIZE);
				if (NULL == ReadConfigInfoBuff)
				{
					CloseHandle(VdiDevice);
					VdiDevice = NULL;
					CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVboxVmdkInfomation:malloc:ReadConfigInfoBuff �����ڴ�ʧ��!"));
					return false;	
				}
				memset(ReadConfigInfoBuff, 0, (VFileSiz + SECTOR_SIZE));


				Ret=ReadSQCharData(VdiDevice, ReadConfigInfoBuff, VFileSiz,
					0,
					&BackBytesCount);		
				if(!Ret)
				{	
					CloseHandle(VdiDevice);
					VdiDevice = NULL;
					free(ReadConfigInfoBuff);
					ReadConfigInfoBuff = NULL;
					CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVboxVmdkInfomation:ReadSQData:����vmx����ʧ��!"));
					return false;	
				}
				CloseHandle(VdiDevice);
				VdiDevice = NULL;
				if (!GetVboxVmdkDescrip(ReadConfigInfoBuff, vmdkName, &ParenUUID, Vmdkpath))
				{
					free(ReadConfigInfoBuff);
					ReadConfigInfoBuff = NULL;
					CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVboxVmdkInfomation:GetVboxVmdkDescripʧ��!"));	
					return false;
				}
				free(ReadConfigInfoBuff);
				ReadConfigInfoBuff = NULL;

			}
		}
		map<DWORD, string>::iterator iter_t;
		char num = 1;
		for (iter_t = VdiFileInfo.begin(); iter_t != VdiFileInfo.end(); iter_t ++)
		{
			if (ParenUUID == iter_t->first)
			{
				string num_str;
				num_str.append(&num, 1);
				VboxVmdkInfo[mainNum].push_back(num_str);
				for (DWORD Num = NULL; Num < vmdkName.size(); Num ++)
				{
					VboxVmdkInfo[mainNum].push_back(vmdkName[Num]);
				}
				
				//VboxVmdkInfo.insert(map<DWORD,vector<string>>::value_type(num,vmdkName));
				break;
			}
			num ++;
		}
		if (VboxVmdkInfo[mainNum].size() == NULL)
		{
			for (DWORD Num = NULL; Num < vmdkName.size(); Num ++)
			{
				VboxVmdkInfo[mainNum].push_back(vmdkName[Num]);
			}
		}
		mainNum ++;
	}
	return true;
}
bool GetVirtualMachineInfo::VboxFileCheck(string VBoxMftFileName, vector<string> checkfilename, const char* virtualFileDir, PFCallbackVirtualMachine VirtualFile)
{
	DWORD dwError = NULL;
	bool Ret = false;
	DWORD BackBytesCount = NULL;
	
	string VdiPath;
	size_t VdiPathPosition;
	size_t _VdiPathPosition;

	_VdiPathPosition = VBoxMftFileName.find("\\");
	while(_VdiPathPosition != string::npos)
	{

		if (VBoxMftFileName.find("\\", _VdiPathPosition + 1) == string::npos)
		{
			VdiPathPosition = VBoxMftFileName.find(".vbox");
			if ((VdiPathPosition - _VdiPathPosition + 5) > 0)
			{				
				VdiPath.append(&VBoxMftFileName[0],( _VdiPathPosition + 1));

			}else
			{
				CFuncs::WriteLogInfo(SLT_ERROR, "VboxFileCheck:��ȡvbox·��ʧ��!");
				return false;
			}

			break;
		}
		_VdiPathPosition = VBoxMftFileName.find("\\", _VdiPathPosition + 1);
	}


	map<DWORD, string>VdiFileInfo;
	int VBoxFileType = NULL;//��������VBox�ṹ�������Vdi����1��VMDK��2
	if(!GetVboxInformation(VdiFileInfo, VBoxMftFileName, VdiPath, &VBoxFileType))
	{
		CFuncs::WriteLogInfo(SLT_ERROR, "VboxFileCheck:GetVboxInformationʧ��!");
		return false;
	}
	if (VBoxFileType == 2)
	{
		map<DWORD, vector<string>> VboxVmdkInfo;
		if (!GetVboxVmdkInfomation(VboxVmdkInfo, VdiFileInfo))
		{
			CFuncs::WriteLogInfo(SLT_ERROR, "VboxFileCheck:GetVboxVmdkInfomationʧ��!");
			return false;
		}
		if (VboxVmdkInfo.size() == VdiFileInfo.size())
		{
			if (!AnalysisVmdkFile(VboxVmdkInfo,  checkfilename, virtualFileDir, VirtualFile))
			{
				CFuncs::WriteLogInfo(SLT_ERROR, "VboxFileCheck:AnalysisVmdkFileʧ��!");
				return false;
			}
		}
	}
	else if (VBoxFileType == 1)
	{
		map<DWORD, string>::iterator Vdiindex;
		for (Vdiindex = VdiFileInfo.begin(); Vdiindex != VdiFileInfo.end(); Vdiindex ++)//Vdi�ļ���ѭ������
		{
			DWORD ParentUUID = NULL;
			vector<DWORD64> VdiNTFSStartAddr;

			ParentUUID = Vdiindex->first;
			if(!GetVdiNTFSStartAddr(&ParentUUID, VdiFileInfo, VdiNTFSStartAddr))
			{
				CFuncs::WriteLogInfo(SLT_ERROR, "VboxFileCheck:GetVdiNTFSStartAddrʧ��!");
				return false;
			}



			for (DWORD NtfsIndex = 0; NtfsIndex < VdiNTFSStartAddr.size(); NtfsIndex ++)
			{
				DWORD64 VirtualPatition = VdiNTFSStartAddr[NtfsIndex];
				DWORD64 VirStartMftAddr = NULL;
				ParentUUID = Vdiindex->first;
				UCHAR m_VirtualCuNum = NULL;

				if (!GetVdiStartMftAddr(&ParentUUID, VdiFileInfo, VirtualPatition, &VirStartMftAddr, &m_VirtualCuNum))
				{

					CFuncs::WriteLogInfo(SLT_ERROR, "VboxFileCheck:GetVdiStartMftAddrʧ��!");
					return false;
				}
				if (NULL == VirStartMftAddr)
				{
					CFuncs::WriteLogInfo(SLT_ERROR, "VboxFileCheck:GetVdiStartMftAddrʧ��!");
					break;
				}
				ParentUUID = Vdiindex->first;
				vector<LONG64> v_VirtualStartMftAddr;
				vector<DWORD64> v_VirtualStartMftLen;

				if (!GetVdiAllMftAddr(&ParentUUID, VdiFileInfo, VirtualPatition, VirStartMftAddr, m_VirtualCuNum, v_VirtualStartMftAddr
					, v_VirtualStartMftLen))
				{

					CFuncs::WriteLogInfo(SLT_ERROR, "VboxFileCheck:GetVdiAllMftAddrʧ��!");
					return false;
				}

				HANDLE VirVdiDevice = CreateFile(Vdiindex->second.c_str(),//����ע�⣬���ֻ��һ�����̣�������Ҫ���ݸ������!!!!!
					GENERIC_READ,
					FILE_SHARE_READ,
					NULL,
					OPEN_EXISTING,
					0,
					NULL);
				if (VirVdiDevice == INVALID_HANDLE_VALUE) 
				{
					dwError=GetLastError();
					CFuncs::WriteLogInfo(SLT_ERROR, _T("VboxFileCheck VdiDevice = CreateFile��ȡvdi�ļ����ʧ��!,\
													   ���󷵻���: dwError = %d"), dwError);
					return false;
				}

				//��ȡͷ����Ϣ
				DWORD VirVdiBatAddr = NULL;
				DWORD VirVdiDataAddr = NULL;
				DWORD VirBatSingleSize = NULL;
				DWORD VirBatBuffSize=NULL;//����Bat����ܴ�С
				DWORD VirParentVdiUUID = NULL;
				if(!GetVdiHeadInformation(VirVdiDevice, &VirVdiBatAddr, &VirVdiDataAddr, &VirBatSingleSize, &VirParentVdiUUID))
				{
					CloseHandle(VirVdiDevice);
					VirVdiDevice = NULL;
					CFuncs::WriteLogInfo(SLT_ERROR, "VboxFileCheck:GetVdiHeadInformationʧ��!");
					return false;
				}
				VirBatBuffSize = VirVdiDataAddr - VirVdiBatAddr;
				if (VirBatBuffSize > 4096 * 5000)
				{
					CloseHandle(VirVdiDevice);
					VirVdiDevice = NULL;
					CFuncs::WriteLogInfo(SLT_ERROR, _T("VboxFileCheck :BatListSize����,ʧ��!"));
					return false;
				}
				UCHAR *VirVdiBatBuff = (UCHAR*) malloc(VirBatBuffSize + FILE_SECTOR_SIZE);
				if (NULL == VirVdiBatBuff)
				{
					CloseHandle(VirVdiDevice);
					VirVdiDevice = NULL;
					CFuncs::WriteLogInfo(SLT_ERROR, _T("VboxFileCheck :malloc:VdiBatBuffʧ��!"));
					return false;
				}
				memset(VirVdiBatBuff, 0, (VirBatBuffSize + FILE_SECTOR_SIZE));
				if(!GetVdiBatListInformation(VirVdiDevice, VirVdiBatBuff, VirVdiBatAddr, VirBatBuffSize))
				{
					CloseHandle(VirVdiDevice);
					VirVdiDevice = NULL;
					free(VirVdiBatBuff);
					VirVdiBatBuff = NULL;
					CFuncs::WriteLogInfo(SLT_ERROR, _T("VboxFileCheck :GetVdiBatListInformationʧ��!"));
					return false;
				}
				DWORD ReferNum = 0;//�ļ���¼����
				vector<LONG64> VirtualFileH80Addr;
				vector<DWORD> VirtualFileH80AddrLen;
				string VirtualFileBuffH80;
				string VirtualFileName;

				DWORD VirtualParentMft = NULL;
				vector<DWORD> VirtualH20Refer;
				DWORD64 VirtualH20DataRun = NULL;
				DWORD64 VirtualBackAddr = NULL;

				UCHAR* RecodeCacheBuff = (UCHAR*) malloc(FILE_SECTOR_SIZE + SECTOR_SIZE);
				if (NULL == RecodeCacheBuff)
				{
					CloseHandle(VirVdiDevice);
					VirVdiDevice = NULL;
					free(VirVdiBatBuff);
					VirVdiBatBuff = NULL;
					CFuncs::WriteLogInfo(SLT_ERROR, "VboxFileCheck:malloc:RecodeCacheBuffʧ��!");
					return false;
				}
				memset(RecodeCacheBuff, 0, FILE_SECTOR_SIZE + SECTOR_SIZE);


				DWORD64 FileRalSize = NULL;
				for (DWORD FileRecNum = 0; FileRecNum < v_VirtualStartMftAddr.size(); FileRecNum++)
				{
					ReferNum=0;

					while(GetVboxVirtualFileAddr(VirVdiDevice, VirtualPatition, v_VirtualStartMftAddr[FileRecNum], m_VirtualCuNum, ReferNum, VirVdiBatBuff, VirBatSingleSize
						, VirBatBuffSize, RecodeCacheBuff, &VirtualParentMft, VirtualFileName, VirtualFileH80Addr, VirtualFileH80AddrLen, VirtualFileBuffH80
						, VirtualH20Refer, checkfilename, VirVdiDataAddr, &FileRalSize))
					{
						if (VirtualH20Refer.size() > 0)
						{
							UCHAR *H20CacheBuff = (UCHAR*)malloc(FILE_SECTOR_SIZE + SECTOR_SIZE);
							if (NULL == H20CacheBuff)
							{
								CloseHandle(VirVdiDevice);
								VirVdiDevice = NULL;
								free(VirVdiBatBuff);
								VirVdiBatBuff = NULL;
								free(RecodeCacheBuff);
								RecodeCacheBuff = NULL;
								CFuncs::WriteLogInfo(SLT_ERROR, _T("VboxFileCheck:malloc: H20CacheBuffʧ��!"));
								return false;
							}
							vector<DWORD>::iterator h20vec;
							for (h20vec = VirtualH20Refer.begin(); h20vec < VirtualH20Refer.end(); h20vec++)
							{
								memset(H20CacheBuff, 0, FILE_SECTOR_SIZE);
								DWORD64 VirMftLen = NULL;
								DWORD64 VirStartMftRfAddr = NULL;
								for (DWORD FRN = 0; FRN < v_VirtualStartMftLen.size(); FRN++)
								{
									if (((*h20vec) * 2) < (VirMftLen + v_VirtualStartMftLen[FRN] * m_VirtualCuNum))
									{
										VirStartMftRfAddr = v_VirtualStartMftAddr[FRN] * m_VirtualCuNum + ((*h20vec) * 2 - VirMftLen);
										break;
									} 
									else
									{
										VirMftLen += (v_VirtualStartMftLen[FRN] * m_VirtualCuNum);
									}
								}
								for (int i = 0; i < 2; i++)
								{
									VirtualBackAddr = NULL;
									if(!VdiOneAddrChange((VirtualPatition * SECTOR_SIZE + VirStartMftRfAddr * SECTOR_SIZE + SECTOR_SIZE * i)
										, VirVdiBatBuff, VirBatSingleSize, &VirtualBackAddr, VirBatBuffSize, VirVdiDataAddr))
									{
										CloseHandle(VirVdiDevice);
										VirVdiDevice = NULL;
										free(VirVdiBatBuff);
										VirVdiBatBuff = NULL;
										free(RecodeCacheBuff);
										RecodeCacheBuff = NULL;
										free(H20CacheBuff);
										H20CacheBuff = NULL;

										CFuncs::WriteLogInfo(SLT_ERROR, _T("VboxFileCheck:VhdOneAddrChange: ת����ʼMft�ļ���¼��ַʧ��!"));
										return false;
									}
									Ret = ReadSQData(VirVdiDevice, &H20CacheBuff[i * SECTOR_SIZE], SECTOR_SIZE,  VirtualBackAddr,
										&BackBytesCount);		
									if(!Ret)
									{		
										CloseHandle(VirVdiDevice);
										VirVdiDevice = NULL;
										free(VirVdiBatBuff);
										VirVdiBatBuff = NULL;
										free(RecodeCacheBuff);
										RecodeCacheBuff = NULL;
										free(H20CacheBuff);
										H20CacheBuff = NULL;
										CFuncs::WriteLogInfo(SLT_ERROR, _T("VboxFileCheck:ReadSQData: ��ȡ��ʼMft�ļ���¼��ַʧ��!"));
										return false;	
									}
								}

								if(!GetVirtualH20FileReferH80Addr(H20CacheBuff, VirtualFileH80Addr, VirtualFileH80AddrLen, VirtualFileBuffH80, &FileRalSize))//��Ϊ��������ⲿ��ȡ�����ݣ�����������0	
								{
									CloseHandle(VirVdiDevice);
									VirVdiDevice = NULL;
									free(VirVdiBatBuff);
									VirVdiBatBuff = NULL;
									free(RecodeCacheBuff);
									RecodeCacheBuff = NULL;
									free(H20CacheBuff);
									H20CacheBuff = NULL;
									CFuncs::WriteLogInfo(SLT_ERROR, _T("VboxFileCheck:GetH20FileReferH80Addr: ʧ��!"));
									return false;
								}

							}
							free(H20CacheBuff);
							H20CacheBuff = NULL;
						}
						if (VirtualFileH80Addr.size() > 0)//����Ϊ��ַ����ȡ���ļ�
						{
							string VirtualPath;
							string StrTemName;
							if (VirtualFileName.length() > 0)
							{
								if(!UnicodeToZifu((UCHAR*)&VirtualFileName[0], StrTemName, VirtualFileName.length()))
								{
									CloseHandle(VirVdiDevice);
									VirVdiDevice = NULL;
									free(VirVdiBatBuff);
									VirVdiBatBuff = NULL;
									free(RecodeCacheBuff);
									RecodeCacheBuff = NULL;

									CFuncs::WriteLogInfo(SLT_ERROR, "VboxFileCheck:UnicodeToZifu : FileNameʧ��!");
									return false;
								}




								DWORD NameSize = VirtualFileName.length() + strlen(virtualFileDir);
								wchar_t * WirteName = new wchar_t[NameSize + 1];
								if (NULL == WirteName)
								{
									CloseHandle(VirVdiDevice);
									VirVdiDevice = NULL;
									free(VirVdiBatBuff);
									VirVdiBatBuff = NULL;
									free(RecodeCacheBuff);
									RecodeCacheBuff = NULL;

									CFuncs::WriteLogInfo(SLT_ERROR, _T("VboxFileCheck:new:WirteName ���������ڴ�ʧ��!"));
									return false;
								}
								memset(WirteName, 0, (NameSize + 1) * 2);
								MultiByteToWideChar(CP_ACP, 0, virtualFileDir, strlen(virtualFileDir), WirteName, NameSize);

								for (DWORD NameIndex = 0; NameIndex < VirtualFileName.length(); NameIndex += 2)
								{
									RtlCopyMemory(&WirteName[strlen(virtualFileDir) + NameIndex / 2],(UCHAR*) &VirtualFileName[NameIndex],2);
								}

								char *WriteFileBuffer=(char*)malloc(m_VirtualCuNum * SECTOR_SIZE + SECTOR_SIZE);
								if (NULL == WriteFileBuffer)
								{
									CloseHandle(VirVdiDevice);
									VirVdiDevice = NULL;
									free(VirVdiBatBuff);
									VirVdiBatBuff = NULL;
									free(RecodeCacheBuff);
									RecodeCacheBuff = NULL;
									delete WirteName;
									WirteName = NULL;
									CFuncs::WriteLogInfo(SLT_ERROR, "VboxFileCheck:malloc����WriteFileBuffer�ڴ�ʧ��!");
									return false;
								}
								memset(WriteFileBuffer, 0, (m_VirtualCuNum * SECTOR_SIZE + SECTOR_SIZE));
								if(!VirtualVdiWriteLargeFile(VirVdiDevice, VirtualFileH80Addr, VirtualFileH80AddrLen, m_VirtualCuNum, VirVdiBatBuff, VirBatSingleSize
									, VirBatBuffSize, WriteFileBuffer, WirteName, VirVdiDataAddr, VirtualPatition, FileRalSize))
								{
									delete WirteName;
									WirteName = NULL;
									free(WriteFileBuffer);
									WriteFileBuffer = NULL;
									CFuncs::WriteLogInfo(SLT_ERROR, "VboxFileCheck:WriteLargeFile:��ȡH20H80��ַʧ��ʧ��");
									break;
								}
								delete WirteName;
								WirteName = NULL;
								free(WriteFileBuffer);
								WriteFileBuffer = NULL;

								if(!GetVirtualVdiFileNameAndPath(VirVdiDevice, StrTemName, RecodeCacheBuff, VirtualParentMft, v_VirtualStartMftAddr, v_VirtualStartMftLen
									, m_VirtualCuNum, VirtualPatition, VirVdiBatBuff, VirBatSingleSize, VirBatBuffSize, VirtualPath, VirVdiDataAddr))
								{
									CFuncs::WriteLogInfo(SLT_ERROR, "VboxFileCheck:GetVirtualFileNameAndPath:��ȡ·��ʧ��");
									break;
								}
								VirtualFile( VirtualPath.c_str(), StrTemName.c_str());
							}

						}else if (VirtualFileBuffH80.length() > 0)//������h80���ȡС�ļ�
						{
							string VirtualPath;
							string StrTemName;
							if (VirtualFileName.length() > 0)
							{
								if(!UnicodeToZifu((UCHAR*)&VirtualFileName[0], StrTemName, VirtualFileName.length()))
								{
									CloseHandle(VirVdiDevice);
									VirVdiDevice = NULL;
									free(VirVdiBatBuff);
									VirVdiBatBuff = NULL;
									free(RecodeCacheBuff);
									RecodeCacheBuff = NULL;

									CFuncs::WriteLogInfo(SLT_ERROR, "VboxFileCheck:UnicodeToZifu : FileNameʧ��!");
									return false;
								}




								DWORD NameSize = VirtualFileName.length() + strlen(virtualFileDir);
								wchar_t * WirteName = new wchar_t[NameSize + 1];
								if (NULL == WirteName)
								{
									CloseHandle(VirVdiDevice);
									VirVdiDevice = NULL;
									free(VirVdiBatBuff);
									VirVdiBatBuff = NULL;
									free(RecodeCacheBuff);
									RecodeCacheBuff = NULL;

									CFuncs::WriteLogInfo(SLT_ERROR, _T("VboxFileCheck:new:WirteName ���������ڴ�ʧ��!"));
									return false;
								}
								memset(WirteName, 0, (NameSize + 1) * 2);
								MultiByteToWideChar(CP_ACP, 0, virtualFileDir, strlen(virtualFileDir), WirteName, NameSize);

								for (DWORD NameIndex = 0; NameIndex < VirtualFileName.length(); NameIndex += 2)
								{
									RtlCopyMemory(&WirteName[strlen(virtualFileDir) + NameIndex / 2],(UCHAR*) &VirtualFileName[NameIndex],2);
								}
								if(!VirtualWriteLitteFile(VirtualFileBuffH80, WirteName))
								{
									delete WirteName;
									WirteName=NULL;
									CFuncs::WriteLogInfo(SLT_ERROR, "VboxFileCheck:WriteLitteFile:ʧ��ʧ��");
									break;
								}
								delete WirteName;
								WirteName=NULL;

								if(!GetVirtualVdiFileNameAndPath(VirVdiDevice, StrTemName, RecodeCacheBuff, VirtualParentMft, v_VirtualStartMftAddr, v_VirtualStartMftLen
									, m_VirtualCuNum, VirtualPatition, VirVdiBatBuff, VirBatSingleSize, VirBatBuffSize, VirtualPath, VirVdiDataAddr))
								{
									CFuncs::WriteLogInfo(SLT_ERROR, "VboxFileCheck:GetVirtualFileNameAndPath:��ȡ·��ʧ��");
									break;
								}
								VirtualFile(VirtualPath.c_str(), StrTemName.c_str());
							}
						}
						ReferNum++;
						if ((ReferNum * 2) > v_VirtualStartMftLen[FileRecNum] * m_VirtualCuNum)
						{
							break;
						}
					}

				}

				free(VirVdiBatBuff);
				VirVdiBatBuff = NULL;
				free(RecodeCacheBuff);
				RecodeCacheBuff = NULL;

				CloseHandle(VirVdiDevice);
				VirVdiDevice = NULL;
			}


		}
	}	
		

	

	return true;
}
bool GetVirtualMachineInfo::GetcheckfileName(const char* checkExt, vector<string> &checkfilename)
{
	string checkname_str;
	string checkname_str_temp;
	checkname_str = string(checkExt);
	if (checkname_str.find(".") == string::npos)
	{
		CFuncs::WriteLogInfo(SLT_ERROR, "GetcheckfileName::Ѱ���ļ����Ͳ���ʧ��!");
		return false;
	}
	size_t sposion = NULL;
	size_t eposion = NULL;
	while (checkname_str.find(";", sposion) != string::npos)
	{
		eposion = checkname_str.find(";", sposion);
		if (checkname_str.length() > eposion)
		{
			eposion += 1;
		}
		else
		{
			break;
		}
		checkname_str_temp.clear();
		if ((eposion - sposion - 1) < 0)
		{
			CFuncs::WriteLogInfo(SLT_ERROR, "GetcheckfileName::(eposion - sposion - 1)ʧ��!");
			return false;
		}
		checkname_str_temp.append(&checkname_str[sposion], eposion - sposion - 1);
		checkfilename.push_back(checkname_str_temp);
		sposion = eposion;

	}
	if (checkname_str.find(";", sposion) == string::npos)
	{
		if (checkname_str.length() > sposion)
		{
			checkname_str_temp.clear();
			checkname_str_temp.append(&checkname_str[sposion], checkname_str.length() - sposion);
			checkfilename.push_back(checkname_str_temp);
		}
	}
	if (NULL == checkfilename.size())
	{
		CFuncs::WriteLogInfo(SLT_ERROR, "GetcheckfileName::Ѱ���ļ�����Ϊ��ʧ��!");
		return false;
	}

	return true;
}
bool GetVirtualMachineInfo::GetcheckVirtualFilePath(const char* CheckvirtualFileDir, vector<string> &VmdkParentMftBuff, string &VhdParentMftBuff
	, string &VboxParentMftBuff)
{
	string checkvirDir = string(CheckvirtualFileDir);
	if (checkvirDir.length() == NULL)
	{
		CFuncs::WriteLogInfo(SLT_ERROR, "GetcheckVirtualFilePath::���������·��Ϊ��ʧ��!");
		return false;
	}
	if (checkvirDir.find(".vmx") != string::npos && checkvirDir.find(".vmsd") != string::npos)
	{
		size_t sposion = NULL;
		if (checkvirDir.find(";") != string::npos)
		{
			sposion = checkvirDir.find(";");
			string vmxpath;
			vmxpath.append(&checkvirDir[0], sposion);
			string vmsdpath;
			if (checkvirDir.length() > (sposion + 1))
			{
				vmsdpath.append(&checkvirDir[sposion + 1], checkvirDir.length() - sposion);
			}
			VmdkParentMftBuff.push_back(vmxpath);
			VmdkParentMftBuff.push_back(vmsdpath);
		}
		else
		{
			CFuncs::WriteLogInfo(SLT_ERROR, "GetcheckVirtualFilePath::����VMWARE�����·������ʧ��!");
			return false;
		}
	} 
	else if(checkvirDir.find(".vbox") != string::npos)
	{
		VboxParentMftBuff.append(checkvirDir);
	}
	else if(checkvirDir.find(".vhd") != string::npos)
	{
		VhdParentMftBuff.append(checkvirDir);
	}
	else
	{
		CFuncs::WriteLogInfo(SLT_ERROR, "GetcheckVirtualFilePath::���������·������ʧ��!");
		return false;
	}

	return true;
}
bool GetVirtualMachineInfo::VirtualFileCheckFuc(const char* checkExt, const char* virtualFileDir,const char* CheckvirtualFileDir, PFCallbackVirtualMachine VirtualFile)
{
	DWORD dwError=NULL;//��ȡlasterror��Ϣ

	
	vector<string> checkfilename;
	if (!GetcheckfileName(checkExt, checkfilename))
	{
		CFuncs::WriteLogInfo(SLT_ERROR, "VirtualFileCheckFuc::GetcheckfileName!");
		return false;
	}
	vector<string> VmdkParentMftBuff;
	string VhdParentMftBuff;
	string VboxParentMftBuff;
	if (!GetcheckVirtualFilePath(CheckvirtualFileDir, VmdkParentMftBuff, VhdParentMftBuff, VboxParentMftBuff))
	{
		CFuncs::WriteLogInfo(SLT_ERROR, "VirtualFileCheckFuc::GetcheckVirtualFilePath!");
		return false;
	}
			
	if (VmdkParentMftBuff.size() > 0)
	{
		if (!VMwareFileCheck(VmdkParentMftBuff,checkfilename, virtualFileDir, VirtualFile))
		{
			CFuncs::WriteLogInfo(SLT_ERROR, "VirtualFileCheckFuc:VMwareFileCheckʧ��!");
			//return false;
		}
	}
	if (VhdParentMftBuff.length() > 0)
	{
		if (!VhdFileCheck(VhdParentMftBuff, checkfilename, virtualFileDir, VirtualFile))
		{
			CFuncs::WriteLogInfo(SLT_ERROR, "VirtualFileCheckFuc:VhdFileCheckʧ��!");
			//return false;
		}
	}
	if (VboxParentMftBuff.length() > 0)
	{
		if (!VboxFileCheck(VboxParentMftBuff,checkfilename, virtualFileDir, VirtualFile))
		{
			CFuncs::WriteLogInfo(SLT_ERROR, "VirtualFileCheckFuc:VboxFileCheckʧ��!");
			//return false;
		}
	}		
	

	
	return true;
}
bool GetVirtualMachineInfo::AnalysisInterPath(map<string,map<string, string>> &PathAndName, const char *recordFilePath)
{
	;
	if (strlen(recordFilePath) > NULL)
	{
		vector<string> singlerecord;
		string RecordPath_str = string(recordFilePath);
		size_t RecordPosion = RecordPath_str.find(';');
		size_t SRecordPosion =NULL;
		string R_Temp_str;
		if (RecordPosion != string::npos)
		{
			R_Temp_str.append(&RecordPath_str[0], RecordPosion);
			singlerecord.push_back(R_Temp_str);
		}
		else
		{
			singlerecord.push_back(RecordPath_str);
		}

		while(RecordPosion != string::npos)
		{
			R_Temp_str.clear();
			SRecordPosion = RecordPath_str.find(';', (RecordPosion + 1));
			if (SRecordPosion == string::npos)
			{
				R_Temp_str.append(&RecordPath_str[(RecordPosion + 1)], (RecordPath_str.length() - (RecordPosion + 1)));
				singlerecord.push_back(R_Temp_str);
				break;
			}
			R_Temp_str.append(&RecordPath_str[RecordPosion + 1], (SRecordPosion - RecordPosion - 1));
			singlerecord.push_back(R_Temp_str);
			RecordPosion = SRecordPosion;
		}

		size_t srposion = NULL;
		for (DWORD i = NULL; i < singlerecord.size(); i ++)
		{

			if (singlerecord[i][(singlerecord[i].length() - 1)] == '\\')
			{
				srposion = singlerecord[i].rfind('\\', (singlerecord[i].length() - 2));
				if (srposion == string::npos)
				{
					break;
				}
				string path;
				string pathname;
				string brower;
				pathname.append(&singlerecord[i][srposion + 1], (singlerecord[i].length() - srposion - 2));
				srposion = singlerecord[i].find(':');
				if (srposion == string::npos)
				{
					break;
				}
				path.append(&singlerecord[i][srposion + 1], (singlerecord[i].length() - srposion - 1));
				srposion = singlerecord[i].find(',');
				if (srposion == string::npos)
				{
					break;
				}
				brower.append(&singlerecord[i][0], srposion);
				PathAndName[brower][path] = pathname;
			} 
			else
			{
				srposion = singlerecord[i].rfind('\\', (singlerecord[i].length() - 1));
				if (srposion == string::npos)
				{
					break;
				}
				string path;
				string pathname;
				string brower;
				pathname.append(&singlerecord[i][srposion + 1], (singlerecord[i].length() - srposion - 1));
				srposion = singlerecord[i].find(':');
				if (srposion == string::npos)
				{
					break;
				}
				path.append(&singlerecord[i][srposion + 1], (singlerecord[i].length() - srposion - 1));
				srposion = singlerecord[i].find(',');
				if (srposion == string::npos)
				{
					break;
				}
				brower.append(&singlerecord[i][0], srposion);
				PathAndName[brower][path] = pathname;
			}
		}
	}
	if (PathAndName.size() != NULL)
	{
		return true;
	}
	else
	{
		CFuncs::WriteLogInfo(SLT_ERROR, "AnalysisInterPath::��ȡ������¼·�����󣬴���ʧ��!,�����´�ֵ");
		return false;
	}
}
bool GetVirtualMachineInfo::LookforVmdkInterFileRefer(DWORD64 VirtualStartNTFS, LONG64 VirStartMftRfAddr, DWORD64 VmdkFiletotalsize, UCHAR *CacheBuff, 
	UCHAR VirtualCuNum, DWORD RereferNumber, vector<LONG64> v_VirtualStartMFTaddr, vector<DWORD64> v_VirtualStartMFTaddrLen, vector<string> VirtualName
	, map<string, map<string, string>> PathName, int *PathFound, string &VirtualFilePath, DWORD *FileRefer, string &BrowerType, string &RecordFileName
	,  DWORD64 Grain_size, DWORD64 GrainNumber,DWORD64 GrainListOff,  int VmdkfileType, DWORD Catalogoff)
{
	BrowerType.clear();
	RecordFileName.clear();
	DWORD ParentMft = NULL;
	*PathFound = 0;
	*FileRefer = 0;
	bool Ret = false;
	DWORD64 VirtualBackAddr = NULL;
	DWORD BackBytesCount = NULL;
	LFILE_Head_Recoding File_head_recod = NULL;
	LATTRIBUTE_HEADS ATTriBase = NULL;
	LAttr_30H H30 = NULL;
	LAttr_20H H20 = NULL;
	UCHAR *H30_NAMES = NULL;
	DWORD AttributeSize = NULL;
	DWORD FirstAttriSize = NULL;

	memset(CacheBuff, 0, FILE_SECTOR_SIZE);

	DWORD64 FileReferBackAddr = NULL;
	DWORD VmdkFileNum = NULL;
	DWORD LeftSector = NULL;

	if(!VMwareAddressConversion(VirtualName, &FileReferBackAddr, (VirtualStartNTFS + VirStartMftRfAddr * VirtualCuNum + RereferNumber), VmdkFiletotalsize
		, &VmdkFileNum, Grain_size, GrainNumber, GrainListOff, &LeftSector, VmdkfileType, Catalogoff))
	{

		//CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualFileAddr:VMwareAddressConversion��Ϣʧ��!"));
		return true;
	}
	if (NULL == FileReferBackAddr)
	{

		//CFuncs::WriteLogInfo(SLT_INFORMATION, _T("GetVirtualFileAddr::�˵�ַΪ��!"));
		return true;
	}
	if (LeftSector >= 2)
	{
		if (!VMwareReadData(VirtualName[VmdkFileNum], CacheBuff, LeftSector, FileReferBackAddr, SECTOR_SIZE * 2))
		{

			CFuncs::WriteLogInfo(SLT_INFORMATION, _T("LookforVmdkInterFileRefer::VMwareReadDataʧ��!"));
			return false;
		}
	}
	else
	{
		if (!VMwareReadData(VirtualName[VmdkFileNum], CacheBuff, LeftSector, FileReferBackAddr, SECTOR_SIZE))
		{

			CFuncs::WriteLogInfo(SLT_INFORMATION, _T("LookforVmdkInterFileRefer::VMwareReadDataʧ��!"));
			return false;
		}
		FileReferBackAddr = NULL;
		if(!VMwareAddressConversion(VirtualName, &FileReferBackAddr, (VirtualStartNTFS + VirStartMftRfAddr * VirtualCuNum + RereferNumber + 1)
			, VmdkFiletotalsize, &VmdkFileNum, Grain_size, GrainNumber, GrainListOff, &LeftSector, VmdkfileType, Catalogoff))
		{

			//CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualFileAddr:VMwareAddressConversionʧ��!"));
			return true;
		}
		if (NULL == FileReferBackAddr)
		{

			//CFuncs::WriteLogInfo(SLT_INFORMATION, _T("GetVirtualFileAddr::�˵�ַΪ��!"));
			return true;
		}
		if (!VMwareReadData(VirtualName[VmdkFileNum], &CacheBuff[SECTOR_SIZE], LeftSector, FileReferBackAddr, SECTOR_SIZE))
		{
			CFuncs::WriteLogInfo(SLT_INFORMATION, _T("LookforVmdkInterFileRefer::VMwareReadDataʧ��!"));
			return false;
		}
	}

	File_head_recod = (LFILE_Head_Recoding)&CacheBuff[0];
	if(File_head_recod->_FILE_Index == 0x454c4946 && File_head_recod->_Flags[0] != 0)
	{
		RtlCopyMemory(&CacheBuff[510], &CacheBuff[File_head_recod->_Update_Sequence_Number[0]+2], 2);//������������	
		RtlCopyMemory(&CacheBuff[1022],&CacheBuff[File_head_recod->_Update_Sequence_Number[0]+4], 2);
		RtlCopyMemory(&FirstAttriSize, &File_head_recod->_First_Attribute_Dev[0], 2);
		if (FirstAttriSize > (FILE_SECTOR_SIZE - sizeof(ATTRIBUTE_HEADS)))
		{
			CFuncs::WriteLogInfo(SLT_ERROR, "LookforVmdkInterFileRefer::File_head_recod->_First_Attribute_Dev[0] > FILE_SECTOR_SIZEʧ��!");
			return false;
		}
		while ((FirstAttriSize + AttributeSize) < FILE_SECTOR_SIZE)
		{
			ATTriBase = (LATTRIBUTE_HEADS)&CacheBuff[FirstAttriSize + AttributeSize];
			if(ATTriBase->_Attr_Type != 0xffffffff)
			{
				if (ATTriBase->_Attr_Type == 0x30)
				{
					DWORD H30Size = NULL;
					switch(ATTriBase->_PP_Attr)
					{
					case 0:
						if (ATTriBase->_AttrName_Length == 0)
						{
							H30 = (LAttr_30H)((UCHAR*)&ATTriBase[0] + 24);
							H30_NAMES = (UCHAR*)&ATTriBase[0] + 24 + 66;
							H30Size = 24 + 66 + AttributeSize + FirstAttriSize;
						} 
						else
						{
							H30 = (LAttr_30H)((UCHAR*)&ATTriBase[0] + 24 + 2 * ATTriBase->_AttrName_Length);
							H30_NAMES = (UCHAR*)&ATTriBase[0] + 24 + 2 * ATTriBase->_AttrName_Length+66;
							H30Size = 24 + 2 * ATTriBase->_AttrName_Length + 66 + AttributeSize + FirstAttriSize;
						}
						break;
					case 0x01:
						if (ATTriBase->_AttrName_Length == 0)
						{
							H30 = (LAttr_30H)((UCHAR*)&ATTriBase[0] + 64);
							H30_NAMES = (UCHAR*)&ATTriBase[0] + 64 + 66;
							H30Size = 64 + 66 + AttributeSize + FirstAttriSize;
						} 
						else
						{
							H30 = (LAttr_30H)((UCHAR*)&ATTriBase[0] + 64 + 2 * ATTriBase->_AttrName_Length);
							H30_NAMES = (UCHAR*)&ATTriBase[0] + 64 + 2 * ATTriBase->_AttrName_Length + 66;
							H30Size = 64 + 2 * ATTriBase->_AttrName_Length + 66 + AttributeSize + FirstAttriSize;
						}
						break;
					}
					if ((FILE_SECTOR_SIZE - H30Size) < 0)
					{
						CFuncs::WriteLogInfo(SLT_ERROR, _T("LookforVmdkInterFileRefer::(FILE_SECTOR_SIZE - H30Size)ʧ��!"));
						return false;
					}
					DWORD H30FileNameLen = H30->_H30_FILE_Name_Length * 2;
					if (H30FileNameLen > (FILE_SECTOR_SIZE - H30Size) || NULL == H30FileNameLen)
					{
						CFuncs::WriteLogInfo(SLT_ERROR, _T("LookforVmdkInterFileRefer::������Χʧ��!"));
						return false;
					}
					else
					{
						
						RtlCopyMemory(&ParentMft,&H30->_H30_Parent_FILE_Reference,4);
						string FileName_str;
						if (!UnicodeToZifu(&H30_NAMES[0], FileName_str, H30FileNameLen))
						{
							CFuncs::WriteLogInfo(SLT_ERROR, _T("LookforVmdkInterFileRefer:ReadSQData: ��ȡ��ʼMft�ļ���¼��ַʧ��!"));
							return false;
						}
						bool Getpathfirst = true;
						map<string, map<string,string>>::iterator AllPathiter;	
						for (AllPathiter = PathName.begin(); AllPathiter != PathName.end(); AllPathiter ++)
						{
							BrowerType = AllPathiter->first;
							map<string, string>::iterator SinglePathiter;
							for (SinglePathiter = AllPathiter->second.begin(); SinglePathiter != AllPathiter->second.end(); SinglePathiter ++)
							{
								if (BrowerType == "UC")
								{
									if (FileName_str.find("History.") != string::npos)
									{
										if (Getpathfirst)
										{
											VirtualFilePath.clear();
											if(!GetVirtualFilePath(VirtualStartNTFS, v_VirtualStartMFTaddr, v_VirtualStartMFTaddrLen, VirtualCuNum, ParentMft
												, VirtualFilePath, FileName_str, VmdkFiletotalsize, Grain_size, GrainNumber, GrainListOff, VirtualName
												, VmdkfileType, Catalogoff))
											{
												CFuncs::WriteLogInfo(SLT_ERROR, "LookforVmdkInterFileRefer:GetVirtualFilePath:��ȡ·��ʧ��");
											}
											if (VirtualFilePath.length() > 0)
											{
												VirtualFilePath.erase((VirtualFilePath.length() - 1), 1);
											}
											if (VirtualFilePath.find("History.") != string::npos)
											{
												size_t posion = VirtualFilePath.find("History.");
												VirtualFilePath.erase(posion + 7, VirtualFilePath.length() - posion + 7);
											}
																						

											Getpathfirst = false;
										}
										string Tem_VirPathName;
										string Tem_PathName;
										if (SinglePathiter->first[SinglePathiter->first.length() -1] == '\\')
										{
											Tem_PathName.append(&SinglePathiter->first[0], SinglePathiter->first.length() -1);
										}else
										{
											Tem_PathName.append(&SinglePathiter->first[0], SinglePathiter->first.length());
										}
										if (VirtualFilePath.length() != Tem_PathName.length())
										{
											if (VirtualFilePath.length() > Tem_PathName.length())
											{
												Tem_VirPathName.append(&VirtualFilePath[VirtualFilePath.length() - Tem_PathName.length()], Tem_PathName.length());
											}
											else
											{
												Tem_VirPathName.append(VirtualFilePath);
											}
											if (Tem_VirPathName == Tem_PathName)
											{
												*FileRefer = File_head_recod->_FR_Refer;

												if (SinglePathiter->first[SinglePathiter->first.length() -1] == '\\')
												{
													*PathFound = 1;//Ŀ¼
												}else
												{
													*PathFound = 2;//�ļ�
													//RecordFileName = SinglePathiter->second;
													RecordFileName.append((char*)&H30_NAMES[0],H30FileNameLen);
												}

												break;
											}
										}else if (VirtualFilePath.length() == Tem_PathName.length())
										{
											if (VirtualFilePath == Tem_PathName)
											{
												*FileRefer = File_head_recod->_FR_Refer;
												if (SinglePathiter->first[SinglePathiter->first.length() -1] == '\\')
												{
													*PathFound = 1;//Ŀ¼
												}else
												{
													*PathFound = 2;//�ļ�
													RecordFileName.append((char*)&H30_NAMES[0], H30FileNameLen);
												}
												break;
											}
										}
									}
								}
								else if (SinglePathiter->second == FileName_str)
								{			
									if (Getpathfirst)
									{
										VirtualFilePath.clear();
										if(!GetVirtualFilePath(VirtualStartNTFS, v_VirtualStartMFTaddr, v_VirtualStartMFTaddrLen, VirtualCuNum, ParentMft
											, VirtualFilePath, FileName_str, VmdkFiletotalsize, Grain_size, GrainNumber, GrainListOff, VirtualName
											, VmdkfileType, Catalogoff))
										{
											CFuncs::WriteLogInfo(SLT_ERROR, "LookforVmdkInterFileRefer:GetVirtualFilePath:��ȡ·��ʧ��");
										}
										if (VirtualFilePath.length() > 0)
										{
											VirtualFilePath.erase((VirtualFilePath.length() - 1), 1);
										}
										if (BrowerType == "firefox")
										{
											size_t Bposion = NULL;
											size_t rBposion = NULL;
											Bposion = VirtualFilePath.rfind("\\");
											if (Bposion != string::npos)
											{
												rBposion = VirtualFilePath.rfind("\\", (Bposion - 1));
												if (rBposion != string::npos)
												{
													string path_Tem;
													path_Tem.append(&VirtualFilePath[0], rBposion);
													path_Tem.append(&VirtualFilePath[Bposion], (VirtualFilePath.length() - Bposion));
													VirtualFilePath.clear();
													VirtualFilePath.append(path_Tem);
													;												}
											}
										}
										else if (BrowerType == "maxthon")
										{
											size_t Bposion = NULL;
											size_t rBposion = NULL;
											Bposion = VirtualFilePath.rfind("History\\");
											if (Bposion != string::npos)
											{
												rBposion = VirtualFilePath.rfind("\\", (Bposion - 2));
												if (rBposion != string::npos)
												{
													string path_Tem;
													path_Tem.append(&VirtualFilePath[0], rBposion);
													path_Tem.append(&VirtualFilePath[Bposion - 1], (VirtualFilePath.length() - (Bposion - 1)));
													VirtualFilePath.clear();
													VirtualFilePath.append(path_Tem);
												}

											}
										}
										
										Getpathfirst = false;
									}
									string Tem_VirPathName;
									string Tem_PathName;
									if (SinglePathiter->first[SinglePathiter->first.length() -1] == '\\')
									{
										Tem_PathName.append(&SinglePathiter->first[0], SinglePathiter->first.length() -1);
									}else
									{
										Tem_PathName.append(&SinglePathiter->first[0], SinglePathiter->first.length());
									}
									if (VirtualFilePath.length() != Tem_PathName.length())
									{
										if (VirtualFilePath.length() > Tem_PathName.length())
										{
											Tem_VirPathName.append(&VirtualFilePath[VirtualFilePath.length() - Tem_PathName.length()], Tem_PathName.length());
										}
										else
										{
											Tem_VirPathName.append(VirtualFilePath);
										}
										if (Tem_VirPathName == Tem_PathName)
										{
											*FileRefer = File_head_recod->_FR_Refer;

											if (SinglePathiter->first[SinglePathiter->first.length() -1] == '\\')
											{
												*PathFound = 1;//Ŀ¼
											}else
											{
												*PathFound = 2;//�ļ�
												//RecordFileName = SinglePathiter->second;
												RecordFileName.append((char*)&H30_NAMES[0], H30FileNameLen);
											}

											break;
										}
									}else if (VirtualFilePath.length() == Tem_PathName.length())
									{
										if (VirtualFilePath == Tem_PathName)
										{
											*FileRefer = File_head_recod->_FR_Refer;
											if (SinglePathiter->first[SinglePathiter->first.length() -1] == '\\')
											{
												*PathFound = 1;//Ŀ¼
											}else
											{
												*PathFound = 2;//�ļ�
												RecordFileName.append((char*)&H30_NAMES[0], H30FileNameLen);
											}
											break;
										}
									}

								}

							}
							if ((*PathFound) > 0)
							{
								return true;
							}
						}
					}
				}

				if (ATTriBase->_Attr_Length < (FILE_SECTOR_SIZE - AttributeSize - FirstAttriSize) && ATTriBase->_Attr_Length > 0)
				{

					AttributeSize += ATTriBase->_Attr_Length;

				}
				else
				{
					CFuncs::WriteLogInfo(SLT_ERROR, _T("LookforVmdkInterFileRefer::������:%lu,ATTriBase[i_attr]->_Attr_Length���ȹ����Ϊ0!")
						,ATTriBase->_Attr_Length);
					return false;
				}
			}
			else if (ATTriBase->_Attr_Type==0xffffffff)
			{

				break;
			}
			else if(ATTriBase->_Attr_Type > 0xff && ATTriBase->_Attr_Type < 0xffffffff)
			{
				CFuncs::WriteLogInfo(SLT_ERROR, _T("LookforVmdkInterFileRefer:��ȡATTriBase[i_attr]->_Attr_Type����0xffffffff����"));
				return false;
			}

		}
	}
	return true;
}
bool GetVirtualMachineInfo::GetIndexHeadInfo(DWORD *IndexMemberSize, DWORD *IndexRealSize, LONG64 HA0Addr, vector<string> VirtualName
	, DWORD64 VirNtfsAddr, UCHAR m_VirtualCuNum, DWORD64 VmdkFiletotalsize,  DWORD *IndexHeadSize, vector<UCHAR> &IndexUpdata,  DWORD64 Grain_size
	, DWORD64 GrainNumber,DWORD64 GrainListOff,  int VmdkfileType, DWORD Catalogoff)
{
	DWORD64 IndexBackAddr = NULL;
	DWORD BackBytesCount = NULL;
	bool Ret = false;
	LSTANDARD_INDEX_HEAD Indexhead = NULL;

	UCHAR *IndexBuff = (UCHAR*)malloc(FILE_SECTOR_SIZE + 1);
	if (NULL == IndexBuff)
	{
		CFuncs::WriteLogInfo(SLT_ERROR, _T("GetIndexHeadInfo:IndexBuff����ʧ��!"));
		return false;
	}
	memset(IndexBuff, 0, FILE_SECTOR_SIZE);


	DWORD VmdkFileNum = NULL;
	DWORD LeftSector = NULL;

	if(!VMwareAddressConversion(VirtualName, &IndexBackAddr,(VirNtfsAddr + HA0Addr * m_VirtualCuNum), VmdkFiletotalsize, &VmdkFileNum
		, Grain_size, GrainNumber, GrainListOff, &LeftSector, VmdkfileType, Catalogoff))
	{
		free(IndexBuff);
		IndexBuff = NULL;
		CFuncs::WriteLogInfo(SLT_ERROR, _T("GetIndexHeadInfo:VMwareAddressConversionʧ��!"));
		return true;
	}
	if (NULL == IndexBackAddr)
	{
		free(IndexBuff);
		IndexBuff = NULL;
		CFuncs::WriteLogInfo(SLT_INFORMATION, _T("GetIndexHeadInfo::IndexBackAddr�˵�ַΪ��!"));
		return true;
	}

	if (!VMwareReadData(VirtualName[VmdkFileNum], IndexBuff, LeftSector, IndexBackAddr, SECTOR_SIZE))
	{
		free(IndexBuff);
		IndexBuff = NULL;
		CFuncs::WriteLogInfo(SLT_INFORMATION, _T("GetIndexHeadInfo::VMwareReadDataʧ��!"));
		return false;
	}
	
	DWORD UpdataOffset = NULL;
	DWORD UpdataSize = NULL;
	Indexhead = (LSTANDARD_INDEX_HEAD)&IndexBuff[0];
	if (Indexhead->_Head_Index != 0x58444e49)
	{
		free(IndexBuff);
		IndexBuff = NULL;
		CFuncs::WriteLogInfo(SLT_INFORMATION, _T("GetIndexHeadInfo:����ͷ��־����ȷ!"));
		return true;
	}

	RtlCopyMemory(&UpdataOffset, (UCHAR*)&Indexhead->_Updat_Sequ_Num_Off[0], 2);
	RtlCopyMemory(&UpdataSize, (UCHAR*)&Indexhead->_Updat_Sequ_Num_Off_Size[0], 2);
	if (UpdataOffset > SECTOR_SIZE || UpdataSize > 0xff)
	{
		free(IndexBuff);
		IndexBuff = NULL;
		CFuncs::WriteLogInfo(SLT_ERROR, _T("GetIndexHeadInfo:UpdataOffset > SECTOR_SIZE || UpdataSize > 0xff!"));
		return false;
	}

	IndexUpdata.clear();
	for (DWORD updnum =0; updnum < (UpdataSize-1) * 2; updnum ++)
	{
		IndexUpdata.push_back(IndexBuff[UpdataOffset + 2 + updnum]);
	}
	(*IndexMemberSize) = Indexhead->_Index_FB_Size; 
	(*IndexRealSize) = Indexhead->_Index_Term_Size;

	DWORD UpdataNum = NULL;
	if ((UpdataSize * 2) % 8 != 0)
	{
		UpdataNum = UpdataNum + ((UpdataSize * 2) / 8 ) + 1;
	}else
	{
		UpdataNum = UpdataNum + ((UpdataSize * 2) / 8 );
	}
	DWORD64 Judge = NULL;
	for (DWORD i = 0; i < 20; i ++)
	{
		RtlCopyMemory(&Judge, &IndexBuff[(5 + UpdataNum + i) * 8], 8);
		if (Judge != 0)
		{
			(*IndexHeadSize) = (5 + UpdataNum + i) * 8;
			break;
		}
	}
	if (NULL == (*IndexHeadSize))
	{
		free(IndexBuff);
		IndexBuff = NULL;
		CFuncs::WriteLogInfo(SLT_ERROR, _T("GetIndexHeadInfo:����ͷ(*IndexHeadSize)��СΪ0!"));
		return false;
	}

	Indexhead = NULL;
	free(IndexBuff);
	IndexBuff = NULL;

	if (NULL == (*IndexMemberSize) || NULL == (*IndexRealSize))
	{
		CFuncs::WriteLogInfo(SLT_ERROR, _T("GetIndexHeadInfo:����������С��ʵ�ʴ�СΪ0!"));
		return false;
	}

	return true;
}
bool GetVirtualMachineInfo::GetHA0FileRecordRefer( UCHAR m_VirtualCuNum, DWORD64 VmdkFiletotalsize, vector<string> VirtualName
	, DWORD64 VirNtfsAddr, vector<LONG64> HA0addr, vector<DWORD> HA0len, DWORD64 HA0RealSize, map<DWORD, string> &BrowerfileRefer
	,  DWORD64 Grain_size, DWORD64 GrainNumber,DWORD64 GrainListOff,  int VmdkfileType, DWORD Catalogoff)
{
	DWORD IndexMemberSize = NULL;
	DWORD IndexRealSize = NULL;
	DWORD IndexheadSize = NULL;
	vector<UCHAR> IndexUpdata;
	bool Ret = false;
	DWORD BackBytesCount = NULL;
	for (DWORD AddrNum = 0; AddrNum < HA0addr.size(); AddrNum ++)
	{
		DWORD IndexNumber = NULL;
		while(IndexNumber < HA0len[AddrNum])
		{
			DWORD ReadCuNum = NULL;
			for (DWORD num = 0; num < AddrNum; num ++)
			{
				ReadCuNum += HA0len[num];
			}
			ReadCuNum += IndexNumber;
			if ((ReadCuNum * m_VirtualCuNum *SECTOR_SIZE) >= HA0RealSize)
			{
				break;
			}
			if (!GetIndexHeadInfo( &IndexMemberSize, &IndexRealSize, (HA0addr[AddrNum] + IndexNumber), VirtualName, VirNtfsAddr
				, m_VirtualCuNum,VmdkFiletotalsize, &IndexheadSize, IndexUpdata, Grain_size, GrainNumber, GrainListOff, VmdkfileType, Catalogoff))
			{
				CFuncs::WriteLogInfo(SLT_ERROR, _T("GetHA0FileRecordRefer:GetIndexHeadInfoͷ��ʧ��!"));
				return false;
			}
			DWORD IndexSingleCuNumber = NULL;
			if ((IndexMemberSize / (SECTOR_SIZE * m_VirtualCuNum)) > 0)
			{
				if ((IndexMemberSize % (SECTOR_SIZE * m_VirtualCuNum)) > 0)
				{
					IndexSingleCuNumber = (IndexMemberSize / (SECTOR_SIZE * m_VirtualCuNum)) + 1;

				} 
				else
				{
					IndexSingleCuNumber = (IndexMemberSize / (SECTOR_SIZE * m_VirtualCuNum));
				}
				if (IndexSingleCuNumber > 0xffff)
				{
					CFuncs::WriteLogInfo(SLT_ERROR, _T("GetHA0FileRecordRefer:IndexSingleSize����ʧ��!"));
					return false;
				}
			} 
			else
			{
				IndexSingleCuNumber = 1;
			}
			if (IndexRealSize > (IndexSingleCuNumber *  m_VirtualCuNum * SECTOR_SIZE))
			{
				CFuncs::WriteLogInfo(SLT_ERROR, _T("GetHA0FileRecordRefer:�����������ȴ��ڷ����Сʧ��!"));
				return false;
			}
			UCHAR *IndexBuff = (UCHAR*)malloc(IndexSingleCuNumber * m_VirtualCuNum *SECTOR_SIZE + SECTOR_SIZE);
			if (NULL == IndexBuff)
			{
				CFuncs::WriteLogInfo(SLT_ERROR, _T("GetHA0FileRecordRefer:malloc:IndexBuffʧ��!"));
				return false;
			}
			memset(IndexBuff, 0, (IndexSingleCuNumber * m_VirtualCuNum *SECTOR_SIZE + SECTOR_SIZE));
			DWORD64 IndexBackAddr = NULL;
			DWORD VmdkFileNum = NULL;
			DWORD LeftSector = NULL;

			if(!VMwareAddressConversion(VirtualName, &IndexBackAddr,(VirNtfsAddr + (HA0addr[AddrNum] + IndexNumber) * m_VirtualCuNum), VmdkFiletotalsize
				, &VmdkFileNum, Grain_size, GrainNumber, GrainListOff, &LeftSector, VmdkfileType, Catalogoff))
			{
				free(IndexBuff);
				IndexBuff = NULL;
				CFuncs::WriteLogInfo(SLT_ERROR, _T("GetHA0FileRecordRefer:VMwareAddressConversionʧ��!"));
				return true;
			}
			if (NULL == IndexBackAddr)
			{
				free(IndexBuff);
				IndexBuff = NULL;
				CFuncs::WriteLogInfo(SLT_INFORMATION, _T("GetHA0FileRecordRefer::IndexBackAddr�˵�ַΪ��!"));
				return true;
			}
			if (LeftSector >= (IndexSingleCuNumber * m_VirtualCuNum))
			{
				if (!VMwareReadData(VirtualName[VmdkFileNum], IndexBuff, LeftSector, IndexBackAddr, (IndexSingleCuNumber * m_VirtualCuNum *SECTOR_SIZE)))
				{
					free(IndexBuff);
					IndexBuff = NULL;
					CFuncs::WriteLogInfo(SLT_INFORMATION, _T("GetHA0FileRecordRefer::VMwareReadDataʧ��!"));
					return false;
				}
			} 
			else
			{
				for(DWORD reNum = 0; reNum < (IndexSingleCuNumber * m_VirtualCuNum); reNum ++)
				{
					IndexBackAddr = NULL;
					VmdkFileNum = NULL;
					LeftSector = NULL;
					if(!VMwareAddressConversion(VirtualName, &IndexBackAddr,(VirNtfsAddr + (HA0addr[AddrNum] + IndexNumber) * m_VirtualCuNum + reNum), VmdkFiletotalsize
						, &VmdkFileNum, Grain_size, GrainNumber, GrainListOff, &LeftSector, VmdkfileType, Catalogoff))
					{
						free(IndexBuff);
						IndexBuff = NULL;
						CFuncs::WriteLogInfo(SLT_ERROR, _T("GetHA0FileRecordRefer:VMwareAddressConversionʧ��!"));
						return true;
					}
					if (NULL == IndexBackAddr)
					{
						free(IndexBuff);
						IndexBuff = NULL;
						CFuncs::WriteLogInfo(SLT_INFORMATION, _T("GetHA0FileRecordRefer::IndexBackAddr�˵�ַΪ��!"));
						return true;
					}
					if (!VMwareReadData(VirtualName[VmdkFileNum], &IndexBuff[reNum * SECTOR_SIZE], LeftSector, IndexBackAddr, SECTOR_SIZE))
					{
						free(IndexBuff);
						IndexBuff = NULL;
						CFuncs::WriteLogInfo(SLT_INFORMATION, _T("GetHA0FileRecordRefer::VMwareReadDataʧ��!"));
						return false;
					}
				}
			}
			if ((IndexUpdata.size() / 2) > IndexSingleCuNumber * m_VirtualCuNum)
			{
				free(IndexBuff);
				IndexBuff = NULL;
				CFuncs::WriteLogInfo(SLT_ERROR, _T("GetHA0FileRecordRefer:�����������������ʧ��!"));
				return false;
			}
			for (DWORD updataNum = 0; updataNum < (IndexUpdata.size() / 2); updataNum ++)
			{
				RtlCopyMemory(&IndexBuff[510 + (updataNum * 512)], &IndexUpdata[updataNum * 2], 1);
				RtlCopyMemory(&IndexBuff[511 + (updataNum * 512)], &IndexUpdata[(updataNum * 2) + 1], 1);
			}
			DWORD IndexTotalSize = IndexheadSize;
			LSTANDARD_INDEX_TERMS IndexTerm = NULL;
			while (IndexTotalSize < (IndexRealSize + 8))
			{
				IndexTerm = (LSTANDARD_INDEX_TERMS)&IndexBuff[IndexTotalSize];

				DWORD fileRefer = NULL;

				RtlCopyMemory(&fileRefer, &IndexTerm->_File_MFT_Refer_Num[0], 4);

				UCHAR *fileName = NULL;

				fileName = (UCHAR*)&IndexTerm[0] + 82;

				BrowerfileRefer[fileRefer].append((char*)&fileName[0], (IndexTerm->_FileName_Length * 2));

				DWORD IndexoneSize = NULL;

				RtlCopyMemory(&IndexoneSize, &IndexTerm->_TIndex_Term_Size[0], 2);

				if (NULL == IndexoneSize || IndexoneSize > ((IndexRealSize + 8) - IndexTotalSize))
				{
					free(IndexBuff);
					IndexBuff = NULL;
					CFuncs::WriteLogInfo(SLT_ERROR, _T("GetHA0FileRecordRefer:��������IndexoneSize��СΪ0!"));
					return false;
				}

				IndexTotalSize += IndexoneSize;
			}

			IndexNumber += IndexSingleCuNumber;
			free(IndexBuff);
			IndexBuff = NULL;
		}

	}

	return true;
}
bool GetVirtualMachineInfo::GetCatalogFileRefer(UCHAR *fileRecordBuff,  UCHAR m_VirtualCuNum, DWORD64 VmdkFiletotalsize, DWORD64 VirNtfsAddr
	, map<DWORD, string> &BrowerfileRefer, DWORD64 Grain_size, DWORD64 GrainNumber,DWORD64 GrainListOff,  int VmdkfileType, DWORD Catalogoff
	, vector<string> VirtualName)
{
	bool Ret = false;
	DWORD BackBytesCount = NULL;
	LFILE_Head_Recoding File_head_recod = NULL;
	LATTRIBUTE_HEADS ATTriBase = NULL;
	DWORD64 VirtualBackAddr = NULL;
	LAttr_90H_Index_ROOT  H90_root;
	LAttr_90H_Index_Head  H90_head;
	LAttr_90H_Index_Entry H90_entry;	
	UCHAR *H90_name;

	DWORD AttributeSize = NULL;
	DWORD FirstAttriSize = NULL;


	File_head_recod = (LFILE_Head_Recoding)&fileRecordBuff[0];
	RtlCopyMemory(&FirstAttriSize, &File_head_recod->_First_Attribute_Dev[0], 2);
	if (FirstAttriSize > (FILE_SECTOR_SIZE - sizeof(ATTRIBUTE_HEADS)))
	{
		CFuncs::WriteLogInfo(SLT_ERROR, "GetCatalogFileRefer::File_head_recod->_First_Attribute_Dev[0] > FILE_SECTOR_SIZEʧ��!");
		return false;
	}
	while ((FirstAttriSize + AttributeSize) < FILE_SECTOR_SIZE)
	{
		ATTriBase = (LATTRIBUTE_HEADS)&fileRecordBuff[FirstAttriSize + AttributeSize];
		if(ATTriBase->_Attr_Type != 0xffffffff)
		{
			if (ATTriBase->_Attr_Type == 0x90)
			{
				DWORD H90Size = NULL;
				switch(ATTriBase->_PP_Attr)
				{
				case 0:
					if (ATTriBase->_AttrName_Length == 0)
					{
						H90_root = (LAttr_90H_Index_ROOT)((UCHAR*)&ATTriBase[0] + 24);
						H90_head = (LAttr_90H_Index_Head)((UCHAR*)&ATTriBase[0] + 24 + 16);
						H90_entry = (LAttr_90H_Index_Entry)((UCHAR*)&ATTriBase[0] + 24 + 32);
						H90Size = 24 + 32 + AttributeSize + FirstAttriSize;
					} 
					else
					{
						H90_root = (LAttr_90H_Index_ROOT)((UCHAR*)&ATTriBase[0] + 24 + 2 * ATTriBase->_AttrName_Length);
						H90_head = (LAttr_90H_Index_Head)((UCHAR*)&ATTriBase[0] + 24 + 2 * ATTriBase->_AttrName_Length + 16);
						H90_entry = (LAttr_90H_Index_Entry)((UCHAR*)&ATTriBase[0] + 24 + 2 * ATTriBase->_AttrName_Length + 32);
						H90Size = 24 + 2 * ATTriBase->_AttrName_Length + 32 + AttributeSize + FirstAttriSize;
					}
					break;
				case 0x01:
					if (ATTriBase->_AttrName_Length == 0)
					{
						H90_root = (LAttr_90H_Index_ROOT)((UCHAR*)&ATTriBase[0] + 64);
						H90_head = (LAttr_90H_Index_Head)((UCHAR*)&ATTriBase[0] + 64 + 16);
						H90_entry = (LAttr_90H_Index_Entry)((UCHAR*)&ATTriBase[0] + 64 + 32);
						H90Size = 64 + 32 + AttributeSize + FirstAttriSize;
					} 
					else
					{
						H90_root = (LAttr_90H_Index_ROOT)((UCHAR*)&ATTriBase[0] + 64 + 2 * ATTriBase->_AttrName_Length);
						H90_head = (LAttr_90H_Index_Head)((UCHAR*)&ATTriBase[0] + 64 + 2 * ATTriBase->_AttrName_Length + 16);
						H90_entry = (LAttr_90H_Index_Entry)((UCHAR*)&ATTriBase[0] + 64 + 2 * ATTriBase->_AttrName_Length + 32);
						H90Size = 64 + 2 * ATTriBase->_AttrName_Length + 32 + AttributeSize + FirstAttriSize;
					}
					break;
				}
				if (H90_head->_H90_IH_Index_Total_Size > 82 && H90_head->_H90_IH_Index_Total_Size < (FILE_SECTOR_SIZE - AttributeSize - FirstAttriSize))
				{
					DWORD entrySize = NULL;
					for (DWORD entryoff = 0; entryoff < (H90_head->_H90_IH_Index_Total_Size - 16); entryoff += entrySize)
					{
						H90_entry = (LAttr_90H_Index_Entry)((UCHAR*)H90_entry + entrySize);
						H90_name = (UCHAR*)H90_entry + 82;
						entrySize = NULL;
						RtlCopyMemory(&entrySize, &H90_entry->_H90_IE_Index_Size[0], 2);
						if (NULL == entrySize || entrySize > (H90_head->_H90_IH_Index_Total_Size - entryoff))
						{
							
							CFuncs::WriteLogInfo(SLT_ERROR, _T("GetCatalogFileRefer:H90entrySizeΪ�ջ�H90entrySize��������������ֽ���!"));
							return false;
						}
						
						DWORD filerecord = NULL;

						RtlCopyMemory(&filerecord, &H90_entry->_H90_IE_MFT_Reference_Index[0], 4);
						if (filerecord != NULL)
						{
							BrowerfileRefer[filerecord].append((char*)&H90_name[0], (H90_entry->_H90_IE_FILE_Name_Length * 2));
						}

					}
				}

			}
			else if (ATTriBase->_Attr_Type == 0xA0)
			{
				DWORD HA0datarunlen = NULL;
				LONG64 HA0datarun = NULL;
				DWORD64 A0FileRealSize = NULL;
				UCHAR *HA0data = NULL;
				vector<DWORD> HA0Addrlen;
				vector<LONG64> HA0Addr;
				bool HA0first = true;
				if (ATTriBase->_PP_Attr == 0x01)
				{
					A0FileRealSize = ATTriBase->TWOATTRIBUTEHEAD.NP_head._NPN_Attr_T_Size;//ȡ�ô��ļ�����ʵ��С
					HA0data = (UCHAR*)&ATTriBase[0];
					DWORD OFFSET = NULL;
					RtlCopyMemory(&OFFSET, &ATTriBase->TWOATTRIBUTEHEAD.NP_head._NPN_RunList_Offset[0], 2);

					if (OFFSET > (FILE_SECTOR_SIZE - FirstAttriSize - AttributeSize))
					{
						
						CFuncs::WriteLogInfo(SLT_ERROR, _T("GetCatalogFileRefer::HA0OFFSET����Χ!"));
						return false;
					}
					if (HA0data[OFFSET] != 0 && HA0data[OFFSET] < 0x50)
					{					
						while(OFFSET < ATTriBase->_Attr_Length)
						{
							HA0datarunlen = NULL;
							HA0datarun = NULL;
							if (HA0data[OFFSET] > 0 && HA0data[OFFSET] < 0x50)
							{
								UCHAR adres_fig = HA0data[OFFSET] >> 4;
								UCHAR len_fig = HA0data[OFFSET] & 0xf;
								for(int w = len_fig;w > 0; w--)
								{
									HA0datarunlen = HA0datarunlen | (HA0data[OFFSET + w] << (8 * (w - 1)));
								}
								if (HA0datarunlen > 0)
								{
									HA0Addrlen.push_back(HA0datarunlen);
								} 
								else
								{
									CFuncs::WriteLogInfo(SLT_ERROR, _T("GetCatalogFileRefer::H80_datarun_lenΪ0!"));
									return false;
								}

								for (int w = adres_fig; w > 0; w --)
								{
									HA0datarun = HA0datarun | (HA0data[OFFSET + w + len_fig] << (8 * (w - 1)));
								}
								if (HA0data[OFFSET + adres_fig + len_fig] > 127)
								{
									if (adres_fig == 3)
									{
										HA0datarun = ~(HA0datarun^0xffffff);
									}
									if (adres_fig == 2)
									{
										HA0datarun = ~(HA0datarun^0xffff);

									}

								} 
								if (HA0first)
								{
									if (HA0datarun > 0)
									{
										HA0Addr.push_back(HA0datarun);
									} 
									else
									{
										CFuncs::WriteLogInfo(SLT_ERROR, _T("GetCatalogFileRefer::H80_datarunΪ0��Ϊ��������!"));
										return false;
									}
									HA0first = false;
								}
								else
								{
									if (HA0Addr.size() > 0)
									{
										HA0datarun = HA0Addr[HA0Addr.size() - 1] + HA0datarun;
										HA0Addr.push_back(HA0datarun);
									}
								}
								
								OFFSET = OFFSET + adres_fig + len_fig + 1;
							}
							else
							{
								break;
							}

						}								
					}
				}
				if (HA0Addr.size() > 0 && HA0Addr.size() == HA0Addrlen.size())
				{
					if(!GetHA0FileRecordRefer(m_VirtualCuNum, VmdkFiletotalsize, VirtualName, VirNtfsAddr, HA0Addr, HA0Addrlen, A0FileRealSize
						, BrowerfileRefer, Grain_size, GrainNumber, GrainListOff, VmdkfileType, Catalogoff))
					{
						CFuncs::WriteLogInfo(SLT_ERROR, _T("GetCatalogFileRefer:GetHA0FileRecordReferʧ��!"));
						return false;
					}
				}
				break;
			}
			if (ATTriBase->_Attr_Length < (FILE_SECTOR_SIZE - AttributeSize - FirstAttriSize) && ATTriBase->_Attr_Length > 0)
			{

				AttributeSize += ATTriBase->_Attr_Length;

			}  
			else
			{
				CFuncs::WriteLogInfo(SLT_ERROR, _T("GetCatalogFileRefer::������:%lu,ATTriBase[i_attr]->_Attr_Length���ȹ����Ϊ0!")
					,ATTriBase->_Attr_Length);
				return false;
			}
		}
		else if (ATTriBase->_Attr_Type == 0xffffffff)
		{
			break;
		}
		else if(ATTriBase->_Attr_Type > 0xff && ATTriBase->_Attr_Type < 0xffffffff)
		{
			CFuncs::WriteLogInfo(SLT_ERROR, _T("GetCatalogFileRefer:��ȡATTriBase[i_attr]->_Attr_Type����0xffffffff����"));
			return false;
		}
	}
	return true;
}
bool GetVirtualMachineInfo::GetInternetFileAddr(UCHAR *FilrRecordBuff, vector<DWORD> &RecordH20Refer, vector<LONG64> &FileH80Addr, vector<DWORD> &FileH80len, 
	string &FileH80Data, DWORD64 *FileRealSize, UCHAR VirtualCuNum,DWORD64 VmdkFiletotalsize, DWORD64 VirtualStartNTFS, DWORD64 Grain_size, DWORD64 GrainNumber,DWORD64 GrainListOff,  int VmdkfileType, DWORD Catalogoff
	, vector<string> VirtualName)
{
	*FileRealSize = NULL;
	bool Ret = false;
	DWORD BackBytesCount = NULL;
	LFILE_Head_Recoding File_head_recod = NULL;
	LATTRIBUTE_HEADS ATTriBase = NULL;
	LAttr_20H H20 = NULL;
	UCHAR *H80_data = NULL;
	DWORD64 VirtualBackAddr = NULL;
	DWORD AttributeSize = NULL;
	DWORD FirstAttriSize = NULL;

	File_head_recod = (LFILE_Head_Recoding)&FilrRecordBuff[0];
	RtlCopyMemory(&FilrRecordBuff[510], &FilrRecordBuff[File_head_recod->_Update_Sequence_Number[0]+2], 2);//������������	
	RtlCopyMemory(&FilrRecordBuff[1022],&FilrRecordBuff[File_head_recod->_Update_Sequence_Number[0]+4], 2);
	RtlCopyMemory(&FirstAttriSize, &File_head_recod->_First_Attribute_Dev[0], 2);
	if (FirstAttriSize > (FILE_SECTOR_SIZE - sizeof(ATTRIBUTE_HEADS)))
	{
		CFuncs::WriteLogInfo(SLT_ERROR, "GetInternetFileAddr::File_head_recod->_First_Attribute_Dev[0] > FILE_SECTOR_SIZEʧ��!");
		return false;
	}
	while ((FirstAttriSize + AttributeSize) < FILE_SECTOR_SIZE)
	{
		ATTriBase = (LATTRIBUTE_HEADS)&FilrRecordBuff[FirstAttriSize + AttributeSize];
		if(ATTriBase->_Attr_Type != 0xffffffff)
		{
			if (ATTriBase->_Attr_Type == 0x20)
			{
				DWORD h20Length=NULL;
				switch(ATTriBase->_PP_Attr)
				{
				case 0:
					if (ATTriBase->_AttrName_Length==0)
					{
						h20Length = 24;
					} 
					else
					{
						h20Length=24 + 2 * ATTriBase->_AttrName_Length;
					}
					break;
				case 0x01:
					if (ATTriBase->_AttrName_Length==0)
					{
						h20Length = 64;
					} 
					else
					{
						h20Length = 64 + 2 * ATTriBase->_AttrName_Length;
					}
					break;
				}
				if (h20Length > (ATTriBase->_Attr_Length))
				{
					CFuncs::WriteLogInfo(SLT_ERROR, _T("GetInternetFileAddr:h20Length > (ATTriBase->_Attr_Length)ʧ��!"));
					return false;
				}
				if (ATTriBase->_PP_Attr == 0)
				{
					H20 = (LAttr_20H)((UCHAR*)&ATTriBase[0] + h20Length);
					while (H20->_H20_TYPE != 0)
					{
												
						if (H20->_H20_TYPE==0x80)
						{							
							RecordH20Refer.push_back(H20->_H20_FILE_Reference_Num.LowPart);
					
						}
						else if (H20->_H20_TYPE == 0)
						{
							break;
						}
						else if (H20->_H20_TYPE > 0xFF)
						{
							break;
						}
						if(H20->_H20_Attr_Name_Length*2>0)
						{
							if ((H20->_H20_Attr_Name_Length*2+26)%8!=0)
							{
								h20Length+=(((H20->_H20_Attr_Name_Length*2+26)/8)*8+8);
							}
							else if ((H20->_H20_Attr_Name_Length*2+26)%8==0)
							{
								h20Length+=(H20->_H20_Attr_Name_Length*2+26);
							}
						}
						else
						{
							h20Length+=32;
						}
						if (h20Length > (ATTriBase->_Attr_Length))
						{
							break;
						}
						H20=(LAttr_20H)((UCHAR*)&ATTriBase[0]+h20Length);

					}
				} 
				else if (ATTriBase->_PP_Attr==1)
				{
					UCHAR *H20Data=NULL;
					DWORD64 H20DataRun=NULL;
					H20Data=(UCHAR*)&ATTriBase[0];
					DWORD H20Offset=ATTriBase->TWOATTRIBUTEHEAD.NP_head._NPN_RunList_Offset[0];

					if (H20Offset > (FILE_SECTOR_SIZE - AttributeSize - FirstAttriSize))
					{
						CFuncs::WriteLogInfo(SLT_ERROR, _T("GetInternetFileAddr:H20Offset������Χʧ��!"));
						return false;
					}

					if (H20Data[H20Offset] != 0 && H20Data[H20Offset] < 0x50)
					{
						UCHAR adres_fig=H20Data[H20Offset]>>4;
						UCHAR len_fig=H20Data[H20Offset]&0xf;
						for (int w=adres_fig;w>0;w--)
						{
							H20DataRun=H20DataRun | (H20Data[H20Offset+w+len_fig] << (8*(w-1)));
						}
					}		
					UCHAR *H20CancheBuff = (UCHAR*)malloc(SECTOR_SIZE * VirtualCuNum + SECTOR_SIZE);
					if (NULL == H20CancheBuff)
					{
						CFuncs::WriteLogInfo(SLT_ERROR, _T("GetInternetFileAddr:malloc: H20CancheBuffʧ��!"));
						return false;
					}
					memset(H20CancheBuff, 0, SECTOR_SIZE * VirtualCuNum + SECTOR_SIZE);
					DWORD VmdkFileNum = NULL;
					DWORD LeftSector = NULL;
					for (int i=0; i < VirtualCuNum; i++)
					{					
						VirtualBackAddr=NULL;
						VmdkFileNum = NULL;
						LeftSector = NULL;
						if(!VMwareAddressConversion(VirtualName, &VirtualBackAddr, (VirtualStartNTFS + H20DataRun * VirtualCuNum + i), VmdkFiletotalsize
							, &VmdkFileNum, Grain_size, GrainNumber, GrainListOff, &LeftSector, VmdkfileType, Catalogoff))
						{
							free(H20CancheBuff);
							H20CancheBuff = NULL;
							CFuncs::WriteLogInfo(SLT_ERROR, _T("GetInternetFileAddr:VMwareAddressConversion��Ϣʧ��!"));
							return true;
						}
						if (NULL == VirtualBackAddr)
						{

							CFuncs::WriteLogInfo(SLT_INFORMATION, _T("GetInternetFileAddr::�˵�ַΪ��!"));
							break;
						}

						if (!VMwareReadData(VirtualName[VmdkFileNum], &H20CancheBuff[i * SECTOR_SIZE], LeftSector, VirtualBackAddr, SECTOR_SIZE))
						{
							free(H20CancheBuff);
							H20CancheBuff = NULL;
							CFuncs::WriteLogInfo(SLT_INFORMATION, _T("GetInternetFileAddr::VMwareReadDataʧ��!"));
							return false;
						}

					}
					h20Length = 0;
					H20 = (LAttr_20H)&H20CancheBuff[h20Length];
					while (H20->_H20_TYPE != 0)
					{
						
						H20 = (LAttr_20H)&H20CancheBuff[h20Length];
						if (H20->_H20_TYPE == 0x80)
						{
							RecordH20Refer.push_back(H20->_H20_FILE_Reference_Num.LowPart);
					
						}
						else if (H20->_H20_TYPE == 0)
						{
							break;
						}
						else if (H20->_H20_TYPE > 0xFF)
						{
							break;
						}
						if(H20->_H20_Attr_Name_Length * 2 > 0)
						{
							if ((H20->_H20_Attr_Name_Length * 2 + 26) % 8 != 0)
							{
								h20Length += (((H20->_H20_Attr_Name_Length * 2 + 26) / 8) * 8 + 8);
							}
							else if ((H20->_H20_Attr_Name_Length*2+26) % 8 == 0)
							{
								h20Length += (H20->_H20_Attr_Name_Length*2+26);
							}
						}
						else
						{
							h20Length += 32;
						}
						if (h20Length > (DWORD)(SECTOR_SIZE * VirtualCuNum))
						{
							break;
						}
					}

					free(H20CancheBuff);
					H20CancheBuff = NULL;
				}
			}

			if (RecordH20Refer.size() > 0)
			{
				vector<DWORD>::iterator vec;
				for (vec = RecordH20Refer.begin(); vec < RecordH20Refer.end(); vec ++)
				{
					if (*vec != File_head_recod->_FR_Refer)
					{
						CFuncs::WriteLogInfo(SLT_INFORMATION, "GetInternetFileAddr ���ļ���¼H80�ض�λ��H20�У��ض�λ�ļ��ο�����:%lu", *vec);						
					}
					else
					{
						RecordH20Refer.erase(vec);//��ͬ�ľ�û�ض�λ������Ϊ��
					}
				}

			}

			if (ATTriBase->_Attr_Type == 0x80)
			{
				DWORD H80_datarun_len = NULL;
				LONG64 H80_datarun = NULL;
				bool FirstIn = true;
				if (ATTriBase->_PP_Attr == 0x01)
				{
					(*FileRealSize) = ((*FileRealSize) + ATTriBase->TWOATTRIBUTEHEAD.NP_head._NPN_Attr_T_Size);//ȡ�ô��ļ�����ʵ��С
					H80_data = (UCHAR*)&ATTriBase[0];
					DWORD OFFSET = NULL;
					RtlCopyMemory(&OFFSET, &ATTriBase->TWOATTRIBUTEHEAD.NP_head._NPN_RunList_Offset[0], 2);
					if (OFFSET > (FILE_SECTOR_SIZE - FirstAttriSize - AttributeSize))
					{
						CFuncs::WriteLogInfo(SLT_ERROR, _T("GetInternetFileAddr::OFFSET������Χ!"));
						return false;
					}
					if (H80_data[OFFSET] != 0 && H80_data[OFFSET] < 0x50)
					{					
						while(OFFSET < ATTriBase->_Attr_Length)
						{
							H80_datarun_len = NULL;
							H80_datarun = NULL;
							if (H80_data[OFFSET] > 0 && H80_data[OFFSET] < 0x50)
							{
								UCHAR adres_fig = H80_data[OFFSET] >> 4;
								UCHAR len_fig = H80_data[OFFSET] & 0xf;
								for(int w = len_fig;w > 0; w--)
								{
									H80_datarun_len = H80_datarun_len | (H80_data[OFFSET + w] << (8 * (w - 1)));
								}
								if (H80_datarun_len > 0)
								{
									FileH80len.push_back(H80_datarun_len);
								} 
								else
								{
									CFuncs::WriteLogInfo(SLT_ERROR, _T("GetInternetFileAddr::H80_datarun_lenΪ0!"));
									return false;
								}

								for (int w = adres_fig; w > 0; w --)
								{
									H80_datarun = H80_datarun | (H80_data[OFFSET+w+len_fig] << (8 * (w - 1)));
								}
								if (H80_data[OFFSET + adres_fig + len_fig] > 127)
								{
									if (adres_fig == 3)
									{
										H80_datarun = ~(H80_datarun^0xffffff);
									}
									if (adres_fig == 2)
									{
										H80_datarun = ~(H80_datarun^0xffff);

									}

								} 
								if (FirstIn)
								{
									if (H80_datarun > 0)
									{
										FileH80Addr.push_back(H80_datarun);
									} 
									else
									{
										CFuncs::WriteLogInfo(SLT_ERROR, _T("GetInternetFileAddr::H80_datarunΪ0��Ϊ��������!"));
										return false;
									}
									FirstIn = false;
								}
								else
								{
									if (FileH80Addr.size() > 0)
									{
										H80_datarun = FileH80Addr[FileH80Addr.size() - 1] + H80_datarun;
										FileH80Addr.push_back(H80_datarun);
									}
								}
								
								OFFSET = OFFSET + adres_fig + len_fig + 1;
							}
							else
							{
								break;
							}

						}								
					}

				}
				else if(ATTriBase->_PP_Attr == 0)
				{
					H80_data = (UCHAR*)&ATTriBase[0];		
					if (ATTriBase->TWOATTRIBUTEHEAD.P_head._PN_AttrBody_Length < (FILE_SECTOR_SIZE - AttributeSize - FirstAttriSize - 24))
					{
						FileH80Data.append((char*)&H80_data[24],ATTriBase->TWOATTRIBUTEHEAD.P_head._PN_AttrBody_Length);
					}
					

				}

			}
			if (ATTriBase->_Attr_Length < (FILE_SECTOR_SIZE - AttributeSize - FirstAttriSize) && ATTriBase->_Attr_Length > 0)
			{

				AttributeSize += ATTriBase->_Attr_Length;

			}  
			else
			{
				CFuncs::WriteLogInfo(SLT_ERROR, _T("GetInternetFileAddr::������:%lu,ATTriBase[i_attr]->_Attr_Length���ȹ����Ϊ0!")
					,ATTriBase->_Attr_Length);
				return false;
			}
		}
		else if (ATTriBase->_Attr_Type == 0xffffffff)
		{
			break;
		}
		else if(ATTriBase->_Attr_Type > 0xff && ATTriBase->_Attr_Type < 0xffffffff)
		{
			CFuncs::WriteLogInfo(SLT_ERROR, _T("GetInternetFileAddr:��ȡATTriBase[i_attr]->_Attr_Type����0xffffffff����"));
			return false;
		}

	}
	return true;
}
bool GetVirtualMachineInfo::ExtractingTheCatalogFile(DWORD64 VmdkFiletotalsize, DWORD64 VirtualStartNTFS, LONG64 VirStartMftRfAddr, UCHAR VirtualCuNum
	, DWORD Rerefer, vector<LONG64> v_VirtualStartMFTaddr, string RecordFileName, vector<DWORD64> v_VirtualStartMFTaddrLen, const char *virtualFileDir
	, DWORD64 Grain_size, DWORD64 GrainNumber,DWORD64 GrainListOff,  int VmdkfileType, DWORD Catalogoff, vector<string> VirtualName)
{
	DWORD64 VirtualBackAddr = NULL;
	bool Ret = false;
	DWORD BackBytesCount = NULL;

	DWORD64 MftLenAdd = NULL;
	LONG64 MftAddr = NULL;

	for (DWORD FMft = 0; FMft < v_VirtualStartMFTaddrLen.size(); FMft++)
	{
		if ((Rerefer * 2) <= (v_VirtualStartMFTaddrLen[FMft] * VirtualCuNum + MftLenAdd))
		{
			MftAddr = (v_VirtualStartMFTaddr[FMft] * VirtualCuNum + (Rerefer * 2) - MftLenAdd);
			break;
		} 
		else
		{
			MftLenAdd += (v_VirtualStartMFTaddrLen[FMft] * VirtualCuNum);
		}
	}
	if (v_VirtualStartMFTaddrLen.size() == NULL)
	{
		MftAddr = v_VirtualStartMFTaddr[0] * VirtualCuNum;
	}
	UCHAR *CacheBuff = (UCHAR*)malloc(FILE_SECTOR_SIZE + SECTOR_SIZE);
	if (NULL == CacheBuff)
	{
		CFuncs::WriteLogInfo(SLT_ERROR, _T("ExtractingTheCatalogFile:malloc:CacheBuffʧ��!"));
		return false;
	}
	memset(CacheBuff, 0, (FILE_SECTOR_SIZE + SECTOR_SIZE));
	VirtualBackAddr=NULL;
	DWORD VmdkFileNum = NULL;
	DWORD LeftSector = NULL;

	if(!VMwareAddressConversion(VirtualName, &VirtualBackAddr, (VirtualStartNTFS + MftAddr), VmdkFiletotalsize
		, &VmdkFileNum, Grain_size, GrainNumber, GrainListOff, &LeftSector, VmdkfileType, Catalogoff))
	{
		free(CacheBuff);
		CacheBuff = NULL;
		//CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualFileAddr:VMwareAddressConversion��Ϣʧ��!"));
		return true;
	}
	if (NULL == VirtualBackAddr)
	{
		free(CacheBuff);
		CacheBuff = NULL;
		//CFuncs::WriteLogInfo(SLT_INFORMATION, _T("GetVirtualFileAddr::�˵�ַΪ��!"));
		return true;
	}
	if (LeftSector >= 2)
	{
		if (!VMwareReadData(VirtualName[VmdkFileNum], CacheBuff, LeftSector, VirtualBackAddr, SECTOR_SIZE * 2))
		{
			free(CacheBuff);
			CacheBuff = NULL;
			CFuncs::WriteLogInfo(SLT_INFORMATION, _T("ExtractingTheCatalogFile::VMwareReadDataʧ��!"));
			return false;
		}
	}
	else
	{
		if (!VMwareReadData(VirtualName[VmdkFileNum], CacheBuff, LeftSector, VirtualBackAddr, SECTOR_SIZE))
		{
			free(CacheBuff);
			CacheBuff = NULL;
			CFuncs::WriteLogInfo(SLT_INFORMATION, _T("ExtractingTheCatalogFile::VMwareReadDataʧ��!"));
			return false;
		}
		VirtualBackAddr = NULL;
		if(!VMwareAddressConversion(VirtualName, &VirtualBackAddr, (VirtualStartNTFS + MftAddr + 1)
			, VmdkFiletotalsize, &VmdkFileNum, Grain_size, GrainNumber, GrainListOff, &LeftSector, VmdkfileType, Catalogoff))
		{
			free(CacheBuff);
			CacheBuff = NULL;
			//CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualFileAddr:VMwareAddressConversionʧ��!"));
			return true;
		}
		if (NULL == VirtualBackAddr)
		{
			free(CacheBuff);
			CacheBuff = NULL;
			//CFuncs::WriteLogInfo(SLT_INFORMATION, _T("GetVirtualFileAddr::�˵�ַΪ��!"));
			return true;
		}
		if (!VMwareReadData(VirtualName[VmdkFileNum], &CacheBuff[SECTOR_SIZE], LeftSector, VirtualBackAddr, SECTOR_SIZE))
		{
			free(CacheBuff);
			CacheBuff = NULL;
			CFuncs::WriteLogInfo(SLT_INFORMATION, _T("ExtractingTheCatalogFile::VMwareReadDataʧ��!"));
			return false;
		}
	}
		

	vector<DWORD> H20Refer;
	vector<LONG64> fileH80Addr;
	vector<DWORD> fileH80Len;
	string fileData;
	DWORD64 fileRealSize = NULL;
	if (!GetInternetFileAddr(CacheBuff, H20Refer, fileH80Addr, fileH80Len, fileData, &fileRealSize,  VirtualCuNum, VmdkFiletotalsize, VirtualStartNTFS
		, Grain_size, GrainNumber, GrainListOff, VmdkfileType, Catalogoff, VirtualName))
	{
		free(CacheBuff);
		CacheBuff = NULL;
		CFuncs::WriteLogInfo(SLT_ERROR, _T("ExtractingTheCatalogFile:ExtractingFileAddrʧ��!"));
		return false;
	}
	free(CacheBuff);
	CacheBuff = NULL;

	if (H20Refer.size() > 0)
	{
		UCHAR *VirH20CacheBuff = (UCHAR*)malloc(FILE_SECTOR_SIZE + SECTOR_SIZE);
		if ( NULL == VirH20CacheBuff)
		{

			CFuncs::WriteLogInfo(SLT_ERROR, "ExtractingTheCatalogFile:malloc VirH20CacheBuffʧ��!");
			return false;
		}
		vector<DWORD>::iterator h20vec;
		for (h20vec = H20Refer.begin(); h20vec < H20Refer.end(); h20vec++)
		{

			memset(VirH20CacheBuff, 0, FILE_SECTOR_SIZE);
			DWORD64 VirMftLen=NULL;
			DWORD64 VirStartMftRfAddr=NULL;
			for (DWORD FRN = 0; FRN < v_VirtualStartMFTaddr.size();FRN++)
			{
				if (((*h20vec) * 2) < (VirMftLen + v_VirtualStartMFTaddrLen[FRN] * VirtualCuNum))
				{
					VirStartMftRfAddr = v_VirtualStartMFTaddr[FRN] * VirtualCuNum + ((*h20vec) * 2 - VirMftLen);
					break;
				} 
				else
				{
					VirMftLen+=(v_VirtualStartMFTaddr[FRN] * VirtualCuNum);
				}
			}
			DWORD64 H20VirtualBackAddr = NULL;

			DWORD VmdkFileNum = NULL;
			DWORD LeftSector = NULL;

			if(!VMwareAddressConversion(VirtualName, &H20VirtualBackAddr, (VirtualStartNTFS + VirStartMftRfAddr * VirtualCuNum), VmdkFiletotalsize
				, &VmdkFileNum, Grain_size, GrainNumber, GrainListOff, &LeftSector, VmdkfileType, Catalogoff))
			{

				free(VirH20CacheBuff);
				VirH20CacheBuff = NULL;
				CFuncs::WriteLogInfo(SLT_ERROR, _T("ExtractingTheCatalogFile:Virtual_to_Host_OneAddr: ��ȡ�����MBR�׵�ַͷ����Ϣʧ��!"));
				break;
			}

			if (LeftSector >= 2)
			{
				if (!VMwareReadData(VirtualName[VmdkFileNum], VirH20CacheBuff, LeftSector, H20VirtualBackAddr, SECTOR_SIZE * 2))
				{
					
					free(VirH20CacheBuff);
					VirH20CacheBuff = NULL;
					CFuncs::WriteLogInfo(SLT_INFORMATION, _T("ExtractingTheCatalogFile::VMwareReadDataʧ��!"));
					return false;
				}
			}
			else
			{
				if (!VMwareReadData(VirtualName[VmdkFileNum], VirH20CacheBuff, LeftSector, H20VirtualBackAddr, SECTOR_SIZE))
				{
					
					free(VirH20CacheBuff);
					VirH20CacheBuff = NULL;
					CFuncs::WriteLogInfo(SLT_INFORMATION, _T("ExtractingTheCatalogFile::VMwareReadDataʧ��!"));
					return false;
				}
				H20VirtualBackAddr = NULL;
				if(!VMwareAddressConversion(VirtualName, &H20VirtualBackAddr, (VirtualStartNTFS + VirStartMftRfAddr * VirtualCuNum + 1)
					, VmdkFiletotalsize, &VmdkFileNum, Grain_size, GrainNumber, GrainListOff, &LeftSector, VmdkfileType, Catalogoff))
				{

					free(VirH20CacheBuff);
					VirH20CacheBuff = NULL;
					CFuncs::WriteLogInfo(SLT_ERROR, _T("ExtractingTheCatalogFile:Virtual_to_Host_OneAddr: ��ȡ�����MBR�׵�ַͷ����Ϣʧ��!"));
					break;
				}

				if (!VMwareReadData(VirtualName[VmdkFileNum], &VirH20CacheBuff[SECTOR_SIZE], LeftSector, H20VirtualBackAddr, SECTOR_SIZE))
				{
					
					free(VirH20CacheBuff);
					VirH20CacheBuff = NULL;
					CFuncs::WriteLogInfo(SLT_INFORMATION, _T("ExtractingTheCatalogFile::VMwareReadDataʧ��!"));
					return false;
				}
			}
			if(!GetVirtualH20FileReferH80Addr(VirH20CacheBuff, fileH80Addr, fileH80Len, fileData
				, &fileRealSize))
			{
				free(VirH20CacheBuff);
				VirH20CacheBuff = NULL;
				CFuncs::WriteLogInfo(SLT_ERROR, "ExtractingTheCatalogFile:GetH20FileReferH80Addrʧ��!");
				return false;
			}

		}
		free(VirH20CacheBuff);
		VirH20CacheBuff = NULL;
	}
	if (fileH80Addr.size() > 0)//����Ϊ��ַ����ȡ���ļ�
	{
		string VirtualFilePath;
		string StrTemName;
		if (RecordFileName.length() > 0)
		{
			if(!UnicodeToZifu((UCHAR*)&RecordFileName[0], StrTemName, RecordFileName.length()))
			{

				CFuncs::WriteLogInfo(SLT_ERROR, "ExtractingTheCatalogFile:UnicodeToZifu : FileNameʧ��!");
				return false;
			}
		
		

		DWORD NameSize = RecordFileName.length() + strlen(virtualFileDir);
		wchar_t * WirteName = new wchar_t[NameSize+1];
		if (NULL == WirteName)
		{
			CFuncs::WriteLogInfo(SLT_ERROR, _T("ExtractingTheCatalogFile:new:WirteName ���������ڴ�ʧ��!"));
		}
		memset(WirteName,0,(NameSize+1)*2);
		MultiByteToWideChar(CP_ACP, 0, virtualFileDir, strlen(virtualFileDir), WirteName, NameSize);

		for (DWORD NameIndex = 0; NameIndex < RecordFileName.length(); NameIndex += 2)
		{

			RtlCopyMemory(&WirteName[strlen(virtualFileDir) + NameIndex / 2], (UCHAR*)&RecordFileName[NameIndex],2);
		}


		if(!VirtualWriteLargeFile(VirtualCuNum, fileH80Addr, fileH80Len, VirtualStartNTFS, WirteName, fileRealSize, VmdkFiletotalsize
			, Grain_size, GrainNumber, GrainListOff, VirtualName, VmdkfileType, Catalogoff))
		{

			CFuncs::WriteLogInfo(SLT_ERROR, "ExtractingTheCatalogFile:VirtualWriteLargeFile:д���ļ�ʧ��");

		}
		delete WirteName;
		WirteName=NULL;

		}

	}
	else if (fileData.length() > 0)
	{
		string StrTemName;
		string VirtualFilePath;

		if (RecordFileName.length() > 0)
		{
			if(!UnicodeToZifu((UCHAR*)&RecordFileName[0], StrTemName, RecordFileName.length()))
			{

				CFuncs::WriteLogInfo(SLT_ERROR, "ExtractingTheCatalogFile:UnicodeToZifu : FileNameʧ��!");
				return false;
			}
		

		DWORD NameSize = RecordFileName.length() + strlen(virtualFileDir) + 1;
		wchar_t * WirteName = new wchar_t[NameSize+1];
		if (NULL == WirteName)
		{
			CFuncs::WriteLogInfo(SLT_ERROR, _T("ExtractingTheCatalogFile:new:WirteName ���������ڴ�ʧ��!"));
		}
		memset(WirteName,0,(NameSize+1)*2);
		MultiByteToWideChar(CP_ACP, 0, virtualFileDir, strlen(virtualFileDir), WirteName, NameSize);
		for (DWORD NameIndex = 0; NameIndex < RecordFileName.length(); NameIndex+=2)
		{

			RtlCopyMemory(&WirteName[strlen(virtualFileDir) + NameIndex / 2], &RecordFileName[NameIndex],2);
		}
		if(!VirtualWriteLitteFile(fileData, WirteName))
		{

			CFuncs::WriteLogInfo(SLT_ERROR, "ExtractingTheCatalogFile:WriteLitteFile:дС�ļ�ʧ��ʧ��");

		}

		delete WirteName;
		WirteName=NULL;
		}
	}

	return true;
}
bool GetVirtualMachineInfo::ReadBigShortCuFileInfo(UCHAR VirCuNum,vector<LONG64> FileH80Addr, vector <DWORD> FileH80Len, DWORD64 VirStartNTFSAddr
	, DWORD64 filerealSize, DWORD64 VmdkFiletotalsize, DWORD64 Grain_size, DWORD64 GrainNumber,DWORD64 GrainListOff, vector<string> VirtualName
	, int VmdkfileType, DWORD Catalogoff, UCHAR *ReadBuff)
{
	BOOL bRet = FALSE;
	DWORD BackBytesCount = NULL;
	if (NULL == filerealSize)
	{
		CFuncs::WriteLogInfo(SLT_ERROR, _T("ReadBigShortCuFileInfo:filerealSizeΪ0!"));
		return true;
	}
	else if (FileH80Addr.size() > 1 || NULL == FileH80Addr.size() || NULL == FileH80Len.size())
	{
		CFuncs::WriteLogInfo(SLT_INFORMATION, _T("ReadBigShortCuFileInfo:����һ�����˻�Ϊ��!"));
		return true;
	}
	if (FileH80Len[0] > 1)
	{
		CFuncs::WriteLogInfo(SLT_INFORMATION, _T("ReadBigShortCuFileInfo:����һ������!"));
		return true;
	}
	DWORD64 DataBackAddr = NULL;
	DWORD VmdkFileNum = NULL;
	DWORD LeftSector = NULL;
	LARGE_INTEGER RecoydwOffse={NULL};
	for (DWORD i = 0; i < FileH80Len[0]; i ++)
	{
		if(!VMwareAddressConversion(VirtualName, &DataBackAddr, (VirStartNTFSAddr + FileH80Addr[0] * VirCuNum), VmdkFiletotalsize
			, &VmdkFileNum, Grain_size, GrainNumber, GrainListOff, &LeftSector, VmdkfileType, Catalogoff))
		{

			CFuncs::WriteLogInfo(SLT_ERROR, _T("ReadBigShortCuFileInfo:VMwareAddressConversion: ��ȡ�����MBR�׵�ַͷ����Ϣʧ��!"));
			return true;
		}
		if (NULL == DataBackAddr)
		{

			CFuncs::WriteLogInfo(SLT_INFORMATION, _T("ReadBigShortCuFileInfo::�˵�ַΪ�� || LeftSector > 4096!"));
			return true;
		}
		RecoydwOffse.QuadPart += SECTOR_SIZE;
		if ((DWORD64)RecoydwOffse.QuadPart > filerealSize)
		{
			break;
		}
		if (!VMwareReadData(VirtualName[VmdkFileNum], &ReadBuff[(RecoydwOffse.QuadPart - SECTOR_SIZE)], LeftSector, DataBackAddr, SECTOR_SIZE))
		{

			CFuncs::WriteLogInfo(SLT_INFORMATION, _T("ReadBigShortCuFileInfo::VMwareReadDataʧ��!"));
			return false;
		}
		
	}
	
	return true;
}
bool GetVirtualMachineInfo::VboxReadBigShortCuFileInfo(UCHAR VirCuNum,vector<LONG64> FileH80Addr, vector <DWORD> FileH80Len
	, DWORD64 filerealSize, UCHAR *ReadBuff, HANDLE hDrive, DWORD64 VirtualPatition, UCHAR *VirVdiBatBuff, DWORD VirBatSingleSize, DWORD VirBatBuffSize
	, DWORD VirVdiDataAddr)
{
	BOOL Ret = FALSE;
	DWORD BackBytesCount = NULL;
	if (NULL == filerealSize)
	{
		CFuncs::WriteLogInfo(SLT_ERROR, _T("VboxReadBigShortCuFileInfo:filerealSizeΪ0!"));
		return true;
	}
	else if (FileH80Addr.size() > 1 || NULL == FileH80Addr.size() || NULL == FileH80Len.size())
	{
		CFuncs::WriteLogInfo(SLT_INFORMATION, _T("VboxReadBigShortCuFileInfo:����һ�����˻�Ϊ��!"));
		return true;
	}
	if (FileH80Len[0] > 1)
	{
		CFuncs::WriteLogInfo(SLT_INFORMATION, _T("VboxReadBigShortCuFileInfo:����һ������!"));
		return true;
	}
	DWORD64 DataBackAddr = NULL;
	DWORD VmdkFileNum = NULL;
	DWORD LeftSector = NULL;
	LARGE_INTEGER RecoydwOffse={NULL};
	for (DWORD i = 0; i < FileH80Len[0]; i ++)
	{
		DataBackAddr = NULL;
		if(!VdiOneAddrChange((VirtualPatition  + FileH80Addr[0] * VirCuNum) * SECTOR_SIZE
			, VirVdiBatBuff, VirBatSingleSize, &DataBackAddr, VirBatBuffSize, VirVdiDataAddr))
		{

			CFuncs::WriteLogInfo(SLT_ERROR, _T("VboxReadBigShortCuFileInfo:VhdOneAddrChange: ת����ʼMft�ļ���¼��ַʧ��!"));
			return false;
		}
		
		if (NULL == DataBackAddr)
		{

			CFuncs::WriteLogInfo(SLT_INFORMATION, _T("VboxReadBigShortCuFileInfo::�˵�ַΪ�� || LeftSector > 4096!"));
			return true;
		}
		RecoydwOffse.QuadPart += SECTOR_SIZE;
		if ((DWORD64)RecoydwOffse.QuadPart > filerealSize)
		{
			break;
		}
		Ret = ReadSQData(hDrive, &ReadBuff[(RecoydwOffse.QuadPart - SECTOR_SIZE)], SECTOR_SIZE,  DataBackAddr,
			&BackBytesCount);		
		if(!Ret)
		{		

			CFuncs::WriteLogInfo(SLT_ERROR, _T("VboxReadBigShortCuFileInfo:ReadSQData: ��ȡ��ʼMft�ļ���¼��ַʧ��!"));
			return false;	
		}


	}

	return true;
}
bool GetVirtualMachineInfo::FileRecordTimeChange(string &timestr, UCHAR *Timeymd, UCHAR *Timehms)
{
	SYSTEMTIME systemTime = {0};	

	if (NULL != Timeymd)
	{
		systemTime.wDay = Timeymd[0];
		systemTime.wDay = systemTime.wDay & 0x1f;
		systemTime.wMonth = Timeymd[1];
		systemTime.wMonth = (systemTime.wMonth & 0x1) << 3;
		systemTime.wYear = Timeymd[0];
		systemTime.wYear = (systemTime.wYear & 0xe0) >> 5;
		systemTime.wMonth = systemTime.wMonth | systemTime.wYear;
		systemTime.wYear = NULL;
		systemTime.wYear = Timeymd[1];
		systemTime.wYear = (systemTime.wYear & 0xfe) >> 1;
		systemTime.wYear = 1980 + systemTime.wYear;
	}

	if (NULL != Timehms)
	{
		systemTime.wSecond = Timehms[0];
		systemTime.wSecond = systemTime.wSecond & 0x1f;
		systemTime.wMinute = Timehms[1];
		systemTime.wMinute = (systemTime.wMinute & 0x7) << 3;
		systemTime.wHour = Timehms[0];
		systemTime.wHour = (systemTime.wHour & 0xe0) >> 5;
		systemTime.wMinute = systemTime.wMinute | systemTime.wHour;
		systemTime.wHour = NULL;
		systemTime.wHour = Timehms[1];
		systemTime.wHour = (systemTime.wHour & 0xf8) >> 3;
		systemTime.wSecond *= 2;
	}


	char szTime[32] = { 0 };
	sprintf_s(szTime, _countof(szTime), "%04d-%02d-%02d %02d:%02d:%02d", systemTime.wYear, systemTime.wMonth,
		systemTime.wDay, systemTime.wHour, systemTime.wMinute, systemTime.wSecond);
	timestr.assign(szTime);

	return true;

}
bool GetVirtualMachineInfo::GetShortCutFileDataInfo(UCHAR *ShortCutBuff, string &FileRecordLastVTM, string &FileRecordPath, DWORD64 filesize)
{
	LShortCutHead shortcuthead = NULL;
	LShortCutLoctionInfo shortlocalinfo = NULL;
	DWORD ShellitemSize = NULL;
	DWORD Localpath = NULL;
	DWORD shortFlag = NULL;
	
	ShellitemSize = ShortCutBuff[76];
	ShellitemSize = ShellitemSize | (ShortCutBuff[77] << 8);
	//RtlCopyMemory(&ShellitemSize, &ShortCutBuff[76], 2);

	if (ShellitemSize > filesize)
	{
		return true;
	}
	shortcuthead = (LShortCutHead)ShortCutBuff;
	
	if (!FileTimeConver(shortcuthead->_ShortCutLastVistTM,FileRecordLastVTM))
	{
		return true;
	}
	shortFlag = shortcuthead->_ShortCutFlags;
	if ((shortFlag & 0x10) > 0)
	{
		shortlocalinfo = (LShortCutLoctionInfo)&ShortCutBuff[78 + ShellitemSize];
		Localpath = shortlocalinfo->_SCLInfoLocalPathOff;
		if (Localpath > filesize)
		{
			return true;
		}
		DWORD pathoff = 78 + ShellitemSize + Localpath;
		if (Localpath > 0)
		{
			for(DWORD i = 0; i < (filesize - pathoff); i ++)
			{
				if (ShortCutBuff[pathoff + i] == 0)
				{
					FileRecordPath.append((char*)&ShortCutBuff[pathoff], i);
					break;
				}
			}
		}
	}

	
	return true;


}
bool GetVirtualMachineInfo::VBoxExtractingFileRecordFile(HANDLE hDrive, DWORD64 VirtualPatition, UCHAR *VirVdiBatBuff, DWORD VirBatSingleSize, DWORD VirBatBuffSize,
	vector<LONG64> v_VirtualStartMFTaddr, vector<DWORD64> v_VirtualStartMFTaddrLen, DWORD Rerefer, UCHAR VirtualCuNum, DWORD VirVdiDataAddr, string RecordFileName
	, string &FileRecordLastVTM, string &FileRecordPath)
{
	DWORD64 VirtualBackAddr = NULL;
	bool Ret = false;
	DWORD BackBytesCount = NULL;

	DWORD64 MftLenAdd = NULL;
	LONG64 MftAddr = NULL;

	for (DWORD FMft = 0; FMft < v_VirtualStartMFTaddrLen.size(); FMft++)
	{
		if ((Rerefer * 2) <= (v_VirtualStartMFTaddrLen[FMft] * VirtualCuNum + MftLenAdd))
		{
			MftAddr = (v_VirtualStartMFTaddr[FMft] * VirtualCuNum + (Rerefer * 2) - MftLenAdd);
			break;
		} 
		else
		{
			MftLenAdd += (v_VirtualStartMFTaddrLen[FMft] * VirtualCuNum);
		}
	}
	if (v_VirtualStartMFTaddrLen.size() == NULL)
	{
		MftAddr = v_VirtualStartMFTaddr[0] * VirtualCuNum;
	}
	UCHAR *CacheBuff = (UCHAR*)malloc(FILE_SECTOR_SIZE + SECTOR_SIZE);
	if (NULL == CacheBuff)
	{
		CFuncs::WriteLogInfo(SLT_ERROR, _T("VBoxExtractingFileRecordFile:malloc:CacheBuffʧ��!"));
		return false;
	}
	memset(CacheBuff, 0, (FILE_SECTOR_SIZE + SECTOR_SIZE));
	for (int i=0;i < 2;i++)
	{
		VirtualBackAddr=NULL;

		if(!VdiOneAddrChange((VirtualPatition + MftAddr  + i) * SECTOR_SIZE
			, VirVdiBatBuff, VirBatSingleSize, &VirtualBackAddr, VirBatBuffSize, VirVdiDataAddr))
		{			
			free(CacheBuff);
			CacheBuff = NULL;
			CFuncs::WriteLogInfo(SLT_ERROR, _T("VBoxExtractingFileRecordFile:Virtual_to_Host_OneAddrʧ��!"));
			return false;

		}

		Ret = ReadSQData(hDrive, &CacheBuff[i*SECTOR_SIZE], SECTOR_SIZE, VirtualBackAddr,
			&BackBytesCount);		
		if(!Ret)
		{			
			free(CacheBuff);
			CacheBuff = NULL;
			CFuncs::WriteLogInfo(SLT_ERROR, _T("VBoxExtractingFileRecordFile:ReadSQData: ��ȡ��ʼMft�ļ���¼��ַʧ��!"));
			return false;	
		}
	}
	vector<DWORD> H20Refer;
	vector<LONG64> fileH80Addr;
	vector<DWORD> fileH80Len;
	string fileData;
	DWORD64 fileRealSize = NULL;
	if (!VBoxGetRecordFileAddr(hDrive, &fileRealSize, CacheBuff, H20Refer, VirtualCuNum, VirtualPatition
		, VirVdiBatBuff, VirBatSingleSize, VirBatBuffSize, VirVdiDataAddr, fileH80Addr, fileH80Len, fileData))
	{

		CFuncs::WriteLogInfo(SLT_ERROR, _T("VBoxExtractingFileRecordFile:GetRecordFileAddrʧ��!"));
		return false;
	}
	if (H20Refer.size() > 0)
	{
		UCHAR *H20CacheBuff = (UCHAR*) malloc(SECTOR_SIZE * VirtualCuNum + SECTOR_SIZE);
		if (NULL == H20CacheBuff)
		{

			CFuncs::WriteLogInfo(SLT_ERROR, "VBoxExtractingFileRecordFile:malloc:H20CacheBuffʧ��!");
			return false;
		}
		memset(H20CacheBuff, 0, SECTOR_SIZE * VirtualCuNum + SECTOR_SIZE);
		vector<DWORD>::iterator h20vec;
		for (h20vec = H20Refer.begin(); h20vec < H20Refer.end(); h20vec++)
		{
			memset(H20CacheBuff, 0, FILE_SECTOR_SIZE);
			DWORD64 VirMftLen = NULL;
			DWORD64 VirStartMftRfAddr = NULL;
			for (DWORD FRN = 0; FRN < v_VirtualStartMFTaddrLen.size(); FRN++)
			{
				if (((*h20vec) * 2) < (VirMftLen + v_VirtualStartMFTaddrLen[FRN] * VirtualCuNum))
				{
					VirStartMftRfAddr = v_VirtualStartMFTaddr[FRN] * VirtualCuNum + ((*h20vec) * 2 - VirMftLen);
					break;
				} 
				else
				{
					VirMftLen += (v_VirtualStartMFTaddrLen[FRN] * VirtualCuNum);
				}
			}
			for (int i = 0; i < 2; i++)
			{
				VirtualBackAddr = NULL;
				if(!VdiOneAddrChange((VirtualPatition * SECTOR_SIZE + VirStartMftRfAddr * SECTOR_SIZE + SECTOR_SIZE * i)
					, VirVdiBatBuff, VirBatSingleSize, &VirtualBackAddr, VirBatBuffSize, VirVdiDataAddr))
				{

					free(H20CacheBuff);
					H20CacheBuff = NULL;

					CFuncs::WriteLogInfo(SLT_ERROR, _T("VBoxExtractingFileRecordFile:VhdOneAddrChange: ת����ʼMft�ļ���¼��ַʧ��!"));
					return false;
				}
				Ret = ReadSQData(hDrive, &H20CacheBuff[i * SECTOR_SIZE], SECTOR_SIZE,  VirtualBackAddr,
					&BackBytesCount);		
				if(!Ret)
				{		

					free(H20CacheBuff);
					H20CacheBuff = NULL;
					CFuncs::WriteLogInfo(SLT_ERROR, _T("VBoxExtractingFileRecordFile:ReadSQData: ��ȡ��ʼMft�ļ���¼��ַʧ��!"));
					return false;	
				}
			}

			if(!GetVirtualH20FileReferH80Addr(H20CacheBuff,fileH80Addr, fileH80Len, fileData
				, &fileRealSize))//��Ϊ��������ⲿ��ȡ�����ݣ�����������0	
			{

				free(H20CacheBuff);
				H20CacheBuff = NULL;
				CFuncs::WriteLogInfo(SLT_ERROR, _T("VBoxExtractingFileRecordFile:GetH20FileReferH80Addr: ʧ��!"));
				return false;
			}

		}
		free(H20CacheBuff);
		H20CacheBuff = NULL;
	}
	if (fileRealSize > 1024 * 4)
	{
		CFuncs::WriteLogInfo(SLT_INFORMATION, "VBoxExtractingFileRecordFile:�ļ�����4K!");
		return true;
	}
	else
	{
		fileRealSize = ((fileRealSize / SECTOR_SIZE) + SECTOR_SIZE);
	}

	if (fileH80Addr.size() > 0)//����Ϊ��ַ����ȡ���ļ�
	{		
		UCHAR *ShortCutBuff = (UCHAR*)malloc((size_t)fileRealSize + SECTOR_SIZE);
		if (NULL == ShortCutBuff)
		{
			CFuncs::WriteLogInfo(SLT_ERROR, "VBoxExtractingFileRecordFile:ShortCutBuffʧ��!");
			return false;
		}
		memset(ShortCutBuff, 0, ((size_t)fileRealSize + SECTOR_SIZE));

		if(!VboxReadBigShortCuFileInfo(VirtualCuNum, fileH80Addr, fileH80Len, fileRealSize, ShortCutBuff
			, hDrive, VirtualPatition, VirVdiBatBuff, VirBatSingleSize, VirBatBuffSize, VirVdiDataAddr))
		{
			free(ShortCutBuff);
			ShortCutBuff = NULL;
			CFuncs::WriteLogInfo(SLT_ERROR, "VBoxExtractingFileRecordFile:ReadBigShortCuFileInfo:����ʧ��");
			return false;

		}
		if (!GetShortCutFileDataInfo(ShortCutBuff, FileRecordLastVTM, FileRecordPath, fileRealSize))
		{
			free(ShortCutBuff);
			ShortCutBuff = NULL;
			CFuncs::WriteLogInfo(SLT_ERROR, "VBoxExtractingFileRecordFile:GetShortCutFileDataInfo:����ʧ��");
			return false;
		}
		free(ShortCutBuff);
		ShortCutBuff = NULL;


	}
	else if (fileData.length() > 0)
	{
		if (!GetShortCutFileDataInfo((UCHAR*)fileData.c_str(), FileRecordLastVTM, FileRecordPath, fileData.length()))
		{

			CFuncs::WriteLogInfo(SLT_ERROR, "VBoxExtractingFileRecordFile:GetShortCutFileDataInfo:����ʧ��");
			return false;
		}
	}

	return true;
}
bool GetVirtualMachineInfo::VmdkExtractingFileRecordFile(DWORD64 VmdkFiletotalsize, DWORD64 VirtualStartNTFS, LONG64 VirStartMftRfAddr, UCHAR VirtualCuNum
	, DWORD Rerefer, vector<LONG64> v_VirtualStartMFTaddr, string RecordFileName, vector<DWORD64> v_VirtualStartMFTaddrLen, DWORD64 Grain_size
	, DWORD64 GrainNumber,DWORD64 GrainListOff,  int VmdkfileType, DWORD Catalogoff, vector<string> VirtualName, string &FileRecordLastVTM
	, string &FileRecordPath)
{
	DWORD64 VirtualBackAddr = NULL;
	bool Ret = false;
	DWORD BackBytesCount = NULL;

	DWORD64 MftLenAdd = NULL;
	LONG64 MftAddr = NULL;

	for (DWORD FMft = 0; FMft < v_VirtualStartMFTaddrLen.size(); FMft++)
	{
		if ((Rerefer * 2) <= (v_VirtualStartMFTaddrLen[FMft] * VirtualCuNum + MftLenAdd))
		{
			MftAddr = (v_VirtualStartMFTaddr[FMft] * VirtualCuNum + (Rerefer * 2) - MftLenAdd);
			break;
		} 
		else
		{
			MftLenAdd += (v_VirtualStartMFTaddrLen[FMft] * VirtualCuNum);
		}
	}
	if (v_VirtualStartMFTaddrLen.size() == NULL)
	{
		MftAddr = v_VirtualStartMFTaddr[0] * VirtualCuNum;
	}
	UCHAR *CacheBuff = (UCHAR*)malloc(FILE_SECTOR_SIZE + SECTOR_SIZE);
	if (NULL == CacheBuff)
	{
		CFuncs::WriteLogInfo(SLT_ERROR, _T("VmdkExtractingFileRecordFile:malloc:CacheBuffʧ��!"));
		return false;
	}
	memset(CacheBuff, 0, (FILE_SECTOR_SIZE + SECTOR_SIZE));
	VirtualBackAddr=NULL;
	DWORD VmdkFileNum = NULL;
	DWORD LeftSector = NULL;

	if(!VMwareAddressConversion(VirtualName, &VirtualBackAddr, (VirtualStartNTFS + MftAddr), VmdkFiletotalsize
		, &VmdkFileNum, Grain_size, GrainNumber, GrainListOff, &LeftSector, VmdkfileType, Catalogoff))
	{
		free(CacheBuff);
		CacheBuff = NULL;
		//CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualFileAddr:VMwareAddressConversion��Ϣʧ��!"));
		return true;
	}
	if (NULL == VirtualBackAddr)
	{
		free(CacheBuff);
		CacheBuff = NULL;
		//CFuncs::WriteLogInfo(SLT_INFORMATION, _T("GetVirtualFileAddr::�˵�ַΪ��!"));
		return true;
	}
	if (LeftSector >= 2)
	{
		if (!VMwareReadData(VirtualName[VmdkFileNum], CacheBuff, LeftSector, VirtualBackAddr, SECTOR_SIZE * 2))
		{
			free(CacheBuff);
			CacheBuff = NULL;
			CFuncs::WriteLogInfo(SLT_INFORMATION, _T("VmdkExtractingFileRecordFile::VMwareReadDataʧ��!"));
			return false;
		}
	}
	else
	{
		if (!VMwareReadData(VirtualName[VmdkFileNum], CacheBuff, LeftSector, VirtualBackAddr, SECTOR_SIZE))
		{
			free(CacheBuff);
			CacheBuff = NULL;
			CFuncs::WriteLogInfo(SLT_INFORMATION, _T("VmdkExtractingFileRecordFile::VMwareReadDataʧ��!"));
			return false;
		}
		VirtualBackAddr = NULL;
		if(!VMwareAddressConversion(VirtualName, &VirtualBackAddr, (VirtualStartNTFS + MftAddr + 1)
			, VmdkFiletotalsize, &VmdkFileNum, Grain_size, GrainNumber, GrainListOff, &LeftSector, VmdkfileType, Catalogoff))
		{
			free(CacheBuff);
			CacheBuff = NULL;
			//CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualFileAddr:VMwareAddressConversionʧ��!"));
			return true;
		}
		if (NULL == VirtualBackAddr)
		{
			free(CacheBuff);
			CacheBuff = NULL;
			//CFuncs::WriteLogInfo(SLT_INFORMATION, _T("GetVirtualFileAddr::�˵�ַΪ��!"));
			return true;
		}
		if (!VMwareReadData(VirtualName[VmdkFileNum], &CacheBuff[SECTOR_SIZE], LeftSector, VirtualBackAddr, SECTOR_SIZE))
		{
			free(CacheBuff);
			CacheBuff = NULL;
			CFuncs::WriteLogInfo(SLT_INFORMATION, _T("VmdkExtractingFileRecordFile::VMwareReadDataʧ��!"));
			return false;
		}
	}


	vector<DWORD> H20Refer;
	vector<LONG64> fileH80Addr;
	vector<DWORD> fileH80Len;
	string fileData;
	DWORD64 fileRealSize = NULL;
	if (!GetInternetFileAddr(CacheBuff, H20Refer, fileH80Addr, fileH80Len, fileData, &fileRealSize,  VirtualCuNum, VmdkFiletotalsize, VirtualStartNTFS
		, Grain_size, GrainNumber, GrainListOff, VmdkfileType, Catalogoff, VirtualName))
	{
		free(CacheBuff);
		CacheBuff = NULL;
		CFuncs::WriteLogInfo(SLT_ERROR, _T("VmdkExtractingFileRecordFile:ExtractingFileAddrʧ��!"));
		return false;
	}
	free(CacheBuff);
	CacheBuff = NULL;

	if (H20Refer.size() > 0)
	{
		UCHAR *VirH20CacheBuff = (UCHAR*)malloc(FILE_SECTOR_SIZE + SECTOR_SIZE);
		if ( NULL == VirH20CacheBuff)
		{

			CFuncs::WriteLogInfo(SLT_ERROR, "VmdkExtractingFileRecordFile:malloc VirH20CacheBuffʧ��!");
			return false;
		}
		vector<DWORD>::iterator h20vec;
		for (h20vec = H20Refer.begin(); h20vec < H20Refer.end(); h20vec++)
		{

			memset(VirH20CacheBuff, 0, FILE_SECTOR_SIZE);
			DWORD64 VirMftLen=NULL;
			DWORD64 VirStartMftRfAddr=NULL;
			for (DWORD FRN = 0; FRN < v_VirtualStartMFTaddr.size();FRN++)
			{
				if (((*h20vec) * 2) < (VirMftLen + v_VirtualStartMFTaddrLen[FRN] * VirtualCuNum))
				{
					VirStartMftRfAddr = v_VirtualStartMFTaddr[FRN] * VirtualCuNum + ((*h20vec) * 2 - VirMftLen);
					break;
				} 
				else
				{
					VirMftLen+=(v_VirtualStartMFTaddr[FRN] * VirtualCuNum);
				}
			}
			DWORD64 H20VirtualBackAddr = NULL;

			DWORD VmdkFileNum = NULL;
			DWORD LeftSector = NULL;

			if(!VMwareAddressConversion(VirtualName, &H20VirtualBackAddr, (VirtualStartNTFS + VirStartMftRfAddr * VirtualCuNum), VmdkFiletotalsize
				, &VmdkFileNum, Grain_size, GrainNumber, GrainListOff, &LeftSector, VmdkfileType, Catalogoff))
			{

				free(VirH20CacheBuff);
				VirH20CacheBuff = NULL;
				CFuncs::WriteLogInfo(SLT_ERROR, _T("VmdkExtractingFileRecordFile:Virtual_to_Host_OneAddr: ��ȡ�����MBR�׵�ַͷ����Ϣʧ��!"));
				break;
			}

			if (LeftSector >= 2)
			{
				if (!VMwareReadData(VirtualName[VmdkFileNum], VirH20CacheBuff, LeftSector, H20VirtualBackAddr, SECTOR_SIZE * 2))
				{

					free(VirH20CacheBuff);
					VirH20CacheBuff = NULL;
					CFuncs::WriteLogInfo(SLT_INFORMATION, _T("VmdkExtractingFileRecordFile::VMwareReadDataʧ��!"));
					return false;
				}
			}
			else
			{
				if (!VMwareReadData(VirtualName[VmdkFileNum], VirH20CacheBuff, LeftSector, H20VirtualBackAddr, SECTOR_SIZE))
				{

					free(VirH20CacheBuff);
					VirH20CacheBuff = NULL;
					CFuncs::WriteLogInfo(SLT_INFORMATION, _T("VmdkExtractingFileRecordFile::VMwareReadDataʧ��!"));
					return false;
				}
				H20VirtualBackAddr = NULL;
				if(!VMwareAddressConversion(VirtualName, &H20VirtualBackAddr, (VirtualStartNTFS + VirStartMftRfAddr * VirtualCuNum + 1)
					, VmdkFiletotalsize, &VmdkFileNum, Grain_size, GrainNumber, GrainListOff, &LeftSector, VmdkfileType, Catalogoff))
				{

					free(VirH20CacheBuff);
					VirH20CacheBuff = NULL;
					CFuncs::WriteLogInfo(SLT_ERROR, _T("VmdkExtractingFileRecordFile:Virtual_to_Host_OneAddr: ��ȡ�����MBR�׵�ַͷ����Ϣʧ��!"));
					break;
				}

				if (!VMwareReadData(VirtualName[VmdkFileNum], &VirH20CacheBuff[SECTOR_SIZE], LeftSector, H20VirtualBackAddr, SECTOR_SIZE))
				{

					free(VirH20CacheBuff);
					VirH20CacheBuff = NULL;
					CFuncs::WriteLogInfo(SLT_INFORMATION, _T("VmdkExtractingFileRecordFile::VMwareReadDataʧ��!"));
					return false;
				}
			}
			if(!GetVirtualH20FileReferH80Addr(VirH20CacheBuff, fileH80Addr, fileH80Len, fileData
				, &fileRealSize))
			{
				free(VirH20CacheBuff);
				VirH20CacheBuff = NULL;
				CFuncs::WriteLogInfo(SLT_ERROR, "VmdkExtractingFileRecordFile:GetH20FileReferH80Addrʧ��!");
				return false;
			}

		}
		free(VirH20CacheBuff);
		VirH20CacheBuff = NULL;
	}
	if (fileRealSize > 1024 * 4)
	{
		CFuncs::WriteLogInfo(SLT_INFORMATION, "VmdkExtractingFileRecordFile:�ļ�����4K!");
		return true;
	}
	else
	{
		fileRealSize = ((fileRealSize / SECTOR_SIZE) + SECTOR_SIZE);
	}
	
	if (fileH80Addr.size() > 0)//����Ϊ��ַ����ȡ���ļ�
	{		
		UCHAR *ShortCutBuff = (UCHAR*)malloc((size_t)fileRealSize + SECTOR_SIZE);
		if (NULL == ShortCutBuff)
		{
			CFuncs::WriteLogInfo(SLT_ERROR, "VmdkExtractingFileRecordFile:ShortCutBuffʧ��!");
			return false;
		}
		memset(ShortCutBuff, 0, ((size_t)fileRealSize + SECTOR_SIZE));

		if(!ReadBigShortCuFileInfo(VirtualCuNum, fileH80Addr, fileH80Len, VirtualStartNTFS, fileRealSize, VmdkFiletotalsize
			, Grain_size, GrainNumber, GrainListOff, VirtualName, VmdkfileType, Catalogoff, ShortCutBuff))
		{
			free(ShortCutBuff);
			ShortCutBuff = NULL;
			CFuncs::WriteLogInfo(SLT_ERROR, "VmdkExtractingFileRecordFile:ReadBigShortCuFileInfo:����ʧ��");
			return false;

		}
		if (!GetShortCutFileDataInfo(ShortCutBuff, FileRecordLastVTM, FileRecordPath, fileRealSize))
		{
			free(ShortCutBuff);
			ShortCutBuff = NULL;
			CFuncs::WriteLogInfo(SLT_ERROR, "VmdkExtractingFileRecordFile:GetShortCutFileDataInfo:����ʧ��");
			return false;
		}
		free(ShortCutBuff);
		ShortCutBuff = NULL;
					

	}
	else if (fileData.length() > 0)
	{
		if (!GetShortCutFileDataInfo((UCHAR*)fileData.c_str(), FileRecordLastVTM, FileRecordPath, fileData.length()))
		{
			
			CFuncs::WriteLogInfo(SLT_ERROR, "VmdkExtractingFileRecordFile:GetShortCutFileDataInfo:����ʧ��");
			return false;
		}
	}

	return true;
}
bool GetVirtualMachineInfo::AnalysisVmdkFileInternet(map<string,map<string, string>> PathAndName, map<DWORD, vector<string>> VMDKNameInfo
	, const char* virtualFileDir, PFCallbackVirtualInternetRecord VirtualRecord)
{
	DWORD dwError = NULL;
	map<DWORD, vector<string> >::iterator  VirtualFileiter;  
	for (VirtualFileiter = VMDKNameInfo.begin(); VirtualFileiter != VMDKNameInfo.end(); VirtualFileiter++)
	{
		int  VmdkFileType = NULL;//������
		if (VirtualFileiter->second.size() > 2)
		{
			VmdkFileType = 1;//���ļ�����
		}
		else if (VirtualFileiter->second.size() > 0 && VirtualFileiter->second.size() < 3)
		{
			VmdkFileType = 2;//���ļ�����
		}
		else
		{
			break;
		}

		//CFuncs::WriteLogInfo(SLT_INFORMATION, _T("AnalysisVmdkFileInternet:%s:�����!"), VirtualFileiter->second[0]);
		vector <DWORD64> v_VirtualStartNTFSAddr;
		if(!GetVirtualNTSFAddr(VMDKNameInfo, VirtualFileiter->first, v_VirtualStartNTFSAddr, VmdkFileType))
		{
			CFuncs::WriteLogInfo(SLT_ERROR, "AnalysisVmdkFileInternet:GetVirtualNTSFAddrʧ��!");
			//return false;
		}
		CFuncs::WriteLogInfo(SLT_INFORMATION, "AnalysisVmdkFileInternet VMware�����һ��%d��NTFS��",v_VirtualStartNTFSAddr.size());
		DWORD64 VirNTFSStart = NULL;
		for (unsigned int virFenq = 0; virFenq < v_VirtualStartNTFSAddr.size(); virFenq++)
		{
			CFuncs::WriteLogInfo(SLT_INFORMATION, "AnalysisVmdkFileInternet VMware�������ʼѰ�ҵ�%u��NTFS��",virFenq);
			VirNTFSStart = v_VirtualStartNTFSAddr[virFenq];
			DWORD64 StartMftAddr = NULL;
			UCHAR m_VirtualCuNum = NULL;
			if(!GetVirtualMFTAddr(VMDKNameInfo, VirtualFileiter->first, &StartMftAddr, VirNTFSStart, &m_VirtualCuNum, VmdkFileType))
			{
				CFuncs::WriteLogInfo(SLT_ERROR, "AnalysisVmdkFileInternet:GetVirtualMFTAddrʧ��!");
				break;
			}

			vector <LONG64> v_VirtualMFTAddr;
			vector <DWORD64> v_VirtualMFTLen; 
			//�ҵ�MFT�ļ���¼��ȡ���е�MFT��ʼ��ַ���ļ���¼��С
			if (!GetVirtualAllMFTStartAddr(VMDKNameInfo, VirtualFileiter->first, StartMftAddr, VirNTFSStart, m_VirtualCuNum, v_VirtualMFTAddr
				, v_VirtualMFTLen, VmdkFileType))
			{
				CFuncs::WriteLogInfo(SLT_ERROR, "AnalysisVmdkFileInternet:GetVirtualAllMFTStartAddrʧ��!");
				return false;
			}
			if (v_VirtualMFTAddr.size() != v_VirtualMFTLen.size())
			{
				CFuncs::WriteLogInfo(SLT_ERROR, "AnalysisVmdkFileInternet:������ļ���¼��ַ�볤�ȸ�����ƥ�䣬ʧ��!");
				return false;
			}
			vector<string> VirtualName_Tem;
			if (VirtualFileiter->second[0].length() == 1)//�ж��ǲ��ǲ����
			{

				for (DWORD num = 1; num < VirtualFileiter->second.size(); num ++)
				{
					VirtualName_Tem.push_back(VirtualFileiter->second[num]);
				}

			}
			else
			{
				for (DWORD num = 0; num < VirtualFileiter->second.size(); num ++)
				{
					VirtualName_Tem.push_back(VirtualFileiter->second[num]);
				}
			}
			HANDLE HeadDrive = CreateFile(VirtualName_Tem[0].c_str(),
				GENERIC_READ,
				FILE_SHARE_READ | FILE_SHARE_WRITE,
				NULL,
				OPEN_EXISTING,
				0,
				NULL);
			if (HeadDrive == INVALID_HANDLE_VALUE) 
			{
				dwError=GetLastError();
				CFuncs::WriteLogInfo(SLT_ERROR, _T("AnalysisVmdkFileInternet VmDevice = CreateFile��ȡVMware�����ļ����ʧ��!,\
												   ���󷵻���: dwError = %d"), dwError);
				return false;
			}
			UCHAR *PatitionAddrBuffer = (UCHAR*) malloc(FILE_SECTOR_SIZE + SECTOR_SIZE);
			if (NULL == PatitionAddrBuffer)
			{
				CloseHandle(HeadDrive);
				HeadDrive = NULL;
				CFuncs::WriteLogInfo(SLT_ERROR, "AnalysisVmdkFileInternet:malloc PatitionAddrBufferʧ��!");
				return false;
			}
			memset(PatitionAddrBuffer, 0, FILE_SECTOR_SIZE + SECTOR_SIZE);


			bool Ret = false;
			DWORD BackBytesCount = NULL;
			Ret = ReadSQData(HeadDrive, &PatitionAddrBuffer[0], SECTOR_SIZE,
				0,
				&BackBytesCount);		
			if(!Ret)
			{		
				CloseHandle(HeadDrive);
				HeadDrive = NULL;
				free(PatitionAddrBuffer);
				PatitionAddrBuffer = NULL;
				CFuncs::WriteLogInfo(SLT_ERROR, _T("AnalysisVmdkFileInternet:ReadSQData: ��ȡvmdk�ļ�ͷ����Ϣʧ��!"));

				return false;	
			}

			LVirtual_head virtual_head = NULL;
			virtual_head = (LVirtual_head)&PatitionAddrBuffer[0];
			DWORD64 VmdkFiletotalsize = virtual_head->_File_capacity;
			DWORD64 Grain_size = virtual_head->_Grain_size;

			DWORD64 GrainListOff = virtual_head->_Grain_list_off;

			UCHAR GrainAddr[4] = { NULL };
			DWORD Catalogoff = NULL;

			Ret=ReadSQData(HeadDrive, &GrainAddr[0], 4, (GrainListOff  * SECTOR_SIZE),
				&BackBytesCount);		
			if(!Ret)
			{			
				CloseHandle(HeadDrive);
				HeadDrive = NULL;
				CFuncs::WriteLogInfo(SLT_ERROR, _T("AnalysisVmdkFileInternet:ReadSQData:��ȡvmdkͷ����Ϣʧ��!"));
				return false;	
			}
			RtlCopyMemory(&Catalogoff, &GrainAddr[0], 4);
			Catalogoff = Catalogoff - (DWORD)GrainListOff;
			DWORD64 GrainNumber = (virtual_head->_Grain_num * SECTOR_SIZE * Catalogoff) / 4;

			CloseHandle(HeadDrive);
			HeadDrive = NULL;

			int PathFileFound = NULL;
			string VirtualFilePath;
			string browetype;
			string RecordFileName;
			DWORD FileRefer = NULL;

			CFuncs::WriteLogInfo(SLT_INFORMATION, "AnalysisVmdkFileInternet �����һ��%d��MFT����",v_VirtualMFTAddr.size());
			DWORD ReferNumber = 0;//�ļ���¼����
			//���������ҵ������Ӧ���ļ���¼��Ȼ��Ա�·���Ƿ���ͬ���ҵ���ͬ·����Ŀ¼ʱ��ȡĿ¼����ȫ���ļ������ļ�����ȡ�ļ���ͬʱ�������ɾ���ҵ���·�����ļ���
			for (DWORD MftFileNum = NULL; MftFileNum < v_VirtualMFTAddr.size(); MftFileNum++)
			{
				CFuncs::WriteLogInfo(SLT_INFORMATION, "AnalysisVmdkFileInternet ��MFTһ��%lu��!",v_VirtualMFTLen[MftFileNum]);
				CFuncs::WriteLogInfo(SLT_INFORMATION, "AnalysisVmdkFileInternet ��ʼѰ�ҵ�%u��MFT��",MftFileNum);
				ReferNumber = 0;
				while(LookforVmdkInterFileRefer(VirNTFSStart, v_VirtualMFTAddr[MftFileNum], VmdkFiletotalsize, PatitionAddrBuffer, m_VirtualCuNum
					, ReferNumber, v_VirtualMFTAddr, v_VirtualMFTLen, VirtualName_Tem, PathAndName, &PathFileFound, VirtualFilePath, &FileRefer, browetype
					, RecordFileName, Grain_size, GrainNumber, GrainListOff, VmdkFileType, Catalogoff))
				{

					if (PathFileFound > 0)
					{
						if (PathFileFound == 1)
						{
							//�����Ŀ¼����Ҫ��ȡĿ¼�������ļ�
							map<DWORD, string> BrowerFileRefer;
							if (!GetCatalogFileRefer(PatitionAddrBuffer, m_VirtualCuNum, VmdkFiletotalsize, VirNTFSStart, BrowerFileRefer
								, Grain_size, GrainNumber, GrainListOff, VmdkFileType, Catalogoff, VirtualName_Tem))
							{
								free(PatitionAddrBuffer);
								PatitionAddrBuffer = NULL;
								
								CFuncs::WriteLogInfo(SLT_ERROR, _T("AnalysisVmdkFileInternet:GetCatalogFileReferʧ��!"));
								return false;
							}
							if (browetype == "FileRecord")
							{
								map<DWORD, string>::iterator Referiter;
								for (Referiter = BrowerFileRefer.begin(); Referiter != BrowerFileRefer.end(); Referiter ++)
								{
									string FileRecordLastVTM;
									string FileRecordPath;
									if (!VmdkExtractingFileRecordFile(VmdkFiletotalsize, VirNTFSStart, v_VirtualMFTAddr[MftFileNum], m_VirtualCuNum, Referiter->first
										, v_VirtualMFTAddr, Referiter->second, v_VirtualMFTLen, Grain_size, GrainNumber, GrainListOff, VmdkFileType
										, Catalogoff, VirtualName_Tem, FileRecordLastVTM, FileRecordPath))
									{
										free(PatitionAddrBuffer);
										PatitionAddrBuffer = NULL;

										CFuncs::WriteLogInfo(SLT_ERROR, _T("AnalysisVmdkFileInternet:VmdkExtractingFileRecordFileʧ��!"));
										return false;
									}
									else
									{
										string filename_tem;

										if (Referiter->second.length() > 0)
										{
											if (!UnicodeToZifu((UCHAR*)&Referiter->second[0], filename_tem, Referiter->second.length()))
											{
												free(PatitionAddrBuffer);
												PatitionAddrBuffer = NULL;

												CFuncs::WriteLogInfo(SLT_ERROR, _T("AnalysisVmdkFileInternet:UnicodeToZifuʧ��!"));
												return false;
											}


											string recorvyName;
											recorvyName = string(virtualFileDir);
											recorvyName.append(filename_tem);
											VirtualRecord(VirtualFilePath.c_str(), filename_tem.c_str(), recorvyName.c_str()
												, 1, browetype.c_str(),  FileRecordLastVTM.c_str(), FileRecordPath.c_str(), "", "", "", "");
										}
									}

								}
							} 
							else
							{
								map<DWORD, string>::iterator Referiter;
								for (Referiter = BrowerFileRefer.begin(); Referiter != BrowerFileRefer.end(); Referiter ++)
								{

									if (!ExtractingTheCatalogFile(VmdkFiletotalsize, VirNTFSStart, v_VirtualMFTAddr[MftFileNum], m_VirtualCuNum, Referiter->first
										, v_VirtualMFTAddr, Referiter->second, v_VirtualMFTLen, virtualFileDir, Grain_size, GrainNumber, GrainListOff, VmdkFileType
										, Catalogoff, VirtualName_Tem))
									{
										free(PatitionAddrBuffer);
										PatitionAddrBuffer = NULL;

										CFuncs::WriteLogInfo(SLT_ERROR, _T("AnalysisVmdkFileInternet:ExtractingTheCatalogFileʧ��!"));
										return false;
									}
									else
									{
										string filename_tem;

										if (Referiter->second.length() > 0)
										{
											if (!UnicodeToZifu((UCHAR*)&Referiter->second[0], filename_tem, Referiter->second.length()))
											{
												free(PatitionAddrBuffer);
												PatitionAddrBuffer = NULL;

												CFuncs::WriteLogInfo(SLT_ERROR, _T("AnalysisVmdkFileInternet:UnicodeToZifuʧ��!"));
												return false;
											}


											string recorvyName;
											recorvyName = string(virtualFileDir);
											recorvyName.append(filename_tem);
											if(!VirtualInternetRecord( VirtualFilePath.c_str(), filename_tem.c_str(),
												browetype.c_str(), recorvyName.c_str(), VirtualRecord))
											{
												CFuncs::WriteLogInfo(SLT_ERROR, "AnalysisVmdkFileInternet ����������¼�ļ�ʧ�ܣ�%s", recorvyName.c_str());
											}
										}
									}
								}
							}
							
						}
						else if (PathFileFound == 2)
						{
							//������ļ���ֱ����ȡ
							vector<DWORD> RecordH20Refer;
							vector<LONG64> RecordFileH80Addr;
							vector<DWORD> RecordFileH80AddrLen;
							string RecordFileBuffH80;
							DWORD64 FileRealSize = NULL;
							if (!GetInternetFileAddr(PatitionAddrBuffer, RecordH20Refer, RecordFileH80Addr, RecordFileH80AddrLen, RecordFileBuffH80
								, &FileRealSize, m_VirtualCuNum, VmdkFiletotalsize, VirNTFSStart, Grain_size, GrainNumber, GrainListOff, VmdkFileType
								, Catalogoff, VirtualName_Tem))
							{
								free(PatitionAddrBuffer);
								PatitionAddrBuffer = NULL;
							
								CFuncs::WriteLogInfo(SLT_ERROR, _T("AnalysisVmdkFileInternet:GetRecordFileAddrʧ��!"));
								return false;
							}
							if (RecordH20Refer.size() > 0)
							{
								UCHAR *VirH20CacheBuff = (UCHAR*)malloc(FILE_SECTOR_SIZE + SECTOR_SIZE);
								if ( NULL == VirH20CacheBuff)
								{
									free(PatitionAddrBuffer);
									PatitionAddrBuffer = NULL;
								
									CFuncs::WriteLogInfo(SLT_ERROR, "AnalysisVmdkFileInternet:malloc VirH20CacheBuffʧ��!");
									return false;
								}
								vector<DWORD>::iterator h20vec;
								for (h20vec = RecordH20Refer.begin(); h20vec < RecordH20Refer.end(); h20vec++)
								{

									memset(VirH20CacheBuff, 0, FILE_SECTOR_SIZE);
									DWORD64 VirMftLen=NULL;
									DWORD64 VirStartMftRfAddr=NULL;
									for (DWORD FRN = 0; FRN < v_VirtualMFTAddr.size();FRN++)
									{
										if (((*h20vec) * 2) < (VirMftLen + v_VirtualMFTLen[FRN] * m_VirtualCuNum))
										{
											VirStartMftRfAddr = v_VirtualMFTAddr[FRN] * m_VirtualCuNum + ((*h20vec) * 2 - VirMftLen);
											break;
										} 
										else
										{
											VirMftLen+=(v_VirtualMFTAddr[FRN] * m_VirtualCuNum);
										}
									}
									DWORD64 H20VirtualBackAddr = NULL;

									bool Ret = false;
									DWORD VmdkFileNum = NULL;
									DWORD LeftSector = NULL;

									if(!VMwareAddressConversion(VirtualName_Tem, &H20VirtualBackAddr, (VirNTFSStart + VirStartMftRfAddr * m_VirtualCuNum), VmdkFiletotalsize
										, &VmdkFileNum, Grain_size, GrainNumber, GrainListOff, &LeftSector, VmdkFileType, Catalogoff))
									{

										
										CFuncs::WriteLogInfo(SLT_ERROR, _T("AnalysisVmdkFileInternet:Virtual_to_Host_OneAddr: ��ȡ�����MBR�׵�ַͷ����Ϣʧ��!"));
										break;
									}

									if (LeftSector >= 2)
									{
										if (!VMwareReadData(VirtualName_Tem[VmdkFileNum], VirH20CacheBuff, LeftSector, H20VirtualBackAddr, SECTOR_SIZE * 2))
										{

											free(VirH20CacheBuff);
											VirH20CacheBuff = NULL;
											CFuncs::WriteLogInfo(SLT_INFORMATION, _T("AnalysisVmdkFileInternet::VMwareReadDataʧ��!"));
											return false;
										}
									}
									else
									{
										if (!VMwareReadData(VirtualName_Tem[VmdkFileNum], VirH20CacheBuff, LeftSector, H20VirtualBackAddr, SECTOR_SIZE))
										{

											free(VirH20CacheBuff);
											VirH20CacheBuff = NULL;
											CFuncs::WriteLogInfo(SLT_INFORMATION, _T("AnalysisVmdkFileInternet::VMwareReadDataʧ��!"));
											return false;
										}
										H20VirtualBackAddr = NULL;
										if(!VMwareAddressConversion(VirtualName_Tem, &H20VirtualBackAddr, (VirNTFSStart + VirStartMftRfAddr * m_VirtualCuNum + 1)
											, VmdkFiletotalsize, &VmdkFileNum, Grain_size, GrainNumber, GrainListOff, &LeftSector, VmdkFileType, Catalogoff))
										{

											
											CFuncs::WriteLogInfo(SLT_ERROR, _T("AnalysisVmdkFileInternet:Virtual_to_Host_OneAddr: ��ȡ�����MBR�׵�ַͷ����Ϣʧ��!"));
											break;
										}

										if (!VMwareReadData(VirtualName_Tem[VmdkFileNum], &VirH20CacheBuff[SECTOR_SIZE], LeftSector, H20VirtualBackAddr, SECTOR_SIZE))
										{

											free(VirH20CacheBuff);
											VirH20CacheBuff = NULL;
											CFuncs::WriteLogInfo(SLT_INFORMATION, _T("AnalysisVmdkFileInternet::VMwareReadDataʧ��!"));
											return false;
										}
									}
									if(!GetVirtualH20FileReferH80Addr(VirH20CacheBuff, RecordFileH80Addr, RecordFileH80AddrLen, RecordFileBuffH80
										, &FileRealSize))
									{
										free(PatitionAddrBuffer);
										PatitionAddrBuffer = NULL;
										
										free(VirH20CacheBuff);
										VirH20CacheBuff = NULL;
										CFuncs::WriteLogInfo(SLT_ERROR, "AnalysisVmdkFileInternet:GetH20FileReferH80Addrʧ��!");
										return false;
									}

								}
								free(VirH20CacheBuff);
								VirH20CacheBuff = NULL;
							}
							if (RecordFileH80Addr.size() > 0)//����Ϊ��ַ����ȡ���ļ�
							{
								//string VirtualFilePath;
								string StrTemName;
								if (RecordFileName.length() > 0)
								{
									if(!UnicodeToZifu((UCHAR*)&RecordFileName[0], StrTemName, RecordFileName.length()))
									{
										free(PatitionAddrBuffer);
										PatitionAddrBuffer = NULL;

										CFuncs::WriteLogInfo(SLT_ERROR, "AnalysisVmdkFileInternet:UnicodeToZifu : FileNameʧ��!");
										return false;
									}
								
								

								DWORD NameSize = RecordFileName.length() + strlen(virtualFileDir);
								wchar_t * WirteName = new wchar_t[NameSize+1];
								if (NULL == WirteName)
								{
									CFuncs::WriteLogInfo(SLT_ERROR, _T("AnalysisVmdkFileInternet:new:WirteName ���������ڴ�ʧ��!"));
								}
								memset(WirteName,0,(NameSize+1)*2);
								MultiByteToWideChar(CP_ACP, 0, virtualFileDir, strlen(virtualFileDir), WirteName, NameSize);

								for (DWORD NameIndex = 0; NameIndex < RecordFileName.length(); NameIndex += 2)
								{

									RtlCopyMemory(&WirteName[strlen(virtualFileDir) + NameIndex / 2], (UCHAR*)&RecordFileName[NameIndex],2);
								}


								if(!VirtualWriteLargeFile(m_VirtualCuNum, RecordFileH80Addr, RecordFileH80AddrLen, VirNTFSStart, WirteName, FileRealSize
									, VmdkFiletotalsize, Grain_size, GrainNumber, GrainListOff, VirtualName_Tem, VmdkFileType, Catalogoff))
								{

									CFuncs::WriteLogInfo(SLT_ERROR, "AnalysisVmdkFileInternet:VirtualWriteLargeFile:д���ļ�ʧ��");

								}
								else
								{
									string recorvyName;
									recorvyName = string(virtualFileDir);
									recorvyName.append(StrTemName);
									//��ȡ������¼�ļ��ɹ�
									if(!VirtualInternetRecord(VirtualFilePath.c_str(), StrTemName.c_str(),
										browetype.c_str(), recorvyName.c_str(), VirtualRecord))
									{
										CFuncs::WriteLogInfo(SLT_ERROR, "AnalysisVmdkFileInternet ����������¼�ļ�ʧ�ܣ�%s", recorvyName.c_str());
									}
								}
								delete WirteName;
								WirteName=NULL;
								}

							}
							else if (RecordFileBuffH80.length() > 0)
							{
								string StrTemName;
								string VirtualFilePath;
								if (RecordFileName.length() > 0)
								{
									if(!UnicodeToZifu((UCHAR*)&RecordFileName[0], StrTemName, RecordFileName.length()))
									{
										free(PatitionAddrBuffer);
										PatitionAddrBuffer = NULL;

										CFuncs::WriteLogInfo(SLT_ERROR, "AnalysisVmdkFileInternet:UnicodeToZifu : FileNameʧ��!");
										return false;
									}
								
								

								DWORD NameSize = RecordFileName.length() + strlen(virtualFileDir) + 1;
								wchar_t * WirteName = new wchar_t[NameSize+1];
								if (NULL == WirteName)
								{
									CFuncs::WriteLogInfo(SLT_ERROR, _T("AnalysisVmdkFileInternet:new:WirteName ���������ڴ�ʧ��!"));
								}
								memset(WirteName,0,(NameSize+1)*2);
								MultiByteToWideChar(CP_ACP, 0, virtualFileDir, strlen(virtualFileDir), WirteName, NameSize);
								for (DWORD NameIndex = 0; NameIndex < RecordFileName.length(); NameIndex+=2)
								{

									RtlCopyMemory(&WirteName[strlen(virtualFileDir) + NameIndex / 2], &RecordFileName[NameIndex],2);
								}
								if(!VirtualWriteLitteFile(RecordFileBuffH80, WirteName))
								{

									CFuncs::WriteLogInfo(SLT_ERROR, "AnalysisVmdkFileInternet:WriteLitteFile:дС�ļ�ʧ��ʧ��");

								}
								else
								{
									string recorvyName;
									recorvyName = string(virtualFileDir);
									recorvyName.append(StrTemName);
									if(!VirtualInternetRecord( VirtualFilePath.c_str(), StrTemName.c_str(),
										browetype.c_str(), recorvyName.c_str(), VirtualRecord))
									{
										CFuncs::WriteLogInfo(SLT_ERROR, "AnalysisVmdkFileInternet ����������¼�ļ�ʧ�ܣ�%s", recorvyName.c_str());
									}
								}

								delete WirteName;
								WirteName=NULL;
								}
							}

						}

					}
					memset(PatitionAddrBuffer,0,FILE_SECTOR_SIZE);
					ReferNumber += 2;

					if ((ReferNumber) > v_VirtualMFTLen[MftFileNum] * m_VirtualCuNum)
					{
						CFuncs::WriteLogInfo(SLT_INFORMATION, "AnalysisVmdkFileInternet:ReferNum:�������MFT���ļ���¼����%lu",ReferNumber);
						break;
					}
				}
							CFuncs::WriteLogInfo(SLT_INFORMATION, "GetVMwareRecordMark:ReferNum:�ļ���¼����%lu",ReferNumber);
			}
			CFuncs::WriteLogInfo(SLT_INFORMATION, "AnalysisVmdkFileInternet:����������MFT�ļ���¼");

			free(PatitionAddrBuffer);
			PatitionAddrBuffer = NULL;
		}
	}
	return true;
}
bool GetVirtualMachineInfo::GetVMwareInternetInfo(map<string,map<string, string>> PathAndName, vector<string> VMwareMftFileName, const char* virtualFileDir
	, PFCallbackVirtualInternetRecord VirtualRecord)
{
	
	DWORD dwError = NULL;

	map<DWORD, vector<string>> VMDKNameInfo;//��Ŷ�Ӧ��Ӧ�� vmdk�����ļ�

	if(!GetVirtualFileName(VMDKNameInfo,  VMwareMftFileName))
	{
		CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVMwareInternetInfo:GetVirtualFileName:��ȡvmdk����ʧ��!"));			
		return false;
	}
	if (!AnalysisVmdkFileInternet(PathAndName, VMDKNameInfo, virtualFileDir
		, VirtualRecord))
	{
		CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVMwareInternetInfo:AnalysisVmdkFile:ʧ��!"));			
		return false;
	}
		


	
	return true;
}
bool GetVirtualMachineInfo::VBoxGetIndexHeadInfo(HANDLE hDrive, DWORD64 VirtualStartPatition, LONG64 HA0Addr, UCHAR m_VirtualCuNum, UCHAR *VdiBatBuff
	, DWORD VdiBatSingleSize, DWORD BatListSize, DWORD VdiDataStartAddr, DWORD *IndexHeadSize, vector<UCHAR> &IndexUpdata, DWORD *IndexMemberSize, DWORD *IndexRealSize)
{
	DWORD64 IndexBackAddr = NULL;
	DWORD BackBytesCount = NULL;
	bool Ret = false;
	LSTANDARD_INDEX_HEAD Indexhead = NULL;

	UCHAR *IndexBuff = (UCHAR*)malloc(SECTOR_SIZE + 1);
	if (NULL == IndexBuff)
	{
		CFuncs::WriteLogInfo(SLT_ERROR, _T("VBoxGetIndexHeadInfo:IndexBuff����ʧ��!"));
		return false;
	}
	memset(IndexBuff, 0, SECTOR_SIZE);
	if(!VdiOneAddrChange((VirtualStartPatition * SECTOR_SIZE + HA0Addr * m_VirtualCuNum * SECTOR_SIZE)
		, VdiBatBuff, VdiBatSingleSize, &IndexBackAddr, BatListSize, VdiDataStartAddr))
	{
		CFuncs::WriteLogInfo(SLT_ERROR, _T("VBoxGetIndexHeadInfo:VhdOneAddrChange: ʧ��!"));
		return true;
	}

	Ret = ReadSQData(hDrive, &IndexBuff[0], SECTOR_SIZE, IndexBackAddr,
		&BackBytesCount);		
	if(!Ret)
	{	
		free(IndexBuff);
		IndexBuff = NULL;
		CFuncs::WriteLogInfo(SLT_ERROR, _T("VBoxGetIndexHeadInfo:ReadSQDataʧ��!"));
		return false;
	}
	DWORD UpdataOffset = NULL;
	DWORD UpdataSize = NULL;
	Indexhead = (LSTANDARD_INDEX_HEAD)&IndexBuff[0];
	if (Indexhead->_Head_Index != 0x58444e49)
	{
		free(IndexBuff);
		IndexBuff = NULL;
		CFuncs::WriteLogInfo(SLT_ERROR, _T("VBoxGetIndexHeadInfo:����ͷ��־����ȷ!"));
		return true;
	}

	RtlCopyMemory(&UpdataOffset, (UCHAR*)&Indexhead->_Updat_Sequ_Num_Off[0], 2);
	RtlCopyMemory(&UpdataSize, (UCHAR*)&Indexhead->_Updat_Sequ_Num_Off_Size[0], 2);
	if (UpdataOffset > SECTOR_SIZE || UpdataSize > 0xff)
	{
		free(IndexBuff);
		IndexBuff = NULL;
		CFuncs::WriteLogInfo(SLT_ERROR, _T("VBoxGetIndexHeadInfo:UpdataOffset > SECTOR_SIZE || UpdataSize > 0xff!"));
		return false;
	}
	IndexUpdata.clear();
	for (DWORD updnum =0; updnum < (UpdataSize-1) * 2; updnum ++)
	{
		IndexUpdata.push_back(IndexBuff[UpdataOffset + 2 + updnum]);
	}
	(*IndexMemberSize) = Indexhead->_Index_FB_Size; 
	(*IndexRealSize) = Indexhead->_Index_Term_Size;

	DWORD UpdataNum = NULL;
	if ((UpdataSize * 2) % 8 != 0)
	{
		UpdataNum = UpdataNum + ((UpdataSize * 2) / 8 ) + 1;
	}else
	{
		UpdataNum = UpdataNum + ((UpdataSize * 2) / 8 );
	}
	DWORD64 Judge = NULL;
	for (DWORD i = 0; i < 20; i ++)
	{
		RtlCopyMemory(&Judge, &IndexBuff[(5 + UpdataNum + i) * 8], 8);
		if (Judge != 0)
		{
			(*IndexHeadSize) = (5 + UpdataNum + i) * 8;
			break;
		}
	}
	if (NULL == (*IndexHeadSize))
	{
		free(IndexBuff);
		IndexBuff = NULL;
		CFuncs::WriteLogInfo(SLT_ERROR, _T("VBoxGetIndexHeadInfo:����ͷ(*IndexHeadSize)��СΪ0!"));
		return false;
	}

	Indexhead = NULL;
	free(IndexBuff);
	IndexBuff = NULL;

	if (NULL == (*IndexMemberSize) || NULL == (*IndexRealSize))
	{
		CFuncs::WriteLogInfo(SLT_ERROR, _T("VBoxGetIndexHeadInfo:����������С��ʵ�ʴ�СΪ0!"));
		return false;
	}

	return true;
}
bool GetVirtualMachineInfo::VBoxGetHA0FileRecordRefer(HANDLE hDrive, vector<LONG64> HA0addr, vector<DWORD> HA0len, UCHAR m_VirtualCuNum, DWORD64 HA0RealSize, DWORD64 VirtualStartPatition
	, UCHAR *VdiBatBuff, DWORD VdiBatSingleSize, DWORD BatListSize, DWORD VdiDataStartAddr, map<DWORD, string> &BrowerfileRefer)
{
	DWORD IndexMemberSize = NULL;
	DWORD IndexRealSize = NULL;
	DWORD IndexheadSize = NULL;
	vector<UCHAR> IndexUpdata;
	bool Ret = false;
	DWORD BackBytesCount = NULL;
	for (DWORD AddrNum = 0; AddrNum < HA0addr.size(); AddrNum ++)
	{
		DWORD IndexNumber = NULL;
		while(IndexNumber < HA0len[AddrNum])
		{
			DWORD ReadCuNum = NULL;
			for (DWORD num = 0; num < AddrNum; num ++)
			{
				ReadCuNum += HA0len[num];
			}
			ReadCuNum += IndexNumber;
			if ((ReadCuNum * m_VirtualCuNum *SECTOR_SIZE) >= HA0RealSize)
			{
				break;
			}
			if (!VBoxGetIndexHeadInfo(hDrive, VirtualStartPatition, (HA0addr[AddrNum] + IndexNumber), m_VirtualCuNum, VdiBatBuff, VdiBatSingleSize, BatListSize, VdiDataStartAddr
				, &IndexheadSize, IndexUpdata, &IndexMemberSize, &IndexRealSize))
			{
				CFuncs::WriteLogInfo(SLT_ERROR, _T("VBoxGetHA0FileRecordRefer:GetIndexHeadInfoͷ��ʧ��!"));
				return false;
			}
			DWORD IndexSingleCuNumber  = NULL;
			if ((IndexMemberSize / (SECTOR_SIZE * m_VirtualCuNum)) > 0)
			{
				if ((IndexMemberSize % (SECTOR_SIZE * m_VirtualCuNum)) > 0)
				{
					IndexSingleCuNumber  = (IndexMemberSize / (SECTOR_SIZE * m_VirtualCuNum)) + 1;

				} 
				else
				{
					IndexSingleCuNumber  = (IndexMemberSize / (SECTOR_SIZE * m_VirtualCuNum));
				}
				if (IndexSingleCuNumber  > 0xffff)
				{
					CFuncs::WriteLogInfo(SLT_ERROR, _T("VBoxGetHA0FileRecordRefer:IndexSingleSize����ʧ��!"));
					return false;
				}
			} 
			else
			{
				IndexSingleCuNumber  = 1;
			}
			if (IndexRealSize > (IndexSingleCuNumber *  m_VirtualCuNum * SECTOR_SIZE))
			{
				CFuncs::WriteLogInfo(SLT_ERROR, _T("VBoxGetHA0FileRecordRefer:�����������ȴ��ڷ����Сʧ��!"));
				return false;
			}
			UCHAR *IndexBuff = (UCHAR*)malloc(IndexSingleCuNumber  * m_VirtualCuNum *SECTOR_SIZE + SECTOR_SIZE);
			if (NULL == IndexBuff)
			{
				CFuncs::WriteLogInfo(SLT_ERROR, _T("VBoxGetHA0FileRecordRefer:malloc:IndexBuffʧ��!"));
				return false;
			}
			memset(IndexBuff, 0, (IndexSingleCuNumber  * m_VirtualCuNum *SECTOR_SIZE + SECTOR_SIZE));
			DWORD64 IndexBackAddr = NULL;
			if(!VdiOneAddrChange((VirtualStartPatition * SECTOR_SIZE + (HA0addr[AddrNum] + IndexNumber) * m_VirtualCuNum * SECTOR_SIZE)
				, VdiBatBuff, VdiBatSingleSize, &IndexBackAddr, BatListSize, VdiDataStartAddr))
			{
				free(IndexBuff);
				IndexBuff = NULL;
				CFuncs::WriteLogInfo(SLT_ERROR, _T("VBoxGetHA0FileRecordRefer:VhdOneAddrChange: ʧ��!"));
				return false;
			}

			Ret = ReadSQData(hDrive, &IndexBuff[0], (IndexSingleCuNumber  * m_VirtualCuNum *SECTOR_SIZE), IndexBackAddr,
				&BackBytesCount);		
			if(!Ret)
			{	
				free(IndexBuff);
				IndexBuff = NULL;
				CFuncs::WriteLogInfo(SLT_ERROR, _T("VBoxGetHA0FileRecordRefer:ReadSQDataʧ��!"));
				return false;
			}
			if ((IndexUpdata.size() / 2) > IndexSingleCuNumber * m_VirtualCuNum)
			{
				free(IndexBuff);
				IndexBuff = NULL;
				CFuncs::WriteLogInfo(SLT_ERROR, _T("VBoxGetHA0FileRecordRefer:�����������������ʧ��!"));
				return false;
			}
			for (DWORD updataNum = 0; updataNum < (IndexUpdata.size() / 2); updataNum ++)
			{
				RtlCopyMemory(&IndexBuff[510 + (updataNum * 512)], &IndexUpdata[updataNum * 2], 1);
				RtlCopyMemory(&IndexBuff[511 + (updataNum * 512)], &IndexUpdata[(updataNum * 2) + 1], 1);
			}
			DWORD IndexTotalSize = IndexheadSize;
			LSTANDARD_INDEX_TERMS IndexTerm = NULL;
			while (IndexTotalSize < (IndexRealSize + 8))
			{
				IndexTerm = (LSTANDARD_INDEX_TERMS)&IndexBuff[IndexTotalSize];

				DWORD fileRefer = NULL;

				RtlCopyMemory(&fileRefer, &IndexTerm->_File_MFT_Refer_Num[0], 4);

				UCHAR *fileName = NULL;

				fileName = (UCHAR*)&IndexTerm[0] + 82;

				BrowerfileRefer[fileRefer].append((char*)&fileName[0], (IndexTerm->_FileName_Length * 2));

				DWORD IndexoneSize = NULL;

				RtlCopyMemory(&IndexoneSize, &IndexTerm->_TIndex_Term_Size[0], 2);

				if (NULL == IndexoneSize || IndexoneSize > ((IndexRealSize + 8) - IndexTotalSize))
				{
					free(IndexBuff);
					IndexBuff = NULL;
					CFuncs::WriteLogInfo(SLT_ERROR, _T("VBoxGetHA0FileRecordRefer:��������IndexoneSize��СΪ0!"));
					return false;
				}
				IndexTotalSize += IndexoneSize;
			}

			IndexNumber += IndexSingleCuNumber ;
			free(IndexBuff);
			IndexBuff = NULL;
		}

	}

	return true;
}
bool GetVirtualMachineInfo::VBoxGetRecordFileAddr(HANDLE hDrive, DWORD64 *FileRealSize, UCHAR *FilrRecordBuff, vector<DWORD> &RecordH20Refer, UCHAR VirtualCuNum, DWORD64 VirtualStartPatition
	, UCHAR *VdiBatBuff, DWORD VdiBatSingleSize, DWORD BatListSize, DWORD VdiDataStartAddr, vector<LONG64> &FileH80Addr, vector<DWORD> &FileH80len
	, string &FileH80Data)
{
	*FileRealSize = NULL;
	bool Ret = false;
	DWORD BackBytesCount = NULL;
	LFILE_Head_Recoding File_head_recod = NULL;
	LATTRIBUTE_HEADS ATTriBase = NULL;
	LAttr_20H H20 = NULL;
	UCHAR *H80_data = NULL;
	DWORD64 VirtualBackAddr = NULL;
	DWORD AttributeSize = NULL;
	DWORD FirstAttriSize = NULL;

	File_head_recod = (LFILE_Head_Recoding)&FilrRecordBuff[0];

	RtlCopyMemory(&FilrRecordBuff[510], &FilrRecordBuff[File_head_recod->_Update_Sequence_Number[0]+2], 2);//������������	
	RtlCopyMemory(&FilrRecordBuff[1022],&FilrRecordBuff[File_head_recod->_Update_Sequence_Number[0]+4], 2);
	RtlCopyMemory(&FirstAttriSize, &File_head_recod->_First_Attribute_Dev[0], 2);
	if (FirstAttriSize > (FILE_SECTOR_SIZE - sizeof(ATTRIBUTE_HEADS)))
	{
		CFuncs::WriteLogInfo(SLT_ERROR, "VBoxGetRecordFileAddr::File_head_recod->_First_Attribute_Dev[0] > FILE_SECTOR_SIZEʧ��!");
		return false;
	}
	while ((FirstAttriSize + AttributeSize) < FILE_SECTOR_SIZE)
	{
		ATTriBase = (LATTRIBUTE_HEADS)&FilrRecordBuff[FirstAttriSize + AttributeSize];
		if(ATTriBase->_Attr_Type != 0xffffffff)
		{
			if (ATTriBase->_Attr_Type == 0x20)
			{
				DWORD h20Length=NULL;
				switch(ATTriBase->_PP_Attr)
				{
				case 0:
					if (ATTriBase->_AttrName_Length==0)
					{
						h20Length = 24;
					} 
					else
					{
						h20Length=24 + 2 * ATTriBase->_AttrName_Length;
					}
					break;
				case 0x01:
					if (ATTriBase->_AttrName_Length==0)
					{
						h20Length = 64;
					} 
					else
					{
						h20Length = 64 + 2 * ATTriBase->_AttrName_Length;
					}
					break;
				}
				if (h20Length > (ATTriBase->_Attr_Length))
				{
					CFuncs::WriteLogInfo(SLT_ERROR, _T("VBoxGetRecordFileAddr:h20Length > (ATTriBase->_Attr_Length)ʧ��!"));
					return false;
				}
				if (ATTriBase->_PP_Attr == 0)
				{
					H20 = (LAttr_20H)((UCHAR*)&ATTriBase[0] + h20Length);
					while (H20->_H20_TYPE != 0)
					{
						
						
						if (H20->_H20_TYPE==0x80)
						{							
							RecordH20Refer.push_back(H20->_H20_FILE_Reference_Num.LowPart);
						
						}
						else if (H20->_H20_TYPE == 0)
						{
							break;
						}
						else if (H20->_H20_TYPE > 0xFF)
						{
							break;
						}
						if(H20->_H20_Attr_Name_Length*2>0)
						{
							if ((H20->_H20_Attr_Name_Length*2+26)%8!=0)
							{
								h20Length+=(((H20->_H20_Attr_Name_Length*2+26)/8)*8+8);
							}
							else if ((H20->_H20_Attr_Name_Length*2+26)%8==0)
							{
								h20Length+=(H20->_H20_Attr_Name_Length*2+26);
							}
						}
						else
						{
							h20Length+=32;
						}
						if (h20Length > (ATTriBase->_Attr_Length))
						{
							break;
						}
						H20=(LAttr_20H)((UCHAR*)&ATTriBase[0]+h20Length);
					}
				} 
				else if (ATTriBase->_PP_Attr==1)
				{
					UCHAR *H20Data=NULL;

					DWORD64 H20DataRun=NULL;

					H20Data=(UCHAR*)&ATTriBase[0];

					DWORD H20Offset=ATTriBase->TWOATTRIBUTEHEAD.NP_head._NPN_RunList_Offset[0];

					if (H20Data[H20Offset] != 0 && H20Data[H20Offset] < 0x50)
					{
						UCHAR adres_fig = H20Data[H20Offset] >> 4;
						UCHAR len_fig = H20Data[H20Offset] & 0xf;
						for (int w = adres_fig; w > 0; w --)
						{
							H20DataRun = H20DataRun | (H20Data[H20Offset + w + len_fig] << (8 * (w - 1)));
						}
					}		
					UCHAR *H20CancheBuff = (UCHAR*)malloc(SECTOR_SIZE * VirtualCuNum + SECTOR_SIZE);
					if (NULL == H20CancheBuff)
					{
						CFuncs::WriteLogInfo(SLT_ERROR, _T("VBoxGetRecordFileAddr:malloc: H20CancheBuffʧ��!"));
						return false;
					}
					memset(H20CancheBuff, 0, SECTOR_SIZE * VirtualCuNum + SECTOR_SIZE);
					for (int i = 0; i < VirtualCuNum; i++)
					{					
						VirtualBackAddr = NULL;
						if(!VdiOneAddrChange((VirtualStartPatition * SECTOR_SIZE + H20DataRun * VirtualCuNum * SECTOR_SIZE + SECTOR_SIZE * i)
							, VdiBatBuff, VdiBatSingleSize, &VirtualBackAddr, BatListSize, VdiDataStartAddr))
						{
							CFuncs::WriteLogInfo(SLT_ERROR, _T("VBoxGetRecordFileAddr:VhdOneAddrChange: ת����ʼMft�ļ���¼��ַʧ��!"));
							return false;
						}
						Ret = ReadSQData(hDrive, &H20CancheBuff[i*SECTOR_SIZE], SECTOR_SIZE, VirtualBackAddr,
							&BackBytesCount);		
						if(!Ret)
						{			
							CFuncs::WriteLogInfo(SLT_ERROR, _T("VBoxGetRecordFileAddr:ReadSQData: ��ȡ��ʼMft�ļ���¼��ַʧ��!"));
							return false;	
						}
					}
					h20Length = 0;
					H20 = (LAttr_20H)&H20CancheBuff[h20Length];
					while (H20->_H20_TYPE != 0)
					{
						
						H20 = (LAttr_20H)&H20CancheBuff[h20Length];
						if (H20->_H20_TYPE == 0x80)
						{
							RecordH20Refer.push_back(H20->_H20_FILE_Reference_Num.LowPart);
							break;
						}
						else if (H20->_H20_TYPE == 0)
						{
							break;
						}
						else if (H20->_H20_TYPE > 0xFF)
						{
							break;
						}
						if(H20->_H20_Attr_Name_Length * 2 > 0)
						{
							if ((H20->_H20_Attr_Name_Length * 2 + 26) % 8 != 0)
							{
								h20Length += (((H20->_H20_Attr_Name_Length * 2 + 26) / 8) * 8 + 8);
							}
							else if ((H20->_H20_Attr_Name_Length*2+26) % 8 == 0)
							{
								h20Length += (H20->_H20_Attr_Name_Length*2+26);
							}
						}
						else
						{
							h20Length += 32;
						}
						if (h20Length > (DWORD)(SECTOR_SIZE * VirtualCuNum))
						{
							break;
						}
					}

					free(H20CancheBuff);
					H20CancheBuff = NULL;
				}
			}

			if (RecordH20Refer.size() > 0)
			{
				vector<DWORD>::iterator vec;
				for (vec = RecordH20Refer.begin(); vec < RecordH20Refer.end(); vec ++)
				{
					if (*vec != File_head_recod->_FR_Refer)
					{
						CFuncs::WriteLogInfo(SLT_INFORMATION, "VBoxGetRecordFileAddr ���ļ���¼H80�ض�λ��H20�У��ض�λ�ļ��ο�����:%lu", *vec);						
					}
					else
					{
						RecordH20Refer.erase(vec);//��ͬ�ľ�û�ض�λ������Ϊ��
					}
				}

			}


			if (ATTriBase->_Attr_Type == 0x80)
			{
				DWORD H80_datarun_len = NULL;
				LONG64 H80_datarun = NULL;
				bool FirstIn = true;
				if (ATTriBase->_PP_Attr == 0x01)
				{
					(*FileRealSize) = ((*FileRealSize) + ATTriBase->TWOATTRIBUTEHEAD.NP_head._NPN_Attr_T_Size);//ȡ�ô��ļ�����ʵ��С
					H80_data = (UCHAR*)&ATTriBase[0];
					DWORD OFFSET = NULL;
					RtlCopyMemory(&OFFSET, &ATTriBase->TWOATTRIBUTEHEAD.NP_head._NPN_RunList_Offset[0], 2);
					if (OFFSET > (FILE_SECTOR_SIZE - FirstAttriSize - AttributeSize))
					{
						CFuncs::WriteLogInfo(SLT_ERROR, _T("VBoxGetRecordFileAddr::OFFSET������Χ!"));
						return false;
					}

					if (H80_data[OFFSET] != 0 && H80_data[OFFSET] < 0x50)
					{					
						while(OFFSET < ATTriBase->_Attr_Length)
						{
							H80_datarun_len = NULL;
							H80_datarun = NULL;
							if (H80_data[OFFSET] > 0 && H80_data[OFFSET] < 0x50)
							{
								UCHAR adres_fig = H80_data[OFFSET] >> 4;
								UCHAR len_fig = H80_data[OFFSET] & 0xf;
								for(int w = len_fig;w > 0; w--)
								{
									H80_datarun_len = H80_datarun_len | (H80_data[OFFSET + w] << (8 * (w - 1)));
								}
								if (H80_datarun_len > 0)
								{
									FileH80len.push_back(H80_datarun_len);
								} 
								else
								{
									CFuncs::WriteLogInfo(SLT_ERROR, _T("VBoxGetRecordFileAddr::H80_datarun_lenΪ0!"));
									return false;
								}

								for (int w = adres_fig; w > 0; w --)
								{
									H80_datarun = H80_datarun | (H80_data[OFFSET+w+len_fig] << (8 * (w - 1)));
								}
								if (H80_data[OFFSET + adres_fig + len_fig] > 127)
								{
									if (adres_fig == 3)
									{
										H80_datarun = ~(H80_datarun^0xffffff);
									}
									if (adres_fig == 2)
									{
										H80_datarun = ~(H80_datarun^0xffff);

									}

								} 
								if (FirstIn)
								{
									if (H80_datarun > 0)
									{
										FileH80Addr.push_back(H80_datarun);
									} 
									else
									{
										CFuncs::WriteLogInfo(SLT_ERROR, _T("VBoxGetRecordFileAddr::H80_datarunΪ0��Ϊ��������!"));
										return false;
									}
									FirstIn = false;
								}
								else
								{
									if (FileH80Addr.size() > 0)
									{
										H80_datarun = FileH80Addr[FileH80Addr.size() - 1] + H80_datarun;
										FileH80Addr.push_back(H80_datarun);
									}
								}
								
								OFFSET = OFFSET + adres_fig + len_fig + 1;
							}
							else
							{
								break;
							}

						}								
					}

				}
				else if(ATTriBase->_PP_Attr == 0)
				{
					H80_data = (UCHAR*)&ATTriBase[0];		
					if (ATTriBase->TWOATTRIBUTEHEAD.P_head._PN_AttrBody_Length < (FILE_SECTOR_SIZE - AttributeSize - FirstAttriSize - 24))
					{
						FileH80Data.append((char*)&H80_data[24],ATTriBase->TWOATTRIBUTEHEAD.P_head._PN_AttrBody_Length);
					}


				}

			}
			if (ATTriBase->_Attr_Length < (FILE_SECTOR_SIZE - AttributeSize - FirstAttriSize) && ATTriBase->_Attr_Length > 0)
			{

				AttributeSize += ATTriBase->_Attr_Length;

			}  
			else
			{
				CFuncs::WriteLogInfo(SLT_ERROR, _T("VBoxGetRecordFileAddr::������:%lu,ATTriBase[i_attr]->_Attr_Length���ȹ����Ϊ0!")
					,ATTriBase->_Attr_Length);
				return false;
			}
		}
		else if (ATTriBase->_Attr_Type == 0xffffffff)
		{
			break;
		}
		else if(ATTriBase->_Attr_Type > 0xff && ATTriBase->_Attr_Type < 0xffffffff)
		{
			CFuncs::WriteLogInfo(SLT_ERROR, _T("VBoxGetRecordFileAddr:��ȡATTriBase[i_attr]->_Attr_Type����0xffffffff����"));
			return false;
		}

	}
	return true;
}
bool GetVirtualMachineInfo::VBoxExtractingTheCatalogFile(HANDLE hDrive, DWORD64 VirtualPatition, UCHAR *VirVdiBatBuff, DWORD VirBatSingleSize, DWORD VirBatBuffSize,
	vector<LONG64> v_VirtualStartMFTaddr, vector<DWORD64> v_VirtualStartMFTaddrLen, DWORD Rerefer, UCHAR VirtualCuNum, DWORD VirVdiDataAddr, string RecordFileName
	, const char *virtualFileDir)
{
	DWORD64 VirtualBackAddr = NULL;
	bool Ret = false;
	DWORD BackBytesCount = NULL;

	DWORD64 MftLenAdd = NULL;
	LONG64 MftAddr = NULL;

	for (DWORD FMft = 0; FMft < v_VirtualStartMFTaddrLen.size(); FMft++)
	{
		if ((Rerefer * 2) <= (v_VirtualStartMFTaddrLen[FMft] * VirtualCuNum + MftLenAdd))
		{
			MftAddr = (v_VirtualStartMFTaddr[FMft] * VirtualCuNum + (Rerefer * 2) - MftLenAdd);
			break;
		} 
		else
		{
			MftLenAdd += (v_VirtualStartMFTaddrLen[FMft] * VirtualCuNum);
		}
	}
	if (v_VirtualStartMFTaddrLen.size() == NULL)
	{
		MftAddr = v_VirtualStartMFTaddr[0] * VirtualCuNum;
	}
	UCHAR *CacheBuff = (UCHAR*)malloc(FILE_SECTOR_SIZE + SECTOR_SIZE);
	if (NULL == CacheBuff)
	{
		CFuncs::WriteLogInfo(SLT_ERROR, _T("VBoxExtractingTheCatalogFile:malloc:CacheBuffʧ��!"));
		return false;
	}
	memset(CacheBuff, 0, (FILE_SECTOR_SIZE + SECTOR_SIZE));
	for (int i=0;i < 2;i++)
	{
		VirtualBackAddr=NULL;

		if(!VdiOneAddrChange((VirtualPatition + MftAddr  + i) * SECTOR_SIZE
			, VirVdiBatBuff, VirBatSingleSize, &VirtualBackAddr, VirBatBuffSize, VirVdiDataAddr))
		{			
			free(CacheBuff);
			CacheBuff = NULL;
			CFuncs::WriteLogInfo(SLT_ERROR, _T("VBoxExtractingTheCatalogFile:Virtual_to_Host_OneAddrʧ��!"));
			return false;

		}

		Ret = ReadSQData(hDrive, &CacheBuff[i*SECTOR_SIZE], SECTOR_SIZE, VirtualBackAddr,
			&BackBytesCount);		
		if(!Ret)
		{			
			free(CacheBuff);
			CacheBuff = NULL;
			CFuncs::WriteLogInfo(SLT_ERROR, _T("VBoxExtractingTheCatalogFile:ReadSQData: ��ȡ��ʼMft�ļ���¼��ַʧ��!"));
			return false;	
		}
	}
	vector<DWORD> H20Refer;
	vector<LONG64> fileH80Addr;
	vector<DWORD> fileH80Len;
	string fileData;
	DWORD64 fileRealSize = NULL;
	if (!VBoxGetRecordFileAddr(hDrive, &fileRealSize, CacheBuff, H20Refer, VirtualCuNum, VirtualPatition
		, VirVdiBatBuff, VirBatSingleSize, VirBatBuffSize, VirVdiDataAddr, fileH80Addr, fileH80Len, fileData))
	{

		CFuncs::WriteLogInfo(SLT_ERROR, _T("VBoxExtractingTheCatalogFile:GetRecordFileAddrʧ��!"));
		return false;
	}
	if (H20Refer.size() > 0)
	{
		UCHAR *H20CacheBuff = (UCHAR*) malloc(SECTOR_SIZE * VirtualCuNum + SECTOR_SIZE);
		if (NULL == H20CacheBuff)
		{

			CFuncs::WriteLogInfo(SLT_ERROR, "VBoxExtractingTheCatalogFile:malloc:H20CacheBuffʧ��!");
			return false;
		}
		memset(H20CacheBuff, 0, SECTOR_SIZE * VirtualCuNum + SECTOR_SIZE);
		vector<DWORD>::iterator h20vec;
		for (h20vec = H20Refer.begin(); h20vec < H20Refer.end(); h20vec++)
		{
			memset(H20CacheBuff, 0, FILE_SECTOR_SIZE);
			DWORD64 VirMftLen = NULL;
			DWORD64 VirStartMftRfAddr = NULL;
			for (DWORD FRN = 0; FRN < v_VirtualStartMFTaddrLen.size(); FRN++)
			{
				if (((*h20vec) * 2) < (VirMftLen + v_VirtualStartMFTaddrLen[FRN] * VirtualCuNum))
				{
					VirStartMftRfAddr = v_VirtualStartMFTaddr[FRN] * VirtualCuNum + ((*h20vec) * 2 - VirMftLen);
					break;
				} 
				else
				{
					VirMftLen += (v_VirtualStartMFTaddrLen[FRN] * VirtualCuNum);
				}
			}
			for (int i = 0; i < 2; i++)
			{
				VirtualBackAddr = NULL;
				if(!VdiOneAddrChange((VirtualPatition * SECTOR_SIZE + VirStartMftRfAddr * SECTOR_SIZE + SECTOR_SIZE * i)
					, VirVdiBatBuff, VirBatSingleSize, &VirtualBackAddr, VirBatBuffSize, VirVdiDataAddr))
				{

					free(H20CacheBuff);
					H20CacheBuff = NULL;

					CFuncs::WriteLogInfo(SLT_ERROR, _T("VBoxExtractingTheCatalogFile:VhdOneAddrChange: ת����ʼMft�ļ���¼��ַʧ��!"));
					return false;
				}
				Ret = ReadSQData(hDrive, &H20CacheBuff[i * SECTOR_SIZE], SECTOR_SIZE,  VirtualBackAddr,
					&BackBytesCount);		
				if(!Ret)
				{		

					free(H20CacheBuff);
					H20CacheBuff = NULL;
					CFuncs::WriteLogInfo(SLT_ERROR, _T("VBoxExtractingTheCatalogFile:ReadSQData: ��ȡ��ʼMft�ļ���¼��ַʧ��!"));
					return false;	
				}
			}

			if(!GetVirtualH20FileReferH80Addr(H20CacheBuff,fileH80Addr, fileH80Len, fileData
				, &fileRealSize))//��Ϊ��������ⲿ��ȡ�����ݣ�����������0	
			{

				free(H20CacheBuff);
				H20CacheBuff = NULL;
				CFuncs::WriteLogInfo(SLT_ERROR, _T("VBoxExtractingTheCatalogFile:GetH20FileReferH80Addr: ʧ��!"));
				return false;
			}

		}
		free(H20CacheBuff);
		H20CacheBuff = NULL;
	}
	if (fileH80Addr.size() > 0)//����Ϊ��ַ����ȡ���ļ�
	{
		string VirtualPath;
		string StrTemName;
		if (RecordFileName.length() > 0)
		{
			if(!UnicodeToZifu((UCHAR*)&RecordFileName[0], StrTemName, RecordFileName.length()))
			{

				CFuncs::WriteLogInfo(SLT_ERROR, "VBoxExtractingTheCatalogFile:UnicodeToZifu : FileNameʧ��!");
				return false;
			}
		
		


		DWORD NameSize = RecordFileName.length() + strlen(virtualFileDir);
		wchar_t * WirteName = new wchar_t[NameSize + 1];
		if (NULL == WirteName)
		{


			CFuncs::WriteLogInfo(SLT_ERROR, _T("VBoxExtractingTheCatalogFile:new:WirteName ���������ڴ�ʧ��!"));
			return false;
		}
		memset(WirteName, 0, (NameSize + 1) * 2);
		MultiByteToWideChar(CP_ACP, 0, virtualFileDir, strlen(virtualFileDir), WirteName, NameSize);

		for (DWORD NameIndex = 0; NameIndex < RecordFileName.length(); NameIndex += 2)
		{
			RtlCopyMemory(&WirteName[strlen(virtualFileDir) + NameIndex / 2],(UCHAR*) &RecordFileName[NameIndex],2);
		}
		char *WriteFileBuffer=(char*)malloc(2048*SECTOR_SIZE+1);
		if (NULL == WriteFileBuffer)
		{
			delete WirteName;
			WirteName = NULL;
			CFuncs::WriteLogInfo(SLT_ERROR, "VBoxExtractingTheCatalogFile:malloc����WriteFileBuffer�ڴ�ʧ��!");
			return false;
		}
		memset(WriteFileBuffer, 0, 2048*SECTOR_SIZE);
		if(!VirtualVdiWriteLargeFile(hDrive, fileH80Addr, fileH80Len, VirtualCuNum, VirVdiBatBuff, VirBatSingleSize
			, VirBatBuffSize, WriteFileBuffer, WirteName, VirVdiDataAddr, VirtualPatition, fileRealSize))
		{
			delete WirteName;
			WirteName = NULL;
			free(WriteFileBuffer);
			WriteFileBuffer = NULL;
			CFuncs::WriteLogInfo(SLT_ERROR, "VBoxExtractingTheCatalogFile:WriteLargeFile:��ȡH20H80��ַʧ��ʧ��");

		}
		delete WirteName;
		WirteName = NULL;
		free(WriteFileBuffer);
		WriteFileBuffer = NULL;
		}


	}else if (fileData.length() > 0)//������h80���ȡС�ļ�
	{
		string VirtualPath;
		string StrTemName;

		if (RecordFileName.length() > 0)
		{
			if(!UnicodeToZifu((UCHAR*)&RecordFileName[0], StrTemName, RecordFileName.length()))
			{

				CFuncs::WriteLogInfo(SLT_ERROR, "VBoxExtractingTheCatalogFile:UnicodeToZifu : FileNameʧ��!");
				return false;
			}
		


		DWORD NameSize = RecordFileName.length() + strlen(virtualFileDir);
		wchar_t * WirteName = new wchar_t[NameSize + 1];
		if (NULL == WirteName)
		{

			CFuncs::WriteLogInfo(SLT_ERROR, _T("VBoxExtractingTheCatalogFile:new:WirteName ���������ڴ�ʧ��!"));
			return false;
		}
		memset(WirteName, 0, (NameSize + 1) * 2);
		MultiByteToWideChar(CP_ACP, 0, virtualFileDir, strlen(virtualFileDir), WirteName, NameSize);

		for (DWORD NameIndex = 0; NameIndex < RecordFileName.length(); NameIndex += 2)
		{
			RtlCopyMemory(&WirteName[strlen(virtualFileDir) + NameIndex / 2],(UCHAR*) &RecordFileName[NameIndex],2);
		}
		if(!VirtualWriteLitteFile(fileData, WirteName))
		{
			delete WirteName;
			WirteName=NULL;
			CFuncs::WriteLogInfo(SLT_ERROR, "VBoxExtractingTheCatalogFile:WriteLitteFile:ʧ��ʧ��");

		}
		delete WirteName;
		WirteName=NULL;
		}

	}

	return true;
}
bool GetVirtualMachineInfo::VBoxGetCatalogFileRefer(HANDLE hDrive, UCHAR *fileRecordBuff, map<DWORD, string> &BrowerfileRefer, UCHAR VirCuNum, DWORD64 VirtualPatition
	, UCHAR *VdiBatBuff, DWORD VdiBatSingleSize, DWORD BatListSize, DWORD VdiDataStartAddr)
{
	bool Ret = false;
	DWORD BackBytesCount = NULL;
	LFILE_Head_Recoding File_head_recod = NULL;
	LATTRIBUTE_HEADS ATTriBase = NULL;
	DWORD64 VirtualBackAddr = NULL;
	LAttr_90H_Index_ROOT  H90_root;
	LAttr_90H_Index_Head  H90_head;
	LAttr_90H_Index_Entry H90_entry;	
	UCHAR *H90_name;


	DWORD AttributeSize = NULL;
	DWORD FirstAttriSize = NULL;

	File_head_recod = (LFILE_Head_Recoding)&fileRecordBuff[0];
	RtlCopyMemory(&FirstAttriSize, &File_head_recod->_First_Attribute_Dev[0], 2);
	if (FirstAttriSize > (FILE_SECTOR_SIZE - sizeof(ATTRIBUTE_HEADS)))
	{
		CFuncs::WriteLogInfo(SLT_ERROR, "VBoxGetCatalogFileRefer::File_head_recod->_First_Attribute_Dev[0] > FILE_SECTOR_SIZEʧ��!");
		return false;
	}
	while ((FirstAttriSize + AttributeSize) < FILE_SECTOR_SIZE)
	{
		ATTriBase = (LATTRIBUTE_HEADS)&fileRecordBuff[FirstAttriSize + AttributeSize];
		if(ATTriBase->_Attr_Type != 0xffffffff)
		{
			if (ATTriBase->_Attr_Type == 0x90)
			{
				DWORD H90Size = NULL;
				switch(ATTriBase->_PP_Attr)
				{
				case 0:
					if (ATTriBase->_AttrName_Length == 0)
					{
						H90_root = (LAttr_90H_Index_ROOT)((UCHAR*)&ATTriBase[0] + 24);
						H90_head = (LAttr_90H_Index_Head)((UCHAR*)&ATTriBase[0] + 24 + 16);
						H90_entry = (LAttr_90H_Index_Entry)((UCHAR*)&ATTriBase[0] + 24 + 32);
						H90Size = 24 + 32 + AttributeSize + FirstAttriSize;
					} 
					else
					{
						H90_root = (LAttr_90H_Index_ROOT)((UCHAR*)&ATTriBase[0] + 24 + 2 * ATTriBase->_AttrName_Length);
						H90_head = (LAttr_90H_Index_Head)((UCHAR*)&ATTriBase[0] + 24 + 2 * ATTriBase->_AttrName_Length + 16);
						H90_entry = (LAttr_90H_Index_Entry)((UCHAR*)&ATTriBase[0] + 24 + 2 * ATTriBase->_AttrName_Length + 32);
						H90Size = 24 + 2 * ATTriBase->_AttrName_Length + 32 + AttributeSize + FirstAttriSize;
					}
					break;
				case 0x01:
					if (ATTriBase->_AttrName_Length == 0)
					{
						H90_root = (LAttr_90H_Index_ROOT)((UCHAR*)&ATTriBase[0] + 64);
						H90_head = (LAttr_90H_Index_Head)((UCHAR*)&ATTriBase[0] + 64 + 16);
						H90_entry = (LAttr_90H_Index_Entry)((UCHAR*)&ATTriBase[0] + 64 + 32);
						H90Size = 64 + 32 + AttributeSize + FirstAttriSize;
					} 
					else
					{
						H90_root = (LAttr_90H_Index_ROOT)((UCHAR*)&ATTriBase[0] + 64 + 2 * ATTriBase->_AttrName_Length);
						H90_head = (LAttr_90H_Index_Head)((UCHAR*)&ATTriBase[0] + 64 + 2 * ATTriBase->_AttrName_Length + 16);
						H90_entry = (LAttr_90H_Index_Entry)((UCHAR*)&ATTriBase[0] + 64 + 2 * ATTriBase->_AttrName_Length + 32);
						H90Size = 64 + 2 * ATTriBase->_AttrName_Length + 32 + AttributeSize + FirstAttriSize;
					}
					break;
				}
				if (H90_head->_H90_IH_Index_Total_Size > 82 && H90_head->_H90_IH_Index_Total_Size < (FILE_SECTOR_SIZE - AttributeSize - FirstAttriSize))
				{
					DWORD entrySize = NULL;
					for (DWORD entryoff = 0; entryoff < (H90_head->_H90_IH_Index_Total_Size - 16); entryoff += entrySize)
					{
						H90_entry = (LAttr_90H_Index_Entry)((UCHAR*)H90_entry + entrySize);
						H90_name = (UCHAR*)H90_entry + 82;
						entrySize = NULL;
						RtlCopyMemory(&entrySize, &H90_entry->_H90_IE_Index_Size[0], 2);
						if (NULL == entrySize || entrySize > (H90_head->_H90_IH_Index_Total_Size - entryoff))
						{

							CFuncs::WriteLogInfo(SLT_ERROR, _T("VBoxGetCatalogFileRefer:H90entrySizeΪ�ջ�H90entrySize��������������ֽ���!"));
							return false;
						}
						DWORD filerecord = NULL;

						RtlCopyMemory(&filerecord, &H90_entry->_H90_IE_MFT_Reference_Index[0], 4);
						if (filerecord != NULL)
						{
							BrowerfileRefer[filerecord].append((char*)&H90_name[0], (H90_entry->_H90_IE_FILE_Name_Length * 2));
						}

					}
				}

			}
			else if (ATTriBase->_Attr_Type == 0xA0)
			{
				DWORD HA0datarunlen = NULL;
				LONG64 HA0datarun = NULL;
				DWORD64 A0FileRealSize = NULL;
				UCHAR *HA0data = NULL;
				vector<DWORD> HA0Addrlen;
				vector<LONG64> HA0Addr;
				bool HA0first = true;

				if (ATTriBase->_PP_Attr == 0x01)
				{
					A0FileRealSize = ATTriBase->TWOATTRIBUTEHEAD.NP_head._NPN_Attr_T_Size;//ȡ�ô��ļ�����ʵ��С
					HA0data = (UCHAR*)&ATTriBase[0];
					DWORD OFFSET = NULL;
					RtlCopyMemory(&OFFSET, &ATTriBase->TWOATTRIBUTEHEAD.NP_head._NPN_RunList_Offset[0], 2);

					if (OFFSET > (FILE_SECTOR_SIZE - FirstAttriSize - AttributeSize))
					{

						CFuncs::WriteLogInfo(SLT_ERROR, _T("VBoxGetCatalogFileRefer::HA0OFFSET����Χ!"));
						return false;
					}
					if (HA0data[OFFSET] != 0 && HA0data[OFFSET] < 0x50)
					{					
						while(OFFSET < ATTriBase->_Attr_Length)
						{
							HA0datarunlen = NULL;
							HA0datarun = NULL;
							if (HA0data[OFFSET] > 0 && HA0data[OFFSET] < 0x50)
							{
								UCHAR adres_fig = HA0data[OFFSET] >> 4;
								UCHAR len_fig = HA0data[OFFSET] & 0xf;
								for(int w = len_fig;w > 0; w--)
								{
									HA0datarunlen = HA0datarunlen | (HA0data[OFFSET + w] << (8 * (w - 1)));
								}
								if (HA0datarunlen > 0)
								{
									HA0Addrlen.push_back(HA0datarunlen);
								} 
								else
								{
									CFuncs::WriteLogInfo(SLT_ERROR, _T("VBoxGetCatalogFileRefer::H80_datarun_lenΪ0!"));
									return false;
								}

								for (int w = adres_fig; w > 0; w --)
								{
									HA0datarun = HA0datarun | (HA0data[OFFSET + w + len_fig] << (8 * (w - 1)));
								}
								if (HA0data[OFFSET + adres_fig + len_fig] > 127)
								{
									if (adres_fig == 3)
									{
										HA0datarun = ~(HA0datarun^0xffffff);
									}
									if (adres_fig == 2)
									{
										HA0datarun = ~(HA0datarun^0xffff);

									}

								} 
								if (HA0first)
								{
									if (HA0datarun > 0)
									{
										HA0Addr.push_back(HA0datarun);
									} 
									else
									{
										CFuncs::WriteLogInfo(SLT_ERROR, _T("VBoxGetCatalogFileRefer::H80_datarunΪ0��Ϊ��������!"));
										return false;
									}
									HA0first = false;
								}
								else
								{
									if (HA0Addr.size() > 0)
									{
										HA0datarun = HA0Addr[HA0Addr.size() - 1] + HA0datarun;
										HA0Addr.push_back(HA0datarun);
									}
								}
								OFFSET = OFFSET + adres_fig + len_fig + 1;
							}
							else
							{
								break;
							}

						}								
					}
				}
				if (HA0Addr.size() > 0 && HA0Addr.size() == HA0Addrlen.size())
				{
					if(!VBoxGetHA0FileRecordRefer(hDrive, HA0Addr, HA0Addrlen, VirCuNum, A0FileRealSize, VirtualPatition, VdiBatBuff, VdiBatSingleSize, BatListSize,
						VdiDataStartAddr, BrowerfileRefer))
					{
						CFuncs::WriteLogInfo(SLT_ERROR, _T("VBoxGetCatalogFileRefer:GetHA0FileRecordReferʧ��!"));
						return false;
					}
				}
				break;
			}
			if (ATTriBase->_Attr_Length < (FILE_SECTOR_SIZE - AttributeSize - FirstAttriSize) && ATTriBase->_Attr_Length > 0)
			{

				AttributeSize += ATTriBase->_Attr_Length;

			}  
			else
			{
				CFuncs::WriteLogInfo(SLT_ERROR, _T("VBoxGetCatalogFileRefer::������:%lu,ATTriBase[i_attr]->_Attr_Length���ȹ����Ϊ0!")
					,ATTriBase->_Attr_Length);
				return false;
			}
		}
		else if (ATTriBase->_Attr_Type == 0xffffffff)
		{
			break;
		}
		else if(ATTriBase->_Attr_Type > 0xff && ATTriBase->_Attr_Type < 0xffffffff)
		{
			CFuncs::WriteLogInfo(SLT_ERROR, _T("VBoxGetCatalogFileRefer:��ȡATTriBase[i_attr]->_Attr_Type����0xffffffff����"));
			return false;
		}
	}
	return true;
}
bool GetVirtualMachineInfo::VBoxFindRecordFile(HANDLE hDrive, DWORD64 VirtualStartPatition, DWORD64 VirStartMftRfAddr, UCHAR VirtualCuNum, DWORD Rerefer
	, UCHAR *VdiBatBuff, DWORD VdiBatSingleSize, DWORD BatListSize, string &BrowerType, string &RecordFileName, int *PathFound, DWORD *FileRefer
	, UCHAR *CacheBuff, DWORD VdiDataStartAddr, map<string, map<string, string>> PathName, string &VirtualFilePath, vector<LONG64> v_VirtualStartMFTaddr
	, vector<DWORD64> v_VirtualStartMFTaddrLen)
{
	BrowerType.clear();
	RecordFileName.clear();
	DWORD ParentMft = NULL;
	*PathFound = 0;
	*FileRefer = 0;
	bool Ret = false;
	DWORD64 VirtualBackAddr = NULL;
	DWORD BackBytesCount = NULL;
	LFILE_Head_Recoding File_head_recod = NULL;
	LATTRIBUTE_HEADS ATTriBase = NULL;
	LAttr_30H H30 = NULL;
	LAttr_20H H20 = NULL;
	UCHAR *H30_NAMES = NULL;
	DWORD AttributeSize = NULL;
	DWORD FirstAttriSize = NULL;

	memset(CacheBuff, 0, FILE_SECTOR_SIZE);

	for (int i = 0; i < 2; i++)
	{
		VirtualBackAddr = NULL;
		if(!VdiOneAddrChange((VirtualStartPatition * SECTOR_SIZE + VirStartMftRfAddr * VirtualCuNum * SECTOR_SIZE + Rerefer * FILE_SECTOR_SIZE + SECTOR_SIZE * i)
			, VdiBatBuff, VdiBatSingleSize, &VirtualBackAddr, BatListSize, VdiDataStartAddr))
		{
			//CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVirtualFileAddr:VhdOneAddrChange: ת����ʼMft�ļ���¼��ַʧ��!"));
			return true;
		}
		Ret = ReadSQData(hDrive, &CacheBuff[i * SECTOR_SIZE], SECTOR_SIZE, VirtualBackAddr,
			&BackBytesCount);		
		if(!Ret)
		{			
			CFuncs::WriteLogInfo(SLT_ERROR, _T("VBoxFindRecordFile:ReadSQData: ��ȡ��ʼMft�ļ���¼��ַʧ��!"));
			return false;	
		}
	}
	File_head_recod = (LFILE_Head_Recoding)&CacheBuff[0];
	if(File_head_recod->_FILE_Index == 0x454c4946 && File_head_recod->_Flags[0] != 0)
	{
		RtlCopyMemory(&CacheBuff[510], &CacheBuff[File_head_recod->_Update_Sequence_Number[0]+2], 2);//������������	
		RtlCopyMemory(&CacheBuff[1022],&CacheBuff[File_head_recod->_Update_Sequence_Number[0]+4], 2);
		RtlCopyMemory(&FirstAttriSize, &File_head_recod->_First_Attribute_Dev[0], 2);
		if (FirstAttriSize > (FILE_SECTOR_SIZE - sizeof(ATTRIBUTE_HEADS)))
		{
			CFuncs::WriteLogInfo(SLT_ERROR, "VBoxFindRecordFile::File_head_recod->_First_Attribute_Dev[0] > FILE_SECTOR_SIZEʧ��!");
			return false;
		}
		while ((FirstAttriSize + AttributeSize) < FILE_SECTOR_SIZE)
		{
			ATTriBase = (LATTRIBUTE_HEADS)&CacheBuff[FirstAttriSize + AttributeSize];
			if(ATTriBase->_Attr_Type != 0xffffffff)
			{
				if (ATTriBase->_Attr_Type == 0x30)
				{
					DWORD H30Size = NULL;
					switch(ATTriBase->_PP_Attr)
					{
					case 0:
						if (ATTriBase->_AttrName_Length == 0)
						{
							H30 = (LAttr_30H)((UCHAR*)&ATTriBase[0] + 24);
							H30_NAMES = (UCHAR*)&ATTriBase[0] + 24 + 66;
							H30Size = 24 + 66 + AttributeSize + FirstAttriSize;
						} 
						else
						{
							H30 = (LAttr_30H)((UCHAR*)&ATTriBase[0] + 24 + 2 * ATTriBase->_AttrName_Length);
							H30_NAMES = (UCHAR*)&ATTriBase[0] + 24 + 2 * ATTriBase->_AttrName_Length+66;
							H30Size = 24 + 2 * ATTriBase->_AttrName_Length + 66 + AttributeSize + FirstAttriSize;
						}
						break;
					case 0x01:
						if (ATTriBase->_AttrName_Length == 0)
						{
							H30 = (LAttr_30H)((UCHAR*)&ATTriBase[0] + 64);
							H30_NAMES = (UCHAR*)&ATTriBase[0] + 64 + 66;
							H30Size = 64 + 66 + AttributeSize + FirstAttriSize;
						} 
						else
						{
							H30 = (LAttr_30H)((UCHAR*)&ATTriBase[0] + 64 + 2 * ATTriBase->_AttrName_Length);
							H30_NAMES = (UCHAR*)&ATTriBase[0] + 64 + 2 * ATTriBase->_AttrName_Length + 66;
							H30Size = 64 + 2 * ATTriBase->_AttrName_Length + 66 + AttributeSize + FirstAttriSize;
						}
						break;
					}
					if ((FILE_SECTOR_SIZE - H30Size) < 0)
					{
						CFuncs::WriteLogInfo(SLT_ERROR, _T("VBoxFindRecordFile::(FILE_SECTOR_SIZE - H30Size)ʧ��!"));
						return false;
					}
					DWORD H30FileNameLen = H30->_H30_FILE_Name_Length * 2;
					if (H30FileNameLen > (FILE_SECTOR_SIZE - H30Size) || NULL == H30FileNameLen)
					{
						CFuncs::WriteLogInfo(SLT_ERROR, _T("VBoxFindRecordFile::������Χʧ��!"));
						return false;
					}
					else
					{
						
						RtlCopyMemory(&ParentMft,&H30->_H30_Parent_FILE_Reference,4);
						string FileName_str;
						if (!UnicodeToZifu(&H30_NAMES[0], FileName_str, H30FileNameLen))
						{
							CFuncs::WriteLogInfo(SLT_ERROR, _T("VBoxFindRecordFile:ReadSQData: ��ȡ��ʼMft�ļ���¼��ַʧ��!"));
							return false;
						}
						bool Getpathfirst = true;
						map<string, map<string,string>>::iterator AllPathiter;	
						for (AllPathiter = PathName.begin(); AllPathiter != PathName.end(); AllPathiter ++)
						{
							BrowerType = AllPathiter->first;
							map<string, string>::iterator SinglePathiter;
							for (SinglePathiter = AllPathiter->second.begin(); SinglePathiter != AllPathiter->second.end(); SinglePathiter ++)
							{
								if (SinglePathiter->second == FileName_str)
								{			
									if (Getpathfirst)
									{
										VirtualFilePath.clear();
										UCHAR *RecodeCacheBuff = (UCHAR*)malloc(FILE_SECTOR_SIZE + SECTOR_SIZE);
										if (NULL == RecodeCacheBuff)
										{
											CFuncs::WriteLogInfo(SLT_ERROR, "VBoxFindRecordFile:malloc:RecodeCacheBuffʧ��");
											return false;
										}
										memset(RecodeCacheBuff, 0, (FILE_SECTOR_SIZE + SECTOR_SIZE));
										if(!GetVirtualVdiFileNameAndPath(hDrive, FileName_str, RecodeCacheBuff, ParentMft, v_VirtualStartMFTaddr, v_VirtualStartMFTaddrLen
											, VirtualCuNum, VirtualStartPatition, VdiBatBuff, VdiBatSingleSize, BatListSize, VirtualFilePath, VdiDataStartAddr))
										{
											free(RecodeCacheBuff);
											RecodeCacheBuff = NULL;
											CFuncs::WriteLogInfo(SLT_ERROR, "VBoxFindRecordFile:GetVirtualFileNameAndPath:��ȡ·��ʧ��");
											return false;
										}
										free(RecodeCacheBuff);
										RecodeCacheBuff = NULL;
										if (VirtualFilePath.length() > 0)
										{
											VirtualFilePath.erase((VirtualFilePath.length() - 1), 1);
										}
										if (BrowerType == "firefox")
										{
											size_t Bposion = NULL;
											size_t rBposion = NULL;
											Bposion = VirtualFilePath.rfind("\\");
											if (Bposion != string::npos)
											{
												rBposion = VirtualFilePath.rfind("\\", (Bposion - 1));
												if (rBposion != string::npos)
												{
													string path_Tem;
													path_Tem.append(&VirtualFilePath[0], rBposion);
													path_Tem.append(&VirtualFilePath[Bposion], (VirtualFilePath.length() - Bposion));
													VirtualFilePath.clear();
													VirtualFilePath.append(path_Tem);
													;												}
											}
										}
										else if (BrowerType == "maxthon")
										{
											size_t Bposion = NULL;
											size_t rBposion = NULL;
											Bposion = VirtualFilePath.rfind("History\\");
											if (Bposion != string::npos)
											{
												rBposion = VirtualFilePath.rfind("\\", (Bposion - 2));
												if (rBposion != string::npos)
												{
													string path_Tem;
													path_Tem.append(&VirtualFilePath[0], rBposion);
													path_Tem.append(&VirtualFilePath[Bposion - 1], (VirtualFilePath.length() - (Bposion - 1)));
													VirtualFilePath.clear();
													VirtualFilePath.append(path_Tem);
												}

											}
										}
										Getpathfirst = false;
									}
									string Tem_VirPathName;
									string Tem_PathName;
									if (SinglePathiter->first[SinglePathiter->first.length() -1] == '\\')
									{
										Tem_PathName.append(&SinglePathiter->first[0], SinglePathiter->first.length() -1);
									}else
									{
										Tem_PathName.append(&SinglePathiter->first[0], SinglePathiter->first.length());
									}
									if (VirtualFilePath.length() != Tem_PathName.length())
									{
										if (VirtualFilePath.length() > Tem_PathName.length())
										{
											Tem_VirPathName.append(&VirtualFilePath[VirtualFilePath.length() - Tem_PathName.length()], Tem_PathName.length());
										}
										else
										{
											Tem_VirPathName.append(VirtualFilePath);
										}
										if (Tem_VirPathName == Tem_PathName)
										{
											*FileRefer = File_head_recod->_FR_Refer;

											if (SinglePathiter->first[SinglePathiter->first.length() -1] == '\\')
											{
												*PathFound = 1;//Ŀ¼
											}else
											{
												*PathFound = 2;//�ļ�
												RecordFileName.append((char*)&H30_NAMES[0], H30FileNameLen);
											}

											break;
										}
									}else if (VirtualFilePath.length() == Tem_PathName.length())
									{
										if (VirtualFilePath == Tem_PathName)
										{
											*FileRefer = File_head_recod->_FR_Refer;
											if (SinglePathiter->first[SinglePathiter->first.length() -1] == '\\')
											{
												*PathFound = 1;//Ŀ¼
											}else
											{
												*PathFound = 2;//�ļ�
												RecordFileName.append((char*)&H30_NAMES[0], H30FileNameLen);
											}
											break;
										}
									}

								}

							}
							if ((*PathFound) > 0)
							{
								return true;
							}
						}
					}
				}

				if (ATTriBase->_Attr_Length < (FILE_SECTOR_SIZE - AttributeSize - FirstAttriSize) && ATTriBase->_Attr_Length > 0)
				{

					AttributeSize += ATTriBase->_Attr_Length;

				}
				else
				{
					CFuncs::WriteLogInfo(SLT_ERROR, _T("VBoxFindRecordFile::������:%lu,ATTriBase[i_attr]->_Attr_Length���ȹ����Ϊ0!")
						,ATTriBase->_Attr_Length);
					return false;
				}
			}
			else if (ATTriBase->_Attr_Type==0xffffffff)
			{
				break;
			}
			else if(ATTriBase->_Attr_Type > 0xff && ATTriBase->_Attr_Type < 0xffffffff)
			{
				CFuncs::WriteLogInfo(SLT_ERROR, _T("VBoxFindRecordFile:��ȡATTriBase[i_attr]->_Attr_Type����0xffffffff����"));
				return false;
			}

		}
	}
	return true;
}
bool GetVirtualMachineInfo::GetVBoxInternetInfo(map<string,map<string, string>> PathAndName, string VBoxMftFileName, const char* virtualFileDir
	, PFCallbackVirtualInternetRecord VirtualRecord)
{
	DWORD dwError = NULL;
	bool Ret = false;
	DWORD BackBytesCount = NULL;
	
	string VdiPath;
		size_t VdiPathPosition;
		size_t _VdiPathPosition;

		_VdiPathPosition = VBoxMftFileName.find("\\");
		while(_VdiPathPosition != string::npos)
		{

			if (VBoxMftFileName.find("\\", _VdiPathPosition + 1) == string::npos)
			{
				VdiPathPosition = VBoxMftFileName.find(".vbox");
				if ((VdiPathPosition - _VdiPathPosition + 5) > 0)
				{				
					VdiPath.append(&VBoxMftFileName[0],( _VdiPathPosition + 1));

				}else
				{
					CFuncs::WriteLogInfo(SLT_ERROR, "GetVBoxInternetInfo:��ȡvbox·��ʧ��!");
					return false;
				}

				break;
			}
			_VdiPathPosition = VBoxMftFileName.find("\\", _VdiPathPosition + 1);
		}


		map<DWORD, string>VdiFileInfo;
		int VBoxFileType = NULL;//��������VBox�ṹ�������Vdi����1��VMDK��2
		if(!GetVboxInformation(VdiFileInfo, VBoxMftFileName, VdiPath, &VBoxFileType))
		{
			CFuncs::WriteLogInfo(SLT_ERROR, "GetVBoxInternetInfo:GetVboxInformationʧ��!");
			return false;
		}
		if (VBoxFileType == 2)
		{
			map<DWORD, vector<string>> VboxVmdkInfo;
			if (!GetVboxVmdkInfomation(VboxVmdkInfo, VdiFileInfo))
			{
				CFuncs::WriteLogInfo(SLT_ERROR, "GetVBoxInternetInfo:GetVboxVmdkInfomationʧ��!");
				return false;
			}
			if (VboxVmdkInfo.size() == VdiFileInfo.size())
			{
				if (!AnalysisVmdkFileInternet(PathAndName, VboxVmdkInfo, virtualFileDir, VirtualRecord))
				{
					CFuncs::WriteLogInfo(SLT_ERROR, "GetVBoxInternetInfo:AnalysisVmdkFileʧ��!");
					return false;
				}
			}
		}
		else if (VBoxFileType == 1)
		{
			map<DWORD, string>::iterator Vdiindex;
			for (Vdiindex = VdiFileInfo.begin(); Vdiindex != VdiFileInfo.end(); Vdiindex ++)//Vdi�ļ���ѭ������
			{
				DWORD ParentUUID = NULL;
				vector<DWORD64> VdiNTFSStartAddr;

				ParentUUID = Vdiindex->first;
				if(!GetVdiNTFSStartAddr(&ParentUUID, VdiFileInfo, VdiNTFSStartAddr))
				{
					CFuncs::WriteLogInfo(SLT_ERROR, "GetVBoxInternetInfo:GetVdiNTFSStartAddrʧ��!");
					return false;
				}



				for (DWORD NtfsIndex = 0; NtfsIndex < VdiNTFSStartAddr.size(); NtfsIndex ++)
				{
					DWORD64 VirtualPatition = VdiNTFSStartAddr[NtfsIndex];
					DWORD64 VirStartMftAddr = NULL;
					ParentUUID = Vdiindex->first;
					UCHAR m_VirtualCuNum = NULL;

					if (!GetVdiStartMftAddr(&ParentUUID, VdiFileInfo, VirtualPatition, &VirStartMftAddr, &m_VirtualCuNum))
					{

						CFuncs::WriteLogInfo(SLT_ERROR, "GetVBoxInternetInfo:GetVdiStartMftAddrʧ��!");
						return false;
					}

					ParentUUID = Vdiindex->first;
					vector<LONG64> v_VirtualStartMftAddr;
					vector<DWORD64> v_VirtualStartMftLen;

					if (!GetVdiAllMftAddr(&ParentUUID, VdiFileInfo, VirtualPatition, VirStartMftAddr, m_VirtualCuNum, v_VirtualStartMftAddr
						, v_VirtualStartMftLen))
					{

						CFuncs::WriteLogInfo(SLT_ERROR, "GetVBoxInternetInfo:GetVdiAllMftAddrʧ��!");
						return false;
					}

					HANDLE VirVdiDevice = CreateFile(Vdiindex->second.c_str(),//����ע�⣬���ֻ��һ�����̣�������Ҫ���ݸ������!!!!!
						GENERIC_READ,
						FILE_SHARE_READ,
						NULL,
						OPEN_EXISTING,
						0,
						NULL);
					if (VirVdiDevice == INVALID_HANDLE_VALUE) 
					{
						dwError=GetLastError();
						CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVBoxInternetInfo VdiDevice = CreateFile��ȡvdi�ļ����ʧ��!,\
														   ���󷵻���: dwError = %d"), dwError);
						return false;
					}

					//��ȡͷ����Ϣ
					DWORD VirVdiBatAddr = NULL;
					DWORD VirVdiDataAddr = NULL;
					DWORD VirBatSingleSize = NULL;
					DWORD VirBatBuffSize=NULL;//����Bat����ܴ�С
					DWORD VirParentVdiUUID = NULL;
					if(!GetVdiHeadInformation(VirVdiDevice, &VirVdiBatAddr, &VirVdiDataAddr, &VirBatSingleSize, &VirParentVdiUUID))
					{
						CloseHandle(VirVdiDevice);
						VirVdiDevice = NULL;
						CFuncs::WriteLogInfo(SLT_ERROR, "GetVBoxInternetInfo:GetVdiHeadInformationʧ��!");
						return false;
					}
					VirBatBuffSize = VirVdiDataAddr - VirVdiBatAddr;
					if (VirBatBuffSize > 4096 * 5000)
					{
						CloseHandle(VirVdiDevice);
						VirVdiDevice = NULL;
						CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVBoxInternetInfo :BatListSize����,ʧ��!"));
						return false;
					}
					UCHAR *VirVdiBatBuff = (UCHAR*) malloc(VirBatBuffSize + FILE_SECTOR_SIZE);
					if (NULL == VirVdiBatBuff)
					{
						CloseHandle(VirVdiDevice);
						VirVdiDevice = NULL;
						CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVBoxInternetInfo :malloc:VdiBatBuffʧ��!"));
						return false;
					}
					memset(VirVdiBatBuff, 0, (VirBatBuffSize + FILE_SECTOR_SIZE));
					if(!GetVdiBatListInformation(VirVdiDevice, VirVdiBatBuff, VirVdiBatAddr, VirBatBuffSize))
					{
						CloseHandle(VirVdiDevice);
						VirVdiDevice = NULL;
						free(VirVdiBatBuff);
						VirVdiBatBuff = NULL;
						CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVBoxInternetInfo :GetVdiBatListInformationʧ��!"));
						return false;
					}
					DWORD ReferNum = 0;//�ļ���¼����

					string VirtualFileName;
					string VirtualFilePath;
					DWORD VirtualParentMft = NULL;
					int PathFileFound = NULL;

					string browetype;
					DWORD64 VirtualBackAddr = NULL;

					UCHAR* RecodeCacheBuff = (UCHAR*) malloc(FILE_SECTOR_SIZE + SECTOR_SIZE);
					if (NULL == RecodeCacheBuff)
					{
						CloseHandle(VirVdiDevice);
						VirVdiDevice = NULL;
						free(VirVdiBatBuff);
						VirVdiBatBuff = NULL;
						CFuncs::WriteLogInfo(SLT_ERROR, "GetVBoxInternetInfo:malloc:RecodeCacheBuffʧ��!");
						return false;
					}
					memset(RecodeCacheBuff, 0, FILE_SECTOR_SIZE + SECTOR_SIZE);
					for (DWORD FileRecNum = 0; FileRecNum < v_VirtualStartMftAddr.size(); FileRecNum++)
					{
						ReferNum=0;

						while(VBoxFindRecordFile(VirVdiDevice, VirtualPatition, v_VirtualStartMftAddr[FileRecNum], m_VirtualCuNum, ReferNum, VirVdiBatBuff, VirBatSingleSize
							,VirBatBuffSize, browetype, VirtualFileName, &PathFileFound, &VirtualParentMft, RecodeCacheBuff, VirVdiDataAddr, PathAndName
							, VirtualFilePath, v_VirtualStartMftAddr, v_VirtualStartMftLen))
						{
							if (PathFileFound > 0)
							{
								if (PathFileFound == 1)
								{
									//�����Ŀ¼����Ҫ��ȡĿ¼�������ļ�
									map<DWORD, string> BrowerFileRefer;
									if (!VBoxGetCatalogFileRefer(VirVdiDevice, RecodeCacheBuff, BrowerFileRefer, m_VirtualCuNum, VirtualPatition, VirVdiBatBuff
										, VirBatSingleSize, VirBatBuffSize, VirVdiDataAddr))
									{
										CloseHandle(VirVdiDevice);
										VirVdiDevice = NULL;
										free(VirVdiBatBuff);
										VirVdiBatBuff = NULL;
										free(RecodeCacheBuff);
										RecodeCacheBuff = NULL;
										CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVBoxInternetInfo:GetCatalogFileReferʧ��!"));
										return false;
									}
									if (browetype == "FileRecord")
									{
										map<DWORD, string>::iterator Referiter;
										for (Referiter = BrowerFileRefer.begin(); Referiter != BrowerFileRefer.end(); Referiter ++)
										{
											string FileRecordLastVTM;
											string FileRecordPath;
											if (!VBoxExtractingFileRecordFile(VirVdiDevice, VirtualPatition, VirVdiBatBuff, VirBatSingleSize, VirBatBuffSize, v_VirtualStartMftAddr
												, v_VirtualStartMftLen, Referiter->first, m_VirtualCuNum, VirVdiDataAddr, Referiter->second, FileRecordLastVTM, FileRecordPath))
											{
												CloseHandle(VirVdiDevice);
												VirVdiDevice = NULL;
												free(VirVdiBatBuff);
												VirVdiBatBuff = NULL;
												free(RecodeCacheBuff);
												RecodeCacheBuff = NULL;

												CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVBoxInternetInfo:VmdkExtractingFileRecordFileʧ��!"));
												return false;
											}
											else
											{ 
												string filename_tem;

												if (Referiter->second.length() > 0)
												{
													if (!UnicodeToZifu((UCHAR*)&Referiter->second[0], filename_tem, Referiter->second.length()))
													{
														CloseHandle(VirVdiDevice);
														VirVdiDevice = NULL;
														free(VirVdiBatBuff);
														VirVdiBatBuff = NULL;
														free(RecodeCacheBuff);
														RecodeCacheBuff = NULL;
														CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVBoxInternetInfo:UnicodeToZifuʧ��!"));
														return false;
													}
											
											
												string recorvyName;
												recorvyName = string(virtualFileDir);
												recorvyName.append(filename_tem);
											    VirtualRecord(VirtualFilePath.c_str(), filename_tem.c_str()
												              , recorvyName.c_str(), 1, browetype.c_str(), FileRecordLastVTM.c_str()
															  , FileRecordPath.c_str(), "", "", "", "");
												
												}
											}
										}
									} 
									else 
									{
										map<DWORD, string>::iterator Referiter;
									for (Referiter = BrowerFileRefer.begin(); Referiter != BrowerFileRefer.end(); Referiter ++)
									{

										if (!VBoxExtractingTheCatalogFile(VirVdiDevice, VirtualPatition, VirVdiBatBuff, VirBatSingleSize, VirBatBuffSize, v_VirtualStartMftAddr
											, v_VirtualStartMftLen, Referiter->first, m_VirtualCuNum, VirVdiDataAddr, Referiter->second, virtualFileDir))
										{
											
											CloseHandle(VirVdiDevice);
											VirVdiDevice = NULL;
											free(VirVdiBatBuff);
											VirVdiBatBuff = NULL;
											free(RecodeCacheBuff);
											RecodeCacheBuff = NULL;
											CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVBoxInternetInfo:ExtractingTheCatalogFileʧ��!"));
											return false;
										}
										else
										{
											string filename_tem;

											if (Referiter->second.length() > 0)
											{
												if (!UnicodeToZifu((UCHAR*)&Referiter->second[0], filename_tem, Referiter->second.length()))
												{
													CloseHandle(VirVdiDevice);
													VirVdiDevice = NULL;
													free(VirVdiBatBuff);
													VirVdiBatBuff = NULL;
													free(RecodeCacheBuff);
													RecodeCacheBuff = NULL;
													CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVBoxInternetInfo:UnicodeToZifuʧ��!"));
													return false;
												}
											
											
											string recorvyName;
											recorvyName = string(virtualFileDir);
											recorvyName.append(filename_tem);
											/*VirtualRecord(HostFilePath[VBoxNumber].c_str(), VirtualFilePath.c_str(), filename_tem.c_str()
												, browetype.c_str(), recorvyName.c_str());*/
											if(!VirtualInternetRecord( VirtualFilePath.c_str(), filename_tem.c_str(),
												browetype.c_str(), recorvyName.c_str(), VirtualRecord))
											{
												CFuncs::WriteLogInfo(SLT_ERROR, "GetVBoxInternetInfo ����������¼�ļ�ʧ�ܣ�%s", recorvyName.c_str());
											}
											}
										}
									}
									}
									
								}
								else if(PathFileFound == 2)
								{
									vector<LONG64> VirtualFileH80Addr;
									vector<DWORD> VirtualFileH80AddrLen;
									string VirtualFileBuffH80;
									vector<DWORD> VirtualH20Refer;
									DWORD64 FileRealSize = NULL;
									if (!VBoxGetRecordFileAddr(VirVdiDevice, &FileRealSize, RecodeCacheBuff, VirtualH20Refer, m_VirtualCuNum, VirtualPatition
										, VirVdiBatBuff, VirBatSingleSize, VirBatBuffSize, VirVdiDataAddr, VirtualFileH80Addr, VirtualFileH80AddrLen, VirtualFileBuffH80))
									{
										CloseHandle(VirVdiDevice);
										VirVdiDevice = NULL;
										free(VirVdiBatBuff);
										VirVdiBatBuff = NULL;
										free(RecodeCacheBuff);
										RecodeCacheBuff = NULL;
										CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVBoxInternetInfo:GetRecordFileAddrʧ��!"));
										return false;
									}
									if (VirtualH20Refer.size() > 0)
									{
										UCHAR *H20CacheBuff = (UCHAR*) malloc(SECTOR_SIZE * m_VirtualCuNum + SECTOR_SIZE);
										if (NULL == H20CacheBuff)
										{
											CloseHandle(VirVdiDevice);
											VirVdiDevice = NULL;
											free(VirVdiBatBuff);
											VirVdiBatBuff = NULL;
											free(RecodeCacheBuff);
											RecodeCacheBuff = NULL;
											CFuncs::WriteLogInfo(SLT_ERROR, "GetVBoxInternetInfo:malloc:H20CacheBuffʧ��!");
											return false;
										}
										memset(H20CacheBuff, 0, SECTOR_SIZE * m_VirtualCuNum + SECTOR_SIZE);
										vector<DWORD>::iterator h20vec;
										for (h20vec = VirtualH20Refer.begin(); h20vec < VirtualH20Refer.end(); h20vec++)
										{
											memset(H20CacheBuff, 0, FILE_SECTOR_SIZE);
											DWORD64 VirMftLen = NULL;
											DWORD64 VirStartMftRfAddr = NULL;
											for (DWORD FRN = 0; FRN < v_VirtualStartMftLen.size(); FRN++)
											{
												if (((*h20vec) * 2) < (VirMftLen + v_VirtualStartMftLen[FRN] * m_VirtualCuNum))
												{
													VirStartMftRfAddr = v_VirtualStartMftAddr[FRN] * m_VirtualCuNum + ((*h20vec) * 2 - VirMftLen);
													break;
												} 
												else
												{
													VirMftLen += (v_VirtualStartMftLen[FRN] * m_VirtualCuNum);
												}
											}
											for (int i = 0; i < 2; i++)
											{
												VirtualBackAddr = NULL;
												if(!VdiOneAddrChange((VirtualPatition * SECTOR_SIZE + VirStartMftRfAddr * SECTOR_SIZE + SECTOR_SIZE * i)
													, VirVdiBatBuff, VirBatSingleSize, &VirtualBackAddr, VirBatBuffSize, VirVdiDataAddr))
												{
													CloseHandle(VirVdiDevice);
													VirVdiDevice = NULL;
													free(VirVdiBatBuff);
													VirVdiBatBuff = NULL;
													free(RecodeCacheBuff);
													RecodeCacheBuff = NULL;
													free(H20CacheBuff);
													H20CacheBuff = NULL;

													CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVBoxInternetInfo:VhdOneAddrChange: ת����ʼMft�ļ���¼��ַʧ��!"));
													return false;
												}
												Ret = ReadSQData(VirVdiDevice, &H20CacheBuff[i * SECTOR_SIZE], SECTOR_SIZE,  VirtualBackAddr,
													&BackBytesCount);		
												if(!Ret)
												{		
													CloseHandle(VirVdiDevice);
													VirVdiDevice = NULL;
													free(VirVdiBatBuff);
													VirVdiBatBuff = NULL;
													free(RecodeCacheBuff);
													RecodeCacheBuff = NULL;
													free(H20CacheBuff);
													H20CacheBuff = NULL;
													CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVBoxInternetInfo:ReadSQData: ��ȡ��ʼMft�ļ���¼��ַʧ��!"));
													return false;	
												}
											}

											if(!GetVirtualH20FileReferH80Addr(H20CacheBuff, VirtualFileH80Addr, VirtualFileH80AddrLen, VirtualFileBuffH80
												, &FileRealSize))//��Ϊ��������ⲿ��ȡ�����ݣ�����������0	
											{
												CloseHandle(VirVdiDevice);
												VirVdiDevice = NULL;
												free(VirVdiBatBuff);
												VirVdiBatBuff = NULL;
												free(RecodeCacheBuff);
												RecodeCacheBuff = NULL;
												free(H20CacheBuff);
												H20CacheBuff = NULL;
												CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVBoxInternetInfo:GetH20FileReferH80Addr: ʧ��!"));
												return false;
											}

										}
										free(H20CacheBuff);
										H20CacheBuff = NULL;
									}
									if (VirtualFileH80Addr.size() > 0)//����Ϊ��ַ����ȡ���ļ�
									{
										string VirtualPath;
										string StrTemName;
										if (VirtualFileName.length() > 0)
										{
											if(!UnicodeToZifu((UCHAR*)&VirtualFileName[0], StrTemName, VirtualFileName.length()))
											{
												CloseHandle(VirVdiDevice);
												VirVdiDevice = NULL;
												free(VirVdiBatBuff);
												VirVdiBatBuff = NULL;
												free(RecodeCacheBuff);
												RecodeCacheBuff = NULL;

												CFuncs::WriteLogInfo(SLT_ERROR, "GetVBoxInternetInfo:UnicodeToZifu : FileNameʧ��!");
												return false;
											}
										
										


										DWORD NameSize = VirtualFileName.length() + strlen(virtualFileDir);
										wchar_t * WirteName = new wchar_t[NameSize + 1];
										if (NULL == WirteName)
										{
											CloseHandle(VirVdiDevice);
											VirVdiDevice = NULL;
											free(VirVdiBatBuff);
											VirVdiBatBuff = NULL;
											free(RecodeCacheBuff);
											RecodeCacheBuff = NULL;

											CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVBoxInternetInfo:new:WirteName ���������ڴ�ʧ��!"));
											return false;
										}
										memset(WirteName, 0, (NameSize + 1) * 2);
										MultiByteToWideChar(CP_ACP, 0, virtualFileDir, strlen(virtualFileDir), WirteName, NameSize);

										for (DWORD NameIndex = 0; NameIndex < VirtualFileName.length(); NameIndex += 2)
										{
											RtlCopyMemory(&WirteName[strlen(virtualFileDir) + NameIndex / 2],(UCHAR*) &VirtualFileName[NameIndex],2);
										}
										char *WriteFileBuffer=(char*)malloc(2048*SECTOR_SIZE+1);
										if (NULL == WriteFileBuffer)
										{
											CloseHandle(VirVdiDevice);
											VirVdiDevice = NULL;
											free(VirVdiBatBuff);
											VirVdiBatBuff = NULL;
											free(RecodeCacheBuff);
											RecodeCacheBuff = NULL;
											delete WirteName;
											WirteName = NULL;
											CFuncs::WriteLogInfo(SLT_ERROR, "GetVBoxInternetInfo:malloc����WriteFileBuffer�ڴ�ʧ��!");
											return false;
										}
										memset(WriteFileBuffer, 0, 2048*SECTOR_SIZE);
										if(!VirtualVdiWriteLargeFile(VirVdiDevice, VirtualFileH80Addr, VirtualFileH80AddrLen, m_VirtualCuNum, VirVdiBatBuff, VirBatSingleSize
											, VirBatBuffSize, WriteFileBuffer, WirteName, VirVdiDataAddr, VirtualPatition, FileRealSize))
										{
											
											CFuncs::WriteLogInfo(SLT_ERROR, "GetVBoxInternetInfo:WriteLargeFile:��ȡH20H80��ַʧ��ʧ��");
										
										}
										else
										{
											string recorvyName;
											recorvyName = string(virtualFileDir);
											recorvyName.append(StrTemName);
											/*VirtualRecord(HostFilePath[VBoxNumber].c_str(), VirtualFilePath.c_str(), StrTemName.c_str()
												, browetype.c_str(), recorvyName.c_str());*/
											if(!VirtualInternetRecord( VirtualFilePath.c_str(), StrTemName.c_str(),
												browetype.c_str(), recorvyName.c_str(), VirtualRecord))
											{
												CFuncs::WriteLogInfo(SLT_ERROR, "GetVBoxInternetInfo ����������¼�ļ�ʧ�ܣ�%s", recorvyName.c_str());
											}
										}
										delete WirteName;
										WirteName = NULL;
										free(WriteFileBuffer);
										WriteFileBuffer = NULL;
										}


									}else if (VirtualFileBuffH80.length() > 0)//������h80���ȡС�ļ�
									{
										string VirtualPath;
										string StrTemName;
										if (VirtualFileName.length() > 0)
										{
											if(!UnicodeToZifu((UCHAR*)&VirtualFileName[0], StrTemName, VirtualFileName.length()))
											{
												CloseHandle(VirVdiDevice);
												VirVdiDevice = NULL;
												free(VirVdiBatBuff);
												VirVdiBatBuff = NULL;
												free(RecodeCacheBuff);
												RecodeCacheBuff = NULL;
												CFuncs::WriteLogInfo(SLT_ERROR, "GetVBoxInternetInfo:UnicodeToZifu : FileNameʧ��!");
												return false;
											}
										
										


										DWORD NameSize = VirtualFileName.length() + strlen(virtualFileDir);
										wchar_t * WirteName = new wchar_t[NameSize + 1];
										if (NULL == WirteName)
										{
											CloseHandle(VirVdiDevice);
											VirVdiDevice = NULL;
											free(VirVdiBatBuff);
											VirVdiBatBuff = NULL;
											free(RecodeCacheBuff);
											RecodeCacheBuff = NULL;
											CFuncs::WriteLogInfo(SLT_ERROR, _T("GetVBoxInternetInfo:new:WirteName ���������ڴ�ʧ��!"));
											return false;
										}
										memset(WirteName, 0, (NameSize + 1) * 2);
										MultiByteToWideChar(CP_ACP, 0, virtualFileDir, strlen(virtualFileDir), WirteName, NameSize);

										for (DWORD NameIndex = 0; NameIndex < VirtualFileName.length(); NameIndex += 2)
										{
											RtlCopyMemory(&WirteName[strlen(virtualFileDir) + NameIndex / 2],(UCHAR*) &VirtualFileName[NameIndex],2);
										}
										if(!VirtualWriteLitteFile(VirtualFileBuffH80, WirteName))
										{

											CFuncs::WriteLogInfo(SLT_ERROR, "GetVBoxInternetInfo:WriteLitteFile:ʧ��ʧ��");
									
										}
										else
										{
											string recorvyName;
											recorvyName = string(virtualFileDir);
											recorvyName.append(StrTemName);
											/*VirtualRecord(HostFilePath[VBoxNumber].c_str(), VirtualFilePath.c_str(), StrTemName.c_str()
												, browetype.c_str(), recorvyName.c_str());*/
											if(!VirtualInternetRecord( VirtualFilePath.c_str(), StrTemName.c_str(),
												browetype.c_str(), recorvyName.c_str(), VirtualRecord))
											{
												CFuncs::WriteLogInfo(SLT_ERROR, "GetVBoxInternetInfo ����������¼�ļ�ʧ�ܣ�%s", recorvyName.c_str());
											}
										}
										delete WirteName;
										WirteName=NULL;
										}

									}
								}
							}
							
							ReferNum++;
							if ((ReferNum * 2) > v_VirtualStartMftLen[FileRecNum] * m_VirtualCuNum)
							{
								break;
							}
						}

					}
					


					free(VirVdiBatBuff);
					VirVdiBatBuff = NULL;
					free(RecodeCacheBuff);
					RecodeCacheBuff = NULL;

					CloseHandle(VirVdiDevice);
					VirVdiDevice = NULL;
				}

				CFuncs::WriteLogInfo(SLT_INFORMATION, "GetVBoxInternetInfo:����������ntfs����!");
			}
		}
		


	

	return true;
}
bool GetVirtualMachineInfo::VirtualInternetCheeckFuc(const char* recordFilePath, const char* virtualFileDir,const char* CheckvirtualFileDir, PFCallbackVirtualInternetRecord VirtualRecord)
{
	DWORD dwError=NULL;//��ȡlasterror��Ϣ
	
	map<string,map<string, string>> PathAndName;
	if (!AnalysisInterPath(PathAndName, recordFilePath))
	{
		CFuncs::WriteLogInfo(SLT_ERROR, "VirtualInternetCheeckFuc::AnalysisInterPath��ʼ��ʧ��!");
		return false;
	}

	vector<string> VmdkParentMftBuff;
	string VhdParentMftBuff;
	string VboxParentMftBuff;

	if (!GetcheckVirtualFilePath(CheckvirtualFileDir, VmdkParentMftBuff, VhdParentMftBuff, VboxParentMftBuff))
	{
		CFuncs::WriteLogInfo(SLT_ERROR, "VirtualFileCheckFuc::GetcheckVirtualFilePath!");
		return false;
	}
	if (VmdkParentMftBuff.size() > 0)
	{
		if (!GetVMwareInternetInfo(PathAndName, VmdkParentMftBuff,  virtualFileDir, VirtualRecord))
		{
			CFuncs::WriteLogInfo(SLT_ERROR, "VirtualInternetCheeckFuc GetVMwareInternetInfoʧ��!");
			return false;
		}
	}

	if (VboxParentMftBuff.length() > 0)
	{
		if (!GetVBoxInternetInfo(PathAndName, VboxParentMftBuff, virtualFileDir, VirtualRecord))
		{
			CFuncs::WriteLogInfo(SLT_ERROR, "VirtualInternetCheeckFuc GetVBoxInternetInfoʧ��!");
			return false;
		}
	}
		

		

	
	return true;
}
bool GetVirtualMachineInfo::VirtualInternetRecord( const char* virtualMachineRecordFilePath, const char* recordFilename,
	const char* browserType, const char* recoveryPath, PFCallbackVirtualInternetRecord VirtualRecord)
{
	if(!CFuncs::FileExist(recoveryPath))
	{
		CFuncs::WriteLogInfo(SLT_ERROR, "VirtualInternetRecord ��VMware��ȡ��������¼�ļ������ڣ� %s", recoveryPath);
		return false;
	}
	
	if (0 == strcmp(browserType, "regdit"))
	{
		VirtualRecord( virtualMachineRecordFilePath, recordFilename, recoveryPath, 0, "", "", "", "", "", "", "");
	}
	if(0 == strcmp(browserType, "firefox"))
	{
		m_BrowserRecord->ParseFirefoxRecordFile( virtualMachineRecordFilePath, recordFilename, recoveryPath, string(""), VirtualRecord);
	}
	if(0 == strcmp(browserType, "maxthon"))
	{
		m_BrowserRecord->ParseMaxthonRecordFile( virtualMachineRecordFilePath, recordFilename, recoveryPath, string(""), VirtualRecord);
	}
	if(0 == strcmp(browserType, "chrome"))
	{
		m_BrowserRecord->ChromeRecordFile( virtualMachineRecordFilePath, recordFilename, recoveryPath, string(""), VirtualRecord);
	}
	if(0 == strcmp(browserType, "opera"))
	{
		m_BrowserRecord->OperaRecordFile( virtualMachineRecordFilePath, recordFilename, recoveryPath, string(""), VirtualRecord);
	}
	if(0 == strcmp(browserType, "sogou"))
	{
		m_BrowserRecord->SogouRecordFile( virtualMachineRecordFilePath, recordFilename, recoveryPath, string(""), VirtualRecord);
	}
	if(0 == strcmp(browserType, "QQ"))
	{
		m_BrowserRecord->QQRecordFile( virtualMachineRecordFilePath, recordFilename, recoveryPath, string(""), VirtualRecord);
	}
	if(0 == strcmp(browserType, "360"))
	{
		m_BrowserRecord->QihuRecordFile(virtualMachineRecordFilePath, recordFilename, recoveryPath, string(""), VirtualRecord);
	}
	if(0 == strcmp(browserType, "360Chrome"))
	{
		m_BrowserRecord->QihuChromeRecordFile( virtualMachineRecordFilePath, recordFilename, recoveryPath, string(""), VirtualRecord);
	}
	if(0 == strcmp(browserType, "2345"))
	{
		m_BrowserRecord->Browser2345RecordFile( virtualMachineRecordFilePath, recordFilename, recoveryPath, string(""), VirtualRecord);
	}
	if(0 == strcmp(browserType, "UC"))
	{
		m_BrowserRecord->UCRecordFile( virtualMachineRecordFilePath, recordFilename, recoveryPath, string(""), VirtualRecord);
	}
	if (0 == strcmp(browserType, "liebao"))
	{
		m_BrowserRecord->LiebaoRecordFile( virtualMachineRecordFilePath, recordFilename, recoveryPath, string(""), VirtualRecord);
	}
	DeleteFile(recoveryPath);
	return true;
}