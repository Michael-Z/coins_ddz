#include "ProcesserBasic.h"
#include "RouteManager.h"
#include "Common.h"

ENPreProcessResult ProcesserBasic::PreProcessRequest(CHandlerTokenBasic * ptoken,CSession * psession)
{
	VLogMsg(CLIB_LOG_LEV_TRACE,"invoke default pre preocess func.session[%d]",psession->sessionid());
	psession->_kvdb_state = EN_Session_KVDB_State_Idle;
	return EN_Pre_Process_Result_Ready;
}

ENProcessResult ProcesserBasic::ProcessUpdateUserDataCompleted(CHandlerTokenBasic * ptoken,CSession * psession)
{
	VLogMsg(CLIB_LOG_LEV_TRACE,"invoke default update user data completed func.session[%d]",psession->sessionid());
	return EN_Process_Result_Completed;
}


ENProcessResult ProcesserBasic::ProcessQueryUserData(CHandlerTokenBasic * ptoken,CSession * psession)
{
	VLogMsg(CLIB_LOG_LEV_TRACE,"invoke default request query user data func.");
	if (psession->_kvdb_state != EN_Session_KVDB_State_Wait_Query_Response)
	{
		ErrMsg("fatal error.invalid state for request user data.session[%d]",psession->sessionid());	
		return EN_Process_Result_Failed;
	}
	if(psession->_kvdb_uid_flag_map.size() == 0)
	{
		ErrMsg("warning.uid flag map is empty.no user data required.session[%d]",psession->sessionid());	
		return ProcessRequestMsg(ptoken,psession);
	}
	//
	psession->QueryUserData(RouteManager::Instance()->GetRouteByRandom());
	return EN_Process_Result_Succ;
}

ENProcessResult ProcesserBasic::ProcessUpdateUserData(CHandlerTokenBasic * ptoken,CSession * psession)
{
	VLogMsg(CLIB_LOG_LEV_TRACE,"invoke default request update user data func.");
	if(psession->_kvdb_state != EN_Session_KVDB_State_Wait_Request_Update)
	{
		ErrMsg("fatal error.invalid state for update user data.session[%d]",psession->sessionid());
		return EN_Process_Result_Failed;
	}
	if(psession->_kvdb_uid_flag_map.size() == 0)
	{
		ErrMsg("warning.uid flag map is empty.no user data setted.session[%d]",psession->sessionid());	
		return ProcessUpdateUserDataCompleted(ptoken,psession);
	}
	psession->UpdateUserData(ptoken->_phandler);
	return EN_Process_Result_Succ;
}


void ProcesserBasic::ProcessQueryUserDataFailed(CHandlerTokenBasic * ptoken,CSession * psession)
{
	VLogMsg(CLIB_LOG_LEV_DEBUG,"invoke default query kvdb data failed func.");
	return;
}

void ProcesserBasic::ProcessUpdateUserDataFailed(CHandlerTokenBasic * ptoken,CSession * psession)
{
	VLogMsg(CLIB_LOG_LEV_DEBUG,"invoke default update kvdb data failed func.");
	return;
}

ENProcessResult ProcesserBasic::ProcessResponseQueryKVDBData(CHandlerTokenBasic * ptoken,CSession * psession)
{
	VLogMsg(CLIB_LOG_LEV_TRACE,"invoke default response query kvdb data func.");
	const SSResponseGetUserData & ss_response_get_user_data = psession->_response_msg.ss_response_get_user_data();
	if (ss_response_get_user_data.result() != EN_MESSAGE_ERROR_OK)
	{
		//取数据失败
		ProcessQueryUserDataFailed(ptoken,psession);
		return EN_Process_Result_Failed;
	}
	else
	{
		const PBDataSet & data_set = ss_response_get_user_data.data_set();
		int64 uid = data_set.uid();
		//
		psession->_kvdb_uid_flag_map[uid] = true;
		psession->_kvdb_uid_data_map[uid].CopyFrom(data_set.user_data());
		if(psession->IsAllUserDataReady())
		{
			//所有数据准备就绪
			return ProcessRequestMsg(ptoken,psession);
		}
	}
	return EN_Process_Result_Succ;
}

ENProcessResult ProcesserBasic::ProcessResponseUpdateKVDBData(CHandlerTokenBasic * ptoken,CSession * psession)
{
	VLogMsg(CLIB_LOG_LEV_TRACE,"invoke default response update kvdb data func.");
	const SSResponseUpdateUserData & ss_response_update_user_data = psession->_response_msg.ss_response_update_user_data();
	if (ss_response_update_user_data.result() != EN_MESSAGE_ERROR_OK)
	{
		//取数据失败
		ProcessUpdateUserDataFailed(ptoken,psession);
		return EN_Process_Result_Failed;
	}
	else
	{
		const PBUserData & user_data = ss_response_update_user_data.user_data();
		int64 uid = ss_response_update_user_data.uid();
		//
		psession->_kvdb_uid_flag_map[uid] = true;

        Common::MergeUserData(psession->_kvdb_uid_data_map[uid], user_data);

		if(psession->IsAllUserDataReady())
		{
			//所有数据准备就绪
			return ProcessUpdateUserDataCompleted(ptoken,psession);
		}
	}
	return EN_Process_Result_Succ;
}

void ProcesserBasic::Process(CHandlerTokenBasic * ptoken,CSession * psession)
{
	//根据session状态调用处理函数
	ENProcessResult iret = EN_Process_Result_Failed;
    if (psession->_message_logic_type == EN_Message_Request)
    {
    	//先设置请求玩家数据的状态
    	ENPreProcessResult state = PreProcessRequest(ptoken,psession);
    	if(state == EN_Pre_Process_Result_Ready)
    	{
			iret = ProcessRequestMsg(ptoken,psession);
    	}
		else if (state == EN_Pre_Process_Result_Query)
		{
			iret = ProcessQueryUserData(ptoken,psession);
		}
    }
    else if (psession->_message_logic_type == EN_Message_Response)
    {
    	if (psession->_kvdb_state == EN_Session_KVDB_State_Idle)
		{
			iret = ProcessResponseMsg(ptoken,psession);
			//如果这里需要请求了数据 则请求并返回
			if(iret == EN_Process_Result_Succ && psession->_kvdb_state == EN_Session_KVDB_State_Wait_Query_Response)
			{
				iret = ProcessQueryUserData(ptoken,psession);
			}
		}
    	else if(psession->_kvdb_state == EN_Session_KVDB_State_Wait_Query_Response)
    	{
    		//没有在请求数据的情况下按照响应处理
        	iret = ProcessResponseQueryKVDBData(ptoken,psession);
    	}
		else if(psession->_kvdb_state == EN_Session_KVDB_State_Wait_Update_Response)
		{
			//等待请求数据的响应
			iret = ProcessResponseUpdateKVDBData(ptoken,psession);
		}
		else
		{
			ErrMsg("fatal error.invalid kvdb state of session");
		}
    }
    else if (psession->_message_logic_type == EN_Message_Push)
    {
		iret = ProcessPushMsg(ptoken,psession);
    }

	if (iret == EN_Process_Result_Completed || iret == EN_Process_Result_Failed)
	{
        psession->StopSessionTimer();
		SessionManager::Instance()->ReleaseSession(psession->sessionid());
	}
	else
	{
		if(psession->_kvdb_state == EN_Session_KVDB_State_Wait_Request_Update)
		{
			//
			iret = ProcessUpdateUserData(ptoken,psession);
		}
	}
}

ENProcessResult ProcesserBasic::ProcessResponseMsg(CHandlerTokenBasic * ptoken,CSession * psession)
{
	ENProcessResult iret = EN_Process_Result_Failed;
	switch(psession->msgtype())
	{
		case EN_Node_Hall:
			iret = ProcessHallMsg(ptoken,psession);
			break;
		case EN_Node_Room:
			iret = ProcessRoomMsg(ptoken,psession);
			break;
		case EN_Node_User:
			iret = ProcessUserMsg(ptoken,psession);
			break;
		case EN_Node_Game:
			iret = ProcessGameMsg(ptoken,psession);
			break;
		case EN_Node_Route:
			iret = ProcessRouteMsg(ptoken,psession);
			break;
		case EN_Node_DBProxy:
			iret = ProcessDBProxyMsg(ptoken,psession);
			break;
		default :
            // unknown node
            iret = ProcessOtherMsg(ptoken,psession);
			break;
	}
    return iret;
}

void ProcesserBasic::AddGetData(CSession* psession, long long uid, int key)
{
    SSRequestGetUserData& request = *psession->_tmp_msg.mutable_ss_request_get_user_data();
    PBDataSet& data_set = *request.mutable_data_set();
    data_set.set_uid(uid);
    bool has_find_key = false;
    for (int i = 0; i < data_set.key_list_size(); ++i)
    {
        const PBRedisData& redis_data = data_set.key_list(i);
        if (redis_data.key() == key)
        {
            has_find_key = true;
            break;
        }
    }
    if (!has_find_key)
    {
        data_set.add_key_list()->set_key(key);
    }
}


