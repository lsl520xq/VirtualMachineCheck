#pragma once

#include <windows.h>

#define DEFAULT_DATA_HEADER_VERSION      (1) 
#define DEFAULT_DATA_HEADER_MAGICNUMBER	 (0xCD91E1F0)

#define MAX_LOG_LEN               (1024 * 3)

#define MAX_IP_LEN                (32)
#define MAX_MAC_LEN               (6)

#define MAX_PROTO_DEVICE_LEN            (20 + 1)
#define MAX_PROTO_GUID_LEN              (64 + 1)
#define MAX_PROTO_PATH_LEN              (512 + 1)
#define MAX_PROTO_USER_LEN              (32 + 1)
#define MAX_PROTO_NAME_LEN              (64 + 1)
#define MAX_PROTO_DESCRIPTION_LEN       (256 + 1)
#define MAX_PROTO_MACHINE_CODE_LEN      (64)
#define MAX_PROTO_DATETIME_LEN          (20 + 1)
#define MAX_PROTO_HOSTNAME_LEN          (64 + 1)
#define MAX_PROTO_VERSION_LEN           (128 + 1)
#define MAX_PROTO_TEXT_LEN              (64 + 1)
#define MAX_PROTO_MEMO_LEN              (128 + 1)
#define MAX_PROTO_CONTENT_LEN           (1024 + 1)
#define MAX_PROTO_TYPE_LEN              (16 + 1)
#define MAX_PROTO_STATUS_LEN            (12 + 1)
#define MAX_PROTO_URL_LEN               (1024 + 1)
#define MAX_PROTO_SUMMARY_LEN           (2048 + 1)
#define MAX_PROTO_TITLE_LEN             (128 + 1)

//������Դ
typedef enum _CommonDataSource_
{
	CDS_MIN = 0,

	CDS_UNKOWN = CDS_MIN,    //δ֪
	CDS_PC,                  //����
	CDS_MOBILE,              //�ƶ��豸
	CDS_DISK,                //����/Ӳ��

	CDS_MAX
}CommonDataSource;

//�����Դ
typedef enum _CommonCheckSource_
{
	CCS_MIN = 0,

	CCS_UNKOWN = CCS_MIN,    //δ֪
	CCS_GENERAL,             //����
	CCS_DEEP,                //���

	CCS_MAX
}CommonCheckSource;

//�ļ���Դ
typedef enum _CommonFileSource_
{
	CFS_MIN = 0,

	CFS_UNKOWN = CFS_MIN,    //δ֪
	CFS_GENERAL,             //����
	CFS_RECOVERY,            //�ָ�

	CFS_MAX
}CommonFileSource;

//�������
typedef enum _CommonCheckType_
{
	CCT_MIN = 0,

	CCT_UNKOWN = CCT_MIN,    //δ֪
	CCT_PC,                  //����
	CCT_DISK,                //����
	CCT_MOBILE,              //�ֻ�
	CCT_VM,                  //�����
	CCT_CLONE,               //��¡�ļ�

	CCT_MAX
}CommonCheckType;

//��������(1:����� 2��������� 3������ 4���ʼ�  5:��ʱͨѶ)
typedef enum _CommonInternetType_
{
	CIT_MIN = 0,

	CIT_UNKOWN = CIT_MIN,    //δ֪
	CIT_BROWSER,             //�����
	CIT_DOWNLOADER,          //�������
	CIT_CLOUD,               //����
	CIT_EMAIL,               //�ʼ�
	CIT_IM,                  //��ʱͨѶ

	CIT_MAX
}CommonInternetType;

//���̼�������
typedef enum _CommonEncryptType_
{
	CET_MIN = 0,

	CET_NORMAL = CET_MIN,  //δ����
	CET_BITLOCKER,         //bitlocker
	CET_TURECRYPT,         //TrueCrypt
	CET_OTHER,             //����

	DET_MAX
}CommonEncryptType;

//����ϵͳ����
typedef enum _CommonOSType_
{
	COS_MIN = 0,

	COS_UNKNOW = COS_MIN,  //δ֪
	COS_ENTITY,            //ʵ���
	COS_VIRTUAL,           //�����
	COS_CLONE,             //��¡��

	COS_MAX
}CommonOSType;

//����Ӳ���жϲ���ϵͳ�汾
typedef enum _DiskOSVersion_
{
	DOV_MIN = 0,

	DOV_UNKNOW = DOV_MIN,
	DOV_WIN_XP,
	DOV_WIN_SERVER_2003,
	DOV_WIN_7,
	DOV_WIN_8,
	DOV_WIN_10,
	DOV_WIN_SERVER_2008,
	DOV_WIN_SERVER_2012,
	DOV_MAX
}DiskOSVersion;

//��������
typedef enum _CommonDataType_
{
	CDT_MIN = 0,
	
	CDT_HOST_INFO,                          //������Ϣ
	CDT_ACCOUNT_INFO,                       //�˺���Ϣ
	CDT_NETWORK_INFO,                       //������Ϣ
	CDT_DISK_INFO,                          //������Ϣ
	CDT_DISKPARTION_INFO,                   //���̷�����Ϣ
	CDT_SHARE_INFO,                         //�ļ�������Ϣ
	CDT_SYSTEM_PATCH,                       //ϵͳ����
	CDT_OS_INSTALLATION,                    //����ϵͳ��װ��Ϣ
	CDT_ANTIVIRUS_INFO,                     //�������������Ϣ
	CDT_BM_SYSTEM,                          //����ҵ��ϵͳ
	CDT_WIREKESS_DEVICE,                    //�����豸���
	CDT_VIRTUAL_MACHINE,                    //��������
	CDT_ACCOUNT_STRATEGY,                   //�˻���ȫ����
	CDT_ACCOUNT_SETTINGS,                   //�˻���ȫ����
	CDT_AUDIT_STRATEGY,                     //��ȫ��Ʋ���
	CDT_SERVICES_INFO,                      //������Ϣ
	CDT_PORTS_INFO,                         //�˿���Ϣ
	CDT_ACCOUNT_PERMISSION,                 //�˻�Ȩ������
	CDT_USB_RECORD,                         //USB��¼���
	CDT_INTERNET_CHECK,                     //������¼���
	CDT_EMAIL_RECORD,						//�ʼ���¼���
	CDT_FILE_CHECK,                         //�ļ����
	CDT_EMAIL_CHECK,                        //�ʼ����
	
	CDT_MAX
}CommonDataType;

#pragma pack(push, 1)

//���ݹ���ͷ��Ϣ
typedef struct _CommonHeader_
{
	unsigned int         HeaderSize;                  //����ͷ(CommonHeader)����
	unsigned int         Version;                     //�汾
	unsigned char        TaskID[MAX_PROTO_GUID_LEN];  //����ID
	unsigned int         MagicNumber;                 //MAGICNUMBER
	unsigned int         DataSource;                  //������Դ  CommonDataSource
	unsigned int         DataType;	                  //��������  CommonDataType
	unsigned int         DataLen;                     //�������ܳ���,��ͷ��Ϣ
}CommonHeader;


//������Ϣ
typedef struct _DHHostInfo
{
	CommonHeader   Header;

	unsigned char Timestamp[MAX_PROTO_DATETIME_LEN];         //���ʱ��
	unsigned int  CheckType;                                 //�������(1��PC 2��Ӳ��  3���ֻ� 4�������  5����¡�ļ�)
	unsigned char CheckPath[MAX_PROTO_PATH_LEN];             //���·������Ӧ�������ƥ���·���������������ݣ�
	unsigned char OSVersion[MAX_PROTO_VERSION_LEN];          //����ϵͳ�汾
	unsigned char InstallTime[MAX_PROTO_DATETIME_LEN];       //��װʱ��
	unsigned char HostName[MAX_PROTO_HOSTNAME_LEN];          //������
	unsigned int  OSType;                                    //����   CommonOSType
	unsigned char GroupName[MAX_PROTO_HOSTNAME_LEN];         //������
	unsigned char Description[MAX_PROTO_DESCRIPTION_LEN];	 //����
} DHHostInfo;

//�˻���Ϣ
typedef struct _DHAccountInfo
{
	CommonHeader   Header;

	unsigned char Timestamp[MAX_PROTO_DATETIME_LEN];         //���ʱ��
	unsigned int  CheckType;                                 //�������
	unsigned char CheckPath[MAX_PROTO_PATH_LEN];             //���·��
	unsigned char CurrentUser[MAX_PROTO_USER_LEN];           //��ǰ�˻�
	unsigned char UserDescription[MAX_PROTO_DESCRIPTION_LEN];//�˻�������Ϣ
	unsigned char UserPermissions[MAX_PROTO_TYPE_LEN];		 //�û�Ȩ�� ����or����
	unsigned char UserType[MAX_PROTO_NAME_LEN];				 //�û�����
} DHAccountInfo;

//������Ϣ
typedef struct _DHNetworkInfo
{
	CommonHeader   Header;

	unsigned char Timestamp[MAX_PROTO_DATETIME_LEN];         //���ʱ��
	unsigned int  CheckType;                                 //�������
	unsigned char CheckPath[MAX_PROTO_PATH_LEN];             //���·��
	unsigned char AdapterName[MAX_PROTO_NAME_LEN];           //����������
	unsigned char IP[MAX_IP_LEN];                            //IP��ַ
	unsigned char MAC[MAX_MAC_LEN];                          //MAC��ַ
} DHNetworkInfo;

//Ӳ����Ϣ
typedef struct _DHDiskInfo
{
	CommonHeader   Header;

	unsigned char Timestamp[MAX_PROTO_DATETIME_LEN];         //���ʱ��
	unsigned int  CheckType;                                 //�������
	unsigned char CheckPath[MAX_PROTO_PATH_LEN];             //���·��
	unsigned char GUID[MAX_PROTO_GUID_LEN];                  //GUID �����������Ϣ����
	unsigned char Company[MAX_PROTO_MEMO_LEN];               //����
	unsigned char Product[MAX_PROTO_MEMO_LEN];               //�ͺ�
	unsigned char SN[MAX_PROTO_MEMO_LEN];                    //���к�
	unsigned __int64  TotalSpace;                            //������
	unsigned short Partition;                                //��������
	unsigned int  BootTimes;                                 //��������
	unsigned char Using[MAX_PROTO_DATETIME_LEN];             //����ʱ��
	unsigned char Status[MAX_PROTO_STATUS_LEN];              //״̬
	unsigned short isSlave;                                   //������ 0: ���� 1������
	unsigned short isVirtual;                                 //������� 0���� 1����
} DHDiskInfo;

//Ӳ�̷�����Ϣ
typedef struct _DHDiskPartitionInfo
{
	CommonHeader   Header;

	unsigned char Timestamp[MAX_PROTO_DATETIME_LEN];         //���ʱ��
	unsigned int CheckType;                                 //�������
	unsigned char CheckPath[MAX_PROTO_PATH_LEN];             //���·��
	unsigned char GUID[MAX_PROTO_GUID_LEN];                  //GUID ��������̹���
	unsigned int  IsHide;                                    //�Ƿ�Ϊ����,0��������1����
	unsigned int  EncryptType;                               //���� ���ͣ�������ͣ�CommonEncryptType
	unsigned __int64  TotalSpace;                            //������
	unsigned __int64  FreeSpace;                             //��� ʣ��ռ�
	unsigned char   LogicalName[MAX_PROTO_DEVICE_LEN];       //��� �̷���C:
	unsigned char   VolumeSerialNumber[MAX_PROTO_NAME_LEN];  //��� ���к�
} DHDiskPartitionInfo;

//������Ϣ
typedef struct _DHShareInfo
{
	CommonHeader   Header;

	unsigned char Timestamp[MAX_PROTO_DATETIME_LEN];         //���ʱ��
	unsigned int  CheckType;                                 //�������
	unsigned char CheckPath[MAX_PROTO_PATH_LEN];             //���·��
	unsigned char Name[MAX_PROTO_NAME_LEN];                  //��������
	unsigned char Path[MAX_PROTO_PATH_LEN];                  //�ļ�·��
	unsigned char Description[MAX_PROTO_DESCRIPTION_LEN];    //����
	unsigned char Authority[MAX_PROTO_CONTENT_LEN];          //Ȩ��
	unsigned char Deduction[MAX_PROTO_CONTENT_LEN];          //Υ����ʾ��Ϣ
} DHShareInfo;

//ϵͳ����
typedef struct _DHSystemPatch
{
	CommonHeader   Header;

	unsigned char Timestamp[MAX_PROTO_DATETIME_LEN];         //���ʱ��
	unsigned int  CheckType;                                 //�������
	unsigned char CheckPath[MAX_PROTO_PATH_LEN];             //���·��
	unsigned char ID[MAX_PROTO_USER_LEN];                    //����ID
	unsigned char Description[MAX_PROTO_DESCRIPTION_LEN];    //��������
	unsigned char VulDescription[MAX_PROTO_DESCRIPTION_LEN]; //©������
	unsigned char Date[MAX_PROTO_DATETIME_LEN];              //����ʱ��
	unsigned char Deduction[MAX_PROTO_CONTENT_LEN];          //Υ����ʾ��Ϣ
} DHSystemPatch;

//����ϵͳ
typedef struct _DHOSInstallation
{
	CommonHeader   Header;

	unsigned char Timestamp[MAX_PROTO_DATETIME_LEN];         //���ʱ��
	unsigned int  CheckType;                                 //�������
	unsigned char CheckPath[MAX_PROTO_PATH_LEN];             //���·��
	unsigned char Name[MAX_PROTO_NAME_LEN];                  //����ϵͳ����
	unsigned char Version[MAX_PROTO_VERSION_LEN];            //����ϵͳ�汾
	unsigned char InstallPath[MAX_PROTO_PATH_LEN];           //��װ·��
	unsigned char InstallDate[MAX_PROTO_DATETIME_LEN];       //��װʱ��
	unsigned char Deduction[MAX_PROTO_CONTENT_LEN];          //Υ����ʾ��Ϣ
} DHOSInstallation;

//��ȫ�������
typedef struct _DHAntivirus
{
	CommonHeader   Header;

	unsigned char Timestamp[MAX_PROTO_DATETIME_LEN];         //���ʱ��
	unsigned int  CheckType;                                 //�������
	unsigned char CheckPath[MAX_PROTO_PATH_LEN];             //���·��
	unsigned char Name[MAX_PROTO_NAME_LEN];                  //����
	unsigned char Version[MAX_PROTO_VERSION_LEN];            //�汾
	unsigned char UpdateDate[MAX_PROTO_DATETIME_LEN];        //����ʱ��
	unsigned char Deduction[MAX_PROTO_CONTENT_LEN];          //Υ����ʾ��Ϣ
} DHAntivirus;

//����ҵ��ϵͳ
typedef struct _DHBMSystem
{
	CommonHeader   Header;

	unsigned char Timestamp[MAX_PROTO_DATETIME_LEN];         //���ʱ��
	unsigned int  CheckType;                                 //�������
	unsigned char CheckPath[MAX_PROTO_PATH_LEN];             //���·��
	unsigned char Name[MAX_PROTO_NAME_LEN];                  //����
}DHBMSysteme;

//����ͨ���豸
typedef struct _DHWirelessDevice
{
	CommonHeader   Header;

	unsigned char Timestamp[MAX_PROTO_DATETIME_LEN];         //���ʱ��
	unsigned int CheckType;                                 //�������
	unsigned char CheckPath[MAX_PROTO_PATH_LEN];             //���·��
	unsigned char Name[MAX_PROTO_NAME_LEN];                  //����
	unsigned char Company[MAX_PROTO_MEMO_LEN];               //����
	unsigned char Description[MAX_PROTO_DESCRIPTION_LEN];    //����
	unsigned char Type;                                      //����
	unsigned char Deduction[MAX_PROTO_NAME_LEN];             //Υ����ʾ��Ϣ
} DHWirelessDevice;

//�������װ
typedef struct _DHVirtualMachine
{
	CommonHeader   Header;

	unsigned char Timestamp[MAX_PROTO_DATETIME_LEN];         //���ʱ��
	unsigned int  CheckType;                                 //�������
	unsigned char CheckPath[MAX_PROTO_PATH_LEN];             //���·��
	unsigned char Name[MAX_PROTO_NAME_LEN];                  //����
	unsigned char Version[MAX_PROTO_VERSION_LEN];            //����汾
	unsigned char Company[MAX_PROTO_MEMO_LEN];               //�����˾
	unsigned char InstallPath[MAX_PROTO_PATH_LEN];           //��װ·��
	unsigned char InstallDate[MAX_PROTO_DATETIME_LEN];       //��װʱ��
	unsigned char Deduction[MAX_PROTO_NAME_LEN];             //Υ����ʾ��Ϣ
} DHVirtualMachine;

//�˻���ȫ����
typedef struct _DHAccountStrategy
{
	CommonHeader   Header;

	unsigned char Timestamp[MAX_PROTO_DATETIME_LEN];         //���ʱ��
	unsigned int  CheckType;                                 //�������
	unsigned char CheckPath[MAX_PROTO_PATH_LEN];             //���·��
	unsigned char PasswordComplex[MAX_PROTO_DATETIME_LEN];   //���������ϸ�����Ҫ��
	unsigned char MinPasswordLength[MAX_PROTO_USER_LEN];     //���볤����Сֵ
	unsigned short MaxValidityPeriod;                        //�����ʹ������
	unsigned short MinValidityPeriod;                        //�������ʹ������
	unsigned char PasswordHistory[256];                      //ǿ��������ʷ
	unsigned char LockTime[MAX_PROTO_DATETIME_LEN];          //�˻�����ʱ��
	unsigned int  RestCounter;                               //��λ�˻�����������
	unsigned int  LockThreshold;                             //�˻�������ֵ
	unsigned char GuestStatus[MAX_PROTO_NAME_LEN];           //Guest�˻�״̬
	unsigned char AutoLogon[MAX_PROTO_NAME_LEN];             //�Զ���½����
	unsigned char Deduction[MAX_PROTO_NAME_LEN];             //Υ����ʾ��Ϣ
} DHAccountStrategy;

//�˻���ȫ����
typedef struct _DHAccountSetting
{
	CommonHeader   Header;

	unsigned char Timestamp[MAX_PROTO_DATETIME_LEN];         //���ʱ��
	unsigned int  CheckType;                                 //�������
	unsigned char CheckPath[MAX_PROTO_PATH_LEN];             //���·��
	unsigned char UserName[MAX_PROTO_USER_LEN];              //�˻���
	unsigned char Password[MAX_PROTO_USER_LEN];              //����
	unsigned char PasswordPrompt[MAX_PROTO_PATH_LEN];        //�����������
	unsigned short Length;                                   //���볤��
	unsigned char Complex[MAX_PROTO_TEXT_LEN];               //���븴�Ӷ�
	unsigned char Deduction[MAX_PROTO_NAME_LEN];             //Υ����ʾ��Ϣ
} DHAccountSetting;

//��ȫ��Ʋ���
typedef struct _DHAuditStrategy
{
	CommonHeader   Header;

	unsigned char Timestamp[MAX_PROTO_DATETIME_LEN];         //���ʱ��
	unsigned int  CheckType;                                 //�������
	unsigned char CheckPath[MAX_PROTO_PATH_LEN];             //���·��
	unsigned char Modify[MAX_PROTO_TEXT_LEN];                //�����Ը���
	unsigned char Login[MAX_PROTO_TEXT_LEN];                 //��˵�¼�¼�
	unsigned char Object[MAX_PROTO_TEXT_LEN];                //��˶������
	unsigned char Process[MAX_PROTO_TEXT_LEN];               //��˹��̸���
	unsigned char AD[MAX_PROTO_TEXT_LEN];                    //���Ŀ¼�������
	unsigned char SpecialUse[MAX_PROTO_TEXT_LEN];            //���Ȩ��ʹ��
	unsigned char Event[MAX_PROTO_TEXT_LEN];                 //���ϵͳ�¼�
	unsigned char LoginOther[MAX_PROTO_TEXT_LEN];            //����˻���¼�¼�
	unsigned char Account[MAX_PROTO_USER_LEN];               //����˻�����
	unsigned char Deduction[MAX_PROTO_TEXT_LEN];             //Υ����ʾ��Ϣ
} DHAuditStrategy;

//�������
typedef struct _DHServices
{
	CommonHeader   Header;

	unsigned char Timestamp[MAX_PROTO_DATETIME_LEN];         //���ʱ��
	unsigned int  CheckType;                                 //�������
	unsigned char CheckPath[MAX_PROTO_PATH_LEN];             //���·��
	unsigned char ServiceShowName[MAX_PROTO_NAME_LEN];       //������ʾ����
	unsigned char ServiceName[MAX_PROTO_NAME_LEN];           //��������
	unsigned int  PID;                                       //����ID
	unsigned char PName[MAX_PROTO_NAME_LEN];                 //��������
	unsigned char Description[256];                          //��������
	unsigned char Path[MAX_PROTO_PATH_LEN];                  //����·��
	unsigned char Status[MAX_PROTO_STATUS_LEN];              //״̬
	unsigned int  Signature;                                 //ǩ����� 0 ��ʾû��ǩ��  1 ��ʾǩ����
	unsigned char AutoRun[MAX_PROTO_STATUS_LEN];             //��������
	unsigned char Deduction[MAX_PROTO_TEXT_LEN];             //Υ����ʾ��Ϣ
} DHServices;

//�˿����
typedef struct _DHPorts
{
	CommonHeader   Header;

	unsigned char Timestamp[MAX_PROTO_DATETIME_LEN];         //���ʱ��
	unsigned int  CheckType;                                 //�������
	unsigned char CheckPath[MAX_PROTO_PATH_LEN];             //���·��
	unsigned short Port;                                     //�˿ں�
	unsigned char Protocol[MAX_PROTO_STATUS_LEN];            //Э��
	unsigned char LocalIP[MAX_IP_LEN];                       //����IP��ַ
	unsigned char RemoteIP[MAX_IP_LEN];                      //�Զ�IP��ַ
	unsigned char Status[MAX_PROTO_STATUS_LEN];              //״̬
	unsigned int  PID;                                       //����ID 
	unsigned char PName[MAX_PROTO_NAME_LEN];                 //��������
	unsigned char Deduction[MAX_PROTO_TEXT_LEN];             //Υ����ʾ��Ϣ
} DHPorts;

//�˻�Ȩ������
typedef struct _DHAccountPermission
{
	CommonHeader   Header;

	unsigned char Timestamp[MAX_PROTO_DATETIME_LEN];         //���ʱ��
	unsigned int  CheckType;                                 //�������
	unsigned char CheckPath[MAX_PROTO_PATH_LEN];             //���·��
	unsigned char UserName[MAX_PROTO_NAME_LEN];              //�˻���
	unsigned char PermissionPrompt[MAX_PROTO_TEXT_LEN];      //�˻�Ȩ���������
	unsigned char Deduction[MAX_PROTO_TEXT_LEN];             //Υ����ʾ��Ϣ
	unsigned char Description[MAX_PROTO_DESCRIPTION_LEN];    //����
} DHAccountPermission;

//USB��¼
typedef struct _DHUSBRecord
{
	CommonHeader   Header;
    
	unsigned char Timestamp[MAX_PROTO_DATETIME_LEN];         //���ʱ��
	unsigned int  CheckType;                                 //������� CommonCheckType
	unsigned int  CheckSource;                               //�����Դ CommonCheckSource
	unsigned char CheckPath[MAX_PROTO_PATH_LEN];             //���·��
	unsigned char Type[MAX_PROTO_TYPE_LEN];                  //�豸����
	unsigned char DeviceName[MAX_PROTO_NAME_LEN];            //�豸����
	unsigned char SN[MAX_PROTO_MEMO_LEN];                    //���к�
	unsigned char VID[MAX_PROTO_MEMO_LEN];
	unsigned char PID[MAX_PROTO_MEMO_LEN];
	unsigned char FirstDate[MAX_PROTO_DATETIME_LEN];      	 //�״�ʹ��ʱ��	
	unsigned char LastDate[MAX_PROTO_DATETIME_LEN];      	 //ĩ��ʹ��ʱ��
	unsigned char Deduction[MAX_PROTO_TEXT_LEN];             //Υ����ʾ��Ϣ
}DHUSBRecord;


//������¼����
typedef struct _DHInternetRecord
{
	CommonHeader   Header;

	unsigned char Timestamp[MAX_PROTO_DATETIME_LEN];         //���ʱ��
	unsigned int  CheckType;                                 //������� CommonCheckType
	unsigned int  CheckSource;                               //�����Դ CommonCheckSource
	unsigned char CheckPath[MAX_PROTO_PATH_LEN];             //���·��
	unsigned int  InternetType;                              //�������� CommonInternetType
	unsigned char Date[MAX_PROTO_DATETIME_LEN];       	     //����ʱ��
	unsigned char AppName[MAX_PROTO_NAME_LEN];		         //�������
	unsigned char AppProcess[MAX_PROTO_NAME_LEN];            //������
	unsigned char Domain[MAX_PROTO_PATH_LEN];      	         //����	
	unsigned char Action[MAX_PROTO_STATUS_LEN];      	     //����	
	unsigned char Address[MAX_PROTO_URL_LEN];      	         //��ַ
	unsigned char User[MAX_PROTO_USER_LEN];      	         //�����û�
	unsigned char Deduction[MAX_PROTO_MEMO_LEN];             //Υ����ʾ��Ϣ
	unsigned char Title[MAX_PROTO_TITLE_LEN];                //��ҳ����
} DHInternetRecord;

//�ʼ���¼
typedef struct _DHEMailRecord
{
	CommonHeader   Header;

	unsigned char Timestamp[MAX_PROTO_DATETIME_LEN];         //���ʱ��
	unsigned int  CheckType;                                 //������� CommonCheckType
	unsigned char CheckPath[MAX_PROTO_PATH_LEN];             //���·��
	unsigned char AppName[MAX_PROTO_NAME_LEN];               //������� Foxmail,Outlook��
	unsigned char ProcName[MAX_PROTO_NAME_LEN];              //���������
	unsigned char BoxType[MAX_PROTO_NAME_LEN];               //��������,�磺�ռ��䣬�������
	unsigned char FileName[MAX_PROTO_NAME_LEN];	             //�ļ���
	unsigned char Sender[MAX_PROTO_DATETIME_LEN];            //������
	unsigned char Receiver[MAX_PROTO_HOSTNAME_LEN];          //�ռ���
	unsigned char Subject[MAX_PROTO_MEMO_LEN];           //����	
	unsigned char ActionTime[MAX_PROTO_DATETIME_LEN];        //����ʱ��	
	unsigned char Deduction[MAX_PROTO_MEMO_LEN];             //Υ����ʾ��Ϣ
} DHEMailRecord;


//�ļ���鳣��
typedef struct _DHFileCheck
{
	CommonHeader   Header;

	unsigned char Timestamp[MAX_PROTO_DATETIME_LEN];         //���ʱ��
	unsigned int  CheckType;                                 //������� CommonCheckType
	unsigned char CheckPath[MAX_PROTO_PATH_LEN];             //���·��
	unsigned char FileName[MAX_PROTO_PATH_LEN];              //�ļ���
	unsigned int  FileSource;                                //�ļ���Դ CommonFileSource
	unsigned char FilePath[MAX_PROTO_PATH_LEN];	             //�ļ�·��
	unsigned long long  FileSize;                            //�ļ���С
	unsigned char FileMD5[MAX_PROTO_NAME_LEN];               //�ļ�MD5
	unsigned char FileModifyTime[MAX_PROTO_DATETIME_LEN];    //�ļ��޸�ʱ��	
	unsigned char FileCreateTime[MAX_PROTO_DATETIME_LEN];    //�ļ�����ʱ��	
	unsigned char OwnerApp[MAX_PROTO_NAME_LEN];      	     //����Ӧ��
	unsigned char Deduction[MAX_PROTO_MEMO_LEN];             //Υ����ʾ��Ϣ
	unsigned char Keywords[MAX_PROTO_TEXT_LEN];      	     //���йؼ��� �ؼ��ּ�ֺŷָ�
	unsigned char PageCounts[MAX_PROTO_TEXT_LEN];            //ҳ�� ���ҳ��ֺŷָ�
	unsigned char Summary[MAX_PROTO_SUMMARY_LEN];            //�ļ�ժҪ����ؼ���ժҪ 	
	unsigned char SecretLevel[MAX_PROTO_DEVICE_LEN];      	 //�ļ��ܼ� ���ܣ����ܣ�����
	unsigned long HitsFileSize;								 //���йؼ��ֻ����ܼ��ļ��Ĵ�С
} DHFileCheck;

//�ʼ����
typedef struct _DHEMailCheck
{
	CommonHeader   Header;

	unsigned char Timestamp[MAX_PROTO_DATETIME_LEN];         //���ʱ��
	unsigned int  CheckType;                                 //������� CommonCheckType
	unsigned char CheckPath[MAX_PROTO_PATH_LEN];             //���·��
	unsigned char AppName[MAX_PROTO_NAME_LEN];               //������� Foxmail,Outlook��
	unsigned char BoxType[MAX_PROTO_NAME_LEN];               //��������,�磺�ռ��䣬�������
	unsigned char FileName[MAX_PROTO_NAME_LEN];	             //�ļ���
	unsigned char Sender[MAX_PROTO_DATETIME_LEN];            //������
	unsigned char Receiver[MAX_PROTO_HOSTNAME_LEN];          //�ռ���
	unsigned char Subject[MAX_PROTO_MEMO_LEN];               //����	
	unsigned char ActionTime[MAX_PROTO_DATETIME_LEN];        //����ʱ��	
	unsigned char Deduction[MAX_PROTO_MEMO_LEN];             //Υ����ʾ��Ϣ
	unsigned char Keywords[MAX_PROTO_TEXT_LEN];      	     //���йؼ��� �ؼ��ּ�ֺŷָ�
	unsigned char PageCounts[MAX_PROTO_TEXT_LEN];            //ҳ�� ���ҳ��ֺŷָ�
	unsigned char Summary[MAX_PROTO_SUMMARY_LEN];            //�ļ�ժҪ����ؼ���ժҪ
	unsigned char SecretLevel[MAX_PROTO_DEVICE_LEN];      	 //�ļ��ܼ� ���ܣ����ܣ�����
	unsigned long HitsFileSize;								 //���йؼ��ֻ����ܼ��ļ��Ĵ�С
} DHEMailCheck;

#pragma pack(pop)