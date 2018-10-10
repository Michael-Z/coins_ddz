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
	//不足一个包头，继续接收
	if (len < this->GetHeadLen(data, len))
		return 0;

	//满足一个包头，判断包头合法性
	if (!this->CheckHead(data, len))
		return -1;

	//满足一个包头，判断整包长合法性
	int pkglen = this->GetPacketLen(data, len);
	if (pkglen<0)
	{
		return -1;
	}
	//包体未接收完整
	if (len < pkglen + this->GetHeadLen(data, len))
	{
		return 0;
	}
	//收到一个完整包
	return pkglen + this->GetHeadLen(data, len);
}

