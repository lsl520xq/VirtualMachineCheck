#pragma once

#include "MimeCommon.h"
#include "../Common/Funcs.h"
#include "../Common/Exception.h"

//
//����߽�綨��֮����������ݣ�����ͷ�������壩��
class CMimeBoundary
{
public:
	/**********************************
	//�������ܣ�
	//		��������
	//������
	//		str -- Ҫ����������
	***********************************/
	CMimeBoundary(CMimeBuffer* pMimebuffer);
	~CMimeBoundary(void);

	bool initBoundary(const char *src, int len);
	void handleBoundaryheader(const string &content);
	void handleContentType(void);
	void handleContentDisposition(void);
	void clear();

	void setContentType(const string &value)		{contentType = value;}
	void setContentDisposition(const string &value)	{contentDisposition = value;}
	void setBoundary(const string &value)			{boundary = value;}
	void setContentEncoder(const string &value)		{contentEncoder = value;}
	void setFileName(const string &value)			{filename = value;}
	void setContent(char *value)					{content = value;}
	void setContentLength(int length)				{contentLength = length;}
	void setCharSet(const string &value)			{charSet = value;}

	string getContentType()	const					{return contentType;}
	string getContentDisposition() const			{return contentDisposition;}
	string getBoundary() const						{return boundary;}
	string getContentEncoder() const				{return contentEncoder;}
	string getFileName() const						{return filename;}
	char *getContent()	const						{return content;}
	int getContentLength() const					{return contentLength;}
	string getCharSet()	const						{return charSet;}
private:
	string contentType;				//ý������
	string contentDisposition;		//���ݴ���ʽ
	string boundary;				//�߽�綨��
	string contentEncoder;			//���뷽ʽ
	string filename;				//������
	char*  content;					//��������
	int    contentLength;			//�������ݳ���
	string charSet;					//�ַ���

	CMimeBuffer* m_MimeBuffer;      //����ת���ڴ�
};
