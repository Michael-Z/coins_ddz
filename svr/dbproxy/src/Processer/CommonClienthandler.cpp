#include "CommonClienthandler.h"
#include "RouteManager.h"
#include "RedisManager.h"
#include <time.h>

ENProcessResult CRegistInnerServer::ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession)
{
	//
	const SSNotifyInnerServer ss_notify_inner_server = psession->_request_msg.ss_notify_inner_server();
	int routeid = ss_notify_inner_server.routeid();
	RouteManager::Instance()->_route_map[routeid] = ptoken->_phandler;
	VLogMsg(CLIB_LOG_LEV_ERROR,"receive noitfy from route[%d]",routeid);
	return EN_Process_Result_Completed;
}

/*
ENProcessResult CRequestLogin::ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession)
{
	const SSRequestLogin& request = psession->_request_msg.ss_request_login();

	PBCSMsg msg;
	SSResponseLogin& response = *msg.mutable_ss_response_login();
	long long uid = -1;
    // 第三方账号验证token
    if (EN_Accout_Guest != request.acc_type() &&
        EN_Account_Robot != request.acc_type())
    {
        if (!RedisManager::Instance()->CheckAccountToken(request.account(), request.acc_type(), request.token()))
        {
            response.set_result(EN_MESSAGE_INVALID_ACC_TOKEN);
            Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
        	return EN_Process_Result_Completed;
        }
    }
    // 取uid
    if (RedisManager::Instance()->QueryAccountUID(request.account(), request.acc_type(), uid))
    {
        PBDataSet& data_set = *response.mutable_data_set();
        data_set.set_uid(uid);
        PBRedisData& redis_data = *data_set.add_key_list();
        redis_data.set_key(PBUserDataField::kUserInfo);
        // 取uid数据
        if (RedisManager::Instance()->QueryUserInfo(uid, redis_data))
        {
            response.set_result(EN_MESSAGE_ERROR_OK);
        }
        else
        {
            response.set_result(EN_MESSAGE_DB_INVALID);
        }
    }
    else
    {
		response.set_result(EN_MESSAGE_DB_NOT_FOUND);
    }
    Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
	return EN_Process_Result_Completed;
}

ENProcessResult CRequestCreateUid::ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession)
{
	PBCSMsg msg;
	SSResponseCreateUid& response = *msg.mutable_ss_response_create_uid();
	long long uid = -1;
    if (RedisManager::Instance()->CreateNewUID(uid))
    {
        response.set_result(EN_MESSAGE_ERROR_OK);
        response.set_uid(uid + 10000);
    }
    else
    {
        response.set_result(EN_MESSAGE_DB_INVALID);
    }

    Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
	return EN_Process_Result_Completed;
}

ENProcessResult CRequestAccBindUid::ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession)
{
    const SSRequestAccBindUid& request = psession->_request_msg.ss_request_acc_bind_uid();
	PBCSMsg msg;
	SSResponseAccBindUid& response = *msg.mutable_ss_response_acc_bind_uid();
    if (RedisManager::Instance()->SetAccountUID(request.account(), request.acc_type(), request.uid()))
    {
        response.set_result(EN_MESSAGE_ERROR_OK);
    }
    else
    {
        response.set_result(EN_MESSAGE_DB_INVALID);
    }

    Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
	return EN_Process_Result_Completed;
}
*/

ENProcessResult CRequestQueryData::ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession)
{
    const SSRequestQueryData& request = psession->_request_msg.ss_request_query_data();
    PBCSMsg msg;
    SSResponseQueryData& response = *msg.mutable_ss_response_query_data();
    response.mutable_data_set()->CopyFrom(request.data_set());
    // key loop
    PBDataSet& data_set = *response.mutable_data_set();
    for (int i = 0; i < data_set.key_list_size(); ++i)
    {
        PBRedisData& redis_data = *data_set.mutable_key_list(i);
        RedisManager::Instance()->QueryDataField(data_set.uid(), redis_data);
    }
    // response
    Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
	return EN_Process_Result_Completed;
}

ENProcessResult CRequestSaveData::ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession)
{
    const SSRequestSaveData& request = psession->_request_msg.ss_request_save_data();
    PBCSMsg msg;
    SSResponseSaveData& response = *msg.mutable_ss_response_save_data();
    response.mutable_data_set()->CopyFrom(request.data_set());

    // key loop
    PBDataSet& data_set = *response.mutable_data_set();
    for (int i = 0; i < data_set.key_list_size(); ++i)
    {
        PBRedisData& redis_data = *data_set.mutable_key_list(i);
        RedisManager::Instance()->SaveDataField(data_set.uid(), redis_data);
    }

    // response
    Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
	return EN_Process_Result_Completed;
}

ENProcessResult CRequestAccountUid::ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession)
{
	const SSRequestAccountUid & request = psession->_request_msg.ss_request_acc_uid();

	PBCSMsg msg;
	SSResponseAccountUid & response = *msg.mutable_ss_response_acc_uid();
	long long uid = -1;
    // 第三方账号验证token
    if (EN_Accout_Guest != request.acc_type() && EN_Account_Robot != request.acc_type())
    {
        if (!RedisManager::Instance()->CheckAccountToken(request.account(), request.acc_type(), request.token()))
        {
            response.set_result(EN_MESSAGE_INVALID_ACC_TOKEN);
            Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
        	return EN_Process_Result_Completed;
        }
    }
    // 取uid
    if (RedisManager::Instance()->QueryAccountUID(request.account(), request.acc_type(), uid))
    {
    	response.set_result(EN_MESSAGE_ERROR_OK);
    	response.set_uid(uid);
    }
	else
	{
		//response.set_result(EN_MESSAGE_DB_NOT_FOUND);
		if(request.auto_create() == false)
		{
			 response.set_result(EN_MESSAGE_DB_NOT_FOUND);
		}
		else
		{
			long long uid = -1;
			//创建uid
		    if (RedisManager::Instance()->CreateNewUID(uid))
		    {
		    	//绑定account
		    	if (RedisManager::Instance()->SetAccountUID(request.account(), request.acc_type(), uid+10000))
			    {
			        response.set_result(EN_MESSAGE_ERROR_OK);
					response.set_uid(uid + 10000);
					response.set_iscreated(true);
			    }
			    else
			    {
			        response.set_result(EN_MESSAGE_DB_INVALID);
			    }
		    }
		    else
		    {
		        response.set_result(EN_MESSAGE_DB_INVALID);
		    }
		}
	}
	Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
	return EN_Process_Result_Completed;
}

ENProcessResult CRequestUpdateRankList::ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession)
{
    SSRequestUpdateRankList request = psession->_request_msg.ss_request_update_rank_list();
    RedisManager::Instance()->UpdateRankList(request.rank_id(), request.rank_key(), request.rank_score());
	return EN_Process_Result_Completed;
}


