#pragma once

#include <assert.h>
#include <arpa/inet.h>
#include <string.h>
#include <string>

#define	WS_TCP_DEFAULT_BUFFER	1024*400

/*
0                   1                   2                   3
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-------+-+-------------+-------------------------------+
|F|R|R|R| opcode|M| Payload len |    Extended payload length    |
|I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
|N|V|V|V|       |S|             |   (if payload len==126/127)   |
| |1|2|3|       |K|             |                               |
+-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
|     Extended payload length continued, if payload len == 127  |
+ - - - - - - - - - - - - - - - +-------------------------------+
|                               |Masking-key, if MASK set to 1  |
+-------------------------------+-------------------------------+
| Masking-key (continued)       |          Payload Data         |
+-------------------------------- - - - - - - - - - - - - - - - +
:                     Payload Data continued ...                :
+ - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
|                     Payload Data continued ...                |
+---------------------------------------------------------------+
*/
typedef unsigned long long  ULONG64;

#define MAX_UNSIGNED_SHORT 65535

#pragma pack(1)
struct  WSPacketHeader
{
	unsigned char option;
	unsigned char len;
	union {
		unsigned short extern_short_len;
		ULONG64 extern_int_len;
	};
	unsigned char masking_key[4];
};
#pragma pack()

#pragma pack(1)
struct  WSCPacketHeader
{
	unsigned char option;
	unsigned char len;
	unsigned char masking_key[4];
};
#pragma pack()

#pragma pack(1)
struct  WSSPacketHeader
{
	unsigned char option;
	unsigned char len;
	unsigned short extern_short_len;	
	unsigned char masking_key[4];
};
#pragma pack()

#pragma pack(1)
struct  WSIPacketHeader
{
	unsigned char option;
	unsigned char len;
	ULONG64 extern_int_len;
	unsigned char masking_key[4];
};
#pragma pack()

class WSPacketBase
{
public:

	enum
	{
		PACKET_HEADER_SIZE = sizeof(WSPacketHeader),
		PACKET_BUFFER_SIZE = WS_TCP_DEFAULT_BUFFER
	};

	ULONG64 htonl64(ULONG64 i64_host)
	{
		ULONG64 i64_net;        
		unsigned int i32_host_h;
		unsigned int i32_host_l;

		i32_host_l = i64_host & 0xffffffff;
		i32_host_h = (i64_host >> 32) & 0xffffffff;

		i64_net = htonl(i32_host_l);
		i64_net = (i64_net << 32) | htonl(i32_host_h);
		return i64_net;
	}

	ULONG64 ntohl64(ULONG64 i64_net)
	{
		ULONG64 i64_host;        
		unsigned int i32_net_h;
		unsigned int i32_net_l;

		i32_net_l = i64_net & 0xffffffff;
		i32_net_h = (i64_net >> 32) & 0xffffffff;

		i64_host = ntohl(i32_net_l);
		i64_host = (i64_host << 32) | ntohl(i32_net_h);
		return i64_host;
	}

	WSPacketBase(void){_reset();}

	char *packet_buf(void)	{return m_strBuf;}
	int packet_size(void)	{return m_nPacketSize;}

	int calc_head_len()
	{
		if (m_nPacketSize < 2)
		{
			return 2;
		}
		else
		{
			WSPacketHeader * phead = (WSPacketHeader *)m_strBuf;
			int unmasklen = phead->len & 0x7f;
			bool hasMask = (phead->len >> 7) > 0;
			int maskkeysize = hasMask ? 4 : 0;
			//unsigned int pkglen = ntohs(unmasklen)
			unsigned int pkglen = unmasklen;
			if (pkglen < 126)
			{
				return 2+ maskkeysize;
			}
			else if (pkglen == 126)
			{
				return 4+ maskkeysize;
			}
			else {
				return 10+ maskkeysize;
			}
		}
		return 10;
	}
	int calc_payload_len()
	{
		WSPacketHeader * phead = (WSPacketHeader *)m_strBuf;
		int unmasklen = phead->len & 0x7f;
		//unsigned int pkglen = ntohs(unmasklen)
		unsigned int pkglen = unmasklen;
		if (pkglen < 126)
		{
			return pkglen;
		}
		else if (pkglen == 126)
		{
			unsigned int pkglen = ntohs(phead->extern_short_len);
			return pkglen;
		}
		else {
			unsigned int pkglen = ntohl64(phead->extern_int_len);
			return pkglen;
		}
	}
	int calc_payload_offset()
	{
		return calc_head_len();
	}
protected:
	////////////////////////////////////////////////////////////////////////////////
	bool _copy(const void *pInBuf, int nLen)
	{
		if(nLen > PACKET_BUFFER_SIZE)
			return false;
		_reset();
		memcpy(m_strBuf, pInBuf, nLen);
		m_nPacketSize = nLen;
		m_nBufPos = calc_head_len();
		WSPacketHeader * phead = (WSPacketHeader *)m_strBuf;
		int unmasklen = phead->len & 0x7f;
		if (unmasklen < 126)
		{
			WSCPacketHeader * phead = (WSCPacketHeader *)m_strBuf;
			bool hasMask = (phead->len >> 7) > 0;
			if (hasMask)
			{
				int payloadlen = calc_payload_len();
				int payloadoffset = calc_payload_offset();
				for (int i = 0; i < payloadlen; i++)
				{
					m_strBuf[payloadoffset + i] = (char)m_strBuf[payloadoffset + i] ^ phead->masking_key[i % 4];
				}
			}
		}
		else if (unmasklen == 126)
		{
			WSSPacketHeader * phead = (WSSPacketHeader *)m_strBuf;
			bool hasMask = (phead->len >> 7) > 0;
			if (hasMask)
			{
				int payloadlen = calc_payload_len();
				int payloadoffset = calc_payload_offset();
				for (int i = 0; i < payloadlen; i++)
				{
					m_strBuf[payloadoffset + i] = (char)m_strBuf[payloadoffset + i] ^ phead->masking_key[i % 4];
				}
			}
		}
		else if (unmasklen == 127)
		{
			WSIPacketHeader * phead = (WSIPacketHeader *)m_strBuf;
			bool hasMask = (phead->len >> 7) > 0;
			if (hasMask)
			{
				int payloadlen = calc_payload_len();
				int payloadoffset = calc_payload_offset();
				for (int i = 0; i < payloadlen; i++)
				{
					m_strBuf[payloadoffset + i] = (char)m_strBuf[payloadoffset + i] ^ phead->masking_key[i % 4];
				}
			}
		}
		else
		{
			return false;
		}
		return true;
	}
	////////////////////////////////////////////////////////////////////////////////
	void _reset(void)
	{
		memset(m_strBuf, 0, PACKET_BUFFER_SIZE);
		m_nBufPos = PACKET_HEADER_SIZE;
		m_nPacketSize = PACKET_HEADER_SIZE;
	}
	// 
	bool _Read(char *pOut, int nLen)
	{
		if((nLen + m_nBufPos) > m_nPacketSize || (nLen + m_nBufPos) > PACKET_BUFFER_SIZE)
			return false ;

		memcpy(pOut, m_strBuf + m_nBufPos, nLen);
		m_nBufPos += nLen;
		return true;
	}
	// 
	void _readundo(int nLen)
	{
		m_nBufPos -= nLen;
	}
	//
	char *_readpoint(int nLen)  
	{
		if((nLen + m_nBufPos) > m_nPacketSize)
			return NULL; 
		char *p = &m_strBuf[m_nBufPos];
		m_nBufPos += nLen;
		return p;

	}
	//  
	bool _Write(const char *pIn, int nLen)
	{
		if((m_nPacketSize < 0) || ((nLen + m_nPacketSize) > PACKET_BUFFER_SIZE))
			return false ;
		memcpy(m_strBuf+m_nPacketSize, pIn, nLen);
		m_nPacketSize += nLen;
		return true;
	}
	// 
	bool _writezero(void)
	{
		if((m_nPacketSize + 1) > PACKET_BUFFER_SIZE)
			return false ;
		memset(m_strBuf+m_nPacketSize, '\0', sizeof(char)) ;
		m_nPacketSize ++;
		return true;
	}
	//
	void _writeHeader(char *pIn, int nLen, int nPos)
	{
		if(nPos > 0 || nPos+nLen < PACKET_HEADER_SIZE)
		{
			memcpy(m_strBuf+nPos, pIn, nLen) ;
		}
	}
	//
	void _end()
	{
		int nBodyLen = m_nPacketSize;
		unsigned char option = 0x82;
		int nHeaderLen = 4;
		unsigned char len = 126;
		if (nBodyLen < 126)
		{
			nHeaderLen = 2;
			len = nBodyLen;
		}
		else if (nBodyLen > MAX_UNSIGNED_SHORT)
		{
			nHeaderLen = 10;
			len = 127;
		}
		memmove(m_strBuf + nHeaderLen, m_strBuf, nBodyLen);
		len &= 0x7f;
		_writeHeader((char*)&option, sizeof(char), 0);
		_writeHeader((char*)&len, sizeof(char), 1);	//
		if (nHeaderLen == 4)
		{
			unsigned short extern_short_len = htons(m_nPacketSize);
			_writeHeader((char*)&extern_short_len, sizeof(extern_short_len), 2);	// 
		}
		else if (nHeaderLen == 10)
		{
			ULONG64 extern_ulong_len = htonl64(m_nPacketSize);
			_writeHeader((char*)&extern_ulong_len, sizeof(extern_ulong_len), 2);	// 
		}
		m_nPacketSize = nHeaderLen + nBodyLen;
	}
public:
	char m_strBuf[PACKET_BUFFER_SIZE]; 
	int m_nPacketSize ;
	int m_nBufPos;
};

class WSInputPacket: public WSPacketBase
{
public:
	typedef WSPacketBase base;

	int ReadInt(void)		{int nValue = 0; base::_Read((char*)&nValue, sizeof(int)); return ntohl(nValue);} //?
	unsigned long ReadULong(void) {unsigned long nValue = 0; base::_Read((char*)&nValue, sizeof(unsigned long)); return ntohl(nValue);}
	short ReadShort(void)	{short nValue = 0; base::_Read((char*)&nValue, sizeof(short)); return ntohs(nValue);}
	unsigned char ReadByte(void)		{unsigned char nValue = 0; base::_Read((char*)&nValue, sizeof(unsigned char)); return nValue;}

	WSInputPacket(){};
	WSInputPacket(const char* data, int len)
	{
		base::_copy(data, len);
	};

	bool ReadString(char *pOutString, int nMaxLen)
	{
		int nLen = ReadInt();
		if(nLen == -1)  //
			return false;
		if(nLen > nMaxLen)
		{
			base::_readundo(sizeof(short));
			return false;
		}
		return base::_Read(pOutString, nLen);
	}

	char *ReadChar(void)
	{
		int nLen = ReadInt();

		if(nLen == -1) 
			return NULL;
		return base::_readpoint(nLen);
	}

	std::string ReadString(void)
	{
		char *p = ReadChar();
		return (p == NULL ? "" : p);
	}

	int ReadBinary(char *pBuf, int nMaxLen)
	{
		int nLen = ReadInt();
		if(nLen < 0) 
			return -1;
		if(nLen > nMaxLen)
		{
			base::_readundo(sizeof(int));
			return -2;
		}
		if(base::_Read(pBuf, nLen))
			return nLen ;
		return 0;
	}

	void Reset(void)
	{
		base::_reset();
	}
	bool Copy(const void *pInBuf, int nLen)
	{
		return base::_copy(pInBuf, nLen);
	}
	bool WriteBody(const char *pIn, int nLen)
	{
		return base::_Write(pIn, nLen);
	}
};


class WSOutputPacket: public WSPacketBase
{
public:
	WSOutputPacket(void)
	{ 
		m_nBufPos = 0;
		m_nPacketSize = 0;
	}

	typedef WSPacketBase base;

	bool WriteInt(int nValue)		{int value = htonl(nValue); return base::_Write((char*)&value, sizeof(int));}
	bool WriteULong(unsigned long nValue) {unsigned long value = htonl(nValue);return base::_Write((char*)&value, sizeof(unsigned long));}
	bool WriteByte(unsigned char nValue)		{return base::_Write((char*)&nValue, sizeof(unsigned char));}
	bool WriteShort(short nValue)	{short value = htons(nValue); return base::_Write((char*)&value, sizeof(short));}
	bool WriteString(const char *pString)
	{
		int nLen = (int)strlen(pString) ;
		WriteInt(nLen + 1) ;
		return base::_Write(pString, nLen) && base::_writezero();
	}

	bool WriteString(const std::string &strDate)
	{
		int nLen = (int)strDate.size();
		WriteInt(nLen + 1) ;
		return base::_Write(strDate.c_str(), nLen) && base::_writezero();
	}

	bool WriteBinary(const char *pBuf, int nLen)
	{
		WriteInt(nLen) ;
		return base::_Write(pBuf, nLen) ;
	}
	bool Copy(const void *pInBuf, int nLen)
	{
		return base::_copy(pInBuf, nLen);
	}
	void End()
	{
		return base::_end();
	}
};


