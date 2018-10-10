#pragma once

#include <assert.h>
#include <arpa/inet.h>
#include <string.h>
#include <string>

#define	TCP_DEFAULT_BUFFER	1024*400

#pragma pack(1)
struct  PacketHeader
{
	unsigned int pack_len;     
	unsigned char soh[2];
};
#pragma pack()


class PacketBase
{
public:

	enum
	{
		PACKET_HEADER_SIZE = sizeof(PacketHeader),
		PACKET_BUFFER_SIZE = TCP_DEFAULT_BUFFER
	};

	PacketBase(void){_reset();}

	char *packet_buf(void)	{return m_strBuf;}
	int packet_size(void)	{return m_nPacketSize;}

protected:
	////////////////////////////////////////////////////////////////////////////////
	bool _copy(const void *pInBuf, int nLen)
	{
		if(nLen > PACKET_BUFFER_SIZE)
			return false;

		_reset();
		memcpy(m_strBuf, pInBuf, nLen);
		m_nPacketSize = nLen;
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
		int nBody = m_nPacketSize - sizeof(int);	//
		int len = htonl(nBody);		
		_writeHeader((char*)&len, sizeof(int), 0);	//
		char soh[2] = {'X','X'};
		_writeHeader((char*)&soh, sizeof(soh), sizeof(int));	// 
	}
private:
	char m_strBuf[PACKET_BUFFER_SIZE]; 
	int m_nPacketSize ;
	int m_nBufPos;
};

class InputPacket: public PacketBase
{
public:
	typedef PacketBase base;

	int ReadInt(void)		{int nValue = 0; base::_Read((char*)&nValue, sizeof(int)); return ntohl(nValue);} //?
	unsigned long ReadULong(void) {unsigned long nValue = 0; base::_Read((char*)&nValue, sizeof(unsigned long)); return ntohl(nValue);}
	short ReadShort(void)	{short nValue = 0; base::_Read((char*)&nValue, sizeof(short)); return ntohs(nValue);}
	unsigned char ReadByte(void)		{unsigned char nValue = 0; base::_Read((char*)&nValue, sizeof(unsigned char)); return nValue;}

	InputPacket(){};
	InputPacket (const char* data, int len)
	{
		base::_copy(data, len);
	};

	bool ReadString(char *pOutString, int nMaxLen)
	{
		int nLen = ReadInt();
		if(nLen == -1)  //ж
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
		int value = htonl(nLen);
		base::_Write((char*)&value, sizeof(int));
		return base::_Write(pIn, nLen);
	}
	void End()
	{
		return base::_end();
	}
};


class OutputPacket: public PacketBase
{
public:
	OutputPacket(void){}

	typedef PacketBase base;

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


