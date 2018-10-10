#pragma once
#include "Session.h"
#include "PBPacket.h"
#include "global.h"
#include "TCPSocketHandler.h"
#include "RouteManager.h"

class Message
{
public:
    static void PushMsg(CTCPSocketHandler* phandler,long long uid,const PBCSMsg& msg, int node_type = EN_Node_Connect, int des_id = 1,ENRouteType route_type = EN_Route_p2p);
    static void PushInnerMsg(CTCPSocketHandler* phandler,long long uid,const PBCSMsg& msg, int node_type, int des_id = 1,ENRouteType route_type = EN_Route_p2p);
    static void SendRequestMsg(CTCPSocketHandler* phandler, CSession* psession, const PBCSMsg& msg, int node_type, int des_id = 1,ENRouteType route_type = EN_Route_p2p);
	static void SendRequestMsgAsClient(CTCPSocketHandler* phandler,const PBCSMsg& msg);
    static void SendResponseMsg(CTCPSocketHandler* phandler, CSession* psession, const PBCSMsg& msg);
    static void SendRequestToRoom(CTCPSocketHandler* phandler, CSession* psession, const PBCSMsg& msg,ENRouteType route_type = EN_Route_p2p);
	static void SendRawRequestToRoom(CTCPSocketHandler* phandler, CSession* psession, ENRouteType route_type = EN_Route_p2p);
    static void SendRequestToUser(CTCPSocketHandler* phandler, CSession* psession, const PBCSMsg& msg ,int64 uid = -1);
	static void SendRequestToGame(CTCPSocketHandler* phandler, CSession* psession, const PBCSMsg& msg);
	static void SendRawRequestToGame(CTCPSocketHandler* phandler, CSession* psession,const PBBPlayerPositionInfo & pos_info);
    static void SendRawRequestToFpfGame(CTCPSocketHandler* phandler, CSession* psession,const PBBPlayerPositionInfo & pos_info);
    static void SendRawRequestToPhzGame(CTCPSocketHandler* phandler, CSession* psession,const PBBPlayerPositionInfo & pos_info, int game_type);
    static void SendRequestToHall(CTCPSocketHandler* phandler, CSession* psession, const PBCSMsg& msg,ENRouteType route_type = EN_Route_p2p);
	static void SendRawRequestToHall(CTCPSocketHandler* phandler, CSession* psession);
    static int SendMsgToClient(CTCPSocketHandler* phandler, const CSession* psession, const PBCSMsg& msg);
    static void SendMsgToPHP(CTCPSocketHandler* phandler, string json_msg);

    static void GMSendRequestToUser(CTCPSocketHandler* phandler, CSession* psession, const PBCSMsg& msg);
    static void GMSendRequestToHall(CTCPSocketHandler* phandler, CSession* psession, const PBCSMsg& msg);

    static void BroadcastToGame(CTCPSocketHandler* phandler, const PBCSMsg& msg);
    static void BroadcastToFpfGame(CTCPSocketHandler* phandler, const PBCSMsg& msg);
    static void BroadcastToPhzGame(CTCPSocketHandler* phandler, const PBCSMsg& msg, int game_type);
	static void BroadcastToRoom(CTCPSocketHandler* phandler, const PBCSMsg& msg);
	static void BroadcastToConnect(CTCPSocketHandler* phandler, const PBCSMsg& msg);

	static void SendRawRequestToTeaBar(CTCPSocketHandler* phandler, CSession* psession, ENRouteType route_type = EN_Route_p2p);
	static void GMSendRequestToTeaBar(CTCPSocketHandler* phandler, CSession* psession, const PBCSMsg& msg);

	static void GMSendRequestToRoom(CTCPSocketHandler* phandler, CSession* psession, const PBCSMsg& msg);
	static int SendMsgToH5Client(CTCPSocketHandler* phandler, const CSession* psession, const PBCSMsg& msg);
};
