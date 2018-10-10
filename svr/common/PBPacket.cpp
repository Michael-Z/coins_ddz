#include "PBPacket.h"

const static int PROTO_BUFF_SIZE = 1024*400;

bool CPBInputPacket::DecodeProtoHead(PBHead & head)
{
	char buff[PROTO_BUFF_SIZE] = {0};
	int iret = ReadBinary(buff,PROTO_BUFF_SIZE);
	if (iret <= 0)
	{
		ErrMsg("read binary failed.iret : %d",iret);
		return false;
	}
	if (!head.ParseFromArray(buff,iret))
	{
		ErrMsg("ProtoBuff Parse From Array Failed.");
		return false;
	}

	std::string strProto;
    google::protobuf::TextFormat::PrintToString(head, &strProto);
    strProto = "\n---Decode Proto Head---\n" + strProto;
    ProtoMsg("%s", strProto.c_str());

	return true;
}

bool CPBInputPacket::DecodeProtoMsg(PBCSMsg & msg)
{	
	char buff[PROTO_BUFF_SIZE] = {0};
	int iret = ReadBinary(buff,PROTO_BUFF_SIZE);
	if (iret <= 0)
	{
		ErrMsg("read binary failed.iret : %d",iret);
		return false;
	}
	if (!msg.ParseFromArray(buff,iret))
	{
		ErrMsg("ProtoBuff Parse From Array Failed.");
		return false;
	}

	std::string strProto;
    google::protobuf::TextFormat::PrintToString(msg, &strProto);
    strProto = "\n---Decode Proto Msg---\n" + strProto;
    ProtoMsg("%s", strProto.c_str());

	return true;
}

bool CPBInputPacket::DecodeProtoRoute(PBRoute & route)
{	
	char buff[PROTO_BUFF_SIZE] = {0};
	int iret = ReadBinary(buff,PROTO_BUFF_SIZE);
	if (iret <= 0)
	{
		//ErrMsg("read binary failed.iret : %d",iret);
		return false;
	}
	if (!route.ParseFromArray(buff,iret))
	{
		ErrMsg("ProtoBuff Parse From Array Failed.");
		return false;
	}

	std::string strProto;
    google::protobuf::TextFormat::PrintToString(route, &strProto);
    strProto = "\n---Decode Proto Route---\n" + strProto;
    ProtoMsg("%s", strProto.c_str());

	return true;
}

bool CPBInputPacket::CoverFrom(CWSPBInputPacket & ws_input)
{
	char buff[PROTO_BUFF_SIZE] = { 0 };
	int iret = ws_input.ReadBinary(buff, PROTO_BUFF_SIZE);
	if (iret <= 0)
	{
		ErrMsg("read binary failed.iret : %d", iret);
		return false;
	}
	WriteBody(buff, iret);
	End();
	return true;
}

bool CPBOutputPacket::EncodeProtoHead(const PBHead & head)
{	
	char buff[PROTO_BUFF_SIZE] = {0};
	int size = head.ByteSize();
	if (size > PROTO_BUFF_SIZE)
	{
		ErrMsg("invalid byte size[%d].",size);
		return false;
	}
	head.SerializeToArray(buff,size);
	WriteBinary(buff,size);

	std::string strProto;
    google::protobuf::TextFormat::PrintToString(head, &strProto);
    strProto = "\n---Encode Proto Head---\n" + strProto;
    ProtoMsg("%s", strProto.c_str());

	return true;
}

bool CPBOutputPacket::EncodeProtoMsg(const PBCSMsg & msg)
{
	char buff[PROTO_BUFF_SIZE] = {0};
	int size = msg.ByteSize();
	if (size > PROTO_BUFF_SIZE)
	{
		ErrMsg("invalid byte size[%d].",size);
		return false;
	}
	msg.SerializeToArray(buff,size);
	WriteBinary(buff,size);

	std::string strProto;
    google::protobuf::TextFormat::PrintToString(msg, &strProto);
    strProto = "\n---Encode Proto Msg---\n" + strProto;
    ProtoMsg("%s", strProto.c_str());

	return true;
}

bool CPBOutputPacket::EnCodeProtoRoute(const PBRoute & route)
{
	char buff[PROTO_BUFF_SIZE] = {0};
	int size = route.ByteSize();
	if (size > PROTO_BUFF_SIZE)
	{
		ErrMsg("invalid byte size[%d].",size);
		return false;
	}
	route.SerializeToArray(buff,size);
	WriteBinary(buff,size);

	std::string strProto;
    google::protobuf::TextFormat::PrintToString(route, &strProto);
    strProto = "\n---Encode Proto Route---\n" + strProto;
    ProtoMsg("%s", strProto.c_str());

	return true;
}

bool CPBOutputPacket::EnCodeProtoPBInpuf(CPBInputPacket & input)
{
	char buff[PROTO_BUFF_SIZE] = {0};
	int iret = input.ReadBinary(buff,PROTO_BUFF_SIZE);
	if (iret <= 0)
	{
		ErrMsg("read binary failed.iret : %d",iret);
		return false;
	}
	WriteBinary(buff,iret);
	return true;
}

bool CWSPBInputPacket::DecodeProtoHead(PBHead & head)
{
	char buff[PROTO_BUFF_SIZE] = { 0 };
	int iret = ReadBinary(buff, PROTO_BUFF_SIZE);
	if (iret <= 0)
	{
		ErrMsg("read binary failed.iret : %d", iret);
		return false;
	}
	if (!head.ParseFromArray(buff, iret))
	{
		ErrMsg("ProtoBuff Parse From Array Failed.");
		return false;
	}

	std::string strProto;
	google::protobuf::TextFormat::PrintToString(head, &strProto);
	strProto = "\n---Decode Proto Head---\n" + strProto;
	ProtoMsg("%s", strProto.c_str());

	return true;
}

bool CWSPBInputPacket::DecodeProtoMsg(PBCSMsg & msg)
{
	char buff[PROTO_BUFF_SIZE] = { 0 };
	int iret = ReadBinary(buff, PROTO_BUFF_SIZE);
	if (iret <= 0)
	{
		ErrMsg("read binary failed.iret : %d", iret);
		return false;
	}
	if (!msg.ParseFromArray(buff, iret))
	{
		ErrMsg("ProtoBuff Parse From Array Failed.");
		return false;
	}

	std::string strProto;
	google::protobuf::TextFormat::PrintToString(msg, &strProto);
	strProto = "\n---Decode Proto Msg---\n" + strProto;
	ProtoMsg("%s", strProto.c_str());

	return true;
}

bool CWSPBInputPacket::DecodeProtoRoute(PBRoute & route)
{
	char buff[PROTO_BUFF_SIZE] = { 0 };
	int iret = ReadBinary(buff, PROTO_BUFF_SIZE);
	if (iret <= 0)
	{
		//ErrMsg("read binary failed.iret : %d",iret);
		return false;
	}
	if (!route.ParseFromArray(buff, iret))
	{
		ErrMsg("ProtoBuff Parse From Array Failed.");
		return false;
	}

	std::string strProto;
	google::protobuf::TextFormat::PrintToString(route, &strProto);
	strProto = "\n---Decode Proto Route---\n" + strProto;
	ProtoMsg("%s", strProto.c_str());

	return true;
}

bool CWSPBOutputPacket::EncodeProtoHead(const PBHead & head)
{
	char buff[PROTO_BUFF_SIZE] = { 0 };
	int size = head.ByteSize();
	if (size > PROTO_BUFF_SIZE)
	{
		ErrMsg("invalid byte size[%d].", size);
		return false;
	}
	head.SerializeToArray(buff, size);
	WriteBinary(buff, size);

	std::string strProto;
	google::protobuf::TextFormat::PrintToString(head, &strProto);
	strProto = "\n---Encode Proto Head---\n" + strProto;
	ProtoMsg("%s", strProto.c_str());

	return true;
}

bool CWSPBOutputPacket::EncodeProtoMsg(const PBCSMsg & msg)
{
	char buff[PROTO_BUFF_SIZE] = { 0 };
	int size = msg.ByteSize();
	if (size > PROTO_BUFF_SIZE)
	{
		ErrMsg("invalid byte size[%d].", size);
		return false;
	}
	msg.SerializeToArray(buff, size);
	WriteBinary(buff, size);

	std::string strProto;
	google::protobuf::TextFormat::PrintToString(msg, &strProto);
	strProto = "\n---Encode Proto Msg---\n" + strProto;
	ProtoMsg("%s", strProto.c_str());

	return true;
}

bool CWSPBOutputPacket::EnCodeProtoRoute(const PBRoute & route)
{
	char buff[PROTO_BUFF_SIZE] = { 0 };
	int size = route.ByteSize();
	if (size > PROTO_BUFF_SIZE)
	{
		ErrMsg("invalid byte size[%d].", size);
		return false;
	}
	route.SerializeToArray(buff, size);
	WriteBinary(buff, size);

	std::string strProto;
	google::protobuf::TextFormat::PrintToString(route, &strProto);
	strProto = "\n---Encode Proto Route---\n" + strProto;
	ProtoMsg("%s", strProto.c_str());

	return true;
}

bool CWSPBOutputPacket::EnCodeProtoPBInpuf(CPBInputPacket & input)
{
	char buff[PROTO_BUFF_SIZE] = { 0 };
	int iret = input.ReadBinary(buff, PROTO_BUFF_SIZE);
	if (iret <= 0)
	{
		ErrMsg("read binary failed.iret : %d", iret);
		return false;
	}
	WriteBinary(buff, iret);
	return true;
}


