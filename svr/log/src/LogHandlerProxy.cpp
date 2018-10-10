#include "LogHandlerProxy.h"
#include "global.h"
#include "Packet.h"
#include "ProcesserManager.h"
#include "SessionManager.h"
#include "HandlerManager.h"
#include "LogHandlerToken.h"

CPacketDecoder LogHandlerProxy::_decoder;

int LogHandlerProxy::OnAttachPoller(CTCPSocketHandler * phandler)
{
	if( phandler->_decode == NULL){
		phandler->_decode = &_decoder;
	}
	assert(phandler->_decode);
	phandler->EnableInput();
	return 0;
}

int LogHandlerProxy::OnConnected(CTCPSocketHandler * phandler)
{
	//句柄被Accept后触发
	//todo 为了防止内存泄漏 应该先检测一下 handler是否已经绑定过
	LogHandlerToken * ptoken = new LogHandlerToken(phandler);
	HandlerManager::Instance()->BindHandlerToken(phandler,ptoken);
	return 0;
}

int LogHandlerProxy::OnConnectedByRequest(CTCPSocketHandler * phandler)
{
	phandler->EnableReconnect();
	LogHandlerToken * ptoken = new LogHandlerToken(phandler);
	HandlerManager::Instance()->BindHandlerToken(phandler,ptoken);
	return 0;
}

int LogHandlerProxy::OnClose(CTCPSocketHandler * phandler)
{
	HandlerManager::Instance()->UnBindHandlerToken(phandler);
	return 0;
}

int LogHandlerProxy::ProcessPacket(LogHandlerToken * ptoken,const PBHead & head,const PBCSMsg & msg,const PBRoute & route)
{
	//VLogMsg(CLIB_LOG_LEV_DEBUG,"Process Packet[cmd:0x%x] From[%d]",msg.msg_union_case(),route.source());
	CSession * psession = NULL;
	switch( route.source() )
	{
		case EN_Node_Client:	
			//来自客户端的请求 直接创建session
			//大厅服才会收到客户端的请求.
			psession = SessionManager::Instance()->AllocSession();
			if (psession == NULL)
			{
				return -1;
			}
			psession->_head.CopyFrom(head);
			psession->_request_msg.CopyFrom(msg);
			psession->set_msgtype(EN_Node_Client);
			psession->_message_logic_type = EN_Message_Request;
			//psession->_head.set_session_cmd(head.cmd());
			psession->set_request_cmd(head.cmd());
			break;
		default :
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
				psession->_response_msg.CopyFrom(msg);
				psession->_response_route.CopyFrom(route);
				psession->set_msgtype(route.source());
				psession->AddResponseMsg(msg);
			}
			else if (route.mtype()==EN_Message_Request)
			{
				psession = SessionManager::Instance()->AllocSession();
				if (psession == NULL)
				{
					return -1;
				}
				psession->_head.CopyFrom(head);
				psession->_request_msg.CopyFrom(msg);
				psession->_request_route.CopyFrom(route);
				psession->set_msgtype(route.source());
				if (route.has_session_id() && route.session_id() >= 0)
				{
					psession->set_request_sessionid(route.session_id());
				}
				//psession->_head.set_session_cmd(head.cmd());
				psession->set_request_cmd(head.cmd());
			}
			else
			{
				ErrMsg("fatal error. invalid message type[%d]",route.mtype());
				return 0;
			}
			psession->_message_logic_type = route.mtype();
			break;
	}
	ProcesserManager::Instance()->Process(ptoken,psession);
	return 0;
}


int LogHandlerProxy::OnPacketComplete(CTCPSocketHandler * phandler,const char * data,int len)
{
	LogHandlerToken * ptoken =(LogHandlerToken *) HandlerManager::Instance()->GetHandlerToken(phandler);
	if(ptoken==NULL)
	{
		VLogMsg(CLIB_LOG_LEV_ERROR,"fatal error.failed to find token for handler.");
		return -1;
	}
	CPBInputPacket pb_input;
	if (!pb_input.Copy(data,len))
	{
		VLogMsg(CLIB_LOG_LEV_ERROR,"fatal error.copy failed.");
		return -1;
	}

	PBHead head;
	PBCSMsg msg;
	PBRoute route;
	if (!pb_input.DecodeProtoMsg(msg))
    {
		ErrMsg("session process packet msg failed.request by client[ip:%s,port:%d]",phandler->_sip.c_str(),phandler->_port);
		return -1;
	}

	return ProcessPacket(ptoken,head,msg,route);
}

int LogHandlerProxy::OnProcessOnTimerOut(CTCPSocketHandler * phandler,int Timerid)
{
	return 0;
}

