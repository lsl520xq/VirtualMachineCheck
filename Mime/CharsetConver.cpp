#include "StdAfx.h"
#include "CharsetConver.h"


CCharsetConver::CCharsetConver(void)
{
}


CCharsetConver::~CCharsetConver(void)
{
}

int CCharsetConver::ANSIToUnicode(const char *src, int srclen, wchar_t *dst, int dstlen)
{
	if(src == NULL && dst == NULL)
	{
		return -1;
	}
	//תUNICODE
	int length = ::MultiByteToWideChar( CP_ACP, 0, src, -1, NULL, 0); 
	if(dstlen < (length + (int)sizeof(wchar_t)))
	{
		return -1;
	}
	length = ::MultiByteToWideChar( CP_ACP, 0, src, -1, dst, length); 
	return length;
}

int CCharsetConver::UnicodeToANSI(const wchar_t *src, int srclen, char *dst, int dstlen)
{
	if(src == NULL && dst == NULL)
	{
		return -1;
	}
	int length = WideCharToMultiByte(CP_ACP, 0, src, srclen ,NULL, 0, NULL, NULL);
	if(dstlen < (length + (int)sizeof(char)))
	{
		return -1;
	}
	memset(dst, 0, dstlen);
	WideCharToMultiByte(CP_ACP, 0, src, srclen, dst, length, NULL, NULL);
	return length;
}

int CCharsetConver::UnicodeToGB2312(const wchar_t *src, int srclen, char *dst, int dstlen)
{
	if(src == NULL && dst == NULL)
	{
		return -1;
	}
	int length = WideCharToMultiByte(936, 0, src, srclen, NULL, 0, NULL, NULL);
	if(length >= dstlen)
	{
		return -1;
	}
	memset(dst, 0, dstlen);
	WideCharToMultiByte(936, 0, src, srclen, dst, length, NULL, NULL);
	return length;
}

/****************************************
�������ܣ�
	��UTF-8��ת��ΪUnicode��
������
	src -- UTF-8�����ַ���
	dst -- Uncode���� �������Ϊsrc��3��
����ֵ��
	-1ʧ��
****************************************/
int CCharsetConver::UTF8ToUnicode(const char *src, int srclen, wchar_t *dst, int dstlen)
{
	if(src == NULL && dst == NULL)
	{
		return -1;
	}
	int length = MultiByteToWideChar(CP_UTF8, 0, src, srclen, NULL, 0);
	if(dstlen < (length + (int)sizeof(wchar_t)))
	{
		return -1;
	}
	memset(dst, 0, dstlen);
	MultiByteToWideChar(CP_UTF8, 0, src, srclen, dst, length);
	return length;
}

int CCharsetConver::UTF8ToGB2312(const char *src, int srclen, wchar_t *wdst, int wdstlen,char *dst, int dstlen)
{
	int len = UTF8ToUnicode(src,srclen,wdst,wdstlen);
	len = UnicodeToGB2312(wdst,len,dst,dstlen);
	return len;
}

int CCharsetConver::ISO2022JPToUnicode(const char *src, int srclen, wchar_t *dst, int dstlen)
{
	if(src == NULL && dst == NULL)
	{
		return -1;
	}

	int length = MultiByteToWideChar(50220, 0, src, srclen, NULL, NULL);
	if(length >= dstlen)
	{
		return -1;
	}
	memset(dst, 0, dstlen);
	MultiByteToWideChar(50220, 0, src, srclen, dst, length);
	
	return length;
}

int CCharsetConver::BIG5ToUnicode(const char *src, int srclen, wchar_t *dst, int dstlen)
{
	if(src == NULL && dst == NULL)
	{
		return -1;
	}
	int length = MultiByteToWideChar(950, 0, src, srclen, NULL, NULL);
	if(length >= dstlen)
	{
		return -1;
	}
	memset(dst, 0, dstlen);
	MultiByteToWideChar(950, 0, src, srclen, dst, length);

	return length;
}

//ץ��������ݽ�������dst��
int CCharsetConver::BIG5ToGB2312(char *src, int srclen, wchar_t *wdst, int wdstlen, char* dst, int dstlen)
{
	if(src == NULL && dst == NULL)
	{
		return -1;
	}
	int length = BIG5ToUnicode(src,srclen,wdst,wdstlen);
	if(length == -1)
	{
		return -1;
	}
	length = UnicodeToGB2312(wdst,length,dst,dstlen);
	if(length == -1)
	{
		return -1;
	}
	return length;
}

//������BIG5�Ѿ�ת��ΪGB2312��ʽ����ܽ��з���,���������BIG5,���ȵ���BIG5ToGB2312�����ٽ��з���
int CCharsetConver::BIG5TranslateGB2312(char *src, int srclen,char* dst, int dstlen)
{
	if(src == NULL && dst == NULL)
	{
		return -1;
	}
	LCID Locale = MAKELCID(MAKELANGID(LANG_CHINESE,SUBLANG_CHINESE_SIMPLIFIED),SORT_CHINESE_PRC);
	int length = LCMapString(Locale,LCMAP_SIMPLIFIED_CHINESE, src,srclen,NULL,0);
	if(length >= dstlen)
	{
		return -1;
	}
	length = LCMapString(Locale,LCMAP_SIMPLIFIED_CHINESE,src,srclen,dst,dstlen);
	return length;
}

int CCharsetConver::QuotedPrintableFlag(char ch)
{
	if((ch >= 'a') && (ch <= 'z'))
	{
		ch = ch - 'a' + 'A';
	}
	if((ch >= '0') && (ch <= '9'))
	{
		return ch - '0';
	}
	else if((ch >= 'A') && (ch <= 'F'))
	{
		return ch - 'A' + 10;
	}
	else
	{
		return -1;
	}
}
/*******************************
�������ܣ�
	quoterprinter����
������
	src -- �����ַ���
	srclen -- ���벿�ֵĳ���
	dst -- �������ַ���
	dstlen -- �����ַ����ĳ���
����ֵ��
	�ɹ����ؽ�����ַ����ĳ���
	ʧ�ܷ���-1
*************************************/
int CCharsetConver::DecoderQuoterPrinter(const char *src, int srclen, char *dst, int dstlen)
{
	int index = 0;
	const char *tmp = src;

	while(*src != '\0')
	{
		if(src - tmp > srclen)
		{
			break;
		}
		if(index >= dstlen)
		{
			return -1;
		}
		if(*src == '=')
		{
			if((src - tmp + 2) <= (INT)strlen(tmp))
			{
				char ch = QuotedPrintableFlag(*(src + 1));
				char cl = QuotedPrintableFlag(*(src + 2));
				if((ch != -1) || (cl != -1))
				{
					dst[index++] = (ch << 4) | cl;
				}
				src += 3;
			}
			else
			{
				break;
			}
		}
		else
		{
			dst[index++] = *src++;
		}
	}
	dst[index] = '\0';
	return index;
}

int CCharsetConver::Decoder7Bit(const char *src, int srclen, char *dst, int dstlen)
	//int            Decode7bit(const char *pSrc, int srcLen, char *pDst, int dstLen);
{
	int nSrc;        // Դ�ַ����ļ���ֵ
    int nDst;        // Ŀ����봮�ļ���ֵ
    int nByte;       // ��ǰ���ڴ���������ֽڵ���ţ���Χ��0-6
	int nCount;			
	int nMaxCount;	 // ���������
    unsigned char nLeft;    // ��һ�ֽڲ��������
    
    // ����ֵ��ʼ��
    nSrc = 0;
    nDst = 0;

    // �����ֽ���źͲ������ݳ�ʼ��
    nByte = 0;
    nLeft = 0;

    nCount = 0;
	nMaxCount = srclen / 7 > (dstlen - 1) / 8 ? (dstlen - 1) / 8 : srclen / 7;

    // ��Դ����ÿ7���ֽڷ�Ϊһ�飬��ѹ����8���ֽ�
    // ѭ���ô�����̣�ֱ��Դ���ݱ�������
    // ������鲻��7�ֽڣ�Ҳ����ȷ����
    while(nSrc < srclen)
    {
        // ��Դ�ֽ��ұ߲��������������ӣ�ȥ�����λ���õ�һ��Ŀ������ֽ�
        *dst = ((*src << nByte) | nLeft) & 0x7f;
        // �����ֽ�ʣ�µ���߲��֣���Ϊ�������ݱ�������
        nLeft = *src >> (7 - nByte);
    
        // �޸�Ŀ�괮��ָ��ͼ���ֵ
        dst++;
        nDst++;
    
        // �޸��ֽڼ���ֵ
        nByte++;
    
        // ����һ������һ���ֽ�
        if(nByte == 7)
        {
            // ����õ�һ��Ŀ������ֽ�
            *dst = nLeft;
    
            // �޸�Ŀ�괮��ָ��ͼ���ֵ
            dst++;
            nDst++;
    
            // �����ֽ���źͲ������ݳ�ʼ��
            nByte = 0;
            nLeft = 0;
			nCount++;
			if(nCount > nMaxCount)
			{
				dst++;
				break;
			}
        }
    
        // �޸�Դ����ָ��ͼ���ֵ
        src++;
        nSrc++;
    }
    
    *dst = 0;
    
    // ����Ŀ�괮����
    return nDst;
}


int CCharsetConver::Decoder8Bit(const char *src, int srclen, char *dst, int dstlen)
{
	int nSrc;        // Դ�ַ����ļ���ֵ
    int nDst;        // Ŀ����봮�ļ���ֵ
    int nByte;       // ��ǰ���ڴ���������ֽڵ���ţ���Χ��0-7
	int nCount;			
	int nMaxCount;	 // ���������
    unsigned char nLeft;    // ��һ�ֽڲ��������
    
    // ����ֵ��ʼ��
    nSrc = 0;
    nDst = 0;

    // �����ֽ���źͲ������ݳ�ʼ��
    nByte = 0;
    nLeft = 0;

    nCount = 0;
	nMaxCount = srclen / 8 > (dstlen - 1) / 9 ? (dstlen - 1) / 9 : srclen / 8;

    // ��Դ����ÿ8���ֽڷ�Ϊһ�飬��ѹ����9���ֽ�
    // ѭ���ô�����̣�ֱ��Դ���ݱ�������
    // ������鲻��8�ֽڣ�Ҳ����ȷ����
    while(nSrc < srclen)
    {
        // ��Դ�ֽ��ұ߲��������������ӣ�ȥ�����λ���õ�һ��Ŀ������ֽ�
        *dst = ((*src << nByte) | nLeft) & 0x7f;
        // �����ֽ�ʣ�µ���߲��֣���Ϊ�������ݱ�������
        nLeft = *src >> (8 - nByte);
    
        // �޸�Ŀ�괮��ָ��ͼ���ֵ
        dst++;
        nDst++;
    
        // �޸��ֽڼ���ֵ
        nByte++;
    
        // ����һ������һ���ֽ�
        if(nByte == 8)
        {
            // ����õ�һ��Ŀ������ֽ�
            *dst = nLeft;
    
            // �޸�Ŀ�괮��ָ��ͼ���ֵ
            dst++;
            nDst++;
    
            // �����ֽ���źͲ������ݳ�ʼ��
            nByte = 0;
            nLeft = 0;
			nCount++;
			if(nCount > nMaxCount)
			{
				dst++;
				break;
			}
        }
    
        // �޸�Դ����ָ��ͼ���ֵ
        src++;
        nSrc++;
    }
    
    *dst = 0;
    
    // ����Ŀ�괮����
    return nDst;
}