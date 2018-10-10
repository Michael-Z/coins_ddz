#include "RouteManager.h"
#include <stdlib.h>
#include "PBConfigBasic.h"
#include "Session.h"
#include "SessionManager.h"
#include "Message.h"

RouteManager * RouteManager::Instance (void)
{
	return CSingleton<RouteManager>::Instance();
}

bool RouteManager::Init(HandlerProxyBasic * pproxy)
{
	if (!PokerPBRouteSvrdConfig::Instance()->Init("../../conf/route.CFG"))
	{
		ErrMsg("failed to load route.CFG");
		return false;
	
	}
	for(int i=0;i<PokerPBRouteSvrdConfig::Instance()->routes_size();i++)
	{
		const PBSvrdNode & route = PokerPBRouteSvrdConfig::Instance()->routes(i);
		string ip = route.ip();
		int port = route.port();
		CTCPSocketHandler * proute = new CTCPSocketHandler();
		proute->_pproxy = pproxy;
		int iret = proute->Connect(ip,port);
		if(iret != 0)
		{
			ErrMsg("failed to connect route[%s,%d]",ip.c_str(),port);
			delete proute;
			return false;
		}
		else
		{
			LogMsg("succ to connect route[%s,%d]",ip.c_str(),port);
            PBCSMsg msg;
            SSRegistInnerServer& notify = *msg.mutable_ss_regist_inner_server();
            notify.set_ntype(TGlobal::_svrd_type);
            notify.set_nvid(TGlobal::_svid);
            notify.set_group_id(TGlobal::_group_id);
            Message::PushInnerMsg(proute, 0, msg, EN_Node_Route);
			_state_attribute = Con_Heat_Send_State_Sending;
		}
	}
	return true;
}

CTCPSocketHandler * RouteManager::GetRouteByRandom()
{
	if(_route_map.size()==0)
	{
		return NULL;
	}
	int index = random()%_route_map.size();
	RouteMap::iterator iter = _route_map.begin();
	int count = 0;
	for(;iter!=_route_map.end();iter++)
	{
		if(index == count)
		{
			return iter->second;
		}
		count++;
	}
	return NULL;
}

void RouteManager::Send(CPBOutputPacket & output)
{
	//send by random
	CTCPSocketHandler * proute = GetRouteByRandom();
	if(proute)
	{
		proute->Send(output.packet_buf(),output.packet_size());
	}
}

int RouteManager::ProcessOnTimerOut(int Timerid) 
{
	CTCPSocketHandler * proute = GetRouteByRandom();
        VLogMsg(CLIB_LOG_LEV_DEBUG, "Timeout of the heartbeat timer");
	if(proute == NULL)
	{
		return 0;
	}
        if(_state_attribute == Con_Heat_Send_State_Sending)
	{
		proute->Reset();
                VLogMsg(CLIB_LOG_LEV_ERROR, "Timeout reconnection -- Reset handle reconnection");
	}

        PBCSMsg msg;
        SSRegistInnerServer& notify = *msg.mutable_ss_regist_inner_server();
        notify.set_ntype(TGlobal::_svrd_type);
        notify.set_nvid(TGlobal::_svid);
        notify.set_group_id(TGlobal::_group_id);
        Message::PushInnerMsg(proute, 0, msg, EN_Node_Route);
        _state_attribute = Con_Heat_Send_State_Sending;
	return 0;
}

int RouteManager::HeartBeatTimerStart()
{
    //定时器 启动
    VLogMsg(CLIB_LOG_LEV_DEBUG, "Heartbeat timer start");
    _monitor_timer.SetTimeEventObj(this,NODE_MONITOR_TIMER);
    _monitor_timer.StartTimerBySecond(60,true);
    return 0;
}

int RouteManager::HeartBeatTimerStop()
{
    //定时器 关闭
    _monitor_timer.StopTimer();
    return 0;
}



