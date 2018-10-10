#include "CommonClienthandler.h"
#include "RouteManager.h"
#include "RobotHandlerToken.h"
#include "PBConfigBasic.h"
#include "RobotMgr.h"
#include "ClientHandlerToken.h"

ENProcessResult CRegistInnerServer::ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession)
{
	//
	const SSNotifyInnerServer ss_notify_inner_server = psession->_request_msg.ss_notify_inner_server();
	int routeid = ss_notify_inner_server.routeid();
	RouteManager::Instance()->_route_map[routeid] = ptoken->_phandler;
	VLogMsg(CLIB_LOG_LEV_ERROR,"receive noitfy from route[%d]",routeid);
	return EN_Process_Result_Completed;
}

ENHandlerResult CRequestRobotJoinMatch::ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession)
{
	const SSRequestRobotJoinMatch& request = psession->_request_msg.ss_request_robot_join_match();
	RobotManager::Instance()->PutRobot(request.matchid(), request.robot_num(), request.source_info());
	return EN_Handler_Done;
}

ENHandlerResult CResponseLogin::ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession)
{
	VLogMsg(CLIB_LOG_LEV_DEBUG, "process response login");
	ClientHandlerToken * p_robot_token = (ClientHandlerToken*)ptoken;
	const CSResponseLogin & cs_response_login = psession->_request_msg.cs_response_login();
	CRobot * probot = p_robot_token->_probot;
	if (probot)
	{
		probot->_response_msg.mutable_cs_response_login()->CopyFrom(cs_response_login);
		probot->ProcessState();
	}
	return EN_Handler_Done;
}

ENHandlerResult CResponseSdrEnterTable::ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession)
{
	VLogMsg(CLIB_LOG_LEV_DEBUG, "process response enter match");
	ClientHandlerToken * p_robot_token = (ClientHandlerToken*)ptoken;
	const CSResponseSdrEnterTable & pbResponse = psession->_request_msg.cs_response_sdr_enter_table();
	CRobot * probot = p_robot_token->_probot;
	if (probot)
	{
		probot->_response_msg.mutable_cs_response_sdr_enter_table()->CopyFrom(pbResponse);
		probot->ProcessState();
	}

	return EN_Handler_Done;
}

ENHandlerResult CResponseSkipMatchGame::ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession)
{
	VLogMsg(CLIB_LOG_LEV_DEBUG, "process response skip match");
	ClientHandlerToken * p_robot_token = (ClientHandlerToken*)ptoken;
	const CSResponseSkipMatchGame & pbResponse = psession->_request_msg.cs_response_skip_match_game();
	CRobot * probot = p_robot_token->_probot;
	if (probot)
	{
		probot->_response_msg.mutable_cs_response_skip_match_game()->CopyFrom(pbResponse);
		probot->ProcessState();
	}

	return EN_Handler_Done;
}

ENHandlerResult CResponseCoinMatchGame::ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession)
{
	VLogMsg(CLIB_LOG_LEV_DEBUG, "process response coin match");
	ClientHandlerToken * p_robot_token = (ClientHandlerToken*)ptoken;
	const CSResponseCoinMatchGame & pbResponse = psession->_request_msg.cs_response_coin_match_game();
	CRobot * probot = p_robot_token->_probot;
	if (probot)
	{
		probot->_response_msg.mutable_cs_response_coin_match_game()->CopyFrom(pbResponse);
		probot->ProcessState();
	}

	return EN_Handler_Done;
}


