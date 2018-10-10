#include "RouteHandlerProxy.h"
#include "global.h"
#include "Packet.h"
#include "ProcesserManager.h"
#include "SessionManager.h"
#include "HandlerManager.h"
#include "RouteHandlerToken.h"
#include "InnerServerManager.h"

CPacketDecoder RouteHandlerProxy::_decoder;

int RouteHandlerProxy::OnAttachPoller(CTCPSocketHandler * phandler)
{
	if( phandler->_decode == NULL){
		phandler->_decode = &_decoder;
	}
	assert(phandler->_decode);
	phandler->EnableInput();
	return 0;
}

int RouteHandlerProxy::OnConnected(CTCPSocketHandler * phandler)
{
	RouteHandlerToken * ptoken = new RouteHandlerToken(phandler);
	HandlerManager::Instance()->BindHandlerToken(phandler,ptoken);
	return 0;
}

int RouteHandlerProxy::OnConnectedByRequest(CTCPSocketHandler * phandler)
{
	RouteHandlerToken * ptoken = new RouteHandlerToken(phandler);
	HandlerManager::Instance()->BindHandlerToken(phandler,ptoken);
	return 0;
}

int RouteHandlerProxy::OnClose(CTCPSocketHandler * phandler)
{
	RouteHandlerToken * ptoken =(RouteHandlerToken *) HandlerManager::Instance()->GetHandlerToken(phandler);
	if(ptoken==NULL)
	{
		return -1;
	}
	InnerServerManager::Instance()->OnSvrdClosed(ptoken);
	HandlerManager::Instance()->UnBindHandlerToken(phandler);
	return 0;
}

int RouteHandlerProxy::ProcessPacket(RouteHandlerToken * ptoken,const PBHead & head,const PBCSMsg & msg,const PBRoute & route)
{
	CSession * psession = SessionManager::Instance()->AllocSession();	
	if (psession == NULL)
	{
		return -1;
	}
	psession->_head.CopyFrom(head);
	psession->_request_msg.CopyFrom(msg);
	psession->_request_route.CopyFrom(route);
	psession->set_msgtype(route.source());
	psession->set_request_sessionid(route.session_id());
	//--
	psession->_message_logic_type = EN_Message_Request;
	ProcesserManager::Instance()->Process(ptoken,psession);
	return 0;
}

int RouteHandlerProxy::OnPacketComplete(CTCPSocketHandler * phandler,const char * data,int len)
{
	RouteHandlerToken * ptoken =(RouteHandlerToken *) HandlerManager::Instance()->GetHandlerToken(phandler);
	if(ptoken==NULL)
	{
		ErrMsg("fatal error.failed to find token for handler.");
		return 0;
	}
	CPBInputPacket pb_input;
	if (!pb_input.Copy(data,len))
	{
		ErrMsg("fatal error.copy failed.");
		return 0;
	}
	
	PBRoute route;
	if (!pb_input.DecodeProtoRoute(route))
	{
		ErrMsg("session process packet route failed.request by client[ip:%s,port:%d]",phandler->_sip.c_str(),phandler->_port);
		return 0;
    }

    if (route.destination() == TGlobal::_svrd_type)
    {
    	PBHead head;
    	if (!pb_input.DecodeProtoHead(head))
        {
    		ErrMsg("session process packet head failed.request by client[ip:%s,port:%d]",phandler->_sip.c_str(),phandler->_port);
    		return 0;
    	}

    	PBCSMsg msg;
    	if (!pb_input.DecodeProtoMsg(msg))
    	{
    		ErrMsg("session process packet msg failed.request by client[ip:%s,port:%d]",phandler->_sip.c_str(),phandler->_port);
    		return 0;
    	}

    	return ProcessPacket(ptoken,head,msg,route);
    }
    else
    {
    	InnerServerManager::Instance()->CheckNode(ptoken,route);
        return DoTransfer(route, pb_input);
    }
}

int RouteHandlerProxy::OnProcessOnTimerOut(CTCPSocketHandler * phandler,int Timerid)
{
	RouteHandlerToken * ptoken =(RouteHandlerToken *) HandlerManager::Instance()->GetHandlerToken(phandler);
	if(ptoken==NULL)
	{
		return 0;
	}
	VLogMsg(CLIB_LOG_LEV_ERROR,"on handler closed . server type:%d,server id:%d",ptoken->_stype,ptoken->_svid);
	InnerServerManager::Instance()->OnSvrdClosed(ptoken);
	return 0;
}

int RouteHandlerProxy::DoTransfer(const PBRoute& route, PacketBase & pkg)
{
	int stype = (int)route.destination();
    int svid = route.des_id();
	int groupid = InnerServerManager::Instance()->GetWorkingGroup(stype);
	int64 uid = route.uid();
	if(route.mtype() == EN_Message_Response)
	{
		groupid = route.groupid();
	}
    if (route.route_type() == EN_Route_p2p)
    {
    	CHandlerTokenBasic * ptoken = InnerServerManager::Instance()->GetInnerServer(groupid,stype,svid);
    	if(ptoken)
    	{
    		ptoken->_phandler->Send(pkg.packet_buf(), pkg.packet_size());
    	}
    	else
    	{
    		VLogMsg(CLIB_LOG_LEV_ERROR,"failed to find handler[%d,%d]",stype,svid);
			return 0;
    	}
    }
    else if (route.route_type() == EN_Route_hash)
    {
		CHandlerTokenBasic * ptoken = InnerServerManager::Instance()->GetInnerServerByHash(groupid,stype,uid);
		if(ptoken)
    	{
    		ptoken->_phandler->Send(pkg.packet_buf(), pkg.packet_size());
    	}
    	else
    	{
    		ErrMsg("failed to find handler[%d] for hash",stype);
			return 0;
    	}
    }
    else if (route.route_type() == EN_Route_broadcast)
    {
		InnerServerCluster & cluster = InnerServerManager::Instance()->_inner_cluster;
    	if(cluster.find(stype) == cluster.end())
		{
			ErrMsg("failed to find group[%d,%d] for broadcast",stype,groupid);
			return 0;
		}
		ServerGroupMap & groups = cluster[stype];
		if(groups.find(groupid) == groups.end())
		{
			ErrMsg("failed to find servers[%d] for broadcast",stype);
			return 0;
		}
		InnerServerMap & servers = groups[groupid];
		for(InnerServerMap::iterator iter=servers.begin();iter!=servers.end();iter++)
		{
			CHandlerTokenBasic * ptoken = iter->second;
			if(ptoken)
	    	{
	    		ptoken->_phandler->Send(pkg.packet_buf(), pkg.packet_size());
	    	}
	    	else
	    	{
	    		ErrMsg("invalid handler[%d] for broadcast",stype);
				return 0;
	    	}
		}
    }
    else
    {
		ErrMsg("unknown route type: %d", route.route_type());
    }
    return 0;
}

