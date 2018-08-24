#include "stdafx.h"
#include "MimeBoundary.h"

CMimeBoundary::CMimeBoundary(CMimeBuffer* pMimebuffer)
{
	content = NULL;
	contentLength = 0;
	m_MimeBuffer = pMimebuffer;
}

CMimeBoundary::~CMimeBoundary(void)
{

}
//����Mimeͷ�����ݵ���ͬ������
bool CMimeBoundary::initBoundary(const char *src, int len)
{
	try
	{
		char *pBegin = NULL, *pIndex = NULL, *pEnd = NULL, *pTmp = NULL;
		string body;
		bool LoopFlag = true;
		size_t headerLength = 0;
		size_t offset = 0;

		pBegin = (char *)src;
		pIndex = CMimeCommon::memfind(pBegin, len, MimeEndFlagOfHeader.c_str(), MimeEndFlagOfHeader.size());
		if(pIndex == NULL)
		{
			return false;
		}

		headerLength = pIndex - pBegin;
		content = pIndex;
		contentLength = len - (int)headerLength - (int)MimeEndFlagOfHeader.size();

		pIndex = CMimeCommon::memfind(pBegin, headerLength, MimeBeginFlag.c_str(), MimeBeginFlag.size());
		if(pIndex == NULL)
		{
			return false;
		}

		while(LoopFlag)
		{
			pTmp = pIndex + MimeBeginFlag.size();
			offset = pTmp - src;
			pIndex = CMimeCommon::memfind(pTmp, headerLength - offset, MimeBeginFlag.c_str(), MimeBeginFlag.size());
			if(pIndex == NULL)
			{
				offset = pBegin - src;
				body = string(pBegin, headerLength - offset);
				LoopFlag = false;
			}
			else
			{
				offset = pIndex - (char *)src;
				pEnd = CMimeCommon::memrfind(src, offset, MimeEndFlag.c_str(), MimeEndFlag.size());
				if(pEnd == NULL)
				{
					continue;
				}
				else
				{
					if(pEnd - pTmp < 0)
					{
						continue;
					}
					else
					{
						body = string(pBegin, pEnd - pBegin);
						pBegin = pEnd + MimeEndFlag.size();
					}
				}
			}
			
			handleBoundaryheader(body);
		}
	}
	catch(const exception &err)
	{
		CFuncs::WriteLogInfo(SLT_ERROR,"initBoundary �����쳣��%s.", err.what());
		return false;
	}
	catch(...)
	{
		CFuncs::WriteLogInfo(SLT_ERROR,"initBoundary ����δ֪�쳣��");
		return false;
	}
	
	return true;
}

void CMimeBoundary::handleBoundaryheader(const string &content)
{ 
	size_t index = 0;
	string key, value;

	index = content.find(MimeBeginFlag);
	if(index == string::npos)
	{
		return;
	}

	key = content.substr(0, index);
	size_t tmp = key.find(MimeEndFlag);
	if(tmp != string::npos)
	{
		key = key.substr(tmp + MimeEndFlag.size());
	}
	size_t tmpIndex = index + MimeBeginFlag.size();
	if((tmpIndex!= content.size()) && content[tmpIndex] == ' ')
	{
		tmpIndex++;
	}
	value = content.substr(tmpIndex, content.size() - tmpIndex);
	if(key == MimeContentType)
	{
		contentType = value;
		handleContentType();

	}
	else if(key == MimeContentEncoder)
	{
		contentEncoder = value;
	}
	else if(key == MimeContentDispostion)
	{
		contentDisposition = value;
		handleContentDisposition();
	}
}

void CMimeBoundary::handleContentType(void)
{
	int j =static_cast<int>(contentType.find(MimeBoundaryFlag));
	if(j != string::npos)
	{
		int ibegin = j + static_cast<int>(MimeBoundaryFlag.size()) + 1;
		int iend = static_cast<int>(contentType.find(MimeEndFlag, ibegin));
		if(iend == string::npos)
		{
			iend = static_cast<int>(contentType.size());
		}
		boundary = contentType.substr(ibegin, iend - ibegin);
		if((j = static_cast<int>(boundary.find("\""))) != string::npos)
		{
			int iend = static_cast<int>(boundary.find("\"", j + 1));
			if(iend != string::npos)
			{
				boundary = boundary.substr(j + 1, iend - j - 1);
			}
		}
	}
	j = static_cast<int>(contentType.find(MimeCharSetFlag));
	if(j != string::npos)
	{
		int ibegin = j + static_cast<int>(MimeCharSetFlag.size()) + 1;
		int iend = static_cast<int>(contentType.find(MimeEndFlag, ibegin));
		if(iend == string::npos)
		{
			iend = static_cast<int>(contentType.size());
		}
		charSet = contentType.substr(ibegin, iend - ibegin);
		if((j = static_cast<int>(charSet.find("\""))) != string::npos)
		{
			int iend = static_cast<int>(charSet.find("\"", j + 1));
			if(iend != string::npos)
			{
				charSet = charSet.substr(j + 1, iend - j - 1);
			}
		}
	}
}

void CMimeBoundary::handleContentDisposition(void)
{
	filename = "";
	int nBegin = static_cast<int>(contentDisposition.find(MimeFileName));	//���Ҹ�������ʼ��ʶ
	if(nBegin == string::npos)
	{
		return;
	}
	nBegin += static_cast<int>(MimeFileName.size());
	int nEnd = static_cast<int>(contentDisposition.find(";", nBegin));	//���Ҹ�����������ʶ
	if(nEnd == string::npos)
	{
		nEnd = static_cast<int>(contentDisposition.size());				//û�ҵ�������ʶ����ֵĩβ
	}
	filename = contentDisposition.substr(nBegin, nEnd - nBegin);		//������
	nBegin = static_cast<int>(filename.find("\""));						//ȥ����������˫����
	if(nBegin != string::npos)
	{
		nBegin += static_cast<int>(strlen("\""));
		nEnd = static_cast<int>(filename.find("\"", nBegin));
		if(nEnd == string::npos)
		{
			nEnd = static_cast<int>(filename.size());
		}
		filename = filename.substr(nBegin, nEnd - nBegin);
	}
	filename = CMimeCommon::mimeDecoder(filename,m_MimeBuffer);			//�������н���
}

void CMimeBoundary::clear()
{
	contentType.clear();				
	contentDisposition.clear();		
	boundary.clear();				
	contentEncoder.clear();			
	filename.clear();				
	content = NULL;
	contentLength = 0;
	charSet.clear();
}