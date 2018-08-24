#pragma once

#include <tchar.h>
#include <map>  
#include <vector>
#include <string>
#include "..\common\define.h"

#define WMI_NAMESPACE_CIMV2               ("root\\CIMV2")
#define WMI_SQL_DISKDRIVE                 ("select * from Win32_DiskDrive")
#define WMI_SQL_DISKPARTITION             ("select * from Win32_DiskPartition")
#define WMI_SQL_LOGICALDISK               ("select * from Win32_LogicalDisk")
#define WMI_SQL_LOGICALDISKTOPARTITION    ("select * from Win32_LogicalDiskToPartition")

#define WMI_SQL_BASEBOARD                 ("select * from Win32_BaseBoard")
#define WMI_SQL_SHARE                     ("select * from Win32_Share")
#define WMI_SQL_GROUP                     ("select * from Win32_Group")

#define WMI_NAMESPACE_CIMV2_SECURITY      ("root\\CIMV2\\Security\\MicrosoftVolumeEncryption")
#define WMI_SQL_ENCRYPTABLEVOLUME         ("select * from Win32_EncryptableVolume")

#define WMI_SQL_PHYSICALMEMORY			  ("select * from  Win32_PhysicalMemory")
#define WMI_SQL_NETWORKADAPTER			  ("select * from  Win32_NetworkAdapter")
#define WMI_SQL_GROUPUSER				  ("select * from  Win32_GroupUser")

#define WMI_NAMESPACE_WMI				  ("ROOT\\WMI")
#define	WMI_SQL_MSSTORAGEDRIVER_ATAPISMARTDATA	  ("select * from MSStorageDriver_ATAPISmartData")


#define LEAST_STRING_LENGTH     (8)
#define MIN_STRING_LENGTH       (64)
#define DEFAULT_STRING_LENGTH   (128)
#define MAX_STRING_LENGTH       (256)

#pragma pack(push, 1)

typedef struct _WmiDiskDriveTime_
{
	uint32 BootTimes;                          //��������
	uint32 UsingTime;						   //ʹ��ʱ�䣨Сʱ��
}WmiDiskDriveTime;

//Ӳ����Ϣ DiskDrive
typedef struct _WmiDiskDrive_
{
	char   Caption[DEFAULT_STRING_LENGTH];     //�ͺ�
	uint32 Index;                              //�����Ŵ�0��ʼ
	char   CheckPath[DEFAULT_STRING_LENGTH];   //������ CheckPath ƥ����̷���
	char   SerialNumber[MAX_STRING_LENGTH];    //���
	uint32 Signature;                          //ʶ���ʶ
	uint64 Size;                               //��С
	char   Status[MIN_STRING_LENGTH];          //״̬
	char   SystemName[DEFAULT_STRING_LENGTH];  //����ϵͳ����
	char   Name[MIN_STRING_LENGTH];            //����
	uint32 Partitions;                         //��������
	char   Manufacturer[MIN_STRING_LENGTH];    //����
	uint32 BootTimes;                          //��������
	uint32 UsingTime;						   //ʹ��ʱ�䣨Сʱ��

	//����Ϊ�Զ����ֶ�,�����ڸ����ͱ�׼�ֶ�.
	uint32 IsSlave;                            //�Ƿ�Ϊ����,0�����̣�1������
	uint32 IsVirtual;                          //�Ƿ�Ϊ�������,0����1����
	uint32 UseStatus;                          //ʹ��״̬��0�����ã�1������
	char   Description[MIN_STRING_LENGTH];     //����
}WmiDiskDrive;

//Ӳ�̷�����Ϣ DiskPartition
typedef struct _WmiDiskPartition_
{
	char   DeviceID[MIN_STRING_LENGTH];    //ID
	uint32 DiskIndex;                      //Ӳ�����
	uint32 Index;                          //�������
	uint64 Size;                           //�ܴ�С
	char   SystemName[MIN_STRING_LENGTH];  //����ϵͳ����
	char   Type[MIN_STRING_LENGTH];        //����

	//����Ϊ�Զ����ֶ�,�����ڸ����ͱ�׼�ֶ�.
	uint32 IsHide;                                     //�Ƿ�Ϊ����,0��������1����

	uint32 IsVirtual;                                  //���� �Ƿ�Ϊ�������,0����1����

	uint32 EncryptType;                                //���� ���ͣ�������ͣ�DiskEncryptType

	uint64 FreeSpace;                                  //��� ʣ��ռ�
	char   LogicalName[LEAST_STRING_LENGTH];           //��� �̷���C:
	char   VolumeSerialNumber[DEFAULT_STRING_LENGTH];  //��� ���к�
	char   Description[MIN_STRING_LENGTH];             //����
}WmiDiskPartition;

////�߼�������Ϣ LogicalDisk
//typedef struct _WmiLogicalDisk_
//{
//	char   DeviceID[LEAST_STRING_LENGTH];              //���� �����(�̷�)
//	uint64 Size;                                       //��С
//	uint64 FreeSpace;                                  //ʣ��ռ�
//	char   SystemName[MIN_STRING_LENGTH];              //����ϵͳ����
//	uint32 DriveType;                                  //��������
//	uint32 MediaType;                                  //ý������
//	char   FileSystem[LEAST_STRING_LENGTH];            //�ļ�ϵͳ
//	char   VolumeSerialNumber[DEFAULT_STRING_LENGTH];  //�������к�
//}WmiLogicalDisk;

//�߼�������Ϣ LogicalDisk
typedef struct _WmiLogicalDisk_
{
	char   DeviceID[LEAST_STRING_LENGTH];              //���� �����(�̷�)
	uint64 Size;                                       //��С
	uint64 FreeSpace;                                  //ʣ��ռ�
	char   SystemName[MIN_STRING_LENGTH];              //����ϵͳ����
	uint32 DriveType;                                  //��������
	uint32 MediaType;                                  //ý������
	char   FileSystem[LEAST_STRING_LENGTH];            //�ļ�ϵͳ
	char   VolumeSerialNumber[DEFAULT_STRING_LENGTH];  //�������к�

    uint32 IsHide;                                     //�Ƿ�Ϊ����,0��������1����

	uint32 EncryptType;                                //���� ���ͣ�������ͣ�DiskEncryptType
}WmiLogicalDisk;

// LogicalDiskToPartition
typedef struct _WmiLogicalDiskToPartition_
{
	char Antecedent[MAX_STRING_LENGTH];     //���̷���������Ϣ
	char Dependent[MAX_STRING_LENGTH];      //�߼����̷���
}WmiLogicalDiskToPartition;	

//�������� EncryptableVolume
typedef struct _WmiEncryptableVolum_
{
	char   DeviceID[DEFAULT_STRING_LENGTH];     //�豸ID
	char   DriveLetter[LEAST_STRING_LENGTH];    //�������磺C
	uint32 ProtectionStatus;                    //����״̬
}WmiEncryptableVolum;

//������Ϣ 
typedef struct _WmiShareInfo_
{
	char   Caption[MAX_PATH];      //����
	char   Description[MAX_PATH];  //����
	char   Name[MAX_PATH];         //����
	char   Path[MAX_PATH];         //·��
	char   Status[MAX_PATH];       //״̬
}WmiShareInfo;

//������Ϣ 
typedef struct _WmiGroupInfo_
{
	char   Caption[MAX_PATH];      //����
	char   Description[MAX_PATH];  //����
	char   Name[MAX_PATH];         //����
	char   Status[MAX_PATH];       //״̬
}WmiGroupInfo;

#pragma pack(pop)

typedef std::vector<WmiDiskDriveTime> WmiDiskDriveTimeVector;
typedef WmiDiskDriveTimeVector::iterator WmiDiskDriveTimeVectorIterator;
//��ؽṹ����
typedef std::vector<WmiDiskDrive> WmiDiskDriveVector;
typedef WmiDiskDriveVector::iterator WmiDiskDriveVectorIterator;

typedef std::vector< WmiDiskPartition> WmiDiskPartitionVector;
typedef WmiDiskPartitionVector::iterator WmiDiskPartitionVectorIterator;

typedef std::vector<WmiLogicalDisk> WmiLogicalDiskVector;
typedef WmiLogicalDiskVector::iterator WmiLogicalDiskVectorIterator;

typedef std::vector<WmiLogicalDiskToPartition> WmiLogicalDiskToPartitionVector;
typedef WmiLogicalDiskToPartitionVector::iterator WmiLogicalDiskToPartitionVectorIterator;

typedef std::vector<WmiEncryptableVolum> WmiEncryptableVolumVector;
typedef WmiEncryptableVolumVector::iterator WmiEncryptableVolumVectorIterator;

typedef std::vector<WmiGroupInfo> WmiGroupInfoVector;
typedef WmiGroupInfoVector::iterator WmiGroupInfoVectorIterator;

typedef std::vector<std::string> WmiVector;
typedef WmiVector::iterator WmiVectorIterator;

typedef std::vector<std::string> WmiPropertyVector;
typedef WmiPropertyVector::iterator WmiPropertyVectorIterator;

typedef std::vector<WmiShareInfo> WmiShareInfoVector;
typedef WmiShareInfoVector::iterator WmiShareInfoVectorIterator;