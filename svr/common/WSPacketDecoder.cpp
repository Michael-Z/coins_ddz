#include "WSPacketDecoder.h"
#include "WSPacket.h"


int CWSPacketDecoder::GetHeadLen(const char * data, int len)
{
	if (len < 2)
	{
		return 2;
	}
	else
	{
		WSPacketHeader * phead = (WSPacketHeader *)data;
		int unmasklen = phead->len & 0x7f;
		//unsigned int pkglen = ntohs(unmasklen);
		bool hasMask = (phead->len >> 7) > 0;
		int maskkeysize = hasMask ? 4 : 0;
		unsigned int pkglen = unmasklen;
		if (pkglen < 126)
		{
			return 2 + maskkeysize;
		}
		else if(pkglen == 126)
		{
			return 4 + maskkeysize;
		}
		else {
			return 10 + maskkeysize;
		}
	}
	return 10;
}

bool CWSPacketDecoder::CheckHead(const char * data, int len)
{
	return true;
}

int CWSPacketDecoder::GetPacketLen(const char * data, int len)
{
	(void)len;
	struct WSPacketHeader * pHead = (struct WSPacketHeader *)data;
	WSPacketHeader * phead = (WSPacketHeader *)data;
	int unmasklen = phead->len & 0x7f;
	//unsigned int pkglen = ntohs(unmasklen);
	unsigned int pkglen = unmasklen;
	if (pkglen < 126)
	{
		//fatal error
		return pkglen;
	}
	else if (pkglen == 126)
	{
		unsigned int pkglen = ntohs(pHead->extern_short_len);
		return pkglen;
	}
	else
	{
		unsigned int pkglen = ntohl(pHead->extern_int_len);
		return pkglen;
	}
}

int CWSPacketDecoder::ParsePacket(char * data, int len)
{
	//����һ����ͷ����������
	if (len < this->GetHeadLen(data, len))
		return 0;

	//����һ����ͷ���жϰ�ͷ�Ϸ���
	if (!this->CheckHead(data, len))
		return -1;

	//����һ����ͷ���ж��������Ϸ���
	int pkglen = this->GetPacketLen(data, len);
	if (pkglen<0)
	{
		return -1;
	}
	//����δ��������
	if (len < pkglen + this->GetHeadLen(data, len))
	{
		return 0;
	}
	//�յ�һ��������
	return pkglen + this->GetHeadLen(data, len);
}

