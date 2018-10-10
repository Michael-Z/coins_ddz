#include "InnerServerManager.h"
#include "Message.h"
#include "RouteHandlerProxy.h"
RouteHandlerToken * InnerServerManager::GetInnerServer(int groupid,int stype,int svid)
{
	RouteHandlerToken * handler = NULL;
	InnerServerCluster::iterator cluster_iter = _inner_cluster.find(stype);
	if(cluster_iter != _inner_cluster.end())
	{
		ServerGroupMap & groups = cluster_iter->second;
		ServerGroupMap::iterator groups_iter = groups.find(groupid);
		if(groups_iter!=groups.end())
		{
			InnerServerMap & servers = groups[groupid];
			InnerServerMap::iterator iter = servers.find(svid);
			if(iter != servers.end())
			{
				handler = iter->second;
			}
		}
	}
	return handler;
}

RouteHandlerToken * InnerServerManager::GetInnerServerByHash(int groupid,int stype,long long key)
{
	RouteHandlerToken * handler = NULL;
	InnerServerCluster::iterator cluster_iter = _inner_cluster.find(stype);
	if(cluster_iter != _inner_cluster.end())
	{
		ServerGroupMap & groups = cluster_iter->second;
		ServerGroupMap::iterator groups_iter = groups.find(groupid);
		if(groups_iter!=groups.end())
		{
			InnerServerMap & servers = groups[groupid];
			if(servers.size()==0)
			{
				return handler;
			}
			int slot = (key%servers.size())+1;
			if(servers.find(slot)!=servers.end())
			{
				handler = servers[slot];
				VLogMsg(CLIB_LOG_LEV_DEBUG,"find server[sid:%d,gourpid:%d,stype:%d] for key[%lld],slot:%d",
					handler->_svid,handler->_groupid,handler->_stype,key,slot);
			}
			else
			{
				ErrMsg("failed to find server for key[%lld],slot:%d",key,slot);
			}
		}
	}
	return handler;
}

void InnerServerManager::RegistInnerServer(RouteHandlerToken * ptoken,const PBRoute & route,int stype,int svid,int groupid)
{
	//
	switch(stype)
	{
		case EN_Node_Game : 
			if(_inner_cluster.find(stype) != _inner_cluster.end())
			{
				ServerGroupMap & group = _inner_cluster[stype];
				if(group.find(groupid) != group.end())
				{
					InnerServerMap & servers = group[groupid];
					if(servers.find(svid) != servers.end())
					{
						return;
					}
				}
			}
		default:
			{
				ServerGroupMap & group = _inner_cluster[stype];
				InnerServerMap & servers = group[groupid];
				servers[svid] = ptoken;
				ptoken->_svid = svid;
				ptoken->_stype = stype;	
				ptoken->_groupid = groupid;
				_work_group[stype] = groupid;
			}
			break;
	}
	PBCSMsg msg;
	SSNotifyInnerServer & ss_notify_inner_server = *msg.mutable_ss_notify_inner_server();
	ss_notify_inner_server.set_routeid(TGlobal::_svid);
    Message::PushInnerMsg(ptoken->_phandler, 0, msg, (ENNodeType)route.source(),route.source_id());
}

void InnerServerManager::CheckNode(RouteHandlerToken * ptoken,const PBRoute & route)
{
	if(route.mtype() == EN_Message_Request || route.mtype() == EN_Message_Push)
	{
		int stype = route.source();
		int svid = route.source_id();
		int groupid = route.groupid();
		
		ServerGroupMap & group = _inner_cluster[stype];
		InnerServerMap & servers = group[groupid];
		servers[svid] = ptoken;
		ptoken->_svid = svid;
		ptoken->_stype = stype;	
		ptoken->_groupid = groupid;
	}
}

void InnerServerManager::OnSvrdClosed(RouteHandlerToken * ptoken)
{
	if(ptoken == NULL) return;
	if(ptoken->_stype == EN_Node_Game)
	{
		PBCSMsg msg;
		SSNotifyGameSvrdClosed & ss_notify_gamesvrd_closed = * msg.mutable_ss_notify_gamesvrd_closed();
		ss_notify_gamesvrd_closed.set_gameid(ptoken->_svid);
		ss_notify_gamesvrd_closed.set_gtype(ptoken->_stype);
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
		RouteHandlerProxy::Instance()->DoTransfer(route,output);
	}
	if(_inner_cluster.find(ptoken->_stype) != _inner_cluster.end())
	{
		ServerGroupMap & group = _inner_cluster[ptoken->_stype];
		if(group.find(ptoken->_groupid)!=group.end())
		{
			InnerServerMap & servers = group[ptoken->_groupid];
			InnerServerMap::iterator iter = servers.find(ptoken->_svid);
			if(iter!=servers.end())
			{
				servers.erase(iter);
				return;
			}
		}
	}
}

int InnerServerManager::GetWorkingGroup(int stype)
{
	if(_work_group.find(stype) == _work_group.end())
	{
		return -1;
	}
	return _work_group[stype];
}

