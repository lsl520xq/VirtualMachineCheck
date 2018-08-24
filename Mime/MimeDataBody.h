#pragma once

#include <vector>
#include <cstdio>
#include <sstream>
#include <cstring>
#include "MimeDataHeader.h"
#include "MimeBoundary.h"
#include "MimeCommon.h"
#include "../Common/Funcs.h"
#include "../Common/Exception.h"


//using std::vector;

typedef void (*WriteFileFuncPointer)(const string &fileName, const char *buf, int len, string &newFileName, size_t time);

struct AttachData
{
	uint32 fileSize;    //�ļ���С
	string fileName;	//ԭʼ�ļ���
	string filePath;	//�洢����ļ���
};

//����Mime����������
class CMimeDataBody
{
public:
	CMimeDataBody(CMimeBuffer* pMimebuffer);
	~CMimeDataBody(void);
	/********************************************
	//�������ܣ�
	//		����Mime��������
	//������
	//		content -- ��������
	//		header -- �������Mimeͷ������
	*********************************************/
	void HandleMimeDataBody(const char *content, size_t len, const CMimeBoundary &boundary);

	void HandleMimeDataBody(const char *content, size_t len, const CMimeBoundary &boundary, const char* savePath);

	void clearMimeDataBody();

	string getDataContent()	const	{return dataContent;}

	void InitAtthPath(const string &filePath);
	void InitAtthTag(const string &tag);
	void InitWriteFilePointer(WriteFileFuncPointer fpWritePointer);
	void InitCurrentTime(size_t currTime);
private:
	/*******************************************
	//��������;
	//		��������ý������multipart��������Ϊmixed������
	//������
	//		str -- ��������
	//		boundary -- ͷ�������еı߽�綨��
	********************************************/
	void multipartMixed(const char *str, int len, const string &boundary);

	/********************************************
	//��������;
	//		��������ý������multipart��������Ϊalternative������
	//������
	//		str -- ��������
	//		boundary -- ͷ�������еı߽�綨��		
	********************************************/
	void multipartAlternative(const char *str, size_t len, const string &boundary);
	
	void multipartAlternative(const char *str, size_t len, const string &boundary, const char* savePath);

	/*****************************************
	//�������ܣ�
	//		��������ý���а�������ɢý������
	//������
	//		boundary -- ��ɢý�����͵�ȫ������
	*****************************************/
	//void boundaryHandler(const Boundary &boundary);

	/*****************************************
	//��������;
	//		����Ϊ��ɢý�����͵�Mime��������
	//������
	//		content -- Mime��������
	//		header -- Mimeͷ������
	*****************************************/
	void discreptionTypeHandle(const char *content, size_t len, const CMimeBoundary &boundary);
		
	void discreptionTypeHandle(const char *content, size_t len, const CMimeBoundary &boundary, const char* savePath);

	/****************************************
	//�������ܣ�
	//		д��������
	//������
	//		filename -- �����ļ���
	//		buf -- ��������
	//		len -- ��������
	****************************************/
	void writeAtthFile(const string &filename, const char *buf, int len);
	
	void writeAtthFile(const string &filename, const char *buf, int len, const char* savePath);

	string renameAtthFile(const string &filename);
public:
	vector<AttachData> atthList;//�������б�
private:
	CMimeBuffer* m_MimeBuffer;  //����ת���ڴ�

	string dataContent;			//Mime�ʼ�����
	string atthPath;			//����·��
	string atthTag;				//��������ʶ eg: POP3, SMTP
	size_t time;				//ʱ��
	WriteFileFuncPointer fpWriteFilePointer;	//д�����ļ�ָ��
};

