#include "JsonHelper.h"
#include "global.h"
#include "Common.h"
#include "Message.h"
#include "RouteManager.h"
#include "Base64.h"

PHPSocketMap JsonHelper::_socketmap;

bool JsonHelper::JsonToProto(InputPacket input, PBHead& head, PBCSMsg& msg, PBRoute& route)
{
    std::string json_msg = input.ReadString();
    Json::Value root;
    Json::Reader reader;
    if (!reader.parse(json_msg, root))
        return false;

    if (!root.isMember("msg_id"))
        return false;

    int msg_id = root["msg_id"].asInt();
    long long uid = root["uid"].asInt64();
    head.set_json_msg_id(msg_id);
    head.set_json_msg(json_msg);
    head.set_uid(uid);

    bool result = false;
    switch (msg_id)
    {
        case EN_Json_Update_Chips:
            result = GenerateUpdateChipsMsg(root, head, msg, route); break;
			
		case EN_Json_Update_Limit:
			result = GenerateUpdateLimitMsg(root, head, msg, route);break;

		case EN_Json_Request_User_Info:
			result = GenerateRequestUserInfo(root, head, msg, route); break;

		case EN_Json_Update_MultiChips:
			result = GenerateRequestMultiChipsMsg(root, head, msg, route);break;
			
		case EN_Json_Push_Msessage:
			result = GeneratePushMessageMsg(root,head,msg,route);break;

		case EN_Json_Update_User_Info:
			result = GenerateUpdateUserInfo(root,head,msg,route);break;

		case EN_Json_Request_Apply_Join_TeaBar:
			result = GenerateRequestApplyJoinTeaBar(root, head, msg, route); break;

		case EN_Json_Request_Del_TeaBar_Table:
			result = GenerateRequestDelTeaBarTable(root, head, msg, route); break;

		case EN_Json_Request_Transfer_TeaBar:
			result = GenerateRequestTransferTeaBar(root, head, msg, route); break;

        case EN_Json_Request_Table_Log:
            result = GenerateRequestTableLogMsg(root,head,msg,route);break;

        case EN_Json_Request_Dissolve_Table:
            result = GenerateRequestDissolveTableMsg(root,head,msg,route);break;

        case EN_Json_Request_Query_TeaBar_User_List:
            result = GenerateRequestQueryTeaBarUserList(root, head, msg, route); break;

        case EN_Json_Request_Update_User_Items:
            result = GenerateUpdateUserItems(root,head,msg,route); break;

        case EN_Json_Update_Diamond:
            result = GenerateUpdateDiamondMsg(root, head, msg, route); break;

        case EN_Json_Update_Bonus:
            result = GenerateUpdateBonusMsg(root, head, msg, route); break;

        case EN_Json_Update_Activity_User_Game_Info:
            result = GenerateUpdateSkipMatchAttributeMsg(root, head, msg, route); break;

        case EN_Json_Notify_Share_Success:
            result = GenerateNotifyShareSuccessMsg(root, head, msg, route); break;

        case EN_Json_Request_Activity_User_Game_Info:
            result = GenerateRequestUserSkipMatchInfo(root, head, msg, route); break;

		case EN_Json_Update_Coins:
			result = GenerateUpdateCoinsMsg(root, head, msg, route); break;

		case EN_Json_Update_Coins_And_Diamond:
			result = GenerateUpdateCoinAndDiamond(root, head, msg, route); break;

        default:
            break;
    }
    if (result)
    {
        head.set_cmd(msg.msg_union_case());
    }
    return result;
}

bool JsonHelper::GenerateUpdateChipsMsg(const Json::Value& root, PBHead& head, PBCSMsg& msg, PBRoute& route)
{
    if (!root.isMember("uid") ||
        !root.isMember("update_int64") ||
        !root.isMember("reason"))
    {
        return false;
    }

    long long uid = root["uid"].asInt64();
    long long update_val = root["update_int64"].asInt64();
    int reason = root["reason"].asInt();

	VLogMsg(CLIB_LOG_LEV_DEBUG,"invoke generate update chips msg.uid:%lld,update_int64:%lld,reason:%d",
		uid,update_val,reason);
	
    SSRequestUpdateUserData & ss_update = *msg.mutable_ss_request_update_user_data();
    ss_update.set_uid(uid);
    PBUpdateData& user_key = *ss_update.add_key_list();
    user_key.set_key(PBUserDataField::kUserInfo);
    PBDBAtomicField& chips_field = *user_key.add_field_list();
    chips_field.set_field(EN_DB_Field_Chips);
    chips_field.set_strategy(EN_Update_Strategy_Inc);
    chips_field.set_intval(update_val);
    chips_field.set_reason(reason);

    return true;
}

bool JsonHelper::GenerateRequestMultiChipsMsg(const  Json::Value & root,PBHead & head,PBCSMsg & msg,PBRoute & route)
{
	if (!root["uid_list"].isArray() || !root["uid_list"].size())
        return false;

	GMUpdateChipsMulti & request = *msg.mutable_gm_update_chips_multi();
    for (unsigned int i = 0; i < root["uid_list"].size(); ++i)
    {
        // 需要扣除
        if (root["uid_list"][i]["need_del"].asBool())
        {
            GMUpdateList& update = *request.add_need_del_list();
            update.set_uid(root["uid_list"][i]["uid"].asInt64());
            update.set_reason(root["uid_list"][i]["reason"].asInt());

            if (!root["uid_list"][i]["chips"].isInt64())
                return false;
            update.set_chips(root["uid_list"][i]["chips"].asInt64());
        }
        else
        {
            GMUpdateList& update = *request.add_update_list();
            update.set_uid(root["uid_list"][i]["uid"].asInt64());
            update.set_reason(root["uid_list"][i]["reason"].asInt());

            if (!root["uid_list"][i]["chips"].isInt64())
                return false;
            update.set_chips(root["uid_list"][i]["chips"].asInt64());
        }
    }
    return true;
}

bool JsonHelper::GenerateUpdateLimitMsg(const Json::Value & root, PBHead & head, PBCSMsg & msg,PBRoute & route)
{
	if (!root.isMember("uid") ||
        !root.isMember("update_int32"))
    {
        return false;
    }

    long long uid = root["uid"].asInt64();
    int update_val = root["update_int32"].asInt();
	VLogMsg(CLIB_LOG_LEV_DEBUG,"invoke generate update limit msg.uid:%lld,update_int32:%d",
		uid,update_val);

    SSRequestUpdateUserData& ss_update = *msg.mutable_ss_request_update_user_data();
    ss_update.set_uid(uid);
    PBUpdateData& user_key = *ss_update.add_key_list();
    user_key.set_key(PBUserDataField::kUserInfo);
    PBDBAtomicField& chips_field = *user_key.add_field_list();
    chips_field.set_field(EN_DB_Field_Limit);
    chips_field.set_strategy(EN_Update_Strategy_Replace);
    chips_field.set_intval(update_val);
	return true;
}

bool JsonHelper::GenerateUpdateUserInfo(const Json::Value& root, PBHead& head, PBCSMsg& msg, PBRoute& route)
{
	if (!root.isMember("uid"))
    {
        return false;
    }
	if(root.isMember("uid") && !root["uid"].isInt64())
	{
		return false;
	}
    long long uid = root["uid"].asInt64();
	if(!(root.isMember("roletype") || root.isMember("nick") || root.isMember("pic_url")))
	{
		return false;
	}
	if(root.isMember("nick"))
	{
		if(!root["nick"].isString())
		{
			return false;
		}
	}
	if(root.isMember("pic_url"))
	{
		if(!root["pic_url"].isString())
		{
			return false;
		}
	}
	if(root.isMember("roletype"))
	{
		if(!root["roletype"].isInt())
		{
			return false;
		}
	}
    SSRequestUpdateUserData& ss_update = *msg.mutable_ss_request_update_user_data();
    ss_update.set_uid(uid);
    PBUpdateData& user_key = *ss_update.add_key_list();
    user_key.set_key(PBUserDataField::kUserInfo);
	if(root.isMember("nick"))
	{
	    PBDBAtomicField& chips_field = *user_key.add_field_list();
	    chips_field.set_field(EN_DB_Field_Nick);
	    chips_field.set_strategy(EN_Update_Strategy_Replace);
    	chips_field.set_strvalue(root["nick"].asString());
    }
	if(root.isMember("pic_url"))
	{
	    PBDBAtomicField& chips_field = *user_key.add_field_list();
	    chips_field.set_field(EN_DB_Field_Pic_Url);
	    chips_field.set_strategy(EN_Update_Strategy_Replace);
    	chips_field.set_strvalue(root["pic_url"].asString());
    }
	if(root.isMember("roletype") == true)
	{
		VLogMsg(CLIB_LOG_LEV_DEBUG,"update user[%lld] roletype[%d]",uid,root["roletpye"].asInt());
		{
		    PBDBAtomicField& chips_field = *user_key.add_field_list();
		    chips_field.set_field(EN_DB_Field_RoleType);
		    chips_field.set_strategy(EN_Update_Strategy_Replace);
	    	chips_field.set_intval(root["roletype"].asInt());
		}
		if(root.isMember("reset_pos"))
		{
			{
				PBDBAtomicField& field = *user_key.add_field_list();
				field.set_field(EN_DB_Field_POS);
				PBBPlayerPositionInfo& pos = *field.mutable_pos();
				pos.set_pos_type(EN_Position_Hall);
				pos.set_table_id(0);
				pos.set_gamesvrd_id(0);
			}
			{
				PBDBAtomicField& field = *user_key.add_field_list();
				field.set_field(EN_DB_Field_Create_Table);
				field.set_intval(0);
			}
		}
        if(root.isMember("reset_skipmatch_pos"))
        {
            {
                PBDBAtomicField& field = *user_key.add_field_list();
                field.set_field(EN_DB_Field_Skipmatch_POS);
                PBBPlayerPositionInfo& pos = *field.mutable_pos();
                pos.set_pos_type(EN_Position_Hall);
                pos.set_table_id(0);
                pos.set_gamesvrd_id(0);
            }
            {
                PBDBAtomicField& field = *user_key.add_field_list();
                field.set_field(EN_DB_Field_Create_Table);
                field.set_intval(0);
            }
        }
    }
	else
	{
		VLogMsg(CLIB_LOG_LEV_DEBUG,"don`t need update user[%lld]",uid);
	}
	return true;
}


bool JsonHelper::GenerateRequestUserInfo(const Json::Value& root, PBHead& head, PBCSMsg& msg, PBRoute& route)
{
	if (!root.isMember("uid"))
    {
        return false;
    }

    long long uid = root["uid"].asInt64();
	VLogMsg(CLIB_LOG_LEV_DEBUG,"invoke generate request user info msg.uid:%lld",uid);
	SSRequestGetUserData& request = *msg.mutable_ss_request_get_user_data();
    PBDataSet& data_set = *request.mutable_data_set();
    data_set.set_uid(uid);
    data_set.add_key_list()->set_key(PBUserDataField::kUserInfo);
    data_set.add_key_list()->set_key(PBUserDataField::kUserTeaBarData);
	return true;
}

bool JsonHelper::GeneratePushMessageMsg(const Json::Value& root, PBHead& head, PBCSMsg& msg, PBRoute& route)
{
	GMPushMessageMulti & request = *msg.mutable_gm_push_message_multi();
    for (unsigned int i = 0; i < root["uid_list"].size(); ++i)
    {
    	if (!root["uid_list"][i].isInt64())
            return false;
        request.add_uid_list(root["uid_list"][i].asInt64());
    }
	if(request.uid_list_size()>100)
		return false;
	if(!root["message"].isString())
		return false;
	if(root["all"].asBool())
	{
		request.set_all(true);
	}
	request.set_message(root["message"].asString());
	return true;
}

bool JsonHelper::GenerateRequestTransferTeaBar(const Json::Value& root, PBHead& head, PBCSMsg& msg, PBRoute& route)
{
	if (root.isMember("old_master_uid") && !root["old_master_uid"].isInt64())
	{
		return false;
	}

	if (root.isMember("new_master_uid") && !root["new_master_uid"].isInt64())
	{
		return false;
	}

	SSRequestTransferTeaBar& ss_request_transfer_tea_bar = *msg.mutable_ss_request_transfer_tea_bar();
	ss_request_transfer_tea_bar.set_old_master_uid(root["old_master_uid"].asInt64());
	ss_request_transfer_tea_bar.set_new_master_uid(root["new_master_uid"].asInt64());
	return true;
}

bool JsonHelper::ProtoToJson(const PBHead& head, const PBCSMsg& msg, std::string& json_msg)
{
    switch (head.json_msg_id())
    {
        case EN_Json_Update_Chips:
        case EN_Json_Update_Limit:
        case EN_Json_Update_Diamond:
        case EN_Json_Update_Bonus:
        case EN_Json_Update_Activity_User_Game_Info:
            return GenerateUpdateRsp(msg, json_msg);

        case EN_Json_Request_User_Info:
			return GenerateGetUserDataRsp(msg, json_msg);

		case EN_Json_Update_User_Info:
        case EN_Json_Request_Update_User_Items:
			return GenerateUpdateRsp(msg,json_msg);

		case EN_Json_Request_Apply_Join_TeaBar:
			return GenerateRequestApplyJoinTeaBarRsp(msg, json_msg);

        case EN_Json_Request_Table_Log:
            return GenerateTableLogRsp(msg, json_msg);

        case EN_Json_Request_Dissolve_Table:
            return GenerateDissolveTableRsp(msg, json_msg);

        case EN_Json_Request_Query_TeaBar_User_List:
            return GenerateResponseQueryTeaBarUserList(msg, json_msg);

        default: return false;
    }
}


bool JsonHelper::GenerateGetUserDataRsp(const PBCSMsg& msg, std::string& json_msg)
{
	const SSResponseGetUserData & response = msg.ss_response_get_user_data();
	const PBDataSet & data_set = response.data_set();
    Json::Value root;
    Json::FastWriter writer;

	const PBUserData & user_data = data_set.user_data();
	const PBUser & user = user_data.user_info();
    root["result"] = response.result();
	
	Json::Value pb_user_root;
	pb_user_root["uid"] = (Json::Int64)user.uid();
	pb_user_root["chips"] = (Json::Int64)user.chips();
	pb_user_root["nick"] = base64_encode(user.nick().c_str(), user.nick().length());
	pb_user_root["pic_url"] = user.pic_url();
	pb_user_root["acc_type"] = user.acc_type();
	pb_user_root["channel"] = user.channel();
	pb_user_root["gender"] = user.gender();
	pb_user_root["roletype"] = user.roletype();
	const PBBPlayerPositionInfo & pos = user.pos();
	Json::Value pb_pos_root;
	pb_pos_root["pos_type"] = pos.pos_type();
	pb_pos_root["table_id"] = (Json::Int64)pos.table_id();
	pb_user_root["pos"] = pb_pos_root;

    pb_user_root["regist_time"] = (Json::Int64)user.regist_time();

    const PBBPlayerPositionInfo & skip_pos = user.skipmatch_pos();
    Json::Value pb_skip_pos_root;
    pb_skip_pos_root["pos_type"] = skip_pos.pos_type();
    pb_skip_pos_root["table_id"] = (Json::Int64)skip_pos.table_id();
    pb_user_root["pos"] = pb_skip_pos_root;

    pb_user_root["diamond"] = (Json::Int64)user.diamond();
    pb_user_root["bonus"] = user.bonus();
    pb_user_root["total_bonus"] = user.total_bonus();
	pb_user_root["coins"] = (Json::Int64)user.coins();


    /*
     * 用户accounts显示
     * 多账号绑定
    */
    Json::Value pb_accounts_root;
    for(int i = 0 ; i < user.accounts_size() ; i ++)
    {
        pb_accounts_root[i] = user.accounts(i);
    }
    pb_user_root["accounts"] = pb_accounts_root;

    /*
     * 茶馆信息，列出他加入的茶馆
    */
    Json::Value pb_teabar_info;
    Json::Value pb_master_teabar_id;
    int iMasterIndex = 0;
    for(int i = 0 ; i < data_set.user_data().user_tea_bar_data().brief_data_size() ; i ++)
    {
        const TeaBarBriefData & pbBrief = data_set.user_data().user_tea_bar_data().brief_data(i);
        long long lTbid = pbBrief.tbid();
        pb_teabar_info[i] = (Json::Int64)lTbid;

        if(pbBrief.master_uid() == user.uid())
        {
            pb_master_teabar_id[iMasterIndex ++] = (Json::Int64)lTbid;
        }
    }
	
    root["PBUser"] = pb_user_root;                  //用户信息
    root["PBTeaBarInfo"] = pb_teabar_info;          //用户加入的茶馆列表
    root["MasterTeaBarId"] = pb_master_teabar_id;   //用户创建的茶馆列表
	
    json_msg = writer.write(root);
	
    return true;
}

bool JsonHelper::GenerateUpdateRsp(const PBCSMsg& msg, std::string& json_msg)
{
    const SSResponseUpdateUserData& response = msg.ss_response_update_user_data();

    Json::Value root;
    Json::FastWriter writer;
    root["result"] = response.result();
    json_msg = writer.write(root);
    return true;
}


bool JsonHelper::GenerateRequestApplyJoinTeaBar(const Json::Value& root, PBHead& head, PBCSMsg& msg, PBRoute& route)
{
	if (!root.isMember("uid") || !root.isMember("tbid"))
	{
		return false;
	}

	CSRequestApplyJoinTeaBar& cs_request_apply_join_tea_bar = *msg.mutable_cs_request_apply_join_tea_bar();
	cs_request_apply_join_tea_bar.set_tbid(root["tbid"].asInt64());
	return true;
}

bool JsonHelper::GenerateRequestApplyJoinTeaBarRsp(const PBCSMsg& msg, std::string& json_msg)
{
	const CSResponseApplyJoinTeaBar & response = msg.cs_response_apply_join_tea_bar();
	Json::Value root;
	Json::FastWriter writer;
	root["result"] = response.result();
	root["tbid"] = (Json::Int64)response.tbid();
	json_msg = writer.write(root);

	return true;
}

bool JsonHelper::GenerateRequestDelTeaBarTable(const Json::Value& root, PBHead& head, PBCSMsg& msg, PBRoute& route)
{
	if (!root.isMember("tid") || !root["tid"].isInt64() || !root.isMember("tbid") || !root["tbid"].isInt64())
	{
		return false;
	}

	int64 tbid = root["tbid"].asInt64();
	int64 tid = root["tid"].asInt64();
	SSNotifyTeaBarTableGameOver& ss_notify_tea_bar_game_over = *msg.mutable_ss_notify_teabar_table_game_over();
	ss_notify_tea_bar_game_over.mutable_statistics();
	ss_notify_tea_bar_game_over.mutable_conf()->set_tbid(tbid);
	ss_notify_tea_bar_game_over.set_tid(tid);
	return true;
}

bool JsonHelper::GenerateRequestTableLogMsg(const Json::Value& root, PBHead& head, PBCSMsg& msg, PBRoute& route)
{
    if (!root.isMember("tid") || !root["tid"].isInt64())
    {
        return false;
    }

    int64 tid = root["tid"].asInt64();
    SSRequestTableLog& ss_request = *msg.mutable_ss_request_table_log();
    ss_request.set_tid(tid);
    return true;
}

bool JsonHelper::GenerateRequestDissolveTableMsg(const Json::Value& root, PBHead& head, PBCSMsg& msg, PBRoute& route)
{
    if (!root.isMember("tid") || !root["tid"].isInt64())
    {
        return false;
    }

    int64 tid = root["tid"].asInt64();
    SSRequestDissolveTable& ss_request = *msg.mutable_ss_request_dissolve_table();
    ss_request.set_tid(tid);
    return true;
}


bool JsonHelper::GenerateTableLogRsp(const PBCSMsg& msg, std::string& json_msg)
{
    const SSResponseTableLog& response = msg.ss_response_table_log();

    Json::Value root;
    Json::FastWriter writer;
    root["result"] = response.table_data();
    json_msg = writer.write(root);
    return true;
}

bool JsonHelper::GenerateDissolveTableRsp(const PBCSMsg& msg, std::string& json_msg)
{
    const SSResponseDissolveTable& response = msg.ss_response_dissolve_table();

    Json::Value root;
    Json::FastWriter writer;
    root["result"] = response.result();
    json_msg = writer.write(root);
    return true;
}

bool JsonHelper::GenerateRequestQueryTeaBarUserList(const Json::Value& root, PBHead& a_pbHead, PBCSMsg& a_pbMsg, PBRoute& a_pbRoute)
{
    if(!root.isMember("tbid"))
    {
        return false;
    }

    long long lTbid = root["tbid"].asInt64();

    GMRequestQueryTeaBarUserList & gm_request_query_teabar_user_list = *a_pbMsg.mutable_gm_request_query_teabar_user_list();
    gm_request_query_teabar_user_list.set_tbid(lTbid);

    return true;
}

bool JsonHelper::GenerateResponseQueryTeaBarUserList(const PBCSMsg& msg, std::string& json_msg)
{
    const GMResponseQueryTeaBarUserList & gm_response_query_teabar_user_list = msg.gm_response_query_teabar_user_list();
    Json::Value root;
    Json::Value root_uid;
    Json::FastWriter writer;
    for(int i = 0 ; i < gm_response_query_teabar_user_list.uids_size() ; i ++)
    {
        long long lUid = gm_response_query_teabar_user_list.uids(i);
        root_uid[i] = (Json::Int64)lUid;
    }
    root["uid"] = root_uid;
    root["result"] = gm_response_query_teabar_user_list.result();
    json_msg = writer.write(root);



    return true;
}

bool JsonHelper::GenerateUpdateUserItems(const Json::Value& root, PBHead& head, PBCSMsg& msg, PBRoute& route)
{
    if(!root.isMember("uid"))
    {
        return false;
    }

    if(root.isMember("uid") && !root["uid"].isInt64())
    {
        return false;
    }

    long long uid = root["uid"].asInt64();

    if(root.isMember("items_info"))
    {
        if(!root["items_info"].isString())
        {
            return false;
        }
    }

    SSRequestUpdateUserData& ss_update = *msg.mutable_ss_request_update_user_data();
    ss_update.set_uid(uid);
    PBUpdateData& user_key = *ss_update.add_key_list();
    user_key.set_key(PBUserDataField::kUserInfo);
    if(root.isMember("items_info"))
    {
        PBDBAtomicField & pbField = *user_key.add_field_list();
        pbField.set_field(EN_DB_Field_Items_Info);
        pbField.set_strategy(EN_Update_Strategy_Replace);
        pbField.set_strvalue(root["items_info"].asString());
    }
    else
    {
        VLogMsg(CLIB_LOG_LEV_DEBUG,"don`t need update user[%lld]",uid);
    }

    return true;
}

bool JsonHelper::GenerateUpdateDiamondMsg(const Json::Value& a_jvRoot, PBHead & a_pbHead, PBCSMsg & a_pbMsg, PBRoute & a_pbRoute)
{
    if(!a_jvRoot.isMember("uid") ||
            !a_jvRoot.isMember("update_int64") ||
            !a_jvRoot.isMember("reason"))
    {
        return false;
    }

    long long iUid = a_jvRoot["uid"].asInt64();
    long long iUpdateValue = a_jvRoot["update_int64"].asInt64();
    int iReason = a_jvRoot["reason"].asInt();

    SSRequestUpdateUserData & Update = *a_pbMsg.mutable_ss_request_update_user_data();
    Update.set_uid(iUid);
    PBUpdateData & Key = *Update.add_key_list();
    Key.set_key(PBUserDataField::kUserInfo);
    PBDBAtomicField & Field = *Key.add_field_list();
    Field.set_field(EN_DB_Field_Diamond);
    Field.set_intval(iUpdateValue);
    Field.set_reason(iReason);

    return true;
}

bool JsonHelper::GenerateUpdateBonusMsg(const Json::Value& a_jvRoot, PBHead & a_pbHead, PBCSMsg & a_pbMsg, PBRoute & a_pbRoute)
{
    if(!a_jvRoot.isMember("uid") ||
            !a_jvRoot.isMember("update_float") ||
            !a_jvRoot.isMember("reason"))
    {
        return false;
    }

    long long iUid = a_jvRoot["uid"].asInt64();
    float fUpdate = a_jvRoot["update_float"].asFloat();
    int iReason = a_jvRoot["reason"].asInt();

    SSRequestUpdateUserData & Update = *a_pbMsg.mutable_ss_request_update_user_data();
    Update.set_uid(iUid);
    PBUpdateData & Key = *Update.add_key_list();
    Key.set_key(PBUserDataField::kUserInfo);
    PBDBAtomicField & Field = *Key.add_field_list();
    Field.set_field(EN_DB_Field_Bonus);
    Field.set_floatval(fUpdate);
    Field.set_reason(iReason);

    return true;
}

bool JsonHelper::GenerateUpdateSkipMatchAttributeMsg(const Json::Value& a_jvRoot, PBHead & a_pbHead, PBCSMsg & a_pbMsg, PBRoute & a_pbRoute)
{
    if(!a_jvRoot.isMember("uid") ||
            !a_jvRoot.isMember("reason"))
    {
        return false;
    }

    if(!a_jvRoot.isMember("update_level") &&
            !a_jvRoot.isMember("update_state"))
    {
        return false;
    }

    if(!a_jvRoot.isMember("game_type") ||
            !a_jvRoot.isMember("activity_type"))
    {
        return false;
    }

    long long iUid = a_jvRoot["uid"].asInt64();
    int iReason = a_jvRoot["reason"].asInt();
    int iUpdateLevel = 0;
    int iUpdateState = -1;
    int iGameType = a_jvRoot["game_type"].asInt();
    int iActivityType = a_jvRoot["activity_type"].asInt();

    //等级
    if(a_jvRoot.isMember("update_level"))
    {
        iUpdateLevel = a_jvRoot["update_level"].asInt();
    }

    //状态
    if(a_jvRoot.isMember("update_state"))
    {
        iUpdateState = a_jvRoot["update_state"].asInt();
    }

    SSRequestUpdateUserData& ss_update = *a_pbMsg.mutable_ss_request_update_user_data();
    ss_update.set_uid(iUid);
    PBUpdateData & pbUpdate = *ss_update.add_key_list();
    pbUpdate.set_key(PBUserDataField::kUserGameData);
    {
        PBDBAtomicField & pbField = *pbUpdate.add_field_list();
        pbField.set_activity_type(iActivityType);
        pbField.set_game_type(iGameType);
        pbField.set_field(EN_DB_Field_Skip_Match_Info_Level);
        pbField.set_strategy(EN_Update_Strategy_Inc);
        pbField.set_intval(iUpdateLevel);
        pbField.set_reason(iReason);
    }
    {
        PBDBAtomicField & pbField = *pbUpdate.add_field_list();
        pbField.set_activity_type(iActivityType);
        pbField.set_game_type(iGameType);
        pbField.set_field(EN_DB_Field_Skip_Match_Info_State);
        pbField.set_intval(iUpdateState);
        pbField.set_reason(iReason);
    }

    return true;
}

bool JsonHelper::GenerateNotifyShareSuccessMsg(const Json::Value& a_jvRoot, PBHead & a_pbHead, PBCSMsg & a_pbMsg, PBRoute & a_pbRoute)
{
    if(!a_jvRoot.isMember("uid") ||
            !a_jvRoot.isMember("update_state") ||
            !a_jvRoot.isMember("session_id") ||
            !a_jvRoot.isMember("game_type") ||
            !a_jvRoot.isMember("activity_type"))
    {
        return false;
    }

    long long iUid = a_jvRoot["uid"].asInt64();
    long long iSessionId = a_jvRoot["session_id"].asInt64();
    int iUpdateState = a_jvRoot["update_state"].asInt();
    int iGameType = a_jvRoot["game_type"].asInt();
    int iActivityType = a_jvRoot["activity_type"].asInt();

    if(iUpdateState != 0)
    {
        return false;
    }

    if(iSessionId > 0)
    {
        GMRequestNotifyShareSuccess & gm_notify_share_success = *a_pbMsg.mutable_gm_request_notify_share_success();
        gm_notify_share_success.set_uid(iUid);
        gm_notify_share_success.set_game_type(iGameType);
        gm_notify_share_success.set_activity_type(iActivityType);
        gm_notify_share_success.set_session_id(iSessionId);
        return true;
    }

    return false;
}

bool JsonHelper::GenerateRequestUserSkipMatchInfo(const Json::Value& a_jvRoot, PBHead & a_pbHead, PBCSMsg & a_pbMsg, PBRoute & a_pbRoute)
{
    if (!a_jvRoot.isMember("uid") || !a_jvRoot.isMember("game_type") || !a_jvRoot.isMember("activity_type"))
    {
        return false;
    }

    long long iUid = a_jvRoot["uid"].asInt64();
    int iActivityType = a_jvRoot["activity_type"].asInt();
    int iGameType = a_jvRoot["game_type"].asInt();
    VLogMsg(CLIB_LOG_LEV_DEBUG,"invoke generate request user info msg.uid:%lld",iUid);

    CSRequestUserActivityGameInfo & pbRequest = *a_pbMsg.mutable_cs_request_user_activity_game_info();
    pbRequest.set_uid(iUid);
    pbRequest.set_activity_type(iActivityType);
    pbRequest.set_game_type(iGameType);

    return true;
}

bool JsonHelper::GenerateUpdateUserSkipMatchDataRsp(const PBCSMsg& a_pbMsg, std::string& a_szJsonMsg)
{
    const GMResponseEditUserActivityGameInfo & response = a_pbMsg.gm_response_edit_user_activity_game_info();

    Json::Value root;
    Json::FastWriter writer;
    root["result"] = response.result();
    a_szJsonMsg = writer.write(root);
    return true;
}

/*
更新金币
*/
bool JsonHelper::GenerateUpdateCoinsMsg(const Json::Value& a_jvRoot, PBHead & a_pbHead, PBCSMsg & a_pbMsg, PBRoute & a_pbRoute)
{
	if (!a_jvRoot.isMember("uid") ||
		!a_jvRoot.isMember("update_int64") ||
		!a_jvRoot.isMember("reason"))
	{
		return false;
	}

	long long iUid = a_jvRoot["uid"].asInt64();
	long long iUpdateValue = a_jvRoot["update_int64"].asInt64();
	int iReason = a_jvRoot["reason"].asInt();

	SSRequestUpdateUserData & Update = *a_pbMsg.mutable_ss_request_update_user_data();
	Update.set_uid(iUid);
	PBUpdateData & Key = *Update.add_key_list();
	Key.set_key(PBUserDataField::kUserInfo);
	PBDBAtomicField & Field = *Key.add_field_list();
	Field.set_field(EN_DB_Field_Coin);
	Field.set_intval(iUpdateValue);
	Field.set_reason(iReason);

	return true;
}

/*
兑换金币和砖石
*/
bool JsonHelper::GenerateUpdateCoinAndDiamond(const Json::Value& a_jvRoot, PBHead & a_pbHead, PBCSMsg & a_pbMsg, PBRoute & a_pbRoute)
{
	if (!a_jvRoot.isMember("uid") ||
		!a_jvRoot.isMember("coins_update_int64") ||
		!a_jvRoot.isMember("diamond_update_int64") ||
		!a_jvRoot.isMember("reason"))
	{
		return false;
	}

	long long iUid = a_jvRoot["uid"].asInt64();
	long long iCoinsUpdateValue = a_jvRoot["coins_update_int64"].asInt64();
	long long iDiamondUpdateValue = a_jvRoot["diamond_update_int64"].asInt64();
	int iReason = a_jvRoot["reason"].asInt();

	SSRequestUpdateUserData & Update = *a_pbMsg.mutable_ss_request_update_user_data();
	Update.set_uid(iUid);

	PBUpdateData & Key = *Update.add_key_list();
	Key.set_key(PBUserDataField::kUserInfo);
	{
		PBDBAtomicField & Field = *Key.add_field_list();
		Field.set_field(EN_DB_Field_Coin);
		Field.set_intval(iCoinsUpdateValue);
		Field.set_reason(iReason);
	}

	{
		PBDBAtomicField & Field = *Key.add_field_list();
		Field.set_field(EN_DB_Field_Diamond);
		Field.set_intval(iDiamondUpdateValue);
		Field.set_reason(iReason);
	}

	return true;
}