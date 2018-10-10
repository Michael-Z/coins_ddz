#include "ClientHandlerProxy.h"
#include "global.h"
#include "Packet.h"
#include "ProcesserManager.h"
#include "SessionManager.h"
#include "AccessUserManager.h"
#include "HandlerManager.h"
#include "ConnectHandlerToken.h"
#include "RouteManager.h"
#include "Message.h"
#include "PBConfigBasic.h"

CPacketDecoder ClientHandlerProxy::_decoder;

int ClientHandlerProxy::OnAttachPoller(CTCPSocketHandler * phandler)
{
	if( phandler->_decode == NULL){
		phandler->_decode = &_decoder;
	}
	assert(phandler->_decode);
	phandler->EnableInput();
	return 0;
}

int ClientHandlerProxy::OnConnected(CTCPSocketHandler * phandler)
{
	//句柄被Accept后触发
	ConnectHandlerToken * ptoken = new ConnectHandlerToken(phandler);
	HandlerManager::Instance()->BindHandlerToken(phandler,ptoken);
	ptoken->EnableHeartBeatCheck();
	return 0;
}

int ClientHandlerProxy::OnConnectedByRequest(CTCPSocketHandler * phandler)
{
	ConnectHandlerToken * ptoken = new ConnectHandlerToken(phandler);
	HandlerManager::Instance()->BindHandlerToken(phandler,ptoken);
	ptoken->_authorized = true;
	return 0;
}

int ClientHandlerProxy::OnClose(CTCPSocketHandler * phandler)
{
	ConnectHandlerToken * ptoken = (ConnectHandlerToken*)HandlerManager::Instance()->GetHandlerToken(phandler);
	AccessUserManager::Instance()->OnHandlerClosed(ptoken);
	HandlerManager::Instance()->UnBindHandlerToken(phandler);
	return 0;
}

int ClientHandlerProxy::ProcessPacket(ConnectHandlerToken * ptoken,CSession * psession)
{
	ProcesserManager::Instance()->Process(ptoken,psession);
	return 0;
}

int ClientHandlerProxy::OnPacketComplete(CTCPSocketHandler * phandler,const char * data,int len)
{

	ConnectHandlerToken * ptoken =(ConnectHandlerToken *) HandlerManager::Instance()->GetHandlerToken(phandler);
	if(ptoken==NULL)
	{
		VLogMsg(CLIB_LOG_LEV_DEBUG,"fatal error.failed to find token for handler.");
		return -1;
	}
	ptoken->ResetHeartBeatCheck();
	CPBInputPacket input;
	if (!input.Copy(data,len))
	{
		ErrMsg("fatal error.copy failed.");
		return -1;
	}
	
	PBHead head;
	if (!input.DecodeProtoHead(head))
	{
		ErrMsg("session process packet head failed.request by client[ip:%s,port:%d]",phandler->_sip.c_str(),phandler->_port);
		return -1;
	}

    if(!ptoken->_authorized && head.cmd()!=PBCSMsg::kCsRequestLogin
       && head.cmd()!=PBCSMsg::kCsRequestEcho)
	{
		VLogMsg(CLIB_LOG_LEV_DEBUG,"unauthorize request[0x%lx]",head.cmd());
		return -1;
	}
	
	CSession * psession = NULL;
	psession = SessionManager::Instance()->AllocSession();
	if (psession == NULL)
	{
		return -1;
	}
	if (ptoken->_authorized)
	{
		psession->_uid = ptoken->_uid;
	}
	//
	psession->set_request_cmd(head.cmd());
	psession->_head.CopyFrom(head);
	psession->set_msgtype(EN_Node_Client);
	psession->_message_logic_type = EN_Message_Request;
	if(IsProcessInLocal(head.cmd()))
	{
    	PBCSMsg msg;
    	if (!input.DecodeProtoMsg(msg))
    	{
    		ErrMsg("decode proto msg failed.request by client[ip:%s,port:%d]",phandler->_sip.c_str(),phandler->_port);
    		return -1;
    	}
		psession->_request_msg.CopyFrom(msg);
		return ProcessPacket(ptoken,psession);
	}
	//
	return DoTransfer(ptoken,psession,input);
}

int ClientHandlerProxy::DoTransfer(ConnectHandlerToken * ptoken,CSession * psession,CPBInputPacket & input)
{
	int cmd = psession->_head.cmd();
	psession->_input_packet = &input;

    // to room
    if (cmd >= 0x1001 && cmd <= 0x1200)
    {
        Message::SendRawRequestToRoom(RouteManager::Instance()->GetRouteByRandom(), psession);
    }
    // to hall
    else if (cmd >= 0x1201 && cmd <= 0x1500)
    {
        Message::SendRawRequestToHall(RouteManager::Instance()->GetRouteByRandom(), psession);
    }
    // to game
	else if(cmd >= 0x2001 && cmd <= 0x3000)
	{
		PBBPlayerPositionInfo & pos = ptoken->_pos;
		VLogMsg(CLIB_LOG_LEV_DEBUG, "transfer pos_type = %d", pos.pos_type());        
        if (PokerPBConnectSvrdConfig::Instance()->CheckPosition(pos.pos_type()))
        {
            int node_type = PokerPBConnectSvrdConfig::Instance()->GetNode(pos.pos_type());
            Message::SendRawRequestToPhzGame(RouteManager::Instance()->GetRouteByRandom(), psession, pos, node_type);
        }
		else
		{
			//释放session
			SessionManager::Instance()->ReleaseSession(psession->sessionid());
			return EN_Process_Result_Completed;
		}
	}
    // to fpf
    else if(cmd >= 0x5001 && cmd <= 0x6000)
    {
        PBBPlayerPositionInfo & pos = ptoken->_pos;
        VLogMsg(CLIB_LOG_LEV_DEBUG, "transfer pos_type = %d", pos.pos_type());
        if (PokerPBConnectSvrdConfig::Instance()->CheckPosition(pos.pos_type()))
        {
            int node_type = PokerPBConnectSvrdConfig::Instance()->GetNode(pos.pos_type());
            Message::SendRawRequestToPhzGame(RouteManager::Instance()->GetRouteByRandom(), psession, pos, node_type);
        }
        else
        {
            //释放session
            SessionManager::Instance()->ReleaseSession(psession->sessionid());
            return EN_Process_Result_Completed;
        }
    }
    // to daer
    else if(cmd >= 0x6001 && cmd <= 0x6500)
    {
        PBBPlayerPositionInfo & pos = ptoken->_pos;
        VLogMsg(CLIB_LOG_LEV_DEBUG, "transfer pos_type = %d", pos.pos_type());
        if (PokerPBConnectSvrdConfig::Instance()->CheckPosition(pos.pos_type()))
        {
            int node_type = PokerPBConnectSvrdConfig::Instance()->GetNode(pos.pos_type());
            Message::SendRawRequestToPhzGame(RouteManager::Instance()->GetRouteByRandom(), psession, pos, node_type);
        }
        else
        {
            //释放session
            SessionManager::Instance()->ReleaseSession(psession->sessionid());
            return EN_Process_Result_Completed;
        }
    }
    // to dss
    else if(cmd >= 0x7001 && cmd <= 0x7200)
    {
        PBBPlayerPositionInfo & pos = ptoken->_pos;
        VLogMsg(CLIB_LOG_LEV_DEBUG, "transfer pos_type = %d", pos.pos_type());
        if (PokerPBConnectSvrdConfig::Instance()->CheckPosition(pos.pos_type()))
        {
            int node_type = PokerPBConnectSvrdConfig::Instance()->GetNode(pos.pos_type());
            Message::SendRawRequestToPhzGame(RouteManager::Instance()->GetRouteByRandom(), psession, pos, node_type);
        }
        else
        {
            //释放session
            SessionManager::Instance()->ReleaseSession(psession->sessionid());
            return EN_Process_Result_Completed;
        }
    }
    // to sdr
    else if(cmd >= 0x7201 && cmd <= 0x7400)
    {
        PBBPlayerPositionInfo & pos = ptoken->_pos;
        VLogMsg(CLIB_LOG_LEV_DEBUG, "transfer pos_type = %d", pos.pos_type());
        if (PokerPBConnectSvrdConfig::Instance()->CheckPosition(pos.pos_type()))
        {
            int node_type = PokerPBConnectSvrdConfig::Instance()->GetNode(pos.pos_type());
            Message::SendRawRequestToPhzGame(RouteManager::Instance()->GetRouteByRandom(), psession, pos, node_type);
        }
        else
        {
            //释放session
            SessionManager::Instance()->ReleaseSession(psession->sessionid());
            return EN_Process_Result_Completed;
        }
    }
	//to tea bar
	else if (cmd >= 0x8001 && cmd <= 0x8100)
	{
		Message::SendRawRequestToTeaBar(RouteManager::Instance()->GetRouteByRandom(), psession);
	}
    //to match
    else if(cmd >= 0x9001 && cmd <= 0x9100)
    {
        Message::SendRawRequestToRoom(RouteManager::Instance()->GetRouteByRandom(), psession);
    }
	else
	{
		psession->_input_packet = NULL;
		VLogMsg(CLIB_LOG_LEV_DEBUG,"invaild command[0x%x]",cmd);
		SessionManager::Instance()->ReleaseSession(psession->sessionid());
		return EN_Process_Result_Failed;
	}
	psession->_input_packet = NULL;
	return EN_Process_Result_Succ;
}

bool ClientHandlerProxy::IsProcessInLocal(int cmd)
{
	switch(cmd)
	{
		case PBCSMsg::kCsRequestLogin:
		case PBCSMsg::kCsRequestEnterTable:
		case PBCSMsg::kCsRequestHeartBeat:
        case PBCSMsg::kCsRequestFpfEnterTable:
        case PBCSMsg::kCsRequestDaerEnterTable:
        case PBCSMsg::kCsRequestEcho:
        case PBCSMsg::kCsRequestDssEnterTable:
        case PBCSMsg::kCsRequestSdrEnterTable:
			return true;
			break;
		default:
			break;
	}
	return false;
}

int ClientHandlerProxy::OnProcessOnTimerOut(CTCPSocketHandler * phandler,int Timerid)
{
	return 0;
}


