#include "NewProcessor.h"
#include "RouteManager.h"
#include "Common.h"

void NewProcessor::Process(CHandlerTokenBasic* ptoken, CSession* psession)
{
    ENHandlerResult result = EN_Handler_Done;
    if (psession->_message_logic_type == EN_Message_Request)
    {
        psession->_kvdb_state = EN_Session_Idle;
        result = ProcessRequestMsg(ptoken, psession);
    }
    else if (psession->_message_logic_type == EN_Message_Response)
    {
        switch (psession->_kvdb_state)
        {
            case EN_Session_Idle:
                result = ProcessResponseMsg(ptoken, psession);
                break;
            case EN_Session_Wait_Get_Data:
                result = OnGetResponse(ptoken, psession);
                break;
            case EN_Session_Wait_Update_Data:
                result = OnUpdateResponse(ptoken, psession);
                break;
            default: break;
        }
    }

    while (1)
    {
        switch (result)
        {
            case EN_Handler_Get:
                result = OnGetData(ptoken, psession);
                break;
            case EN_Handler_Save:
                result = OnUpdateData(ptoken, psession);
                break;
            case EN_Handler_Done:
                EndProcess(ptoken, psession);
                return;
            case EN_Handler_Succ:
                return;
            default: return;
        }
    }
}

void NewProcessor::EndProcess(CHandlerTokenBasic* ptoken, CSession* psession)
{
    psession->StopSessionTimer();
    SessionManager::Instance()->ReleaseSession(psession->sessionid());
}

ENHandlerResult NewProcessor::ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession)
{
    VLogMsg(CLIB_LOG_LEV_DEBUG, "default process request");
    return EN_Handler_Done;
}

ENHandlerResult NewProcessor::ProcessResponseMsg(CHandlerTokenBasic* ptoken, CSession* psession)
{
    VLogMsg(CLIB_LOG_LEV_DEBUG, "default process response");
    return EN_Handler_Done;
}

ENHandlerResult NewProcessor::ProcessGetSucc(CHandlerTokenBasic* ptoken, CSession* psession)
{
    VLogMsg(CLIB_LOG_LEV_DEBUG, "default process get succ");
    return EN_Handler_Done;
}

ENHandlerResult NewProcessor::ProcessGetFailed(CHandlerTokenBasic* ptoken, CSession* psession)
{
    VLogMsg(CLIB_LOG_LEV_DEBUG, "default process get failed,msg cmd:[%d]",psession->_request_msg.msg_union_case());
    return EN_Handler_Done;
}

ENHandlerResult NewProcessor::ProcessUpdateSucc(CHandlerTokenBasic* ptoken, CSession* psession)
{
    VLogMsg(CLIB_LOG_LEV_DEBUG, "default process update succ");
    return EN_Handler_Done;
}

ENHandlerResult NewProcessor::ProcessUpdateFailed(CHandlerTokenBasic* ptoken, CSession* psession)
{
    VLogMsg(CLIB_LOG_LEV_DEBUG, "default process update failed");
    return EN_Handler_Done;
}

void NewProcessor::ProcessSessionTimeOut(CHandlerTokenBasic* ptoken, CSession* psession)
{
	VLogMsg(CLIB_LOG_LEV_DEBUG, "default process update failed");
	return;
}

ENHandlerResult NewProcessor::OnGetData(CHandlerTokenBasic* ptoken, CSession* psession)
{
    if (EN_Session_Idle != psession->_kvdb_state)
    {
        ErrMsg("can't get, state: %d", psession->_kvdb_state);
        return EN_Handler_Done;
    }
    if (0 == psession->_get_data_map.size())
        return EN_Handler_Done;

    map<int64, set<int> >::iterator map_iter = psession->_get_data_map.begin();
    for (; map_iter != psession->_get_data_map.end(); ++map_iter)
    {
        PBCSMsg msg;
        PBDataSet& data_set = *msg.mutable_ss_request_get_user_data()->mutable_data_set();
        data_set.set_uid(map_iter->first);
        set<int>::iterator set_iter = map_iter->second.begin();
        for (; set_iter != map_iter->second.end(); ++set_iter)
        {
            PBRedisData& key = *data_set.add_key_list();
            key.set_key(*set_iter);
        }
        Message::SendRequestToUser(RouteManager::Instance()->GetRouteByRandom(), psession, msg ,map_iter->first);
    }
    psession->_kvdb_state = EN_Session_Wait_Get_Data;
    return EN_Handler_Succ;
}

ENHandlerResult NewProcessor::OnGetResponse(CHandlerTokenBasic* ptoken, CSession* psession)
{
    if (!psession->_response_msg.has_ss_response_get_user_data())
    {
        ErrMsg("wrong msg id: 0x%x.", psession->_response_msg.msg_union_case());
        return EN_Handler_Done;
    }

    const SSResponseGetUserData& response = psession->_response_msg.ss_response_get_user_data();
    if (response.result() != EN_MESSAGE_ERROR_OK)
    {
        return ProcessGetFailed(ptoken, psession);
    }

    const PBDataSet& data_set = response.data_set();
    Common::MergeUserData(psession->_kvdb_uid_data_map[data_set.uid()], data_set.user_data());
    psession->_get_data_map.erase(data_set.uid());
    if (0 != psession->_get_data_map.size())
    {
        return EN_Handler_Succ;
    }

    psession->_kvdb_state = EN_Session_Idle;
    return ProcessGetSucc(ptoken, psession);
}

ENHandlerResult NewProcessor::OnUpdateData(CHandlerTokenBasic* ptoken, CSession* psession)
{
    if (EN_Session_Idle != psession->_kvdb_state)
    {
        ErrMsg("can't get, state: %d", psession->_kvdb_state);
        return EN_Handler_Done;
    }
    if (0 == psession->_update_data_map.size())
        return EN_Handler_Done;

    map<int64, vector<PBUpdateData> >::iterator map_iter = psession->_update_data_map.begin();
    for (; map_iter != psession->_update_data_map.end(); ++map_iter)
    {
        PBCSMsg msg;
        SSRequestUpdateUserData& request = *msg.mutable_ss_request_update_user_data();
        request.set_uid(map_iter->first);
        vector<PBUpdateData>::iterator set_iter = map_iter->second.begin();
        for (; set_iter != map_iter->second.end(); ++set_iter)
        {
            PBUpdateData& update_data = *request.add_key_list();
            update_data.CopyFrom(*set_iter);
        }
        Message::SendRequestToUser(RouteManager::Instance()->GetRouteByRandom(), psession, msg , map_iter->first);
    }

    psession->_kvdb_state = EN_Session_Wait_Update_Data;
    return EN_Handler_Succ;
}

ENHandlerResult NewProcessor::OnUpdateResponse(CHandlerTokenBasic* ptoken, CSession* psession)
{
    if (!psession->_response_msg.has_ss_response_update_user_data())
    {
        ErrMsg("wrong msg id: 0x%x.", psession->_response_msg.msg_union_case());
        return EN_Handler_Done;
    }

    const SSResponseUpdateUserData& response = psession->_response_msg.ss_response_update_user_data();
    if (response.result() != EN_MESSAGE_ERROR_OK)
    {
        return ProcessUpdateFailed(ptoken, psession);
    }

    Common::MergeUserData(psession->_kvdb_uid_data_map[response.uid()], response.user_data());
    psession->_update_data_map.erase(response.uid());
    if (0 != psession->_update_data_map.size())
    {
        return EN_Handler_Succ;
    }

    psession->_kvdb_state = EN_Session_Idle;
    return ProcessUpdateSucc(ptoken, psession);
}

