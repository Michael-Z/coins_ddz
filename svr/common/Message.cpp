#include "Message.h"

void Message::SendRequestMsg(CTCPSocketHandler* phandler, CSession* psession, const PBCSMsg& msg, int node_type, int des_id , ENRouteType route_type)
{
    if (!phandler)
    {
		ErrMsg("phandler = NULL.");
        return;
    }
    // route info 
    PBRoute route;
	route.set_destination(node_type);
	route.set_des_id(des_id);
	route.set_source(TGlobal::_svrd_type);
	route.set_source_id(TGlobal::_svid);
	route.set_mtype(EN_Message_Request);
	route.set_session_id(psession->sessionid());
	route.set_uid(psession->_uid);
	route.set_route_type(route_type);
	route.set_groupid(TGlobal::_group_id);
	psession->_head.set_cmd(msg.msg_union_case());
    // encode packet
	CPBOutputPacket output;
	output.EnCodeProtoRoute(route);
    output.EncodeProtoHead(psession->_head);
	output.EncodeProtoMsg(msg);
	output.End();

    // send
	phandler->Send(output.packet_buf(),output.packet_size());

    // timer
    psession->StartSessionTimer(5);
}

void Message::SendRequestMsgAsClient(CTCPSocketHandler* phandler,const PBCSMsg& msg)
{
	if (!phandler)
    {
		ErrMsg("phandler = NULL.");
        return;
    }
    // route info 
    // encode packet
    PBHead head;
	head.set_channel_id(999);
	head.set_cmd(msg.msg_union_case());
	CPBOutputPacket output;
	output.EncodeProtoHead(head);
	output.EncodeProtoMsg(msg);
	output.End();
    // send
	phandler->Send(output.packet_buf(),output.packet_size());
}

void Message::PushMsg(CTCPSocketHandler* phandler,long long uid,const PBCSMsg& msg, int node_type, int des_id, ENRouteType route_type)
{
    if (!phandler)
    {
		ErrMsg("phandler = NULL.");
        return;
    }
    // route info 
  	PBRoute route;
	route.set_destination(node_type);
	route.set_des_id(des_id);
	route.set_source(TGlobal::_svrd_type);
	route.set_source_id(TGlobal::_svid);
	route.set_mtype(EN_Message_Push);
	route.set_uid(uid);
	route.set_route_type(route_type);
	route.set_groupid(TGlobal::_group_id);
    // encode packet
    CPBOutputPacket output;
    PBHead head;
	head.set_cmd(msg.msg_union_case());
	head.set_proto_version(0);
	output.EnCodeProtoRoute(route);
	output.EncodeProtoHead(head);
	output.EncodeProtoMsg(msg);
	output.End();

    // send
	phandler->Send(output.packet_buf(),output.packet_size());
}

void Message::PushInnerMsg(CTCPSocketHandler* phandler,long long uid,const PBCSMsg& msg, int node_type, int des_id, ENRouteType route_type)
{
    if (!phandler)
    {
		ErrMsg("phandler = NULL.");
        return;
    }
    // route info 
  	PBRoute route;
	route.set_destination(node_type);
	route.set_des_id(des_id);
	route.set_source(TGlobal::_svrd_type);
	route.set_source_id(TGlobal::_svid);
	route.set_mtype(EN_Message_Request);
	route.set_uid(uid);
	route.set_route_type(route_type);
	route.set_groupid(TGlobal::_group_id);
    // encode packet
    CPBOutputPacket output;
    PBHead head;
	head.set_cmd(msg.msg_union_case());
	head.set_proto_version(0);
	output.EnCodeProtoRoute(route);
	output.EncodeProtoHead(head);
	output.EncodeProtoMsg(msg);
	output.End();

    // send
	phandler->Send(output.packet_buf(),output.packet_size());
}


void Message::SendResponseMsg(CTCPSocketHandler* phandler, CSession* psession, const PBCSMsg& msg)
{
    if (!psession->has_request_sessionid())
    {
        // 无session id不回复
        return;
    }
    if (!phandler)
    {
		ErrMsg("phandler = NULL.");
        return;
    }

    // route info 
  	PBRoute route;
	route.set_destination(psession->_request_route.source());
	route.set_des_id(psession->_request_route.source_id());
	route.set_source(TGlobal::_svrd_type);
	route.set_source_id(TGlobal::_svid);
	route.set_mtype(EN_Message_Response);
	route.set_session_id(psession->request_sessionid());
	route.set_uid(psession->_uid);
	route.set_groupid(psession->_request_route.groupid());
	psession->_head.set_cmd(msg.msg_union_case());
    // encode packet
	CPBOutputPacket output;
	output.EnCodeProtoRoute(route);
	output.EncodeProtoHead(psession->_head);
	output.EncodeProtoMsg(msg);
	output.End();

    // send
	phandler->Send(output.packet_buf(),output.packet_size());
}

void Message::SendRequestToRoom(CTCPSocketHandler* phandler, CSession* psession, const PBCSMsg& msg, ENRouteType route_type)
{
    if (!phandler)
    {
		ErrMsg("phandler = NULL.");
        return;
    }
    // route info 
  	PBRoute route;
	route.set_destination(EN_Node_Room);
	route.set_des_id(1);
	route.set_source(TGlobal::_svrd_type);
	route.set_source_id(TGlobal::_svid);
	route.set_mtype(EN_Message_Request);
	route.set_session_id(psession->sessionid());
	route.set_uid(psession->_uid);
	route.set_route_type(route_type);
	route.set_groupid(TGlobal::_group_id);
	if(psession->msgtype() != EN_Node_Client)
	{
		psession->_head.set_cmd(msg.msg_union_case());
	}
    // encode packet
	CPBOutputPacket output;
	output.EnCodeProtoRoute(route);
	output.EncodeProtoHead(psession->_head);
	output.EncodeProtoMsg(msg);
	output.End();

    // send
	phandler->Send(output.packet_buf(),output.packet_size());

    // timer
    psession->StartSessionTimer(5);
}

void Message::SendRawRequestToRoom(CTCPSocketHandler* phandler, CSession* psession, ENRouteType route_type)
{
    if (!phandler)
    {
		ErrMsg("phandler = NULL.");
        return;
    }
    // route info 
  	PBRoute route;
	route.set_destination(EN_Node_Room);
	route.set_des_id(1);
	route.set_source(TGlobal::_svrd_type);
	route.set_source_id(TGlobal::_svid);
	route.set_mtype(EN_Message_Request);
	route.set_session_id(psession->sessionid());
	route.set_uid(psession->_uid);
	route.set_route_type(route_type);
	route.set_groupid(TGlobal::_group_id);
	
	CPBOutputPacket output;
	output.EnCodeProtoRoute(route);
	output.EncodeProtoHead(psession->_head);
	output.EnCodeProtoPBInpuf(*psession->_input_packet);
	output.End();

    // send
	phandler->Send(output.packet_buf(),output.packet_size());

    // timer
    psession->StartSessionTimer(5);
}


void Message::SendRequestToUser(CTCPSocketHandler* phandler, CSession* psession, const PBCSMsg& msg,int64 uid)
{
    if (!phandler)
    {
		ErrMsg("phandler = NULL.");
        return;
    }
    // route info 
  	PBRoute route;
	route.set_destination(EN_Node_User);
	route.set_des_id(1);
	route.set_source(TGlobal::_svrd_type);
	route.set_source_id(TGlobal::_svid);
	route.set_mtype(EN_Message_Request);
	route.set_session_id(psession->sessionid());
	if(uid != -1)
	{
		route.set_uid(uid);
	}
	else
	{
		route.set_uid(psession->_uid);
	}
	route.set_route_type(EN_Route_hash);
	route.set_groupid(TGlobal::_group_id);
	//
	PBHead head;
	head.CopyFrom(psession->_head);
	head.set_cmd(msg.msg_union_case());
    // encode packet
	CPBOutputPacket output;
	output.EnCodeProtoRoute(route);
	output.EncodeProtoHead(head);
	output.EncodeProtoMsg(msg);
	output.End();

    // send
	phandler->Send(output.packet_buf(),output.packet_size());

    // timer
    psession->StartSessionTimer(5);
}

void Message::SendRequestToGame(CTCPSocketHandler* phandler, CSession* psession, const PBCSMsg& msg)
{
    if (!phandler)
    {
		ErrMsg("phandler = NULL.");
        return;
    }
    // route info 
  	PBRoute route;
	route.set_destination(EN_Node_Game);
	route.set_des_id(1);
	route.set_source(TGlobal::_svrd_type);
	route.set_source_id(TGlobal::_svid);
	route.set_mtype(EN_Message_Request);
	route.set_session_id(psession->sessionid());
	route.set_uid(psession->_uid);
	route.set_groupid(TGlobal::_group_id);
	
	if(psession->msgtype() != EN_Node_Client)
	{
		psession->_head.set_cmd(msg.msg_union_case());
	}
    // encode packet
	CPBOutputPacket output;
	output.EnCodeProtoRoute(route);
	output.EncodeProtoHead(psession->_head);
	output.EncodeProtoMsg(msg);
	output.End();

    // send
	phandler->Send(output.packet_buf(),output.packet_size());

    // timer
    psession->StartSessionTimer(5);
}

void Message::SendRawRequestToGame(CTCPSocketHandler* phandler, CSession* psession,const PBBPlayerPositionInfo & pos_info)
{
    if (!phandler)
    {
		ErrMsg("phandler = NULL.");
        return;
    }
	if(psession->_input_packet == NULL)
	{
		ErrMsg("input packet is null");
		return;
	}
    // route info 
  	PBRoute route;
	route.set_destination(EN_Node_Game);
	route.set_des_id(pos_info.gamesvrd_id());
	route.set_source(TGlobal::_svrd_type);
	route.set_source_id(TGlobal::_svid);
	route.set_mtype(EN_Message_Request);
	route.set_session_id(psession->sessionid());
	route.set_uid(psession->_uid);
	route.set_groupid(TGlobal::_group_id);
	
    // encode packet
	CPBOutputPacket output;
	output.EnCodeProtoRoute(route);
	output.EncodeProtoHead(psession->_head);
	output.EnCodeProtoPBInpuf(*psession->_input_packet);
	output.End();

    // send
	phandler->Send(output.packet_buf(),output.packet_size());

    // timer
    psession->StartSessionTimer(5);
}

void Message::SendRawRequestToFpfGame(CTCPSocketHandler* phandler, CSession* psession,const PBBPlayerPositionInfo & pos_info)
{
    if (!phandler)
    {
        ErrMsg("phandler = NULL.");
        return;
    }
    if(psession->_input_packet == NULL)
    {
        ErrMsg("input packet is null");
        return;
    }
    // route info
    PBRoute route;
    route.set_destination(EN_Node_FPF);
    route.set_des_id(pos_info.gamesvrd_id());
    route.set_source(TGlobal::_svrd_type);
    route.set_source_id(TGlobal::_svid);
    route.set_mtype(EN_Message_Request);
    route.set_session_id(psession->sessionid());
    route.set_uid(psession->_uid);
    route.set_groupid(TGlobal::_group_id);

    // encode packet
    CPBOutputPacket output;
    output.EnCodeProtoRoute(route);
    output.EncodeProtoHead(psession->_head);
    output.EnCodeProtoPBInpuf(*psession->_input_packet);
    output.End();

    // send
    phandler->Send(output.packet_buf(),output.packet_size());

    // timer
    psession->StartSessionTimer(5);
}

void Message::SendRawRequestToPhzGame(CTCPSocketHandler* phandler, CSession* psession,const PBBPlayerPositionInfo & pos_info, int game_type)
{
    if (!phandler)
    {
        ErrMsg("phandler = NULL.");
        return;
    }
    if(psession->_input_packet == NULL)
    {
        ErrMsg("input packet is null");
        return;
    }
    // route info
    PBRoute route;
    route.set_destination(game_type);
    route.set_des_id(pos_info.gamesvrd_id());
    route.set_source(TGlobal::_svrd_type);
    route.set_source_id(TGlobal::_svid);
    route.set_mtype(EN_Message_Request);
    route.set_session_id(psession->sessionid());
    route.set_uid(psession->_uid);
    route.set_groupid(TGlobal::_group_id);

    // encode packet
    CPBOutputPacket output;
    output.EnCodeProtoRoute(route);
    output.EncodeProtoHead(psession->_head);
    output.EnCodeProtoPBInpuf(*psession->_input_packet);
    output.End();

    // send
    phandler->Send(output.packet_buf(),output.packet_size());

    // timer
    psession->StartSessionTimer(5);
}

void Message::SendRequestToHall(CTCPSocketHandler* phandler, CSession* psession, const PBCSMsg& msg, ENRouteType route_type)
{
    if (!phandler)
    {
		ErrMsg("phandler = NULL.");
        return;
    }
    // route info 
  	PBRoute route;
	route.set_destination(EN_Node_Hall);
	route.set_des_id(1);
	route.set_source(TGlobal::_svrd_type);
	route.set_source_id(TGlobal::_svid);
	route.set_mtype(EN_Message_Request);
	route.set_session_id(psession->sessionid());
	route.set_uid(psession->_uid);
	route.set_groupid(TGlobal::_group_id);
	route.set_route_type(EN_Route_hash);

	if(psession->msgtype() != EN_Node_Client)
	{
		psession->_head.set_cmd(msg.msg_union_case());
	}
    // encode packet
	CPBOutputPacket output;
	output.EnCodeProtoRoute(route);
	output.EncodeProtoHead(psession->_head);
	output.EncodeProtoMsg(msg);
	output.End();
	
    // send
	phandler->Send(output.packet_buf(),output.packet_size());

    // timer
    psession->StartSessionTimer(5);
}

void Message::SendRawRequestToHall(CTCPSocketHandler* phandler, CSession* psession)
{
    if (!phandler)
    {
		ErrMsg("phandler = NULL.");
        return;
    }
    // route info 
  	PBRoute route;
	route.set_destination(EN_Node_Hall);
	route.set_des_id(1);
	route.set_source(TGlobal::_svrd_type);
	route.set_source_id(TGlobal::_svid);
	route.set_mtype(EN_Message_Request);
	route.set_session_id(psession->sessionid());
	route.set_uid(psession->_uid);
	route.set_groupid(TGlobal::_group_id);
	route.set_route_type(EN_Route_hash);
	
    // encode packet
	CPBOutputPacket output;
	output.EnCodeProtoRoute(route);
	output.EncodeProtoHead(psession->_head);
	output.EnCodeProtoPBInpuf(*psession->_input_packet);
	output.End();
	
    // send
	phandler->Send(output.packet_buf(),output.packet_size());

    // timer
    psession->StartSessionTimer(5);
}

int Message::SendMsgToClient(CTCPSocketHandler* phandler, const CSession* psession, const PBCSMsg& msg)
{
    if (!phandler)
    {
		ErrMsg("phandler = NULL.");
        return 0;
    }

    // encode packet
	CPBOutputPacket output;
	output.EncodeProtoHead(psession->_head);
	output.EncodeProtoMsg(msg);
	output.End();
	
    // send
	return phandler->Send(output.packet_buf(),output.packet_size());
}

void Message::SendMsgToPHP(CTCPSocketHandler* phandler, string json_msg)
{
    if (!phandler)
    {
        ErrMsg("phandler = NULL.");
        return;
    }

    // encode packet
    OutputPacket output;
    output.WriteBinary(json_msg.c_str(), json_msg.size());
    output.End();

    // send
    phandler->Send(output.packet_buf(), output.packet_size());
}


void Message::GMSendRequestToHall(CTCPSocketHandler* phandler, CSession* psession, const PBCSMsg& msg)
{
    if (!phandler)
    {
		ErrMsg("phandler = NULL.");
        return;
    }
    // route info 
  	PBRoute route;
	route.set_destination(EN_Node_Hall);
	route.set_route_type(EN_Route_hash);
	route.set_source(TGlobal::_svrd_type);
	route.set_source_id(TGlobal::_svid);
	route.set_mtype(EN_Message_Request);
	route.set_session_id(psession->sessionid());
	route.set_uid(psession->_uid);
	route.set_groupid(TGlobal::_group_id);

	if(psession->msgtype() != EN_Node_Client)
	{
		psession->_head.set_cmd(msg.msg_union_case());
	}
    // encode packet
	CPBOutputPacket output;
	output.EnCodeProtoRoute(route);
	output.EncodeProtoHead(psession->_head);
	output.EncodeProtoMsg(msg);
	output.End();
	
    // send
	phandler->Send(output.packet_buf(),output.packet_size());

    // timer
    //psession->StartSessionTimer(5);
}


void Message::GMSendRequestToUser(CTCPSocketHandler* phandler, CSession* psession, const PBCSMsg& msg)
{
    if (!phandler)
    {
		ErrMsg("phandler = NULL.");
        return;
    }
    // route info 
  	PBRoute route;
	route.set_destination(EN_Node_User);
	route.set_route_type(EN_Route_hash);
	route.set_source(TGlobal::_svrd_type);
	route.set_source_id(TGlobal::_svid);
	route.set_mtype(EN_Message_Request);
	route.set_session_id(psession->sessionid());
	route.set_uid(psession->_uid);
	route.set_groupid(TGlobal::_group_id);

	if(psession->msgtype() != EN_Node_Client)
	{
		psession->_head.set_cmd(msg.msg_union_case());
	}
    // encode packet
	CPBOutputPacket output;
	output.EnCodeProtoRoute(route);
	output.EncodeProtoHead(psession->_head);
	output.EncodeProtoMsg(msg);
	output.End();

    // send
	phandler->Send(output.packet_buf(),output.packet_size());

    // timer
    //psession->StartSessionTimer(5);
}


void Message::BroadcastToGame(CTCPSocketHandler* phandler, const PBCSMsg& msg)
{
    if (!phandler)
    {
		ErrMsg("phandler = NULL.");
        return;
    }
    // route info 
  	PBRoute route;
	route.set_destination(EN_Node_Game);
	route.set_route_type(EN_Route_broadcast);
	route.set_source(TGlobal::_svrd_type);
	route.set_source_id(TGlobal::_svid);
	route.set_mtype(EN_Message_Request);
	route.set_groupid(TGlobal::_group_id);
    // encode packet
    CPBOutputPacket output;
    PBHead head;
	head.set_cmd(msg.msg_union_case());
	head.set_proto_version(0);
	output.EnCodeProtoRoute(route);
	output.EncodeProtoHead(head);
	output.EncodeProtoMsg(msg);
	output.End();

    // send
	phandler->Send(output.packet_buf(),output.packet_size());
}

void Message::BroadcastToFpfGame(CTCPSocketHandler* phandler, const PBCSMsg& msg)
{
    if (!phandler)
    {
        ErrMsg("phandler = NULL.");
        return;
    }
    // route info
    PBRoute route;
    route.set_destination(EN_Node_FPF);
    route.set_route_type(EN_Route_broadcast);
    route.set_source(TGlobal::_svrd_type);
    route.set_source_id(TGlobal::_svid);
    route.set_mtype(EN_Message_Request);
    route.set_groupid(TGlobal::_group_id);
    // encode packet
    CPBOutputPacket output;
    PBHead head;
    head.set_cmd(msg.msg_union_case());
    head.set_proto_version(0);
    output.EnCodeProtoRoute(route);
    output.EncodeProtoHead(head);
    output.EncodeProtoMsg(msg);
    output.End();

    // send
    phandler->Send(output.packet_buf(),output.packet_size());
}

void Message::BroadcastToPhzGame(CTCPSocketHandler* phandler, const PBCSMsg& msg, int game_type)
{
    if (!phandler)
    {
        ErrMsg("phandler = NULL.");
        return;
    }
    // route info
    PBRoute route;
    route.set_destination(game_type);
    route.set_route_type(EN_Route_broadcast);
    route.set_source(TGlobal::_svrd_type);
    route.set_source_id(TGlobal::_svid);
    route.set_mtype(EN_Message_Request);
    route.set_groupid(TGlobal::_group_id);
    // encode packet
    CPBOutputPacket output;
    PBHead head;
    head.set_cmd(msg.msg_union_case());
    head.set_proto_version(0);
    output.EnCodeProtoRoute(route);
    output.EncodeProtoHead(head);
    output.EncodeProtoMsg(msg);
    output.End();

    // send
    phandler->Send(output.packet_buf(),output.packet_size());
}

void Message::BroadcastToRoom(CTCPSocketHandler* phandler, const PBCSMsg& msg)
{
    if (!phandler)
    {
		ErrMsg("phandler = NULL.");
        return;
    }
    // route info 
  	PBRoute route;
	route.set_destination(EN_Node_Room);
	route.set_route_type(EN_Route_broadcast);
	route.set_source(TGlobal::_svrd_type);
	route.set_source_id(TGlobal::_svid);
	route.set_mtype(EN_Message_Request);
	route.set_groupid(TGlobal::_group_id);
    // encode packet
    CPBOutputPacket output;
    PBHead head;
	head.set_cmd(msg.msg_union_case());
	head.set_proto_version(0);
	output.EnCodeProtoRoute(route);
	output.EncodeProtoHead(head);
	output.EncodeProtoMsg(msg);
	output.End();

    // send
	phandler->Send(output.packet_buf(),output.packet_size());
}

void Message::BroadcastToConnect(CTCPSocketHandler* phandler, const PBCSMsg& msg)
{
    if (!phandler)
    {
		ErrMsg("phandler = NULL.");
        return;
    }
    // route info 
  	PBRoute route;
	route.set_destination(EN_Node_Connect);
	route.set_route_type(EN_Route_broadcast);
	route.set_source(TGlobal::_svrd_type);
	route.set_source_id(TGlobal::_svid);
	route.set_mtype(EN_Message_Request);
	route.set_groupid(TGlobal::_group_id);
    // encode packet
    CPBOutputPacket output;
    PBHead head;
	head.set_cmd(msg.msg_union_case());
	head.set_proto_version(0);
	output.EnCodeProtoRoute(route);
	output.EncodeProtoHead(head);
	output.EncodeProtoMsg(msg);
	output.End();

    // send
	phandler->Send(output.packet_buf(),output.packet_size());
}


void Message::SendRawRequestToTeaBar(CTCPSocketHandler* phandler, CSession* psession, ENRouteType route_type)
{
	if (!phandler)
	{
		ErrMsg("phandler = NULL.");
		return;
	}
	// route info 
	PBRoute route;
	route.set_destination(EN_Node_TeaBar);
	route.set_des_id(1);
	route.set_source(TGlobal::_svrd_type);
	route.set_source_id(TGlobal::_svid);
	route.set_mtype(EN_Message_Request);
	route.set_session_id(psession->sessionid());
	route.set_uid(psession->_uid);
	route.set_route_type(route_type);
	route.set_groupid(TGlobal::_group_id);

	CPBOutputPacket output;
	output.EnCodeProtoRoute(route);
	output.EncodeProtoHead(psession->_head);
	output.EnCodeProtoPBInpuf(*psession->_input_packet);
	output.End();
	// send
	phandler->Send(output.packet_buf(), output.packet_size());

	// timer
	psession->StartSessionTimer(5);
}

void Message::GMSendRequestToTeaBar(CTCPSocketHandler* phandler, CSession* psession, const PBCSMsg& msg)
{
	if (!phandler)
	{
		ErrMsg("phandler = NULL.");
		return;
	}
	// route info 
	PBRoute route;
	route.set_destination(EN_Node_TeaBar);
	route.set_route_type(EN_Route_hash);
	route.set_source(TGlobal::_svrd_type);
	route.set_source_id(TGlobal::_svid);
	route.set_mtype(EN_Message_Request);
	route.set_session_id(psession->sessionid());
	route.set_uid(psession->_uid);
	route.set_groupid(TGlobal::_group_id);

	if (psession->msgtype() != EN_Node_Client)
	{
		psession->_head.set_cmd(msg.msg_union_case());
	}
	// encode packet
	CPBOutputPacket output;
	output.EnCodeProtoRoute(route);
	output.EncodeProtoHead(psession->_head);
	output.EncodeProtoMsg(msg);
	output.End();

	// send
	phandler->Send(output.packet_buf(), output.packet_size());

	// timer
	//psession->StartSessionTimer(5);
}

void Message::GMSendRequestToRoom(CTCPSocketHandler* phandler, CSession* psession, const PBCSMsg& msg)
{
    if (!phandler)
    {
        ErrMsg("phandler = NULL.");
        return;
    }
    // route info
    PBRoute route;
    route.set_destination(EN_Node_Room);
    route.set_route_type(EN_Route_hash);
    route.set_source(TGlobal::_svrd_type);
    route.set_source_id(TGlobal::_svid);
    route.set_mtype(EN_Message_Request);
    route.set_session_id(psession->sessionid());
    route.set_uid(psession->_uid);
    route.set_groupid(TGlobal::_group_id);

    if(psession->msgtype() != EN_Node_Client)
    {
        psession->_head.set_cmd(msg.msg_union_case());
    }
    // encode packet
    CPBOutputPacket output;
    output.EnCodeProtoRoute(route);
    output.EncodeProtoHead(psession->_head);
    output.EncodeProtoMsg(msg);
    output.End();

    // send
    phandler->Send(output.packet_buf(),output.packet_size());

    // timer
    //psession->StartSessionTimer(5);
}

int Message::SendMsgToH5Client(CTCPSocketHandler* phandler, const CSession* psession, const PBCSMsg& msg)
{
	if (!phandler)
	{
		ErrMsg("phandler = NULL.");
		return 0;
	}

	// encode packet
	CWSPBOutputPacket output;
	output.EncodeProtoHead(psession->_head);
	output.EncodeProtoMsg(msg);
	output.End();

	// send
	return phandler->Send(output.packet_buf(), output.packet_size());
}
