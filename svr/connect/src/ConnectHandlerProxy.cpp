#include "ConnectHandlerProxy.h"
#include "global.h"
#include "Packet.h"
#include "ProcesserManager.h"
#include "SessionManager.h"
#include "AccessUserManager.h"
#include "HandlerManager.h"
#include "ConnectHandlerToken.h"
#include "Message.h"
#include "RouteManager.h"

CPacketDecoder ConnectHandlerProxy::_decoder;

int ConnectHandlerProxy::OnAttachPoller(CTCPSocketHandler * phandler)
{
	if( phandler->_decode == NULL){
		phandler->_decode = &_decoder;
	}
	assert(phandler->_decode);
	phandler->EnableInput();
	return 0;
}

int ConnectHandlerProxy::OnConnected(CTCPSocketHandler * phandler)
{
	ConnectHandlerToken * ptoken = new ConnectHandlerToken(phandler);
	phandler->DisableReconnect();
	HandlerManager::Instance()->BindHandlerToken(phandler,ptoken);
	return 0;
}

int ConnectHandlerProxy::OnConnectedByRequest(CTCPSocketHandler * phandler)
{
	phandler->EnableReconnect();
	ConnectHandlerToken * ptoken = new ConnectHandlerToken(phandler);
	HandlerManager::Instance()->BindHandlerToken(phandler,ptoken);
	ptoken->_authorized = true;
	return 0;
}

int ConnectHandlerProxy::OnClose(CTCPSocketHandler * phandler)
{
	return 0;
}

int ConnectHandlerProxy::ProcessPacket(ConnectHandlerToken * ptoken,CSession * psession)
{
	return ProcesserManager::Instance()->Process(ptoken,psession);
}

int ConnectHandlerProxy::OnPacketComplete(CTCPSocketHandler * phandler,const char * data,int len)
{
	ConnectHandlerToken * ptoken =(ConnectHandlerToken *) HandlerManager::Instance()->GetHandlerToken(phandler);
	if(ptoken==NULL)
	{
		VLogMsg(CLIB_LOG_LEV_DEBUG,"fatal error.failed to find token for handler.");
		return 0;
	}
	CPBInputPacket input;
	if (!input.Copy(data,len))
	{
		ErrMsg("fatal error.copy failed.");
		return 0;
	}

	PBRoute route;
	if (!input.DecodeProtoRoute(route))
	{
		ErrMsg("session process packet route failed.request by client[ip:%s,port:%d]",
			ptoken->_phandler->_sip.c_str(),ptoken->_phandler->_port);
		return 0;
    }

	PBHead head;
	if (!input.DecodeProtoHead(head))
	{
		ErrMsg("session process packet head failed.request by client[ip:%s,port:%d]",
			ptoken->_phandler->_sip.c_str(),ptoken->_phandler->_port);
		return 0;
	}
	
	//VLogMsg(CLIB_LOG_LEV_DEBUG,"Process Packet[cmd:0x%lx]",head.cmd());
	int session_cmd = 0;
	CSession * psession = NULL;
	if(route.mtype()==EN_Message_Response)
	{
		//响应处理
		int session_id = route.session_id();
		psession = SessionManager::Instance()->GetSession(session_id);
		if(psession == NULL)
		{
			//返回超时.
			ErrMsg("failed to found session[%d].",session_id);
			return 0;
		}
		session_cmd = psession->request_cmd();
		psession->_response_route.CopyFrom(route);
	}
	else if (route.mtype()==EN_Message_Request || route.mtype() == EN_Message_Push)
	{
		psession = SessionManager::Instance()->AllocSession();
		if (psession == NULL)
		{
			return -1;
		}
		session_cmd = head.cmd();
		psession->_request_route.CopyFrom(route);
		psession->_head.CopyFrom(head);
		psession->_uid = route.uid();
	}
	else
	{
		ErrMsg("fatal error. invalid message type[%d]",route.mtype());
		return 0;
	}
	psession->_message_logic_type = route.mtype();
	psession->set_msgtype(route.source());
	if(IsProcessInLocal(session_cmd))
	{
		PBCSMsg msg;
    	if (!input.DecodeProtoMsg(msg))
    	{
    		ErrMsg("session process packet msg failed.request by client[ip:%s,port:%d]",phandler->_sip.c_str(),phandler->_port);
    		return -1;
    	}
		if(route.mtype()==EN_Message_Response)
		{
			psession->_response_msg.CopyFrom(msg);
		}
		else
		{
			psession->_request_msg.CopyFrom(msg);
		}
		return ProcessPacket(ptoken,psession);
	}
	return DoTransfer(psession,input);
}

bool ConnectHandlerProxy::IsProcessInLocal(int cmd)
{
	switch(cmd)
	{
		case PBCSMsg::kSsNotifyInnerServer:
		case PBCSMsg::kCsRequestLogin:
		case PBCSMsg::kCsRequestEnterTable:
		case PBCSMsg::kCsRequestFpfEnterTable:
        case PBCSMsg::kCsRequestDaerEnterTable:
		case PBCSMsg::kGmPushMessageMulti:
        case PBCSMsg::kCsRequestDssEnterTable:
        case PBCSMsg::kCsRequestSdrEnterTable:
        case PBCSMsg::kCsResponseSdrEnterTable:
        case PBCSMsg::kSsNotifyPlayerSkipMatchPosChange:
		case PBCSMsg::kSsNotifyPlayerPosChange:
			return true;
			break;
		default:
			break;
	}
	return false;
}

int ConnectHandlerProxy::DoTransfer(CSession * psession,CPBInputPacket & input)
{
	long long uid = psession->_uid;
	ConnectHandlerToken * pusertoken = AccessUserManager::Instance()->GetHandlerTokenByUid(uid);
	if (pusertoken != NULL && pusertoken->_phandler != NULL)
	{
		CPBOutputPacket output;
		output.EncodeProtoHead(psession->_head);
		output.EnCodeProtoPBInpuf(input);
		output.End();
		if ( pusertoken->_phandler->Send(output.packet_buf(),output.packet_size()) >= 0 )
		{
			/*
			if(psession->_head.cmd() == PBCSMsg::kCsRequestLogoutTable || 
			psession->_head.cmd() == PBCSMsg::kCsResponseLogoutTable )
			{
				VLogMsg(CLIB_LOG_LEV_DEBUG,"user[token:0x%lx,id:%lld] logout table",(long)pusertoken,uid);
				pusertoken->_pos.set_pos_type(EN_Position_Hall);
			}
			*/
		}
	}
	else
	{
		VLogMsg(CLIB_LOG_LEV_DEBUG,"handler for user[%lld] not exist,maybe lost connection.",uid);
		PBCSMsg msg;
		msg.mutable_ss_notify_player_handler_close()->set_uid(uid);
		if(psession->_request_route.source() == EN_Node_Game)
		{
			Message::PushInnerMsg(RouteManager::Instance()->GetRouteByRandom(),uid,msg,EN_Node_Game,psession->_request_route.source_id());
			VLogMsg(CLIB_LOG_LEV_DEBUG,"unbind uid[%lld] at game[%d] for token",uid,psession->_request_route.source_id());
		}
	}
	SessionManager::Instance()->ReleaseSession(psession->sessionid());
	return 0;
}

int ConnectHandlerProxy::OnProcessOnTimerOut(CTCPSocketHandler * phandler,int Timerid)
{
	return 0;
}

