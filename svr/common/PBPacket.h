#pragma once
#include <google/protobuf/message.h>
#include <google/protobuf/text_format.h>

#include "poker_msg.pb.h"
#include "global.h"
#include "Packet.h"
#include "WSPacket.h"

class CWSPBInputPacket;

class CPBInputPacket : public InputPacket
{
public:
	CPBInputPacket(const char * buf,int bsize)
	{
		InputPacket(buf,bsize);
	}
	CPBInputPacket(){};
public:
	bool DecodeProtoHead(PBHead & head);
	bool DecodeProtoMsg(PBCSMsg & msg);
	bool DecodeProtoRoute(PBRoute & route);
	bool CoverFrom(CWSPBInputPacket & ws_input);
};

class CPBOutputPacket : public OutputPacket
{
public:
	bool EncodeProtoHead(const PBHead & head);
	bool EncodeProtoMsg(const PBCSMsg & msg);
	bool EnCodeProtoRoute(const PBRoute & route);
	bool EnCodeProtoPBInpuf(CPBInputPacket & input);
};

class CWSPBInputPacket : public WSInputPacket
{
public:
	CWSPBInputPacket(const char * buf, int bsize)
	{
		InputPacket(buf, bsize);
	}
	CWSPBInputPacket() {};
public:
	bool DecodeProtoHead(PBHead & head);
	bool DecodeProtoMsg(PBCSMsg & msg);
	bool DecodeProtoRoute(PBRoute & route);
};

class CWSPBOutputPacket : public WSOutputPacket
{
public:
	bool EncodeProtoHead(const PBHead & head);
	bool EncodeProtoMsg(const PBCSMsg & msg);
	bool EnCodeProtoRoute(const PBRoute & route);
	bool EnCodeProtoPBInpuf(CPBInputPacket & input);
};
