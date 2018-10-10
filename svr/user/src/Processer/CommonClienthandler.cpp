#include "CommonClienthandler.h"
#include "RouteManager.h"
#include "UserManager.h"
#include "UserHandlerToken.h"

ENProcessResult CRegistInnerServer::ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession)
{
	//
	const SSNotifyInnerServer ss_notify_inner_server = psession->_request_msg.ss_notify_inner_server();
	int routeid = ss_notify_inner_server.routeid();
	RouteManager::Instance()->_route_map[routeid] = ptoken->_phandler;
	VLogMsg(CLIB_LOG_LEV_ERROR,"receive noitfy from route[%d]",routeid);
	return EN_Process_Result_Completed;
}

ENProcessResult CRequestGetUserData::ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession)
{
	const SSRequestGetUserData& request = psession->_request_msg.ss_request_get_user_data();
    const PBDataSet& data_set = request.data_set();

    // 本地数据不齐，向db拉取
    PBCSMsg db_msg;
    SSRequestQueryData& db_request = *db_msg.mutable_ss_request_query_data();
    if (!CheckUserData(data_set, *db_request.mutable_data_set()))
    {
    	psession->_uid = data_set.uid();
        Message::SendRequestMsg(RouteManager::Instance()->GetRouteByRandom(), psession, db_msg, EN_Node_DBProxy,1,EN_Route_hash);
		return EN_Process_Result_Succ;
    }

	//在本地找到请求的数据 直接返回
    PBCSMsg msg;
    SSResponseGetUserData& response = *msg.mutable_ss_response_get_user_data();
    response.set_result(EN_MESSAGE_ERROR_OK);
    response.mutable_data_set()->set_uid(data_set.uid());
	FillResponseUserData(data_set, *response.mutable_data_set()->mutable_user_data());
    Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
	return EN_Process_Result_Completed;
}

ENProcessResult CRequestGetUserData::ProcessDBProxyMsg(CHandlerTokenBasic * ptoken,CSession *psession)
{
    // 从db返回填充本地数据
	const SSResponseQueryData& db_response = psession->_response_msg.ss_response_query_data();
    UserManager::Instance()->OnParseUserData(db_response.data_set());

    // 检查是否有足够数据
	const SSRequestGetUserData& request = psession->_request_msg.ss_request_get_user_data();
    const PBDataSet& data_set = request.data_set();
    PBDataSet tmp_data_set;
    if (!CheckUserData(data_set, tmp_data_set))
    {
    	PBCSMsg msg;
    	SSResponseGetUserData& response = *msg.mutable_ss_response_get_user_data();
        response.set_result(EN_MESSAGE_DB_NOT_FOUND);
        Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Process_Result_Completed;
    }

	// 返回数据
	PBCSMsg msg;
	SSResponseGetUserData& response = *msg.mutable_ss_response_get_user_data();
    response.set_result(EN_MESSAGE_ERROR_OK);
    response.mutable_data_set()->set_uid(data_set.uid());
	FillResponseUserData(data_set, *response.mutable_data_set()->mutable_user_data());
    Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
	return EN_Process_Result_Completed;
}

ENProcessResult CReuqestUpdateUserData::ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession)
{
    const SSRequestUpdateUserData& request = psession->_request_msg.ss_request_update_user_data();

    // 先检查本地是否有缓存
    PBCSMsg db_msg;
    SSRequestQueryData& db_request = *db_msg.mutable_ss_request_query_data();
    if (!CheckUserData(request, *db_request.mutable_data_set()))
    {
    	psession->_uid = db_request.data_set().uid();
        Message::SendRequestMsg(RouteManager::Instance()->GetRouteByRandom(), psession, db_msg, EN_Node_DBProxy,1,EN_Route_hash);
        psession->_fsm_state = EN_Wait_DB_Query_Data;
		return EN_Process_Result_Succ;
    }
    else
    {
        // 本地有所有数据
        return OnUpdateData(ptoken, psession);
    }
}

ENProcessResult CReuqestUpdateUserData::ProcessDBProxyMsg(CHandlerTokenBasic * ptoken,CSession * psession)
{
    if (psession->_fsm_state == EN_Wait_DB_Query_Data)
    {
    	const SSResponseQueryData& db_response = psession->_response_msg.ss_response_query_data();
    	//根据dbproxy返回先填充本地数据
        UserManager::Instance()->OnParseUserData(db_response.data_set());
        return OnUpdateData(ptoken, psession);
    }
    else if (psession->_fsm_state == EN_Wait_DB_Save_Data)
    {
        return OnUpdateDataRsp(ptoken, psession);
    }
    else
    {
        return EN_Process_Result_Failed;
    }
}

ENProcessResult CReuqestUpdateUserData::OnUpdateData(CHandlerTokenBasic * ptoken,CSession * psession)
{
    const SSRequestUpdateUserData& request = psession->_request_msg.ss_request_update_user_data();
    PBCSMsg db_msg;
    SSRequestSaveData& db_request = *db_msg.mutable_ss_request_save_data();

    PBDataSet& data_set = *db_request.mutable_data_set();
    int64 uid = request.uid();
    data_set.set_uid(uid);
    // 备份数据
    PBUserData user_bak;
    PBUserData* puser = UserManager::Instance()->GetUserData(uid);
    if (puser) user_bak.CopyFrom(*puser);
    // key loop
	bool result = true;
    for (int i = 0; i < request.key_list_size(); ++i)
    {
        const PBUpdateData& update_key = request.key_list(i);
        PBRedisData& redis_data = *data_set.add_key_list();
        switch (update_key.key())
        {
            case PBUserDataField::kUserInfo:    result = UserManager::Instance()->OnUpdateUserInfo(uid, update_key, redis_data); break;
			case PBUserDataField::kUserRecord:	result = UserManager::Instance()->OnUpdateUserRecord(uid, update_key, redis_data);break;
			case PBUserDataField::kUserTableInfo:	result = UserManager::Instance()->OnUpdateUserTableInfo(uid, update_key, redis_data);break;
			case PBUserDataField::kUserTeaBarData:	result = UserManager::Instance()->OnUpdateUserTeaBarData(uid, update_key, redis_data); break;
            case PBUserDataField::kUserGameData:    result = UserManager::Instance()->OnUpdateUserGameData(uid, update_key, redis_data); break;
			default: result = false; break;
        }
        if (!result)
        {
            // 回滚数据
            if (puser) UserManager::Instance()->UpdateUserData(uid, user_bak);

			ErrMsg("update uid[%lld] key[%d] failed", uid, update_key.key());
     		PBCSMsg msg;
    		SSResponseUpdateUserData & response = *msg.mutable_ss_response_update_user_data();
    		response.set_result(EN_MESSAGE_DB_SAVE_FAILED);
            Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
            return EN_Process_Result_Failed;
        }
    }
	psession->_uid = uid;
    Message::SendRequestMsg(RouteManager::Instance()->GetRouteByRandom(), psession, db_msg, EN_Node_DBProxy,1,EN_Route_hash);
    psession->_fsm_state = EN_Wait_DB_Save_Data;
    return EN_Process_Result_Succ;
}

ENProcessResult CReuqestUpdateUserData::OnUpdateDataRsp(CHandlerTokenBasic * ptoken,CSession * psession)
{
    const SSResponseSaveData& db_response = psession->_response_msg.ss_response_save_data();
    const PBDataSet& data_set = db_response.data_set();

    for (int i = 0; i < data_set.key_list_size(); ++i)
    {
        if (!data_set.key_list(i).result())
        {
            PBCSMsg msg;
            SSResponseUpdateUserData& response = *msg.mutable_ss_response_update_user_data();
            response.set_result(EN_MESSAGE_DB_SAVE_FAILED);
            Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
    		return EN_Process_Result_Completed;
        }
    }

    PBDataSet tmp_data_set;
    if (!CheckUserData(data_set, tmp_data_set))
    {
    	PBCSMsg msg;
    	SSResponseUpdateUserData& response = *msg.mutable_ss_response_update_user_data();
        response.set_result(EN_MESSAGE_DB_NOT_FOUND);
        Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Process_Result_Completed;
    }

    PBCSMsg msg;
    SSResponseUpdateUserData& response = *msg.mutable_ss_response_update_user_data();
    response.set_result(EN_MESSAGE_ERROR_OK);
	response.set_uid(data_set.uid());
    FillResponseUserData(data_set, *response.mutable_user_data());
    Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
	return EN_Process_Result_Completed;
}

