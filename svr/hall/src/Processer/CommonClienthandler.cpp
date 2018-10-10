#include "CommonClienthandler.h"
#include "RouteManager.h"
#include "PBConfigBasic.h"
#include "Common.h"
#include "clib_time.h"
#include "LogWriter.h"
#include "GlobalRedisClient.h"
#include "ReplayCodeManager.h"
#include "RecordRedisManager.h"

ENProcessResult CRegistInnerServer::ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession)
{
    const SSNotifyInnerServer & ss_notify_inner_server = psession->_request_msg.ss_notify_inner_server();
    int routeid = ss_notify_inner_server.routeid();
    RouteManager::Instance()->_route_map[routeid] = ptoken->_phandler;
    VLogMsg(CLIB_LOG_LEV_ERROR, "receive noitfy from route[%d]", routeid);
    return EN_Process_Result_Completed;
}

ENHandlerResult CLoginProcesser::ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession)
{
    //请求用户account对应的uid信息
    const CSRequestLogin & cs_request_login = psession->_request_msg.cs_request_login();
    PBCSMsg msg;
    SSRequestAccountUid & ss_request_account_uid = *msg.mutable_ss_request_acc_uid();
    ss_request_account_uid.set_account(cs_request_login.account());
    ss_request_account_uid.set_acc_type(cs_request_login.acc_type());
    ss_request_account_uid.set_token(cs_request_login.token());
    Message::SendRequestMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg, EN_Node_DBProxy);
    psession->_fsm_state = EN_Login_State_Request_Uid;
    return EN_Handler_Succ;
}

ENHandlerResult CLoginProcesser::OnResponseAccountUid(CHandlerTokenBasic * ptoken, CSession * psession)
{
    const SSResponseAccountUid & response = psession->_response_msg.ss_response_acc_uid();
    if (response.result() == EN_MESSAGE_INVALID_ACC_TOKEN)
    {
        PBCSMsg msg;
        SSResponseLogin & response = *msg.mutable_ss_response_login();
        response.set_result(EN_MESSAGE_INVALID_ACC_TOKEN);
        Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
        return EN_Handler_Done;
    }
    else if (response.result() == EN_MESSAGE_DB_INVALID)
    {
        PBCSMsg msg;
        SSResponseLogin & response = *msg.mutable_ss_response_login();
        response.set_result(EN_MESSAGE_DB_INVALID);
        Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
        return EN_Handler_Done;
    }
    else
    {
        //拉取到uid 请求玩家数据
        psession->_fsm_state = EN_Login_State_Request_User_Data;
        long long uid = response.uid();
        psession->_request_route.set_uid(uid);
        if (!SessionManager::Instance()->LockProcess(psession))
        {
            PBCSMsg msg;
            SSResponseLogin & response = *msg.mutable_ss_response_login();
            response.set_result(EN_MESSAGE_ERROR_REQUEST_PROCESSING);
            Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
            return EN_Handler_Done;
        }
        if (response.iscreated())
        {
            //create uid succ
            const CSRequestLogin & login_request = psession->_request_msg.cs_request_login();
            long long uid = response.uid();
            // 初始化注册数据并保存db
            PBCSMsg msg;
            SSRequestSaveData& request = *msg.mutable_ss_request_save_data();
            PBDataSet& data_set = *request.mutable_data_set();
            data_set.set_uid(uid);

            // 构造数据
            // user info

            PBUser user_info;
            user_info.add_accounts(login_request.account());
            user_info.set_hallsvid(psession->_request_route.source_id());
            user_info.set_uid(uid);
            if (!login_request.has_nick())
            {
                char name[32] = { 0 };
                snprintf(name, sizeof(name), "Guest%lld", uid);
                user_info.set_nick(name);
            }
            else
            {
                user_info.set_nick(login_request.nick());
            }
            long long init_money = /*100*/ PokerPBHallSvrdConfig::Instance()->init_money();

			user_info.set_chips(init_money);
			user_info.mutable_pos()->set_pos_type(EN_Position_Hall);
			user_info.mutable_skipmatch_pos()->set_pos_type(EN_Position_Hall);
			user_info.mutable_coin_pos()->set_pos_type(EN_Position_Hall);
            user_info.set_channel(psession->_head.channel_id());
			if(login_request.has_pic_url())
			{
				user_info.set_pic_url(login_request.pic_url());
			}
			user_info.set_acc_type(login_request.acc_type());

			if(login_request.has_nick())
			{
				user_info.set_nick(login_request.nick());
			}
			if(login_request.has_gender())
			{
				user_info.set_gender(login_request.gender());
			}
            user_info.set_total_bonus(0);

            int64 iTime = time(0);
            user_info.set_regist_time(iTime);

            data_set.mutable_user_data()->mutable_user_info()->CopyFrom(user_info);

            google::protobuf::uint8 buff[10240] = { 0 };
            unsigned int buflen = user_info.ByteSize();
            if (buflen > sizeof(buff))
            {
                ErrMsg("user[%lld] serialize exceed max length = %u.", uid, buflen);
                return EN_Handler_Done;
            }
            user_info.SerializeWithCachedSizesToArray(buff);

            PBRedisData& redis_data1 = *data_set.add_key_list();
            redis_data1.set_key(PBUserDataField::kUserInfo);
            redis_data1.set_buff(buff, buflen);

            psession->_uid = uid;
            Message::SendRequestMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg, EN_Node_DBProxy, 1, EN_Route_hash);

            psession->_fsm_state = EN_Login_State_Wait_Save;
            return EN_Handler_Succ;
        }
        else
        {
            psession->NewAddGetData(uid, PBUserDataField::kUserInfo);
        }
        return EN_Handler_Get;
    }
}

ENHandlerResult CLoginProcesser::OnResponseSave(CHandlerTokenBasic * ptoken, CSession * psession)
{
    const SSResponseSaveData& db_response = psession->_response_msg.ss_response_save_data();
    const PBDataSet& data_set = db_response.data_set();
    for (int i = 0; i < data_set.key_list_size(); ++i)
    {
        const PBRedisData& redis_data = data_set.key_list(i);
        if (!redis_data.result())
        {
            PBCSMsg msg;
            SSResponseLogin& response = *msg.mutable_ss_response_login();
            response.set_result(EN_MESSAGE_DB_SAVE_FAILED);
            Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
            return EN_Handler_Done;
        }
    }
    PBCSMsg msg;
    SSResponseLogin & response = *msg.mutable_ss_response_login();
    CSRequestLogin & request = *psession->_request_msg.mutable_cs_request_login();
    // const string & account = request.account();

    PBUserData user_data;
    user_data.CopyFrom(db_response.data_set().user_data());
    response.set_result(EN_MESSAGE_ERROR_OK);
    response.mutable_data_set()->mutable_user_data()->CopyFrom(user_data);
    response.set_is_created(true);
    Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);

    request.set_hsvid(psession->_request_route.source_id());
    request.set_uid(user_data.user_info().uid());
    Message::PushInnerMsg(RouteManager::Instance()->GetRouteByRandom(), user_data.user_info().uid(),
                          psession->_request_msg, EN_Node_List_Dispather);

    PBCSMsg log_msg;
    LogRegist & reg_log = *log_msg.mutable_log_regist();
    reg_log.set_uid(user_data.user_info().uid());
    reg_log.set_time_stamp(time(NULL));
    reg_log.set_acc_type(user_data.user_info().acc_type());
    reg_log.set_name(user_data.user_info().nick());
    reg_log.set_channel(psession->_head.channel_id());
    reg_log.set_device_name(psession->_head.device_name());
    reg_log.set_band(psession->_head.band());
    CLogWriter::Instance()->Send(log_msg);

    LogChipJournal& chip_log = *log_msg.mutable_log_chip_journal();
    chip_log.set_uid(user_data.user_info().uid());
    chip_log.set_act_num(/*100*/PokerPBHallSvrdConfig::Instance()->init_money());
    chip_log.set_total_num(user_data.user_info().chips());
    chip_log.set_reason(EN_Reason_Regist);
    chip_log.set_time_stamp(time(NULL));
    chip_log.set_acc_type(user_data.user_info().acc_type());
    chip_log.set_channel(psession->_head.channel_id());
    CLogWriter::Instance()->Send(log_msg);

    LogLogin& log = *log_msg.mutable_log_login();
    log.set_uid(user_data.user_info().uid());
    log.set_time_stamp(time(NULL));
    log.set_acc_type(request.acc_type());
    log.set_channel(psession->_head.channel_id());
    log.set_device_name(psession->_head.device_name());
    log.set_band(psession->_head.band());
    CLogWriter::Instance()->Send(log_msg);

    return EN_Handler_Done;
}

ENHandlerResult CLoginProcesser::ProcessGetSucc(CHandlerTokenBasic * ptoken, CSession * psession)
{
    const PBCSMsg & response_uid_msg = psession->_response_msg_map[EN_Login_State_Request_Uid];
    const SSResponseAccountUid & ss_response_account_uid = response_uid_msg.ss_response_acc_uid();
    long long uid = ss_response_account_uid.uid();
    PBCSMsg msg;
    SSResponseLogin& response = *msg.mutable_ss_response_login();

	CSRequestLogin & request = *psession->_request_msg.mutable_cs_request_login();
	response.set_result(EN_MESSAGE_ERROR_OK);
    response.mutable_data_set()->mutable_user_data()->CopyFrom(psession->_kvdb_uid_data_map[uid]);
	//判断是否在不同大厅重复登录 
	const PBUserData & user_data = psession->_kvdb_uid_data_map[uid];
	const PBUser & user_info = user_data.user_info();
	if(user_info.hallsvid() != 0 && user_info.hallsvid() != psession->_request_route.source_id())
	{
		//重复登录
		PBCSMsg msg;
		SSNotifyPlayerRepeatedLogin & ss_notify_repeated_login = *msg.mutable_ss_notify_player_repeated_login();
		ss_notify_repeated_login.set_uid(uid);
		Message::PushMsg(RouteManager::Instance()->GetRouteByRandom(),uid,msg,EN_Node_Connect,user_info.hallsvid());
	}
	Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);

    request.set_hsvid(psession->_request_route.source_id());
    Message::PushInnerMsg(RouteManager::Instance()->GetRouteByRandom(), uid, psession->_request_msg, EN_Node_List_Dispather);

    PBCSMsg ss_cid_notify;
    SSRequestUpdateUserData& ss_update = *ss_cid_notify.mutable_ss_request_update_user_data();
    ss_update.set_uid(uid);
    PBUpdateData& user_key = *ss_update.add_key_list();
    user_key.set_key(PBUserDataField::kUserInfo);
    PBDBAtomicField& hallid_field = *user_key.add_field_list();
    hallid_field.set_field(EN_DB_Field_Hsvid);
    hallid_field.set_strategy(EN_Update_Strategy_Replace);
    hallid_field.set_intval(psession->_request_route.source_id());
    Message::PushInnerMsg(RouteManager::Instance()->GetRouteByRandom(), uid, ss_cid_notify, EN_Node_User, 1, EN_Route_hash);

    PBCSMsg log_msg;
    LogLogin& log = *log_msg.mutable_log_login();
    log.set_uid(user_data.user_info().uid());
    log.set_time_stamp(time(NULL));
    log.set_acc_type(request.acc_type());
    log.set_channel(psession->_head.channel_id());
    log.set_device_name(psession->_head.device_name());
    log.set_band(psession->_head.band());
    CLogWriter::Instance()->Send(log_msg);
    return EN_Handler_Done;
}

ENHandlerResult CLoginProcesser::ProcessGetFailed(CHandlerTokenBasic * ptoken, CSession * psession)
{
    PBCSMsg msg;
    SSResponseLogin& response = *msg.mutable_ss_response_login();
    response.set_result(EN_MESSAGE_DB_NOT_FOUND);
    Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
    return EN_Handler_Done;
}

ENHandlerResult CLoginProcesser::ProcessResponseMsg(CHandlerTokenBasic * ptoken, CSession * psession)
{
    switch (psession->_fsm_state)
    {
    case EN_Login_State_Request_Uid:
        return OnResponseAccountUid(ptoken, psession);
        break;
    case EN_Login_State_Wait_Save:
        return OnResponseSave(ptoken, psession);
        break;
    default:
        break;
    }
    return EN_Handler_Done;
}

ENHandlerResult CRequestUserRecord::ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession)
{
    long long uid = psession->_request_route.uid();
    psession->NewAddGetData(uid, PBUserDataField::kUserInfo);
    psession->NewAddGetData(uid, PBUserDataField::kUserRecord);
    return EN_Handler_Get;
}

ENHandlerResult CRequestUserRecord::ProcessGetSucc(CHandlerTokenBasic * ptoken, CSession * psession)
{
    //const CSRequestUserRecord& request = psession->_request_msg.cs_request_user_record();
    PBCSMsg msg;
    long long uid = psession->_request_route.uid();
    const PBUserRecord & user_record = psession->_kvdb_uid_data_map[uid].user_record();
    CSResponseUserRecord & cs_response_user_record = *msg.mutable_cs_response_user_record();
    //    if (request.game_type() == 3)  // 上大人
    {
        cs_response_user_record.mutable_records()->CopyFrom(user_record.sdr_records());
        std::reverse(cs_response_user_record.mutable_records()->begin(), cs_response_user_record.mutable_records()->end());
    }

    Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
    return EN_Handler_Done;
}

/*
ENProcessResult CRequestGetUserInfo::ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession)
{
    long long uid = psession->_request_msg.cs_request_get_user_info().uid();
    AddGetData(psession, uid, PBUserDataField::kUserInfo);
    AddGetData(psession, uid, PBUserDataField::kIcyInfo);
    AddGetData(psession, uid, PBUserDataField::kItemInfo);
    Message::SendRequestToUser(RouteManager::Instance()->GetRouteByRandom(), psession, psession->_tmp_msg);

    return EN_Process_Result_Succ;
}

ENProcessResult CRequestGetUserInfo::ProcessUserMsg(CHandlerTokenBasic * ptoken,CSession * psession)
{
    PBCSMsg msg;
    CSResponseGetUserInfo & response = *msg.mutable_cs_response_get_user_info();

    const SSResponseGetUserData & db_response = psession->_response_msg.ss_response_get_user_data();
    if (db_response.result() != EN_MESSAGE_ERROR_OK)
    {
        response.set_result(EN_MESSAGE_ERROR_USER_NOT_EXIST);
        Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
        return EN_Process_Result_Failed;
    }

    response.set_result(EN_MESSAGE_ERROR_OK);
    CopyUserInfo(*response.mutable_user_info(), db_response.data_set().user_data());

    long long uid = psession->_request_msg.cs_request_get_user_info().uid();
    const PBUserState* pstate = CUserStateMng::Instance()->GetUserState(uid);
    if (pstate)
    {
        response.mutable_user_state()->CopyFrom(*pstate);
    }

    Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);

    return EN_Process_Result_Completed;
}

void CRequestGetUserInfo::CopyUserInfo(CSUserInfo& user_info, const PBUserData user_data)
{
    user_info.set_uid(user_data.user_info().uid());
    user_info.set_nick_name(user_data.user_info().nick());
    user_info.set_pic_url(user_data.user_info().pic_url());
    user_info.set_gender(user_data.icy_info().gender());
    user_info.set_birthday(user_data.icy_info().birthday());
    user_info.set_description(user_data.icy_info().description());
    user_info.set_exp(user_data.user_info().exp());
    user_info.set_chips(user_data.user_info().money());
    user_info.set_gold(user_data.user_info().gold());
    user_info.set_vip_level(Common::GetMaxVipLevel(user_data));
    user_info.set_location(user_data.icy_info().location());
    if (Common::ItemGetNum(user_data.item_info(), user_data.item_info().gift_config_id()))
        user_info.set_gift_config_id(user_data.item_info().gift_config_id());
    else
        user_info.set_gift_config_id(0);
}

ENProcessResult CRequestUpdateConfig::ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession)
{
    const CSRequestUpdateConfig & request = psession->_request_msg.cs_request_update_config();

    PBCSMsg msg;
    CSResponseUpdateConfig & response = *msg.mutable_cs_response_update_config();
    for(int i=0;i<request.client_config_items_size();i++)
    {
        const PBClientConfigItem & client_config_item = request.client_config_items(i);
        int client_version = client_config_item.version();
        switch(client_config_item.ctype())
        {
            case EN_Config_Type_Match_Reward:
                if(PokerPBMatchRewardConfig::Instance()->GetVersion() != client_version)
                {
                    PBServerConfigItem & config_item = *response.add_server_config_items();
                    PBConfig & config = *config_item.mutable_configs();
                    config.mutable_match_reward_config()->CopyFrom(*PokerPBMatchRewardConfig::Instance());
                }
                break;
            case EN_Config_Type_Match_Raise_Blind:
                if(PokerPBMatchRaiseBlindConfig::Instance()->GetVersion() != client_version)
                {
                    PBServerConfigItem & config_item = *response.add_server_config_items();
                    PBConfig & config = *config_item.mutable_configs();
                    config.mutable_match_raise_blind_config()->CopyFrom(*PokerPBMatchRaiseBlindConfig::Instance());
                }
                break;
            case EN_Config_Type_Play_Time:
                if (PokerPBPlayTimeConfig::Instance()->version() != client_version)
                {
                    PBServerConfigItem& config_item = *response.add_server_config_items();
                    PBConfig& config = *config_item.mutable_configs();
                    config.mutable_playtime_config()->CopyFrom(*PokerPBPlayTimeConfig::Instance());
                }
                break;
            case EN_Config_Type_Club:
                if (PokerPBClubConfig::Instance()->version() != client_version)
                {
                    PBServerConfigItem& config_item = *response.add_server_config_items();
                    PBConfig& config = *config_item.mutable_configs();
                    config.mutable_club()->CopyFrom(*PokerPBClubConfig::Instance());
                }
                break;
            case EN_Config_Type_Activity:
                if (PokerActivityConfig::Instance()->version() != client_version)
                {
                    PBServerConfigItem& config_item = *response.add_server_config_items();
                    PBConfig& config = *config_item.mutable_configs();
                    config.mutable_activity_config()->CopyFrom(*PokerActivityConfig::Instance());
                }
                break;
            case EN_Config_Type_Task:
                if (PokerTaskConfig::Instance()->version() != client_version)
                {
                    PBServerConfigItem& config_item = *response.add_server_config_items();
                    PBConfig& config = *config_item.mutable_configs();
                    config.mutable_task_config()->CopyFrom(*PokerTaskConfig::Instance());
                }
                break;
            case EN_Config_Type_Item:
                if (PokerItemConfig::Instance()->version() != client_version)
                {
                    PBServerConfigItem& config_item = *response.add_server_config_items();
                    PBConfig& config = *config_item.mutable_configs();
                    config.mutable_item()->CopyFrom(*PokerItemConfig::Instance());
                }
                break;
            case EN_Config_Type_Play_Count:
                if (PokerPlayCountConfig::Instance()->version() != client_version)
                {
                    PBServerConfigItem& config_item = *response.add_server_config_items();
                    PBConfig& config = *config_item.mutable_configs();
                    config.mutable_play_count()->CopyFrom(*PokerPlayCountConfig::Instance());
                }
                break;
            case EN_Config_Type_Shop:
                if (PokerShopConfig::Instance()->version() != client_version)
                {
                    PBServerConfigItem& config_item = *response.add_server_config_items();
                    PBConfig& config = *config_item.mutable_configs();
                    config.mutable_shop()->CopyFrom(*PokerShopConfig::Instance());
                }
                break;
            case EN_Config_Type_Slot:
                if (PokerSlotConfig::Instance()->version() != client_version)
                {
                    PBServerConfigItem& config_item = *response.add_server_config_items();
                    PBConfig& config = *config_item.mutable_configs();
                    config.mutable_slot()->set_version(PokerSlotConfig::Instance()->version());
                    config.mutable_slot()->mutable_bet_list()->CopyFrom(PokerSlotConfig::Instance()->bet_list());
                }
                break;
            case EN_Config_Type_Achievement:
                if(PokerAchievementConfig::Instance()->version() != client_version)
                {
                    PBServerConfigItem & config_item =  * response.add_server_config_items();
                    PBConfig & config = * config_item.mutable_configs();
                    config.mutable_achivement_config()->CopyFrom(*PokerAchievementConfig::Instance());
                }
                break;
            case EN_Config_Type_Title:
                if(PokerTitleConfig::Instance()->version() != client_version)
                {
                    PBServerConfigItem & config_item =  * response.add_server_config_items();
                    PBConfig & config = * config_item.mutable_configs();
                    config.mutable_title_info_config()->CopyFrom(*PokerTitleConfig::Instance());
                }
                break;
            default:
                break;
        }
    }
    Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
    return EN_Process_Result_Completed;
}
*/

ENProcessResult CRequestUpdateUserInfo::ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession)
{
    const CSRequestUpdateUserInfo & user_info = psession->_request_msg.cs_request_update_user_info();

    PBCSMsg msg;
    SSRequestUpdateUserData& ss_update = *msg.mutable_ss_request_update_user_data();
    ss_update.set_uid(psession->_request_route.uid());

    if (user_info.has_nick_name() ||
        user_info.has_pic_url())
    {
        PBUpdateData& user_key = *ss_update.add_key_list();
        user_key.set_key(PBUserDataField::kUserInfo);
        if (user_info.has_nick_name())
        {
            PBDBAtomicField& nick_field = *user_key.add_field_list();
            nick_field.set_field(EN_DB_Field_Nick);
            nick_field.set_strategy(EN_Update_Strategy_Replace);
            nick_field.set_strvalue(user_info.nick_name());
        }
        if (user_info.has_pic_url())
        {
            PBDBAtomicField& pic_field = *user_key.add_field_list();
            pic_field.set_field(EN_DB_Field_Pic_Url);
            pic_field.set_strategy(EN_Update_Strategy_Replace);
            pic_field.set_strvalue(user_info.pic_url());
        }
        if (user_info.has_gender())
        {
            PBDBAtomicField& gender_field = *user_key.add_field_list();
            gender_field.set_field(EN_DB_Field_Gender);
            gender_field.set_strategy(EN_Update_Strategy_Replace);
            gender_field.set_intval(user_info.gender());
        }
    }

    if (ss_update.key_list_size())
    {
        Message::SendRequestToUser(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
        return EN_Process_Result_Succ;
    }

    CSResponseUpdateUserInfo & response = *msg.mutable_cs_response_update_user_info();
    response.set_result(EN_MESSAGE_ERROR_OK);
    Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
    return EN_Process_Result_Completed;
}

ENProcessResult CRequestUpdateUserInfo::ProcessUserMsg(CHandlerTokenBasic * ptoken, CSession * psession)
{
    const SSResponseUpdateUserData db_response = psession->_response_msg.ss_response_update_user_data();

    PBCSMsg msg;
    CSResponseUpdateUserInfo& response = *msg.mutable_cs_response_update_user_info();
    response.set_result(db_response.result());
    Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
    return EN_Process_Result_Completed;
}

ENHandlerResult CRequestTableFlowRecord::ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession)
{
    const CSRequestTableFlowRecord & request = psession->_request_msg.cs_request_table_flow_record();
    int64 recordid = request.recordid();
    int round = request.round();
    int stamp = request.stamp();
    //    ENTableType game_type = request.game_type();

    //    if (game_type == EN_Table_SDR_ES)
    {
        time_t t_now = stamp;
        struct tm *p;
        p = localtime(&t_now);
        char mainkey[125] = { 0 };
        snprintf(mainkey, sizeof(mainkey), "SDR_TABLE_FLOW_RECORD_%04d%02d%02d", p->tm_year + 1900, p->tm_mon + 1, p->tm_mday);
        char subkey[125] = { 0 };
        snprintf(subkey, sizeof(subkey), "%lld_%d", recordid, round);
        PBHashDataField pb_hash_data_field;
        pb_hash_data_field.mutable_sdr_table_flow_record_item();
        if (!RecordRedisManager::Instance()->QueryHashObject(mainkey, subkey, pb_hash_data_field))
        {
            PBCSMsg msg;
            CSResponseTableFlowRecord & response = *msg.mutable_cs_reponse_table_flow_record();
            response.set_result(EN_MESSAGE_DB_NOT_FOUND);
            Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
            return EN_Handler_Done;
        }

        PBCSMsg msg;
        CSResponseTableFlowRecord & response = *msg.mutable_cs_reponse_table_flow_record();
        response.set_round(round);
        response.set_result(EN_MESSAGE_ERROR_OK);
        response.mutable_sdr_record()->CopyFrom(pb_hash_data_field.sdr_table_flow_record_item());
        Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
        return EN_Handler_Done;
    }
    return EN_Handler_Done;
}

ENHandlerResult CRequestReplayCode::ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession)
{
    const CSRequestReplayCode& request = psession->_request_msg.cs_request_replay_code();
    int64 recordid = request.recordid();
    int round = request.round();
    int stamp = request.stamp();
    //    ENTableType game_type = request.game_type();

    //    if (game_type == EN_Table_SDR_ES)
    {
        time_t t_now = stamp;
        struct tm *p;
        p = localtime(&t_now);
        char mainkey[125] = { 0 };
        snprintf(mainkey, sizeof(mainkey), "SDR_TABLE_FLOW_RECORD_%04d%02d%02d", p->tm_year + 1900, p->tm_mon + 1, p->tm_mday);
        char subkey[125] = { 0 };
        snprintf(subkey, sizeof(subkey), "%lld_%d", recordid, round);
        // 1. 检测回放数据是否过期
        if (!RecordRedisManager::Instance()->CheckExist(mainkey, subkey))
        {
            VLogMsg(CLIB_LOG_LEV_ERROR, "回放数据已过期");
            PBCSMsg msg;
            CSResponseReplayCode & response = *msg.mutable_cs_response_replay_code();
            response.set_result(EN_MESSAGE_DB_NOT_FOUND);
            Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
            return EN_Handler_Done;
        }
        // 2. 分配回放码
        int replay_code = ReplayCodeManager::Instance()->AllocNewCode(mainkey, subkey);
        if (replay_code == -1)
        {
            VLogMsg(CLIB_LOG_LEV_ERROR, "分配回放码失败");
            return EN_Handler_Done;
        }

        // 3. 回包
        PBCSMsg msg;
        CSResponseReplayCode & response = *msg.mutable_cs_response_replay_code();
        response.set_result(EN_MESSAGE_ERROR_OK);
        response.set_replay_code(replay_code);
        Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
        return EN_Handler_Done;
    }

    return EN_Handler_Done;
}

ENHandlerResult CRequestReplayCodeData::ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession)
{
    const CSRequestReplayCodeData& request = psession->_request_msg.cs_request_replay_code_data();
    int64 replay_code = request.replay_code();
    // 1. 检测回放码是否失效
    if (!ReplayCodeManager::Instance()->CheckAllocedCode(replay_code))
    {
        VLogMsg(CLIB_LOG_LEV_ERROR, "回放码不存在");
        PBCSMsg msg;
        CSResponseReplayCodeData & response = *msg.mutable_cs_response_replay_code_data();
        response.set_result(EN_MESSAGE_INVALID_REPLAY_CODE);
        Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
        return EN_Handler_Done;
    }
    string mainkey;
    string subkey;
    if (!ReplayCodeManager::Instance()->GetReplayForCode(replay_code, mainkey, subkey))
    {
        VLogMsg(CLIB_LOG_LEV_ERROR, "获取回放key失败");
        PBCSMsg msg;;
        CSResponseReplayCodeData & response = *msg.mutable_cs_response_replay_code_data();
        response.set_result(EN_MESSAGE_DB_NOT_FOUND);
        Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
        return EN_Handler_Done;
    }
    // 2. 拉取回放数据
//    if (game_type == EN_Table_SDR_ES)
    {
        PBHashDataField pb_hash_data_field;
        pb_hash_data_field.mutable_sdr_table_flow_record_item();
        if (!RecordRedisManager::Instance()->QueryHashObject(mainkey, subkey, pb_hash_data_field))
        {
            // 回收回放码 ...
            ReplayCodeManager::Instance()->ReleaseCode(replay_code);

            VLogMsg(CLIB_LOG_LEV_ERROR, "获取回放数据失败");
            PBCSMsg msg;
            CSResponseReplayCodeData & response = *msg.mutable_cs_response_replay_code_data();
            response.set_result(EN_MESSAGE_DB_NOT_FOUND);
            Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
            return EN_Handler_Done;
        }

        int round = 0;
        {
            size_t pos = subkey.find_first_of('_', 0);
            if (pos != std::string::npos)
            {
                string str_round = subkey.substr(pos + 1, subkey.length());
                round = atoi(str_round.c_str());
            }
        }
        PBCSMsg msg;
        CSResponseReplayCodeData & response = *msg.mutable_cs_response_replay_code_data();
        //        response.set_round(round);
        //        response.set_result(EN_MESSAGE_ERROR_OK);
        //        response.mutable_record()->CopyFrom(pb_hash_data_field.lzmj_table_flow_record_item());
        response.set_round(round);
        response.set_result(EN_MESSAGE_ERROR_OK);
        response.mutable_sdr_record()->CopyFrom(pb_hash_data_field.sdr_table_flow_record_item());
        Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
        return EN_Handler_Done;
    }

    return EN_Handler_Done;
}

ENHandlerResult CRequestOnClickAvatar::ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession)
{
    const long long uid = psession->_request_route.uid();
    psession->NewAddGetData(uid, PBUserDataField::kUserInfo);
    psession->NewAddGetData(uid, PBUserDataField::kUserRecord);
    return EN_Handler_Get;
}
ENHandlerResult CRequestUserActivityGameInfo::ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession)
{
    const CSRequestUserActivityGameInfo & pbRequest = psession->_request_msg.cs_request_user_activity_game_info();
    int64 iUid = pbRequest.uid();
    psession->NewAddGetData(iUid, PBUserDataField::kUserInfo);
    psession->NewAddGetData(iUid, PBUserDataField::kUserGameData);
    return EN_Handler_Get;
}

ENHandlerResult CRequestOnClickAvatar::ProcessGetSucc(CHandlerTokenBasic * ptoken, CSession * psession)
{
    PBCSMsg msg;
    const long long uid = psession->_request_route.uid();
    const PBUserRecord & user_record = psession->_kvdb_uid_data_map[uid].user_record();
    SCResponseOnClickAvatar & sc_response_on_click_avatar = *msg.mutable_sc_response_on_click_avatar();
    sc_response_on_click_avatar.mutable_last_week_game_info()->CopyFrom(user_record.user_last_week_game_info());

    Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
    return EN_Handler_Done;
}
ENHandlerResult CRequestUserActivityGameInfo::ProcessGetSucc(CHandlerTokenBasic * ptoken,CSession * psession)
{
    const CSRequestUserActivityGameInfo & pbRequest = psession->_request_msg.cs_request_user_activity_game_info();
    int64 iUid = pbRequest.uid();
    int iActivityType = pbRequest.activity_type();
    int iGameType = pbRequest.game_type();

    PBUserGameData & pbUserGameData = *psession->_kvdb_uid_data_map[iUid].mutable_user_game_data();
    switch (iActivityType)
    {
    case EN_User_Game_Info_Skip_Match:
    {
        PBUserSkipMatchInfo & pbUserSkipMatchInfo = *pbUserGameData.mutable_user_skip_match_info();
        PBUserSkipMatchInfoItem pbItemTmp;
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
            pbItemTmp.set_skipmatch_type(iGameType);
            pbItemTmp.set_skipmatch_level(1);
            pbItemTmp.set_skipmatch_state(EN_Skip_Match_State_Initial);
            pbItemTmp.set_old_skipmatch_level(1);
        }
        else
        {
            pbItemTmp.CopyFrom(*pbInfoItem);
        }

        PBCSMsg pbMsg;
        CSResponseUserActivityGameInfo & response = *pbMsg.mutable_cs_response_user_activity_game_info();
        response.set_result(EN_MESSAGE_ERROR_OK);
        response.set_activity_type(iActivityType);
        response.set_game_type(iGameType);
        PBUserGameData & pbMsgUserGameData = *response.mutable_user_game_data();
        PBUserSkipMatchInfo & pbMsgUserSkipMatchInfo = *pbMsgUserGameData.mutable_user_skip_match_info();
        PBUserSkipMatchInfoItem & pbMsgInfoItem = *pbMsgUserSkipMatchInfo.add_skip_match_info_item();
        pbMsgInfoItem.CopyFrom(pbItemTmp);

        Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(),psession,pbMsg);
        return EN_Handler_Done;
    }
        break;
    default:
        break;
    }

    PBCSMsg pbMsg;
    CSResponseUserActivityGameInfo & response = *pbMsg.mutable_cs_response_user_activity_game_info();
    response.set_result(EN_MESSAGE_INVALID_ACTIVE_GAME_INFO);
    Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, pbMsg);
    return EN_Handler_Done;
}

ENHandlerResult CRequestGiveUpSkipMatchLevel::ProcessRequestMsg(CHandlerTokenBasic *ptoken, CSession *psession)
{
    psession->m_pVoidTmp = &m_CNoInfoGetState; //设置这个sessionid的默认get状态

    long long iUid = psession->_request_route.uid();
    psession->NewAddGetData(iUid, PBUserDataField::kUserInfo);
    psession->NewAddGetData(iUid, PBUserDataField::kUserGameData);
    return EN_Handler_Get;
}

ENHandlerResult CRequestGiveUpSkipMatchLevel::ProcessGetSucc(CHandlerTokenBasic *ptoken, CSession *psession)
{
    return ((GetState*)(psession->m_pVoidTmp))->ProcessGet(ptoken,psession);
}

ENHandlerResult CRequestGiveUpSkipMatchLevel::GetState_NoInfo::ProcessGet(CHandlerTokenBasic *ptoken, CSession *psession)
{
    psession->m_pVoidTmp = &this->m_pCRequestGiveUpSkipMatchLevel->m_CHadInfoGetState; //状态置为已获得信息状态

    //判定状态
    const CSRequestGiveUpSkipMatchLevel pbRequest = psession->_request_msg.cs_request_give_up_skip_match_level();
    int64 iUid = psession->_request_route.uid();
    PBUserGameData & pbUserGameData = *psession->_kvdb_uid_data_map[iUid].mutable_user_game_data();
    int iGameType = pbRequest.game_type();

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

    int iState = pbInfoItem == NULL ? 0 : pbInfoItem->skipmatch_state();

    if(iState != EN_Skip_Match_State_Failed)
    {
        VLogMsg(CLIB_LOG_LEV_DEBUG, "iRequestState != iState,iRequestState : %d ,iState = %d,maybe gm change", EN_Skip_Match_State_Failed, iState);
        PBCSMsg msg;
        CSResponseGiveUpSkipMatchLevel & response = *msg.mutable_cs_response_give_up_skip_match_level();
        response.set_result(EN_MESSAGE_INVALID_STATE);
        Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
        return EN_Handler_Done;
    }
    //如果没有最近的记录
    if(!pbInfoItem)
    {
        VLogMsg(CLIB_LOG_LEV_DEBUG, "Can not find lately pbInfoItem");
        PBCSMsg msg;
        CSResponseGiveUpSkipMatchLevel & response = *msg.mutable_cs_response_give_up_skip_match_level();
        response.set_result(EN_MESSAGE_INVALID_STATE);
        Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
        return EN_Handler_Done;
    }

    PBUpdateData pbUpdate;
    pbUpdate.set_key(PBUserDataField::kUserGameData);
    {
        PBDBAtomicField & pbField = *pbUpdate.add_field_list();
        pbField.set_activity_type(EN_User_Game_Info_Skip_Match);
        pbField.set_game_type(iGameType);
        pbField.set_field(EN_DB_Field_Skip_Match_Info_Level);
        pbField.set_skipmatch_session_id(pbInfoItem->lately_session_id());
        pbField.set_intval(1);
        pbField.set_reason(EN_Reason_LevelAndBonus_Choice_Reset);
    }

    {
        PBDBAtomicField & pbField = *pbUpdate.add_field_list();
        pbField.set_activity_type(EN_User_Game_Info_Skip_Match);
        pbField.set_game_type(iGameType);
        pbField.set_field(EN_DB_Field_Skip_Match_Info_State);
        pbField.set_skipmatch_session_id(pbInfoItem->lately_session_id());
        pbField.set_intval(EN_Skip_Match_State_Initial);
        pbField.set_reason(EN_Reason_LevelAndBonus_Choice_Reset);
    }
    psession->NewAddUpdateData(psession->_request_route.uid(), pbUpdate);

    return EN_Handler_Save;
}

ENHandlerResult CRequestGiveUpSkipMatchLevel::ProcessUpdateSucc(CHandlerTokenBasic *ptoken, CSession *psession)
{
    long long iUid = psession->_request_route.uid();
    psession->NewAddGetData(iUid, PBUserDataField::kUserInfo);
    psession->NewAddGetData(iUid, PBUserDataField::kUserGameData);
    return EN_Handler_Get;
}

ENHandlerResult CRequestGiveUpSkipMatchLevel::ProcessUpdateFailed(CHandlerTokenBasic *ptoken, CSession *psession)
{
    VLogMsg(CLIB_LOG_LEV_DEBUG, "db save failed");
    PBCSMsg msg;
    CSResponseGiveUpSkipMatchLevel & response = *msg.mutable_cs_response_give_up_skip_match_level();
    response.set_result(EN_MESSAGE_DB_SAVE_FAILED);
    Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
    return EN_Handler_Done;
}

ENHandlerResult CRequestGiveUpSkipMatchLevel::GetState_HadInfo::ProcessGet(CHandlerTokenBasic *ptoken, CSession *psession)
{
    const CSRequestGiveUpSkipMatchLevel pbRequest = psession->_request_msg.cs_request_give_up_skip_match_level();
    long long iUid = psession->_request_route.uid();
    PBUserGameData & pbUserGameData = *psession->_kvdb_uid_data_map[iUid].mutable_user_game_data();
    int iGameType = pbRequest.game_type();

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
        VLogMsg(CLIB_LOG_LEV_DEBUG, "db find failed");
        PBCSMsg msg;
        CSResponseGiveUpSkipMatchLevel & response = *msg.mutable_cs_response_give_up_skip_match_level();
        response.set_result(EN_MESSAGE_DB_NOT_FOUND);
        Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
        return EN_Handler_Done;
    }

    PBCSMsg msg;
    CSResponseGiveUpSkipMatchLevel & response = *msg.mutable_cs_response_give_up_skip_match_level();
    response.set_result(EN_MESSAGE_ERROR_OK);
    response.mutable_user_game_data()->mutable_user_skip_match_info()->add_skip_match_info_item()->CopyFrom(*pbInfoItem);
    Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
    return EN_Handler_Done;
}
