#pragma once

#include "Base64.h"
#include "MimeBuffer.h"
#include "CharsetConver.h"

const string MimeMailFrom = "MAIL FROM";
const string MimeRcptTo = "RCPT TO";

const string MimeFrom = "From";										//�ռ��˱�־
const string MimeTo = "To";											//�����˱�־
const string MimeCc	=  "Cc";										//���ͱ�־
const string MimeBcc = "Bcc";										//���ͱ�־
const string MimeSubject = "Subject";								//�����־
const string MimeDate = "Date";										//����
const string MimeContentType =	"Content-Type";						//ý�����ͱ�־
const string MimeContentEncoder = "Content-Transfer-Encoding";		//�����ʽ��־
const string MimeBoundaryFlag = "boundary";							//�߽�綨�߱�־
const string MimeCharSetFlag = "charset";							//�ַ�����־
const string MimeContentDispostion = "Content-Disposition";			//���ݴ���ʽ��־
const string MimeFileName = "filename=";							//��������־
const string MimeBeginFlag = ":";									//�ֶ�ֵ��ʼ��־
const string MimeEndFlag = "\r\n";									//�ֶ�ֵ������־
const string MimeEndFlagEx = "\n";									//�ֶ�ֵ������־
const string MimeEndFlagOfHeader = "\r\n\r\n";						//ͷ��������ָ��־

const string MimeAttachmentFlag =	"attachment";					//������־
const string MimeBase64Encoder = "base64";							//base64�����־
const string MimeQuoterPrinterEncoder = "quoted-printable";			//quoter_printed�����־
const string Mime7BitEncoder = "7bit";								//7bit�����־
const string Mime8BitEncoder = "8bit";								//8bit�����־	

const string MimeContentFlag = "text";								//���ı�־
const string MimeMultiPartMixedFlag = "multipart/mixed";			//����ý�������е�mixed�����ͱ�־
const string MimeMultiPartAlternativeFlag = "multipart/alternative";//����ý�������е�alternative�����ͱ�־
const string MimeMultiPartFlag = "multipart";						//����ý�����ͱ�־

const string MimeEncoderBeginFlag = "=?";							//���뿪ʼ��־
const string MimeEncoderFlag = "?";									//�ָ��־
const string MimeEncoderEndFlag = "?=";								//���������־

const string MimeChunkedEndFlag = "\r\n0\r\n\r\n";                  //chunked�������

class CMimeCommon
{
private:
	static char* memchr_ex(const char *src, char c, size_t len);
	static char* memchr_rex(const char *src, char c, size_t len);
public:
	CMimeCommon(void);
	~CMimeCommon(void);
public:
	static char* memfind(const char *src, size_t srclen, const char *dst, size_t dstlen);
	static char* memrfind(const char *src, size_t srclen, const char *dst, size_t dstlen);

	static string lowerToUpper(const string &src);
	static string upperToLower(const string &src);

	static string mimeDecoder(const string &src, CMimeBuffer* pMimebuffer);

	static string GetSessionID(const char* proto, const uint32 sip, const uint16 sport);
};

