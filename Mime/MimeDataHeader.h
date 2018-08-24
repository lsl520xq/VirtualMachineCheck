#pragma once


#include <vector>
#include <sstream>
#include "MimeBoundary.h"
#include "../Common/Funcs.h"
#include "../Common/Exception.h"

using std::vector;

//����Mimeͷ��������
class CMimeDataHeader
{
private:
	string from;					//������
	string to;						//�ռ���
	string cc;						//����
	string bcc;						//����
	string subject;					//����
	//string contentType;			//ý������
	//string contentEncoder;		//���뷽ʽ
	//string boundary;				//�߽�ֽ���
	//string charSet;				//�ַ���
	string date;					//����

	//2011-07-12
	//���RCPT TO�ֶε��ռ����б�
	vector<string> rcptToList;
	//2011-07-12

	CMimeBuffer* m_MimeBuffer;      //����ת���ڴ�

	string handleDate(const string &date);
	//void handleContentType(const string &value);
	void handleHeader(const string &content);
	//2011-07-12
	//��rcptTolist�е��ռ����ԡ���������
	string GetToFromRcptToList();
	//2011-07-12
public:
	CMimeDataHeader(CMimeBuffer* pMimebuffer);
	~CMimeDataHeader(void);
public:
	CMimeBoundary boundary;

	/***************************************
	//�������ܣ�
	//		��ʼ��Mime����ͷ��
	//������
	//		str -- Mimeͷ������
	***************************************/
	bool initMimeDataHeader(const char *src, size_t len);

	void clearMimeDataHeader();


	string getFrom() const		{return from;}
	string getTo() const		{return to;}
	string getCc() const		{return cc;}
	string getBcc()	const		{return bcc;}
	string getSubject() const	{return subject;}
	//string getContentType()	{return contentType;}
	//string getContentEncoder(){return contentEncoder;}
	//string getBoundary()		{return boundary;}
	//string getCharSet()		{return charSet;}
	string getDate() const		{return date;}
};

