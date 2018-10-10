#pragma once 
#include "DefaultStreamDecoder.h"

class CWSPacketDecoder : public CDefaultStreamDecoder
{
	protected :
		virtual int GetHeadLen(const char * data, int len);
		virtual bool CheckHead(const char * data, int len);
		virtual int GetPacketLen(const char * data, int len);
		virtual int ParsePacket(char * data, int len);
};

