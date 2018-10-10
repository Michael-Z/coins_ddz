#include "PacketDecoder.h"
#include "Packet.h"


int CPacketDecoder::GetHeadLen(const char * data, int len)
{
	(void)len;
	return sizeof(PacketHeader);
}

bool CPacketDecoder::CheckHead(const char * data, int len)
{
	(void)len;
	struct PacketHeader * pHeader = (struct PacketHeader *) data;
	if ((pHeader->soh[0] == 'X' && pHeader->soh[1] == 'X') ||
        (pHeader->soh[0] == 'y' && pHeader->soh[1] == 'y'))
	{
		return true;
	}
	else
	{
		return false;
	}
}

int CPacketDecoder::GetPacketLen(const char * data, int len)
{
	(void)len;
	struct PacketHeader * pHead = (struct PacketHeader *)data;
	unsigned int pkglen = sizeof(int) + ntohl(pHead->pack_len);
	if (pkglen < sizeof(PacketHeader) || pkglen > 1024*400 )
	{
		//fatal error
		return -1;
	}
	else
	{
		return pkglen;
	}
}

