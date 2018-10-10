#include "CommonClienthandler.h"
#include "InnerServerManager.h"

ENHandlerResult CRegistInnerServer::ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession)
{
	//
	const SSRegistInnerServer & notify = psession->_request_msg.ss_regist_inner_server();
	//
	int ntype =  notify.ntype();
	int nvid  = notify.nvid();
	int groupid = notify.group_id();
	//
	InnerServerManager::Instance()->RegistInnerServer((RouteHandlerToken*)ptoken,psession->_request_route,ntype,nvid,groupid);
	
	return EN_Handler_Done;
}

