#include "CommonClienthandler.h"
#include "RouteManager.h"
#include "JsonHelper.h"
#include "Common.h"
#include <string>

ENProcessResult CRegistInnerServer::ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession)
{
	const SSNotifyInnerServer ss_notify_inner_server = psession->_request_msg.ss_notify_inner_server();
	int routeid = ss_notify_inner_server.routeid();
	RouteManager::Instance()->_route_map[routeid] = ptoken->_phandler;
	VLogMsg(CLIB_LOG_LEV_ERROR,"receive noitfy from route[%d]",routeid);
	return EN_Process_Result_Completed;
}

ENProcessResult CProcessPHPMsg::ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession)
{
    // 根据msg id 转发消息
	int cmd = psession->_request_msg.msg_union_case();
    // to hall
    psession->_head.set_cmd(cmd);
    if (cmd >= 0x1201 && cmd <= 0x1500)
    {
        Message::GMSendRequestToHall(RouteManager::Instance()->GetRouteByRandom(), psession, psession->_request_msg);
    }
	// to teabar
	else if (cmd >= 0x8001 && cmd <= 0x8100)
	{
		Message::GMSendRequestToTeaBar(RouteManager::Instance()->GetRouteByRandom(), psession, psession->_request_msg);		
		if (cmd == 0x8012 || cmd == 0x802E)
		{
			PHPSocketMap::iterator iter = JsonHelper::_socketmap.find(psession->sessionid());
			if (iter != JsonHelper::_socketmap.end())
			{
				Json::Value root;
				root["result"] = 0;
				Json::FastWriter writer;
				string json_msg = writer.write(root);
				Message::SendMsgToPHP(iter->second, json_msg);
			}
		}
	}
    // to room
    else if (cmd >= 0x1001 && cmd <= 0x1200)
    {
        Message::GMSendRequestToRoom(RouteManager::Instance()->GetRouteByRandom(), psession, psession->_request_msg);
    }
    // to user
    else
    {
        Message::GMSendRequestToUser(RouteManager::Instance()->GetRouteByRandom(), psession, psession->_request_msg);
    }

    return EN_Process_Result_Succ;
}

ENProcessResult CProcessPHPMsg::ProcessResponseMsg(CHandlerTokenBasic * ptoken, CSession * psession)
{
    PHPSocketMap::iterator iter = JsonHelper::_socketmap.find(psession->sessionid());
    if (iter == JsonHelper::_socketmap.end())
    {
        return EN_Process_Result_Failed;
    }

    std::string json_msg;
    if (JsonHelper::ProtoToJson(psession->_head, psession->_response_msg, json_msg))
    {
        Message::SendMsgToPHP(iter->second, json_msg);
    }
    return EN_Process_Result_Completed;
}

ENHandlerResult CUpdateChipsMulit::ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession)
{
    const GMUpdateChipsMulti & request = psession->_request_msg.gm_update_chips_multi();
    if (request.need_del_list_size())
    {
        for (int i = 0; i < request.need_del_list_size(); ++i)
        {
            const GMUpdateList& gm_update_data = request.need_del_list(i);
            PBUpdateData update;
            update.set_key(PBUserDataField::kUserInfo);
            PBDBAtomicField& field = *update.add_field_list();
            field.set_field(EN_DB_Field_Chips);
            field.set_strategy(EN_Update_Strategy_Inc);
            field.set_intval(gm_update_data.chips());
            field.set_reason(gm_update_data.reason());
            psession->NewAddUpdateData(gm_update_data.uid(), update);
        }
        psession->_fsm_state = Wait_Del;
        return EN_Handler_Save;
    }
    else if (request.update_list_size())
    {
        for (int i = 0; i < request.update_list_size(); ++i)
        {
            const GMUpdateList& gm_update_data = request.update_list(i);
            PBUpdateData update;
            update.set_key(PBUserDataField::kUserInfo);
            PBDBAtomicField& field = *update.add_field_list();
            field.set_field(EN_DB_Field_Chips);
            field.set_strategy(EN_Update_Strategy_Inc);
            field.set_intval(gm_update_data.chips());
            field.set_reason(gm_update_data.reason());
            psession->NewAddUpdateData(gm_update_data.uid(), update);
        }
        psession->_fsm_state = Wait_Update;
        return EN_Handler_Save;
    }
    return EN_Handler_Done;
}

ENHandlerResult CUpdateChipsMulit::ProcessUpdateSucc(CHandlerTokenBasic* ptoken, CSession * psession)
{
    PHPSocketMap::iterator iter = JsonHelper::_socketmap.find(psession->sessionid());
    if (iter == JsonHelper::_socketmap.end())
        return EN_Handler_Done;

    const GMUpdateChipsMulti & request = psession->_request_msg.gm_update_chips_multi();
    if (psession->_fsm_state == Wait_Del)
    {
        if (request.update_list_size())
        {
            for (int i = 0; i < request.update_list_size(); ++i)
            {
                const GMUpdateList& gm_update_data = request.update_list(i);
                PBUpdateData update;
	            update.set_key(PBUserDataField::kUserInfo);
	            PBDBAtomicField& field = *update.add_field_list();
	            field.set_field(EN_DB_Field_Chips);
	            field.set_strategy(EN_Update_Strategy_Inc);
	            field.set_intval(gm_update_data.chips());
	            field.set_reason(gm_update_data.reason());
	            psession->NewAddUpdateData(gm_update_data.uid(), update);
            }
            psession->_fsm_state = Wait_Update;
            return EN_Handler_Save;
        }
        else
        {
            std::string json_msg;
            Json::Value root;
            Json::FastWriter writer;
            root["result"] = 0;
            json_msg = writer.write(root);
            Message::SendMsgToPHP(iter->second, json_msg);
            return EN_Handler_Done;
        }
    }
    else if (psession->_fsm_state == Wait_Update)
    {
        std::string json_msg;
        Json::Value root;
        Json::FastWriter writer;
        root["result"] = 0;
        json_msg = writer.write(root);
        Message::SendMsgToPHP(iter->second, json_msg);
        return EN_Handler_Done;
    }
    return EN_Handler_Done;
}

ENHandlerResult CUpdateChipsMulit::ProcessUpdateFailed(CHandlerTokenBasic* ptoken, CSession * psession)
{
    PHPSocketMap::iterator iter = JsonHelper::_socketmap.find(psession->sessionid());
    if (iter != JsonHelper::_socketmap.end())
    {
        std::string json_msg;
        Json::Value root;
        Json::FastWriter writer;
        root["result"] = EN_MESSAGE_DB_SAVE_FAILED;
        json_msg = writer.write(root);
        Message::SendMsgToPHP(iter->second, json_msg);
    }
    return EN_Handler_Done;
}

ENHandlerResult CPushMessageMulit::ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession)
{
	PHPSocketMap::iterator iter = JsonHelper::_socketmap.find(psession->sessionid());
	const GMPushMessageMulti & gm_push_message_multi = psession->_request_msg.gm_push_message_multi();
	if(gm_push_message_multi.all())
	{
		Message::BroadcastToConnect(RouteManager::Instance()->GetRouteByRandom(),psession->_request_msg);	
		if (iter != JsonHelper::_socketmap.end())
	    {
	        std::string json_msg;
	        Json::Value root;
	        Json::FastWriter writer;
	        root["result"] = 0;
	        json_msg = writer.write(root);
	        Message::SendMsgToPHP(iter->second, json_msg);
	    }
		return EN_Handler_Done;
	}
	else
	{
		if(gm_push_message_multi.uid_list_size()>20)
		{
			if (iter != JsonHelper::_socketmap.end())
		    {
		        std::string json_msg;
		        Json::Value root;
		        Json::FastWriter writer;
		        root["result"] = 0;
		        json_msg = writer.write(root);
		        Message::SendMsgToPHP(iter->second, json_msg);
		    }
			return EN_Handler_Done;
		}
		for(int i=0;i<gm_push_message_multi.uid_list_size();i++)
		{
			int64 uid = gm_push_message_multi.uid_list(i);
			psession->NewAddGetData(uid,PBUserDataField::kUserInfo);
		}
		return EN_Handler_Get;
	}
	if (iter != JsonHelper::_socketmap.end())
    {
        std::string json_msg;
        Json::Value root;
        Json::FastWriter writer;
        root["result"] = 0;
        json_msg = writer.write(root);
        Message::SendMsgToPHP(iter->second, json_msg);
    }
	return EN_Handler_Done;
}

ENHandlerResult CPushMessageMulit::ProcessGetSucc(CHandlerTokenBasic * ptoken,CSession * psession)
{
	const GMPushMessageMulti & gm_push_message_multi = psession->_request_msg.gm_push_message_multi();
	for(int i=0;i<gm_push_message_multi.uid_list_size();i++)
	{
		int64 uid = gm_push_message_multi.uid_list(i);
		const PBUser & user = psession->_kvdb_uid_data_map[uid].user_info();
		Message::PushInnerMsg(RouteManager::Instance()->GetRouteByRandom(),0,psession->_request_msg,EN_Node_Connect,user.hallsvid());
	}
	PHPSocketMap::iterator iter = JsonHelper::_socketmap.find(psession->sessionid());
	if (iter != JsonHelper::_socketmap.end())
    {
        std::string json_msg;
        Json::Value root;
        Json::FastWriter writer;
        root["result"] = 0;
        json_msg = writer.write(root);
        Message::SendMsgToPHP(iter->second, json_msg);
    }
	return EN_Handler_Done;
}

ENHandlerResult CRequestUserActivityGameInfo::ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession)
{
    const CSRequestUserActivityGameInfo & pbRequest = psession->_request_msg.cs_request_user_activity_game_info();
    int iUid = pbRequest.uid();

    psession->NewAddGetData(iUid, PBUserDataField::kUserInfo);
    psession->NewAddGetData(iUid,PBUserDataField::kUserGameData);

    return EN_Handler_Get;
}

ENHandlerResult CRequestUserActivityGameInfo::ProcessGetSucc(CHandlerTokenBasic * ptoken,CSession * psession)
{
    const CSRequestUserActivityGameInfo & pbRequest = psession->_request_msg.cs_request_user_activity_game_info();
    int iUid = pbRequest.uid();
    int iActivityType = pbRequest.activity_type();
    int iGameType = pbRequest.game_type();

    PBUserGameData & pbUserGameData = *psession->_kvdb_uid_data_map[iUid].mutable_user_game_data();
    switch (iActivityType)
    {
        case EN_User_Game_Info_Skip_Match:
            {
                PBUserSkipMatchInfo & pbUserSkipMatchInfo = *pbUserGameData.mutable_user_skip_match_info();
                PBUserSkipMatchInfoItem * pbInfoItem = NULL;
                for(int i = 0 ; i < pbUserSkipMatchInfo.skip_match_info_item_size() ; i ++)
                {
                    PBUserSkipMatchInfoItem * pbItem = pbUserSkipMatchInfo.mutable_skip_match_info_item(i);
                    if(pbItem->skipmatch_type() == iGameType)
                    {
                        pbInfoItem = pbItem;
                    }
                }

                //如果这类游戏的游戏数据不存在，则重新让user保存
                if(!pbInfoItem)
                {
                    PBUpdateData pbUpdate;
                    pbUpdate.set_key(PBUserDataField::kUserGameData);
                    PBDBAtomicField & pbField = *pbUpdate.add_field_list();
                    pbField.set_activity_type(EN_User_Game_Info_Skip_Match);
                    pbField.set_game_type(iGameType);
                    pbField.set_field(EN_DB_Field_Skip_Match_Info_Init);
                    psession->NewAddUpdateData(iUid, pbUpdate);

                    return EN_Handler_Save;
                }
                else
                {
                    std::string szJsonMsg;
                    FileResponseMsgBySkipInfo(pbUserSkipMatchInfo,szJsonMsg,iGameType);

                    PHPSocketMap::iterator iter = JsonHelper::_socketmap.find(psession->sessionid());
                    if (iter != JsonHelper::_socketmap.end())
                    {
                        Message::SendMsgToPHP(iter->second, szJsonMsg);
                        return EN_Handler_Done;
                    }
                }
            }
            break;
        default:
            break;
    }

    //没有这类数据则返回为false
    PHPSocketMap::iterator iter = JsonHelper::_socketmap.find(psession->sessionid());
    if (iter != JsonHelper::_socketmap.end())
    {
        std::string szJsonMsg;
        Json::Value jvUserGameDataRoot;
        Json::FastWriter writer;
        jvUserGameDataRoot["result"] = 1;
        szJsonMsg = writer.write(jvUserGameDataRoot);
        Message::SendMsgToPHP(iter->second, szJsonMsg);
    }

    return EN_Handler_Done;
}

ENHandlerResult CRequestUserActivityGameInfo::ProcessUpdateSucc(CHandlerTokenBasic * ptoken,CSession * psession)
{
    const CSRequestUserActivityGameInfo & pbRequest = psession->_request_msg.cs_request_user_activity_game_info();
    int iActivityType = pbRequest.activity_type();
    int iGameType = pbRequest.game_type();

    SSResponseUpdateUserData & pbResponse = *psession->_response_msg.mutable_ss_response_update_user_data();
    PBUserData & pbUserData = *pbResponse.mutable_user_data();
    PBUserGameData & pbUserGameData = *pbUserData.mutable_user_game_data();
    if(pbResponse.result() == EN_MESSAGE_ERROR_OK)
    {
        switch (iActivityType)
        {
        case EN_User_Game_Info_Skip_Match:
        {
            PBUserSkipMatchInfo & pbUserSkipMatchInfo = *pbUserGameData.mutable_user_skip_match_info();
            PBUserSkipMatchInfoItem * pbInfoItem = NULL;
            for(int i = 0 ; i < pbUserSkipMatchInfo.skip_match_info_item_size() ; i ++)
            {
                PBUserSkipMatchInfoItem * pbItem = pbUserSkipMatchInfo.mutable_skip_match_info_item(i);
                if(pbItem->skipmatch_type() == iGameType)
                {
                    pbInfoItem = pbItem;
                }
            }

            if(!pbInfoItem)
            {
                break;
            }

            std::string szJsonMsg;
            FileResponseMsgBySkipInfo(pbUserSkipMatchInfo,szJsonMsg,iGameType);

            PHPSocketMap::iterator iter = JsonHelper::_socketmap.find(psession->sessionid());
            if (iter != JsonHelper::_socketmap.end())
            {
                Message::SendMsgToPHP(iter->second, szJsonMsg);
                return EN_Handler_Done;
            }
        }

            break;
        default:
            break;
        }
    }

    //没有这类数据则返回为false
    PHPSocketMap::iterator iter = JsonHelper::_socketmap.find(psession->sessionid());
    if (iter != JsonHelper::_socketmap.end())
    {
        std::string szJsonMsg;
        Json::Value jvUserGameDataRoot;
        Json::FastWriter writer;
        jvUserGameDataRoot["result"] = 1;
        szJsonMsg = writer.write(jvUserGameDataRoot);
        Message::SendMsgToPHP(iter->second, szJsonMsg);
    }

    return EN_Handler_Done;
}

void CRequestUserActivityGameInfo::FileResponseMsgBySkipInfo(const PBUserSkipMatchInfo & a_pbUserSkipMatchInfo,std::string & a_szJsonMsg,int a_iGameType)
{
    Json::FastWriter writer;
    Json::Value jvUserGameDataRoot;
    jvUserGameDataRoot["result"] = EN_MESSAGE_ERROR_OK;
    Json::Value jvSkipMatchInfoRoot;
    Json::Value jvSkipMatchInfoItemRoot;
    for(int i = 0 ; i < a_pbUserSkipMatchInfo.skip_match_info_item_size() ; i ++)
    {
        const PBUserSkipMatchInfoItem & pbUserSkipMatcahInfoItem = a_pbUserSkipMatchInfo.skip_match_info_item(i);
        if(pbUserSkipMatcahInfoItem.skipmatch_type() == a_iGameType)
        {
            jvSkipMatchInfoItemRoot["skipmatch_type"] = pbUserSkipMatcahInfoItem.skipmatch_type();
            jvSkipMatchInfoItemRoot["skipmatch_level"] = pbUserSkipMatcahInfoItem.skipmatch_level();
            jvSkipMatchInfoItemRoot["skipmatch_state"] = pbUserSkipMatcahInfoItem.skipmatch_state();
            jvSkipMatchInfoItemRoot["lately_session_id"] = (Json::Int64)pbUserSkipMatcahInfoItem.lately_session_id();
            jvSkipMatchInfoItemRoot["skipmatch_total_win_num_on_type"] = pbUserSkipMatcahInfoItem.skipmatch_total_win_num_on_type();
            jvSkipMatchInfoItemRoot["skipmatch_success_skip_level_num"] = pbUserSkipMatcahInfoItem.skipmatch_success_skip_level_num();
            jvSkipMatchInfoItemRoot["skipmatch_need_share_num"] = pbUserSkipMatcahInfoItem.skipmatch_need_share_num();
            jvSkipMatchInfoItemRoot["skipmatch_need_diamond_num"] = pbUserSkipMatcahInfoItem.skipmatch_need_diamond_num();
            Json::Value jvResultFlowsRoot;
            for(int j = 0 ; j < pbUserSkipMatcahInfoItem.result_flows_size() ; j ++)
            {
                Json::Value result_flows;
                const PBUserSkipMatchResultFlow pbFlow = pbUserSkipMatcahInfoItem.result_flows(j);
                result_flows["session_id"] = (Json::Int64)pbFlow.session_id();
                result_flows["skipmatch_level_change_val"] = pbFlow.skipmatch_level_change_val();
                result_flows["skipmatch_level_after_change"] = pbFlow.skipmatch_level_after_change();
                result_flows["skipmatch_state"] = pbFlow.skipmatch_state();
                result_flows["reason"] = pbFlow.reason();
                result_flows["is_over"] = pbFlow.is_over();
                jvResultFlowsRoot[j] = result_flows;
            }
            jvSkipMatchInfoItemRoot["result_flows"] = jvResultFlowsRoot;
            jvSkipMatchInfoRoot["skip_match_info_this_game"] = jvSkipMatchInfoItemRoot;
        }
        jvSkipMatchInfoRoot["skipmatch_total_win_num"] = a_pbUserSkipMatchInfo.skipmatch_total_win_num();
    }
    jvUserGameDataRoot["skip_match_info"] = jvSkipMatchInfoRoot;
    a_szJsonMsg = writer.write(jvUserGameDataRoot);
}

ENHandlerResult CRequestNotifyShareSuccess::ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession)
{
    const GMRequestNotifyShareSuccess & pbRequest = psession->_request_msg.gm_request_notify_share_success();
    int iUid = pbRequest.uid();

    psession->NewAddGetData(iUid,PBUserDataField::kUserInfo);
    psession->NewAddGetData(iUid,PBUserDataField::kUserGameData);

    return EN_Handler_Get;
}

ENHandlerResult CRequestNotifyShareSuccess::ProcessGetSucc(CHandlerTokenBasic * ptoken,CSession * psession)
{
    const GMRequestNotifyShareSuccess & pbRequest = psession->_request_msg.gm_request_notify_share_success();
    int iUid = pbRequest.uid();
    int iGameType = pbRequest.game_type();
    int iActivityType = pbRequest.activity_type();
    int64 iSessionId = pbRequest.session_id();

    PBUserGameData & pbUserGameData = *psession->_kvdb_uid_data_map[iUid].mutable_user_game_data();
    switch (iActivityType)
    {
    case EN_User_Game_Info_Skip_Match:
    {
        PBUserSkipMatchInfo & pbUserSkipMatchInfo = *pbUserGameData.mutable_user_skip_match_info();
        PBUserSkipMatchInfoItem * pbInfoItem = NULL;
        for(int i = 0 ; i < pbUserSkipMatchInfo.skip_match_info_item_size() ; i ++)
        {
            PBUserSkipMatchInfoItem * pbItem = pbUserSkipMatchInfo.mutable_skip_match_info_item(i);
            if(pbItem->skipmatch_type() == iGameType)
            {
                pbInfoItem = pbItem;
            }
        }

        if(!pbInfoItem)
        {
            break;
        }

        if(pbInfoItem->lately_session_id() != iSessionId)
        {
            break;
        }

        if(pbInfoItem->skipmatch_state() != EN_Skip_Match_State_Failed)
        {
            break;
        }

        PBUpdateData pbUpdate;
        pbUpdate.set_key(PBUserDataField::kUserGameData);
        if(pbInfoItem->skipmatch_level() == 1)
        {
            //如果是1级，则需用等级加1
            {
                PBDBAtomicField & pbField = *pbUpdate.add_field_list();
                pbField.set_activity_type(EN_User_Game_Info_Skip_Match);
                pbField.set_game_type(iGameType);
                pbField.set_field(EN_DB_Field_Skip_Match_Info_Level);
                pbField.set_skipmatch_session_id(iSessionId);
                pbField.set_strategy(EN_Update_Strategy_Inc);
                pbField.set_intval(1);
                pbField.set_reason(EN_Reason_LevelAndBonus_Share_Success);
            }
            {
                PBDBAtomicField & pbField = *pbUpdate.add_field_list();
                pbField.set_activity_type(EN_User_Game_Info_Skip_Match);
                pbField.set_game_type(iGameType);
                pbField.set_field(EN_DB_Field_Skip_Match_Info_State);
                pbField.set_skipmatch_session_id(iSessionId);
                pbField.set_intval(EN_Skip_Match_State_Initial);
                pbField.set_reason(EN_Reason_LevelAndBonus_Share_Success);
                pbField.set_skipmatch_need_share_num(0);
                pbField.set_skipmatch_need_diamond_num(0);
            }
            psession->NewAddUpdateData(iUid, pbUpdate);
        }
        else
        {
            {
                PBDBAtomicField & pbField = *pbUpdate.add_field_list();
                pbField.set_activity_type(EN_User_Game_Info_Skip_Match);
                pbField.set_game_type(iGameType);
                pbField.set_field(EN_DB_Field_Skip_Match_Info_State);
                pbField.set_skipmatch_session_id(iSessionId);
                pbField.set_intval(EN_Skip_Match_State_Initial);
                pbField.set_reason(EN_Reason_LevelAndBonus_Share_Success);
                pbField.set_skipmatch_need_share_num(0);
                pbField.set_skipmatch_need_diamond_num(0);
            }
            psession->NewAddUpdateData(iUid, pbUpdate);
        }

        return EN_Handler_Save;
    }
        break;
    default:
        break;
    }

    //没有这类数据则返回为false
    PHPSocketMap::iterator iter = JsonHelper::_socketmap.find(psession->sessionid());
    if (iter != JsonHelper::_socketmap.end())
    {
        std::string szJsonMsg;
        Json::Value jvUserGameDataRoot;
        Json::FastWriter writer;
        jvUserGameDataRoot["result"] = 1;
        szJsonMsg = writer.write(jvUserGameDataRoot);
        Message::SendMsgToPHP(iter->second, szJsonMsg);
    }

    return EN_Handler_Done;
}

ENHandlerResult CRequestNotifyShareSuccess::ProcessUpdateSucc(CHandlerTokenBasic* ptoken, CSession * psession)
{
    const SSResponseUpdateUserData& pbUpdateResponse = psession->_response_msg.ss_response_update_user_data();
    const GMRequestNotifyShareSuccess & pbRequest = psession->_request_msg.gm_request_notify_share_success();
    int iUid = pbRequest.uid();
    const PBUser & pbUser = psession->_kvdb_uid_data_map[iUid].user_info();

    PHPSocketMap::iterator iter = JsonHelper::_socketmap.find(psession->sessionid());
    if (iter != JsonHelper::_socketmap.end())
    {
        std::string szJsonMsg;
        Json::Value jvUserGameDataRoot;
        Json::FastWriter writer;
        jvUserGameDataRoot["result"] = 0;
        szJsonMsg = writer.write(jvUserGameDataRoot);
        Message::SendMsgToPHP(iter->second, szJsonMsg);
    }

    PBCSMsg pbMsg;
    CSNotifyShareSuccess & pbResponse = *pbMsg.mutable_cs_notify_share_success();
    pbResponse.set_result(EN_MESSAGE_ERROR_OK);
    pbResponse.mutable_user_game_data_after_edit()->CopyFrom(pbUpdateResponse.user_data().user_game_data());
    Message::PushInnerMsg(RouteManager::Instance()->GetRouteByRandom(),iUid,pbMsg,EN_Node_Connect,pbUser.hallsvid());

    return EN_Handler_Done;
}
