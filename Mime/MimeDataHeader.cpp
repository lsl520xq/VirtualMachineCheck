#include "stdafx.h"
#include "MimeDataHeader.h"

CMimeDataHeader::CMimeDataHeader(CMimeBuffer* pMimebuffer):boundary(pMimebuffer)
{
	m_MimeBuffer = pMimebuffer;
}

CMimeDataHeader::~CMimeDataHeader(void)
{

}

//�����ֶ�ֵ��Ҳ���԰���������־����Content_Type�ֶΡ�
//�ʲ��÷��������һ����ʼ��־֮��Ľ�����־����ȡ��ص�
//�ֶ��ּ���ֵ��
bool CMimeDataHeader::initMimeDataHeader(const char *src, size_t len)
{
	char* pBegin = NULL;
	char* pIndex = NULL;
	char* pEnd = NULL;
	char* pEndEx = NULL;
	char* pTmp = NULL;
	string body;
	bool LoopFlag = true;
	int headerLength = 0;
	size_t offset = 0;
	bool isMimeEndFlagEx = false;
	try
	{
		pBegin = (char *)src;
		pIndex = CMimeCommon::memfind(pBegin, len, MimeBeginFlag.c_str(), MimeBeginFlag.size());		//���ҵ�һ����ʼ��ʶ
		if(pIndex == NULL)
		{
			return false;
		}

		while(LoopFlag)
		{
			isMimeEndFlagEx = false;
			pTmp = pIndex + MimeBeginFlag.size();   //�ֶ�ֵValue�Ŀ�ʼ��ʶ
			offset = pTmp - src;
			pIndex = CMimeCommon::memfind(pTmp, len - offset, MimeBeginFlag.c_str(), MimeBeginFlag.size());	//������һ����ʼ��ʶ
			if(pIndex == NULL)
			{
				offset = pBegin - src;
				body = string(pBegin, len - offset);
				LoopFlag = false;
			}
			else
			{
				offset = pIndex - src;
				pEnd = CMimeCommon::memrfind(src, offset, MimeEndFlag.c_str(), MimeEndFlag.size());  //������ҽ�����ʶ
				pEndEx = CMimeCommon::memrfind(src, offset, MimeEndFlagEx.c_str(), MimeEndFlagEx.size());  //������ҽ�����ʶ
				if(NULL == pEnd && NULL == pEndEx)
				{
						continue;
				}
				else
				{
					if(NULL == pEnd && NULL != pEndEx)
					{
						pEnd = pEndEx;
						isMimeEndFlagEx = true;
					}

					if(pEnd - pTmp < 0)		//������ʶС���ֶ�ֵ�Ŀ�ʼ��ʶ���ý��������������Ľ�����ʶ
					{
						continue;
					}
					else
					{
						body = string(pBegin, pEnd - pBegin);		//�ҵ�������ʶ����ȡkey_value�ԣ��磺From��hanxin110000@sina.com
						if(isMimeEndFlagEx)
						{
							pBegin = pEnd + MimeEndFlagEx.size();				//����key�Ŀ�ʼλ��
						}
						else
						{
							pBegin = pEnd + MimeEndFlag.size();				//����key�Ŀ�ʼλ��
						}
					}
				}
			}
			handleHeader(body);			//����key_vaue�ԣ��������е�key��value
		}
	}
	catch(const exception &err)
	{
		CFuncs::WriteLogInfo(SLT_ERROR,"InitMimeDataHeader �����쳣��%s.", err.what());
		return false;
	}
	catch(...)
	{
		CFuncs::WriteLogInfo(SLT_ERROR,"InitMimeDataHeader ����δ֪�쳣��");
		return false;
	}
	return true;
}

void CMimeDataHeader::handleHeader(const string &content)
{
	string key;
	string value;
	size_t index = 0;

	index = content.find(MimeBeginFlag);
	if(index == string::npos)
	{
		return;
	}
	key = content.substr(0, index);		//��ȡkey
	size_t tmp = key.find(MimeEndFlag);		//ȥ��key�н�����ʶ
	if(tmp != string::npos)
	{
		key = key.substr(tmp + MimeEndFlag.size());	
	}
	size_t tmpIndex = index + MimeBeginFlag.size();
	if((tmpIndex!= content.size()) && content[tmpIndex] == ' ')
	{
		tmpIndex++;		//ȥ��value�е�diһ���ո�
	}
	value = content.substr(tmpIndex, content.size() - tmpIndex);
	//�洢����ֶ�ֵ
	if(key == MimeFrom)
	{
		from = CMimeCommon::mimeDecoder(value,m_MimeBuffer);
	}
	else if(key == MimeTo)
	{
		to = CMimeCommon::mimeDecoder(value,m_MimeBuffer);
		//2011-07-12
		//���ռ���Ϊ�գ�����rcptToList���
		if(to.size() == 0)
		{
			to = GetToFromRcptToList();
		}
		//2011-07-12
	}
	else if(key == MimeCc)
	{
		cc = CMimeCommon::mimeDecoder(value,m_MimeBuffer);
	}
	else if(key == MimeBcc)
	{
		bcc = CMimeCommon::mimeDecoder(value,m_MimeBuffer);
	}
	else if(key == MimeSubject)
	{
		subject = CMimeCommon::mimeDecoder(value,m_MimeBuffer);
	}
	else if(key == MimeDate)
	{
		try
		{
			date = handleDate(value);
		}
		catch(...)
		{
			date = "";
		}
	}
	else if(key == MimeContentType)		//�˴�δ����Content-type���ķ��������
	{
		//contentType = value;
		//handleContentType(value);
		boundary.setContentType(value);
		boundary.handleContentType();

	}
	else if(key == MimeContentEncoder)
	{
		//contentEncoder = value;
		boundary.setContentEncoder(value);
	}
	else if(key == MimeContentDispostion)
	{
		boundary.setContentDisposition(value);
		boundary.handleContentDisposition();
	}
	//2011-07-12
	else if(key == MimeRcptTo)
	{
		size_t index = 0;
		index = value.rfind("DATA");
		if(index != string::npos)
		{
			value = value.substr(0, index);		//ȥ��RCPT�е�"DATA"
		}
		rcptToList.push_back(value);
	}
	//2011-07-12
}

string CMimeDataHeader::GetToFromRcptToList()
{
	string result = "";
	for(size_t i = 0; i < rcptToList.size(); i++)
	{
		result += rcptToList[i];
		if(i != rcptToList.size() - 1)
		{
			result += ";";
		}
	}
	return result;
}

void CMimeDataHeader::clearMimeDataHeader()
{
	date.clear();
	from.clear();
	to.clear();
	cc.clear();
	bcc.clear();
	subject.clear();
	//contentType.clear();
	//contentEncoder.clear();
	//charSet.clear();
	boundary.clear();
}

string CMimeDataHeader::handleDate(const std::string &date)
{
	if(date.empty())
	{
		return "";
	}
	static char *month[12] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
	
	string result;
	int index = 0;
	int begin = 0;
	int size = 0;
	vector<string> list;
	bool flag = true;
	while(flag)
	{
		index = static_cast<int>(date.find(" ", begin));
		if(index == string::npos)
		{
			index = static_cast<int>(date.size());
			flag = false;
		}
		list.push_back(date.substr(begin, index - begin));
		begin = index + static_cast<int>(strlen(" "));
	}
	if(!list.size())
	{
		return ""; 
	}
	vector<string>::iterator iter = list.begin();
	size = list.size();
	if(size < 3 || size == 3)
	{
		for(int j = 1; j < size; j++)
		{
			result.append(*iter);
			if(j == size -1)
			{
				return result;
			}
			result.append(" ");
			*iter++;
		}
		return *iter;
	}
	iter += 3;
	result += *iter--;
	result += "-";
	for(int i = 0; i < 12; i++)
	{
		if(CMimeCommon::lowerToUpper(*iter) == month[i])
		{
			index = i + 1;
			break;
		}
	}
	stringstream ss;
	ss << index;
	result += ss.str();
	result += "-";
	iter--;
	result += *iter;
	result += " ";
	iter += 3;
	result += *iter;
	return result;
}
