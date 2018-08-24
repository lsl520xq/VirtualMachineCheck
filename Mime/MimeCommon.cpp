#include "StdAfx.h"
#include "MimeCommon.h"


CMimeCommon::CMimeCommon(void)
{

}


CMimeCommon::~CMimeCommon(void)
{

}

char* CMimeCommon::memchr_ex(const char *src, char c, size_t len)
{
	try
	{
		char *p1 = (char *)src;
		for(size_t i = 0;i < len; i++)
		{
			if(*p1 == c ) 
			{
				return p1;  
			}
			p1++;
		}
	}
	catch (...)
	{

	}
	return NULL;
}

//�����Ӵ�
char* CMimeCommon::memfind(const char *src, size_t srclen, const char *dst, size_t dstlen)
{
	try
	{
		char *p1 = (char *)src;
		char *p2 = NULL;
		size_t  offset = 0;
		size_t  plen = srclen - dstlen;
		if((src!=NULL) && (dst!=NULL)  && (srclen>0) && (dstlen>0) && (srclen>=dstlen))
		{
			p2 = memchr_ex(src,*dst,srclen-dstlen);
			while(p2)
			{
				if(memcmp(p2,dst,dstlen)==0)
				{
					break;
				}
				else
				{
					//����ƫ����
					p2++;
					offset =  (int)(p2 - p1);

					if (plen >= offset)
					{
						p2 = memchr_ex(p2,*dst,(plen -offset));
					}
					else
					{
						return NULL;
					}
				}
			}
		}
		return p2;
	}
	catch (...)
	{
		return NULL;
	}
}

char* CMimeCommon::memchr_rex(const char *src, char c, size_t len)
{
	try
	{
		char *p1 = (char *)src + len;
		for(size_t i = 0;i < len; i++)
		{
			if(*p1 == c ) 
			{
				return p1;  
			}
			p1--;
		}
	}
	catch (...)
	{

	}
	return NULL;
}

//��������Ӵ�
char* CMimeCommon::memrfind(const char *src, size_t srclen, const char *dst, size_t dstlen)
{
	try
	{
		char *p1 = (char *)src + srclen;
		char *p2 = NULL;
		size_t  offset = 0;
		size_t  plen = srclen - dstlen;
		if((src!=NULL)  && (dst!=NULL)  &&(srclen>0) && (dstlen>0) &&(srclen>=dstlen))
		{
			p2 = memchr_rex(src,*dst,plen);
			while(p2)
			{
				if(memcmp(p2,dst,dstlen)==0)
				{
					break;
				}
				else
				{
					//����ƫ����
					//p2--;
					offset =  (int)(p1 - p2 + 1);

					if (srclen >= offset)
					{
						p2 = memchr_rex(src,*dst,(srclen -offset));
					}
					else
					{
						return NULL;
					}
				}
			}
		}
		return p2;
	}
	catch (...)
	{
		return NULL;
	}
}

string CMimeCommon::upperToLower(const string &src)
{
	string result = src;
	for(int i = 0; i < static_cast<int>(result.size()); i++)
	{
		if((result[i] >= 'A') && (result[i] <= 'Z'))
		{
			result[i] = result[i] - 'A' + 'a';
		}
	}
	return result;
}


string CMimeCommon::lowerToUpper(const string &src)
{
	string result = src;
	for(int i = 0; i < static_cast<int>(result.size()); i++)
	{
		if((result[i] >= 'a') && (result[i] <= 'z'))
		{
			result[i] = result[i] - 'a' + 'A';
		}
	}
	return result;
}

/**********************************
�������ܣ�
	������"=?gbk?B?vNHEvsu5v6q3orDs?="�ַ������н���
����:
	src -- �����ַ���
����ֵ��
	�������ַ���
**********************************/
string CMimeCommon::mimeDecoder(const string& str, CMimeBuffer* pMimebuffer)
{
	int index = 0;
	int begin = 0, end = 0;
	string result, tmp(str);
	string charSet, key, content;

	if(pMimebuffer == NULL)
	{
		return "";
	}

	while((index = static_cast<int>(tmp.find(MimeEncoderBeginFlag, begin))) != string::npos)
	{
		//��ȡ����ǰ���ַ�
		if(index != begin)
		{
			//��ʱ��ȥ����
			result += tmp.substr(begin, index - begin);			
		}
		begin = index + static_cast<int>(MimeEncoderBeginFlag.size());
		index = static_cast<int>(tmp.find(MimeEncoderFlag, begin));
		if(index == string::npos)
		{
			continue;
		}
		//��ȡ�ַ�������ǰ���ַ���
		charSet = tmp.substr(begin, index - begin);
		begin = index + static_cast<int>(MimeEncoderFlag.size());
		index = static_cast<int>(tmp.find(MimeEncoderFlag, begin));
		if(index == string::npos)
		{
			continue;
		}
		//��ȡ���뷽ʽ
		key = tmp.substr(begin, index - begin);
		begin = index + static_cast<int>(MimeEncoderFlag.size());
		
		end = static_cast<int>(tmp.find(MimeEncoderEndFlag, begin));
		if(end == string::npos)
		{
			continue;
		}
		//��ȡʵ�ʱ����ַ���
		int len = 0;
		content = tmp.substr(begin, end -begin);
		memset(pMimebuffer->getDecodeBuffer(), 0, content.size() + 1);
		//���뷽ʽΪ��B��ʱ����base64����������quoter_printer����
		key = lowerToUpper(key);
		if(key == "B")
		{
			len= CBase64::Decode((uint8 *)(content.c_str()), static_cast<size_t>(content.size()), 
				(uint8 *)(pMimebuffer->getDecodeBuffer()), pMimebuffer->getDecodeBufferSize());
			if(len == -1)
			{
				CFuncs::WriteLogInfo(SLT_ERROR,"Base64�������:%s",content.c_str());
				return "";
			}
		}
		else if(key == "Q")
		{
			len = CCharsetConver::DecoderQuoterPrinter(content.c_str(), static_cast<int>(content.size()),
				pMimebuffer->getDecodeBuffer(), pMimebuffer->getDecodeBufferSize());
			if (len == -1)
			{
				CFuncs::WriteLogInfo(SLT_ERROR,"quoter_printer�������:%s",content.c_str());
				return "";
			}
		}
		//�ַ���Ϊutf-8ʱ����utf-8��Ansi��ת��
		if(upperToLower(charSet).find("utf-8") != string::npos)
		{
			len = CCharsetConver::UTF8ToUnicode(pMimebuffer->getDecodeBuffer(),len,
				pMimebuffer->getDecodeWideBuffer(),pMimebuffer->getDecodeWideBufferSize());
			len = CCharsetConver::UnicodeToANSI(pMimebuffer->getDecodeWideBuffer(),len,
				pMimebuffer->getDecodeBuffer(),pMimebuffer->getDecodeBufferSize());
		}
		else if(upperToLower(charSet).find("iso-2022-jp") != string::npos)
		{
			len = CCharsetConver::ISO2022JPToUnicode(pMimebuffer->getDecodeBuffer(),len,
				pMimebuffer->getDecodeWideBuffer(),pMimebuffer->getDecodeWideBufferSize());
			len = CCharsetConver::UnicodeToANSI(pMimebuffer->getDecodeWideBuffer(),len,
				pMimebuffer->getDecodeBuffer(),pMimebuffer->getDecodeBufferSize());
		}
		else if(upperToLower(charSet).find("big5") != string::npos)
		{
			len = CCharsetConver::BIG5ToUnicode(pMimebuffer->getDecodeBuffer(),len,
				pMimebuffer->getDecodeWideBuffer(),pMimebuffer->getDecodeWideBufferSize());
			len = CCharsetConver::UnicodeToGB2312(pMimebuffer->getDecodeWideBuffer(),len,
				pMimebuffer->getDecodeBuffer(),pMimebuffer->getDecodeBufferSize());
		}
		result.append(pMimebuffer->getDecodeBuffer());
		begin = end + static_cast<int>(MimeEncoderEndFlag.size());
		tmp = tmp.substr(begin, tmp.size() - begin);
		begin = 0;
	}
	result.append(tmp);
	return result;
}

string CMimeCommon::GetSessionID(const char* proto, const uint32 sip, const uint16 sport)
{
	char tmpBuf[1024 * 3];
	memset(tmpBuf, 0, sizeof(tmpBuf));
	sprintf_s(tmpBuf, _countof(tmpBuf),"%s_%u_%u_%s",proto,sip,sport,CFuncs::GetGUID().c_str());
	return string(tmpBuf);
}