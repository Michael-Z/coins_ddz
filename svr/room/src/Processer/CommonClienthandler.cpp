#include "CommonClienthandler.h"
#include "RouteManager.h"
#include "Message.h"
#include "PBConfigBasic.h"
#include "Common.h"
#include <algorithm>
#include "GameSvrdManager.h"
#include "TableManager.h"
#include "ZoneTableManager.h"
#include "RoomHandlerProxy.h"
#include "CostManager.h"
#include "LogWriter.h"
//比赛场
//#include "JsonHelper.h"
#include "QuickMatchManager.h"
#include "ActivityRedisClient.h"

ENProcessResult CRegistInnerServer::ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession)
{
    const SSNotifyInnerServer ss_notify_inner_server = psession->_request_msg.ss_notify_inner_server();
    int routeid = ss_notify_inner_server.routeid();
    RouteManager::Instance()->_route_map[routeid] = ptoken->_phandler;
    VLogMsg(CLIB_LOG_LEV_ERROR,"receive noitfy from route[%d]",routeid);
    PBCSMsg msg;
    msg.mutable_ss_notify_room_svrd();

    /*ccc---------beg
     * 读取room——svrd配置
    */
    {
        for (int i = 0; i <  PokerPBRoomSvrdConfig::Instance()->games_size(); i++)
        {
            int node_type = PokerPBRoomSvrdConfig::Instance()->games(i).node_type();
            Message::BroadcastToPhzGame(RouteManager::Instance()->GetRouteByRandom(), msg, node_type);
        }
    }
    //ccc---------end
    return EN_Process_Result_Completed;
}

ENHandlerResult CReportGameInfo::ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession)
{
    const SSReportGameSvrdInfo & ss_report_gamesvrd_info = psession->_request_msg.ss_report_game_info();
    int gameid = ss_report_gamesvrd_info.gameid();
    int gtype = ss_report_gamesvrd_info.gtype();

    GameSvrdManager::Instance()->OnReportGame(gtype,gameid);
    return EN_Handler_Done;
}

ENHandlerResult CNotifyGamesvrdClosed::ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession)
{
    const SSNotifyGameSvrdClosed & ss_notify_gamesvrd_closed = psession->_request_msg.ss_notify_gamesvrd_closed();
    int gtype = ss_notify_gamesvrd_closed.gtype();
    int gameid = ss_notify_gamesvrd_closed.gameid();
    GameSvrdManager::Instance()->OnCloseGame(gtype,gameid);
    return EN_Handler_Done;
}

ENHandlerResult CNotifyGamesvrdRetired::ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession)
{
    const SSNotifyGameSvrdRetire & ss_notify_gamesvrd_retired = psession->_request_msg.ss_notify_gamesvrd_retired();
    int gtype = ss_notify_gamesvrd_retired.gtype();
    int gameid = ss_notify_gamesvrd_retired.gameid();
    GameSvrdManager::Instance()->OnRetireGame(gtype,gameid);
    return EN_Handler_Done;
}

ENHandlerResult CNotifyTableDissolved::ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession)
{
    const SSNotifyTableDissolved & ss_notify_table_dissolved = psession->_request_msg.ss_notify_table_dissolved();
    int tid = ss_notify_table_dissolved.tid();
    int game_type = ss_notify_table_dissolved.game_type();

    /*ccc---------beg
     * 读取room——svrd配置
    */
    {
        bool has_find = false;
        for (int i = 0; i <  PokerPBRoomSvrdConfig::Instance()->games_size(); i++)
        {
            int node_type = PokerPBRoomSvrdConfig::Instance()->games(i).node_type();
            if(game_type == node_type)
            {
                has_find = true;
                int zone_type = PokerPBRoomSvrdConfig::Instance()->zone_type();
                ZoneTableManager::Instance()->DissolveTable(tid, zone_type);
            }
        }
        if(!has_find)
        {
            ErrMsg("failed to find game type .");
        }
    }
    //ccc---------end

    return EN_Handler_Done;
}

ENHandlerResult CRequestDelegateTableInfo::ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession)
{
    psession->NewAddGetData(psession->_request_route.uid(), PBUserDataField::kUserInfo);
    psession->NewAddGetData(psession->_request_route.uid(), PBUserDataField::kUserTableInfo);
    return EN_Handler_Get;
}

ENHandlerResult CRequestDelegateTableInfo::ProcessGetSucc(CHandlerTokenBasic * ptoken,CSession * psession)
{
    long long uid = psession->_request_route.uid();
    const PBUser & user = psession->_kvdb_uid_data_map[uid].user_info();
    PBUserTableInfo & user_table_info = *psession->_kvdb_uid_data_map[uid].mutable_user_table_info();
    if(user.roletype() != EN_Role_Type_Saler)
    {
        PBCSMsg msg;
        CSResponseDelegateTableInfo & response = *msg.mutable_cs_response_delegate_table_info();
        response.set_result(EN_MESSAGE_SALER_EXPECTED);
        Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
        return EN_Handler_Done;
    }
    bool need_refresh = false;
    for(int i=0;i<user_table_info.running_table_list_size();i++)
    {
        const PBDelegateTableInfo & info = user_table_info.running_table_list(i);
        int64 tid = info.tid();
        if (!TableManager::Instance()->CheckAllocedTable(tid))
        {
            //代开房已被回收
            {
                PBUpdateData update;
                update.set_key(PBUserDataField::kUserTableInfo);
                PBDBAtomicField & field = *update.add_field_list();
                field.set_field(EN_DB_Field_Table_Running);
                field.set_strategy(EN_Update_Strategy_Del);
                PBDelegateTableInfo & field_delegate_table_info = *field.mutable_tableinfo();
                field_delegate_table_info.CopyFrom(info);
                psession->NewAddUpdateData(uid, update);
            }
            {
                PBUpdateData update;
                update.set_key(PBUserDataField::kUserTableInfo);
                PBDBAtomicField & field = *update.add_field_list();
                field.set_field(EN_DB_Field_Table_Closed);
                PBDelegateTableInfo & field_delegate_table_info = *field.mutable_tableinfo();
                field_delegate_table_info.CopyFrom(info);
                field_delegate_table_info.set_state(EN_Delegate_Table_State_TimeOut);
                psession->NewAddUpdateData(uid, update);
            }
            need_refresh = true;
        }
    }
    if(need_refresh == true)
    {
        return EN_Handler_Save;
    }
    PBCSMsg msg;
    CSResponseDelegateTableInfo & response = *msg.mutable_cs_response_delegate_table_info();
    response.set_result(EN_MESSAGE_ERROR_OK);
    response.mutable_delegate_table_info()->CopyFrom(user_table_info);
    Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
    return EN_Handler_Done;
}

ENHandlerResult CRequestDelegateTableInfo::ProcessUpdateSucc(CHandlerTokenBasic * ptoken,CSession * psession)
{
    long long uid = psession->_request_route.uid();
    PBUserTableInfo & user_table_info = *psession->_kvdb_uid_data_map[uid].mutable_user_table_info();
    PBCSMsg msg;
    CSResponseDelegateTableInfo & response = *msg.mutable_cs_response_delegate_table_info();
    response.set_result(EN_MESSAGE_ERROR_OK);
    response.mutable_delegate_table_info()->CopyFrom(user_table_info);
    Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
    return EN_Handler_Done;
}

ENHandlerResult CRequestDissolveDelegateTable::ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession)
{
    psession->NewAddGetData(psession->_request_route.uid(), PBUserDataField::kUserInfo);
    psession->NewAddGetData(psession->_request_route.uid(), PBUserDataField::kUserTableInfo);
    return EN_Handler_Get;
}

ENHandlerResult CRequestDissolveDelegateTable::ProcessGetSucc(CHandlerTokenBasic * ptoken,CSession * psession)
{
    const CSRequestDissolveDelegateTable & cs_request_dissolve_delegate_table = psession->_request_msg.cs_request_dissolve_delegate_table();
    long long uid = psession->_request_route.uid();
    PBUserTableInfo & user_table_info = *psession->_kvdb_uid_data_map[uid].mutable_user_table_info();
    bool found = false;
    PBDelegateTableInfo delegate_table_info;
    for(int i=0;i<user_table_info.running_table_list_size();i++)
    {
        const PBDelegateTableInfo & info = user_table_info.running_table_list(i);
        int64 tid = info.tid();
        if (tid == cs_request_dissolve_delegate_table.tid())
        {
            found = true;
            delegate_table_info.CopyFrom(info);
        }
    }
    if(!found)
    {
        PBCSMsg msg;
        CSResponseDissolveDelegateTable & response = *msg.mutable_cs_response_dissolve_delegate_table();
        response.set_result(EN_MESSAGE_TABLE_NOT_EXIST);
        Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
        return EN_Handler_Done;
    }
    /*
    if (!TableManager::Instance()->CheckAllocedTable(cs_request_dissolve_delegate_table.tid()))
    {
        PBCSMsg msg;
        CSResponseDissolveDelegateTable & response = *msg.mutable_cs_response_dissolve_delegate_table();
        response.set_result(EN_MESSAGE_TABLE_NOT_EXIST);
        Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
        return EN_Handler_Done;
    }
    */
    int gamesvid = TableManager::Instance()->GetGamesvrdIDForTable(cs_request_dissolve_delegate_table.tid());
    //int gamesvid = 1;
    if(!GameSvrdManager::Instance()->IsGameExist(EN_Node_Game,gamesvid) ||
                !TableManager::Instance()->CheckAllocedTable(cs_request_dissolve_delegate_table.tid()))
    {
        //代开房已被回收
        {
            PBUpdateData update;
            update.set_key(PBUserDataField::kUserTableInfo);
            PBDBAtomicField & field = *update.add_field_list();
            field.set_field(EN_DB_Field_Table_Running);
            field.set_strategy(EN_Update_Strategy_Del);
            PBDelegateTableInfo & field_delegate_table_info = *field.mutable_tableinfo();
            field_delegate_table_info.CopyFrom(delegate_table_info);
            psession->NewAddUpdateData(uid, update);
        }
        {
            PBUpdateData update;
            update.set_key(PBUserDataField::kUserTableInfo);
            PBDBAtomicField & field = *update.add_field_list();
            field.set_field(EN_DB_Field_Table_Closed);
            PBDelegateTableInfo & field_delegate_table_info = *field.mutable_tableinfo();
            field_delegate_table_info.CopyFrom(delegate_table_info);
            field_delegate_table_info.set_state(EN_Delegate_Table_State_TimeOut);
            psession->NewAddUpdateData(uid, update);
        }
        return EN_Handler_Save;
    }
    Message::SendRequestMsg(RouteManager::Instance()->GetRouteByRandom(),psession,psession->_request_msg,
                            EN_Node_Game,gamesvid);
    return EN_Handler_Succ;
}

ENHandlerResult CRequestDissolveDelegateTable::ProcessUpdateSucc(CHandlerTokenBasic * ptoken,CSession * psession)
{
    PBCSMsg msg;
    CSResponseDissolveDelegateTable & response = *msg.mutable_cs_response_dissolve_delegate_table();
    response.set_result(EN_MESSAGE_ERROR_OK);
    Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
    return EN_Handler_Done;
}

ENHandlerResult CRequestDissolveDelegateTable::ProcessResponseMsg(CHandlerTokenBasic * ptoken,CSession * psession)
{
    const CSRequestDissolveDelegateTable & cs_request_dissolve_delegate_table = psession->_request_msg.cs_request_dissolve_delegate_table();
    CSResponseDissolveDelegateTable & cs_response_dissolve_delegate_table = *psession->_response_msg.mutable_cs_response_dissolve_delegate_table();
    long long uid = psession->_request_route.uid();
    PBUserTableInfo & user_table_info = *psession->_kvdb_uid_data_map[uid].mutable_user_table_info();
    if(cs_response_dissolve_delegate_table.result() == EN_MESSAGE_TABLE_NOT_EXIST)
    {
        //游戏服重启了 桌子不存在 要释放掉
        bool found = false;
        PBDelegateTableInfo delegate_table_info;
        for(int i=0;i<user_table_info.running_table_list_size();i++)
        {
            const PBDelegateTableInfo & info = user_table_info.running_table_list(i);
            int64 tid = info.tid();
            if (tid == cs_request_dissolve_delegate_table.tid())
            {
                found = true;
                delegate_table_info.CopyFrom(info);
            }
        }
        if(found)
        {
            //代开房已被回收
            {
                PBUpdateData update;
                update.set_key(PBUserDataField::kUserTableInfo);
                PBDBAtomicField & field = *update.add_field_list();
                field.set_field(EN_DB_Field_Table_Running);
                field.set_strategy(EN_Update_Strategy_Del);
                PBDelegateTableInfo & filed_delegate_table_info = *field.mutable_tableinfo();
                filed_delegate_table_info.CopyFrom(delegate_table_info);
                psession->NewAddUpdateData(uid, update);
            }
            {
                PBUpdateData update;
                update.set_key(PBUserDataField::kUserTableInfo);
                PBDBAtomicField & field = *update.add_field_list();
                field.set_field(EN_DB_Field_Table_Closed);
                PBDelegateTableInfo & filed_delegate_table_info = *field.mutable_tableinfo();
                filed_delegate_table_info.CopyFrom(delegate_table_info);
                filed_delegate_table_info.set_state(EN_Delegate_Table_State_TimeOut);
                psession->NewAddUpdateData(uid, update);
            }
            return EN_Handler_Save;
        }
    }
	 
    Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(),psession,psession->_response_msg);
    return EN_Handler_Done;
}

ENHandlerResult CRequestSDRCreateTable::ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession)
{
    //room服处于退休状态
    if(ZoneTableManager::Instance()->_state == EN_Game_Service_State_Retired)
    {
        PBCSMsg msg;
        CSResponseSdrCreateTable & response = *msg.mutable_cs_response_sdr_create_table();
        response.set_result(EN_MESSAGE_IN_MAINTENANCE);
        Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
        return EN_Handler_Done;
    }
    if (!SessionManager::Instance()->LockProcess(psession))
    {
        PBCSMsg msg;
        CSResponseSdrCreateTable & response = *msg.mutable_cs_response_sdr_create_table();
        response.set_result(EN_MESSAGE_ERROR_REQUEST_PROCESSING);
        Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
        return EN_Handler_Done;
    }

    const CSRequestSdrCreateTable & cs_request_create_table = psession->_request_msg.cs_request_sdr_create_table();
    if (cs_request_create_table.has_conf() && cs_request_create_table.conf().has_tbid() && cs_request_create_table.conf().has_master_uid())//创建茶馆房间
    {
        psession->NewAddGetData(cs_request_create_table.conf().master_uid(), PBUserDataField::kUserInfo);
        psession->NewAddGetData(cs_request_create_table.conf().master_uid(), PBUserDataField::kUserTeaBarData);
        psession->NewAddGetData(psession->_request_route.uid(), PBUserDataField::kUserTeaBarData);
    }
    psession->NewAddGetData(psession->_request_route.uid(), PBUserDataField::kUserInfo);
    return EN_Handler_Get;
}

ENHandlerResult CRequestSDRCreateTable::ProcessGetSucc(CHandlerTokenBasic * ptoken,CSession * psession)
{
    long long uid = psession->_request_route.uid();
    // 封号逻辑
    {
        const PBUser & user = psession->_kvdb_uid_data_map[uid].user_info();
        if (user.limit() != 0)
        {
            PBCSMsg msg;
            CSResponseSdrCreateTable & response = *msg.mutable_cs_response_sdr_create_table();
            response.set_result(EN_MESSAGE_PERMISSION_DENY);
            Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
            return EN_Handler_Done;
        }
    }
    PBCSMsg msg;
    CSResponseSdrCreateTable & cs_response_create_table = *msg.mutable_cs_response_sdr_create_table();
    CSRequestSdrCreateTable & cs_request_create_table = * psession->_request_msg.mutable_cs_request_sdr_create_table();

    ////代开房 停用 0522
    //if (cs_request_create_table.conf().is_master_delegate())
    //{
    //    cs_response_create_table.set_result(EN_MESSAGE_SYSTEM_FAILED);
    //    Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
    //    return EN_Handler_Done;
    //}

	//如果请求进入试玩场
	if(cs_request_create_table.freegame() == true){
		const PBFreeGameConfItem * pconfItem = PokerPBFreeGameConfig::Instance()->GetConfItem(cs_request_create_table.ttype());
		if(pconfItem == NULL){
			//不支持free game
			cs_response_create_table.set_result(EN_MESSAGE_SYSTEM_FAILED);
	        Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
	        return EN_Handler_Done;
		}
		cs_request_create_table.mutable_conf()->CopyFrom(pconfItem->conf());
		VLogMsg(CLIB_LOG_LEV_ERROR,"Player[%lld] request create or enter free game[%d]",uid,cs_request_create_table.ttype());
	}
	// 1 检查配置 当前table type 是否支持试玩
	// 1 检查当前配置 试玩场是否有空的座位

	

    //创建茶馆房间
    bool is_creat_tea_bar_table = false;
    if (cs_request_create_table.conf().has_tbid() && cs_request_create_table.conf().has_master_uid())
    {
        is_creat_tea_bar_table = true;
    }

    const TeaBarBriefData* pownerbrief = NULL;
    if (is_creat_tea_bar_table)
    {
        bool is_in_tea_bar = false;
        const PBUserTeaBarData & tbdata = psession->_kvdb_uid_data_map[uid].user_tea_bar_data();
        for (int i = 0; i < tbdata.brief_data_size(); i++)
        {
            const TeaBarBriefData& brief = tbdata.brief_data(i);
            if (brief.tbid() == cs_request_create_table.conf().tbid())
            {
                is_in_tea_bar = true;
                break;
            }
        }

        if (!is_in_tea_bar)
        {
            cs_response_create_table.set_result(EN_MESSAGE_NOT_IN_TEA_BAR);
            Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
            return EN_Handler_Done;
        }

        bool is_can_creat_table = true;
        const PBUserTeaBarData & tbownerdata = psession->_kvdb_uid_data_map[cs_request_create_table.conf().master_uid()].user_tea_bar_data();
        for (int i = 0; i < tbownerdata.brief_data_size(); i++)
        {
            const TeaBarBriefData& brief = tbownerdata.brief_data(i);
            if (brief.tbid() == cs_request_create_table.conf().tbid())
            {
                pownerbrief = &brief;
                if (brief.wait_table_num() >= 50)
                {
                    is_can_creat_table = false;
                }
                break;
            }
        }

        if (pownerbrief == NULL)
        {
            cs_response_create_table.set_result(EN_MESSAGE_OWNER_AND_TEA_BAR_NOT_MATCH);
            Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
            return EN_Handler_Done;
        }

        if (!is_can_creat_table)
        {
            cs_response_create_table.set_result(EN_MESSAGE_TEA_BAR_CAN_NOT_CREATE_WAIT_TABLE);
            Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
            return EN_Handler_Done;
        }
    }
    //创建茶馆房间--end
    int zone_type = cs_request_create_table.ztype();
    PBSDRTableConfig conf;
    if (cs_request_create_table.has_conf())
    {
        conf.CopyFrom(cs_request_create_table.conf());
    }
    conf.set_creator_uid(uid);
    conf.set_ttype(cs_request_create_table.ttype());
    conf.set_count_way(cs_request_create_table.conf().count_way());

    /*
     * 必须有座位数目
    */
    if(!conf.has_seat_num() || conf.seat_num() <= 0)
    {
        cs_response_create_table.set_result(EN_MESSAGE_INVALID_TABLE_CONF);
        Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
        return EN_Handler_Done;
    }

    // 兼容处理 默认8局
    if (conf.round() == 0)
    {
        conf.set_round(8);
    }

    bool is_free = false;
    for (int i = 0; i <  PokerPBCostConfig::Instance()->free_games_size(); i++)
    {
        int free_game = PokerPBCostConfig::Instance()->free_games(i);
        if (conf.ttype() == free_game)
        {
            is_free = true;
            break;
        }
    }
    for (int i = 0; i <  PokerPBCostConfig::Instance()->free_items_size(); i++)
    {
        const PBFreeItem & item = PokerPBCostConfig::Instance()->free_items(i);
        int free_game = item.table_type();
        int seat_num = item.seat_num();

        if (conf.ttype() == free_game && conf.seat_num() == seat_num)
        {
            is_free = true;
            break;
        }
    }
    // if (is_free || cs_request_create_table.freegame() == true)
    if (is_free)
    {
        conf.set_cost(0);
    }
    else
    {
        bool has_find_cost = false;
        //是否使用新的房卡配置
        if(PokerPBCostConfig::Instance()->cost_item_list_size() > 0)
        {
            for(int s = 0; s < PokerPBCostConfig::Instance()->cost_item_list_size(); s++)
            {
                if(PokerPBCostConfig::Instance()->cost_item_list(s).game_type() == conf.ttype())
                {
                    has_find_cost = true;
                    int per_n_cost = PokerPBCostConfig::Instance()->cost_item_list(s).cost() * conf.seat_num();
                    int cost_before_discount = per_n_cost * conf.round()/PokerPBCostConfig::Instance()->cost_item_list(s).round();
                    conf.set_cost(cost_before_discount * CostManager::Instance()->GetDiscountVal(conf.ttype(), conf.round()) / 100);
                }
            }
        }
        //使用老的房卡配置
        if(!has_find_cost)
        {
            int per_8_cost = PokerPBCostConfig::Instance()->cost_lzdaer() / 3 * conf.seat_num();
            int cost_before_discount = (conf.round() + 3) / 4 * per_8_cost;
            conf.set_cost(cost_before_discount * CostManager::Instance()->GetDiscountVal(conf.ttype(), conf.round()) / 100);
        }

        //特殊牌配置(覆盖前面的配置)
        for(int i = 0 ; i < PokerPBCostConfig::Instance()->special_cost_item_list_size() ; i ++)
        {
            const PBCostItem & pbCostItem = PokerPBCostConfig::Instance()->special_cost_item_list(i);
            if(pbCostItem.game_type() == conf.ttype() && pbCostItem.round() == conf.round() && pbCostItem.seat_num() == conf.seat_num())
            {
                //每人的房卡数
                int iCostEveryUser = pbCostItem.cost();
                conf.set_cost(iCostEveryUser);
            }
        }
    }

    /*
     * 扣卡必须为正
    */
    if(conf.cost() < 0)
    {
        cs_response_create_table.set_result(EN_MESSAGE_INVALID_TABLE_CONF);
        Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
        return EN_Handler_Done;
    }
	
    const PBUser & user = psession->_kvdb_uid_data_map[uid].user_info();
    const PBBPlayerPositionInfo & pos = user.pos();
    if(pos.pos_type() != EN_Position_Hall && !cs_request_create_table.conf().is_master_delegate())
    {
        // 若是申请代开房，则不用考虑是否玩家是否不在大厅的问题
        //已经在房间内 拒绝创建
        cs_response_create_table.set_result(EN_MESSAGE_ALREADY_IN_TABLE);
        cs_response_create_table.set_ret_tid(pos.table_id());
        Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(),psession,msg);
        return EN_Handler_Done;
    }
    if(user.has_create_table_id() && user.create_table_id() != 0  && !cs_request_create_table.conf().is_master_delegate())
    {
        // 若是申请代开房，则不用考虑重复创建桌子的问题
        //已经创建过一个桌子 并且桌子没有失效
        cs_response_create_table.set_result(EN_MESSAGE_ALREADY_CREATE_TABLE);
        cs_response_create_table.set_ret_tid(user.create_table_id());
        Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(),psession,msg);
        return EN_Handler_Done;
    }
	// if(cs_request_create_table.freegame() == true)
	// {
	// 	bool is_create = false;
	// 	psession->_tbid = ZoneTableManager::Instance()->AllocNewPublicTableID(conf.ttype(), zone_type,is_create);
	// 	if(is_create){
	// 		cs_request_create_table.set_is_create_for_new(is_create);
	// 	}
	// }
	// else 
	{
		psession->_tbid = ZoneTableManager::Instance()->AllocNewTableID(conf.ttype(), zone_type);
        VLogMsg(CLIB_LOG_LEV_ERROR,"alloc new tableid[%lld] for player[%lld] . game[%d]",psession->_tbid,uid,conf.ttype());
	}
    
	if (psession->_tbid < 0)
	{
		cs_response_create_table.set_result(EN_MESSAGE_SYSTEM_FAILED);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}


    // 创建房间花费
    int cost = conf.cost();
    // 扣房卡
    if (is_creat_tea_bar_table)
    {
        conf.set_tb_pay_type(pownerbrief->pay_type());
        if (pownerbrief->pay_type() == EN_TeaBar_Pay_Type_Master)
        {
            if (pownerbrief->chips() < cost)
            {
                cs_response_create_table.set_result(EN_MESSAGE_TEA_BAR_NO_ENOUGH_CHIP);
                Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
                return EN_Handler_Done;
            }
            {
                TeaBarChipsFlow flow;
                flow.set_tbid(cs_request_create_table.conf().tbid());
                flow.set_chips(0 - cost);
                flow.set_reason(EN_Reason_Create_Tea_Bar_Table);
                PBUpdateData update;
                update.set_key(PBUserDataField::kUserTeaBarData);
                PBDBAtomicField & field = *update.add_field_list();
                field.set_field(EN_DB_Field_TeaBar_Chips);
                field.mutable_tea_bar_chips_flow()->CopyFrom(flow);
                psession->NewAddUpdateData(cs_request_create_table.conf().master_uid(), update);
            }
        }

        if (conf.is_master_delegate() == false)
        {
            {
                PBUpdateData update;
                update.set_key(PBUserDataField::kUserInfo);
                PBDBAtomicField & field = *update.add_field_list();
                field.set_field(EN_DB_Field_Create_Table);
                field.set_intval(psession->_tbid);
                psession->NewAddUpdateData(uid, update);
            }

            {
                PBUpdateData update;
                update.set_key(PBUserDataField::kUserInfo);
                PBDBAtomicField& field = *update.add_field_list();
                field.set_field(EN_DB_Field_Sdr_Conf);
                conf.set_tb_pay_type(pownerbrief->pay_type());
                field.mutable_sdr_conf()->CopyFrom(conf);
                psession->NewAddUpdateData(uid, update);
            }
        }
        else
        {
            //如果是茶馆主人代开房间 无需记录房间信息在本人身上 均摊模式下直接创建房间
            if (pownerbrief->pay_type() == EN_TeaBar_Pay_Type_AA)
            {
                ZoneTableManager::Instance()->BindTidForUser(uid, psession->_tbid, zone_type);

                int node_type = ZoneTableManager::Instance()->GetNodeTypeForTableId(psession->_tbid, zone_type);
                if (node_type == -1)
                {
                    ErrMsg("failed to get node type, tid:[%lld]", psession->_tbid);
                    cs_response_create_table.set_result(EN_MESSAGE_TABLE_NOT_EXIST);
                    Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
                    return EN_Handler_Done;
                }

                int gamesvid = ZoneTableManager::Instance()->GetGamesvrdIDForTable(psession->_tbid, zone_type);
                if (gamesvid < 0)
                {
                    if (!GameSvrdManager::Instance()->GetGameIDByRandom(node_type, gamesvid))
                    {
                        ErrMsg("failed to get gamesvrd id, tid:[%lld], node type:[%d]", psession->_tbid, node_type);
                        cs_response_create_table.set_result(EN_MESSAGE_SYSTEM_FAILED);
                        Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
                        return EN_Handler_Done;
                    }
                    ZoneTableManager::Instance()->RefreshTable(psession->_tbid, zone_type);
                    ZoneTableManager::Instance()->SetGamesvrdIDForTable(psession->_tbid, gamesvid, zone_type);
                }

                PBCSMsg notify;
                SSNotifyCreateTable & ss_notify_create_table = *notify.mutable_ss_notify_create_table();
                ss_notify_create_table.set_uid(uid);
                ss_notify_create_table.set_tid(psession->_tbid);
                ss_notify_create_table.mutable_conf()->CopyFrom(conf);
                Message::PushInnerMsg(RouteManager::Instance()->GetRouteByRandom(), uid, notify, (ENNodeType)node_type, gamesvid);

                cs_response_create_table.set_result(EN_MESSAGE_ERROR_OK);
                cs_response_create_table.set_tid(psession->_tbid);
                cs_response_create_table.set_ttype(cs_request_create_table.ttype());
                cs_response_create_table.mutable_conf()->CopyFrom(conf);
                Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
                return EN_Handler_Done;
            }
        }
        return EN_Handler_Save;
    }
    // 均摊房卡
    if(conf.pay_type() == 1 && user.chips() < cost/conf.seat_num())
    {
        cs_response_create_table.set_result(EN_MESSAGE_NO_ENOUGH_CHIP);
        Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(),psession,msg);
        return EN_Handler_Done;
    }
    // 房主出房卡
    if(conf.pay_type() == 0 && user.chips() < cost)
    {
        cs_response_create_table.set_result(EN_MESSAGE_NO_ENOUGH_CHIP);
        Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(),psession,msg);
        return EN_Handler_Done;
    }

    {
        if(cs_request_create_table.freegame() == false)
        {
            PBUpdateData update;
            update.set_key(PBUserDataField::kUserInfo);
            PBDBAtomicField& field = *update.add_field_list();
            field.set_field(EN_DB_Field_Create_Table);
            field.set_intval(psession->_tbid);
            psession->NewAddUpdateData(uid, update);
        }
        {
            PBUpdateData update;
            update.set_key(PBUserDataField::kUserInfo);
            PBDBAtomicField& field = *update.add_field_list();
            field.set_field(EN_DB_Field_Sdr_Conf);
            field.mutable_sdr_conf()->CopyFrom(conf);
            psession->NewAddUpdateData(uid, update);
        }
        return EN_Handler_Save;
    }
}

ENHandlerResult CRequestSDRCreateTable::ProcessUpdateSucc(CHandlerTokenBasic* ptoken, CSession* psession)
{
    const CSRequestSdrCreateTable & cs_request_create_table = psession->_request_msg.cs_request_sdr_create_table();
    int zone_type = cs_request_create_table.ztype();
    long long uid = psession->_request_route.uid();
//    const PBUser & user = psession->_kvdb_uid_data_map[uid].user_info();
    long long create_tid = psession->_tbid;
    PBCSMsg msg;
    CSResponseSdrCreateTable & cs_response_create_table = *msg.mutable_cs_response_sdr_create_table();

    bool need_reset_gameid = true;
    if(cs_request_create_table.freegame() && cs_request_create_table.is_create_for_new() == false){
        need_reset_gameid = false;
    }
    ZoneTableManager::Instance()->BindTidForUser(uid, create_tid, zone_type,need_reset_gameid);

    //如果是茶馆主人代开房间，通知游戏服务器创建房间
    bool is_create_tea_bar_table = false;
    if (cs_request_create_table.conf().has_tbid() && cs_request_create_table.conf().has_master_uid())
    {
        is_create_tea_bar_table = true;
    }

	if (is_create_tea_bar_table /*&& cs_request_create_table.conf().is_master_delegate()*/)
	{
		const TeaBarBriefData* pownerbrief = NULL;
		const PBUserTeaBarData & tbownerdata = psession->_kvdb_uid_data_map[cs_request_create_table.conf().master_uid()].user_tea_bar_data();
		for (int i = 0; i < tbownerdata.brief_data_size(); i++)
		{
			const TeaBarBriefData& brief = tbownerdata.brief_data(i);
			if (brief.tbid() == cs_request_create_table.conf().tbid())
			{
				pownerbrief = &brief;
				break;
			}
		}

		PBSDRTableConfig conf;
		if (cs_request_create_table.has_conf())
		{
			conf.CopyFrom(cs_request_create_table.conf());
		}
		conf.set_creator_uid(uid);
		conf.set_ttype(cs_request_create_table.ttype());
        conf.set_count_way(cs_request_create_table.conf().count_way());

		// 兼容处理 默认8局
		if (conf.round() == 0)
		{
			conf.set_round(8);
		}

		bool is_free = false;
		for (int i = 0; i < PokerPBCostConfig::Instance()->free_games_size(); i++)
		{
			int free_game = PokerPBCostConfig::Instance()->free_games(i);
			if (conf.ttype() == free_game)
			{
				is_free = true;
				break;
			}
		}
		for (int i = 0; i < PokerPBCostConfig::Instance()->free_items_size(); i++)
		{
			const PBFreeItem& item = PokerPBCostConfig::Instance()->free_items(i);
			int free_game = item.table_type();
			int seat_num = item.seat_num();

			if (conf.ttype() == free_game && conf.seat_num() == seat_num)
			{
				is_free = true;
				break;
			}
		}
		if (is_free)
		{
			conf.set_cost(0);
		}
		else
		{
			bool has_find_cost = false;
			//是否使用新的房卡配置
			if (PokerPBCostConfig::Instance()->cost_item_list_size() > 0)
			{
				for (int s = 0; s < PokerPBCostConfig::Instance()->cost_item_list_size(); s++)
				{
					if (PokerPBCostConfig::Instance()->cost_item_list(s).game_type() == conf.ttype())
					{
						has_find_cost = true;
						int per_n_cost = PokerPBCostConfig::Instance()->cost_item_list(s).cost() * conf.seat_num();
						int cost_before_discount = per_n_cost * (conf.round() / PokerPBCostConfig::Instance()->cost_item_list(s).round());
						conf.set_cost(cost_before_discount * CostManager::Instance()->GetDiscountVal(conf.ttype(), conf.round()) / 100);
					}
				}
			}
			//使用老的房卡配置
			if (!has_find_cost)
			{
				int per_8_cost = PokerPBCostConfig::Instance()->cost_lzdaer() / 3 * conf.seat_num();
				int cost_before_discount = (conf.round() + 3) / 4 * per_8_cost;
				conf.set_cost(cost_before_discount * CostManager::Instance()->GetDiscountVal(conf.ttype(), conf.round()) / 100);
			}

            //特殊牌配置(覆盖前面的配置)
            for(int i = 0 ; i < PokerPBCostConfig::Instance()->special_cost_item_list_size() ; i ++)
            {
                const PBCostItem & pbCostItem = PokerPBCostConfig::Instance()->special_cost_item_list(i);
                if(pbCostItem.game_type() == conf.ttype() && pbCostItem.round() == conf.round() && pbCostItem.seat_num() == conf.seat_num())
                {
                    //每人的房卡数
                    int iCostEveryUser = pbCostItem.cost();
                    conf.set_cost(iCostEveryUser);
                }
            }
		}

		conf.set_tb_pay_type(EN_TeaBar_Pay_Type_Master);
		if (pownerbrief != NULL)
		{
			conf.set_tb_pay_type(pownerbrief->pay_type());
		}

		cs_response_create_table.mutable_conf()->CopyFrom(conf);


        int node_type = ZoneTableManager::Instance()->GetNodeTypeForTableId(create_tid, zone_type);
        if (node_type == -1)
        {
            ErrMsg("failed to get node type, tid:[%lld]", create_tid);
            cs_response_create_table.set_result(EN_MESSAGE_TABLE_NOT_EXIST);
            Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
            return EN_Handler_Done;
        }

        int gamesvid = ZoneTableManager::Instance()->GetGamesvrdIDForTable(create_tid, zone_type);
        if (gamesvid < 0)
        {
            if (!GameSvrdManager::Instance()->GetGameIDByRandom(node_type, gamesvid))
            {
                ErrMsg("failed to get gamesvrd id, tid:[%lld], node type:[%d]", create_tid, node_type);
                cs_response_create_table.set_result(EN_MESSAGE_SYSTEM_FAILED);
                Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
                return EN_Handler_Done;
            }
            ZoneTableManager::Instance()->RefreshTable(create_tid, zone_type);
            ZoneTableManager::Instance()->SetGamesvrdIDForTable(create_tid, gamesvid, zone_type);
        }

        PBCSMsg notify;
        SSNotifyCreateTable & ss_notify_create_table = *notify.mutable_ss_notify_create_table();
        ss_notify_create_table.set_uid(uid);
        ss_notify_create_table.set_tid(create_tid);
        ss_notify_create_table.mutable_conf()->CopyFrom(conf);
        Message::PushInnerMsg(RouteManager::Instance()->GetRouteByRandom(), uid, notify, (ENNodeType)node_type, gamesvid);
    }

    // 写入牌桌信息日志
    {
        PBCSMsg msg_ct;
        LogCreateTable & log = *msg_ct.mutable_log_create_table();
        log.set_creator_uid(uid);
        if (cs_request_create_table.conf().has_master_uid())
            log.set_master_uid(cs_request_create_table.conf().master_uid());
        else
            log.set_master_uid(0);
        log.set_game_type(cs_request_create_table.conf().ttype());
        log.set_table_id(create_tid);
        CLogWriter::Instance()->Send(msg_ct);
    }

	cs_response_create_table.set_result(EN_MESSAGE_ERROR_OK);
	cs_response_create_table.set_tid(create_tid);
	cs_response_create_table.set_ttype(cs_request_create_table.ttype());
	Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);

    return EN_Handler_Done;
}

ENHandlerResult CRequestSDREnterTable::ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession)
{
//    版本管理，需要的时候将注释打开
//    if(psession->_head.proto_version() <3)
//    {
//        PBCSMsg msg;
//        CSResponseSdrEnterTable & response = *msg.mutable_cs_response_sdr_enter_table();
//        response.set_result(EN_MESSAGE_INVALID_PROTOCOL_VERSION);
//        Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
//        return EN_Handler_Done;
//    }

    if (!SessionManager::Instance()->LockProcess(psession))
    {
        PBCSMsg msg;
        CSResponseSdrEnterTable & response = *msg.mutable_cs_response_sdr_enter_table();
        response.set_result(EN_MESSAGE_ERROR_REQUEST_PROCESSING);
        Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
        return EN_Handler_Done;
    }
    psession->NewAddGetData(psession->_request_route.uid(), PBUserDataField::kUserInfo);
    return EN_Handler_Get;
}

ENHandlerResult CRequestSDREnterTable::ProcessGetSucc(CHandlerTokenBasic * ptoken,CSession * psession)
{
    long long uid = psession->_request_route.uid();
    // 封号逻辑
    {
        const PBUser & user = psession->_kvdb_uid_data_map[uid].user_info();
        if (user.limit() != 0)
        {
            PBCSMsg msg;
            CSResponseSdrEnterTable & response = *msg.mutable_cs_response_sdr_enter_table();
            response.set_result(EN_MESSAGE_PERMISSION_DENY);
            Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
            return EN_Handler_Done;
        }
    }

    psession->_uid = uid;
    const PBUser & user = psession->_kvdb_uid_data_map[uid].user_info();
    if(user.has_sdr_conf() && user.sdr_conf().is_free_game() == true){
        {
            PBUpdateData update;
            update.set_key(PBUserDataField::kUserInfo);
            PBDBAtomicField& field = *update.add_field_list();
            field.set_field(EN_DB_Field_POS);
            PBBPlayerPositionInfo& pos = *field.mutable_pos();
            pos.set_pos_type(EN_Position_Hall);
            pos.set_table_id(0);
            pos.set_gamesvrd_id(0);
            psession->NewAddUpdateData(psession->_request_route.uid(), update);
        }
        {
            PBUpdateData update;
            update.set_key(PBUserDataField::kUserInfo);
            PBDBAtomicField& field = *update.add_field_list();
            field.set_field(EN_DB_Field_Sdr_Conf);
            PBSDRTableConfig conf;
            {
                conf.Clear();
                conf.set_is_free_game(false);
            }
            field.mutable_sdr_conf()->CopyFrom(conf);
            psession->NewAddUpdateData(psession->_request_route.uid(), update);
        }
        return EN_Handler_Save;
    }
    
    const PBBPlayerPositionInfo & pos = user.pos();

    CSRequestSdrEnterTable& cs_request_enter_table = *psession->_request_msg.mutable_cs_request_sdr_enter_table();
    int64 tid = cs_request_enter_table.tid();
    int zone_type = cs_request_enter_table.ztype();
//   int node_type = ZoneTableManager::Instance()->GetNodeTypeForTableId(tid, zone_type);
//    if (node_type == -1)
//    {
//        ErrMsg("failed to get node type for tid[%lld]", tid);
////        if(user.create_table_id() != 0 || pos.pos_type() != EN_Position_Hall)
////        {
////            // 解锁玩家 ...
////            PBCSMsg msg;
////            msg.mutable_inter_event_unlock()->set_uid(uid);
////            RoomHandlerProxy::Instance()->PushInnerMsg(msg);
////        }
//        PBCSMsg msg;
//        CSResponseSdrEnterTable & response = *msg.mutable_cs_response_sdr_enter_table();
//        response.set_result(EN_MESSAGE_TABLE_NOT_EXIST);
//        Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
//        return EN_Handler_Done;
//    }

    cs_request_enter_table.set_connect_id(psession->_request_route.source_id());

    if(pos.pos_type() != EN_Position_Hall && (cs_request_enter_table.postype() <= 199 || cs_request_enter_table.postype() > 300))
    {
        //玩家请求进入房间 但是玩家已经卡房间了
        int cur_node_type;
        /*ccc---------beg
         * 读取room——svrd配置
        */
        {
            for (int i = 0; i <  PokerPBRoomSvrdConfig::Instance()->games_size(); i++)
            {
                int pos_type =  PokerPBRoomSvrdConfig::Instance()->games(i).pos_type();
                int node_type = PokerPBRoomSvrdConfig::Instance()->games(i).node_type();
                if(pos.pos_type() == pos_type)
                {
                    cur_node_type = node_type;
                }
            }
        }
        //ccc---------end

        int gamesvid = pos.gamesvrd_id();
        if ( !GameSvrdManager::Instance()->IsGameExist(cur_node_type, gamesvid) ||
                        !ZoneTableManager::Instance()->CheckAllocedTable(pos.table_id(), zone_type))
        {
            //之前的牌桌的服务器已经不存在 直接重置玩家数据 如果是房主 返还用户房卡
            if(user.create_table_id() == pos.table_id())
            {
                {
                    PBUpdateData update;
                    update.set_key(PBUserDataField::kUserInfo);
                    PBDBAtomicField & field = *update.add_field_list();
                    field.set_field(EN_DB_Field_Create_Table);
                    field.set_intval(0);
                    psession->NewAddUpdateData(uid, update);
                }
            }
            {
                PBUpdateData update;
                update.set_key(PBUserDataField::kUserInfo);
                PBDBAtomicField& field = *update.add_field_list();
                field.set_field(EN_DB_Field_POS);
                PBBPlayerPositionInfo& pos = *field.mutable_pos();
                pos.set_pos_type(EN_Position_Hall);
                pos.set_table_id(0);
                pos.set_gamesvrd_id(0);
                psession->NewAddUpdateData(psession->_request_route.uid(), update);
            }
            return EN_Handler_Save;
        }
        else
        {
            //重连的服务器还存在 转发过去
            //转发过去后如果桌子不存在 也会重置玩家位置信息
            //如果是房主 则会重新创建桌子 但此时的牌局记录已经变化
            Message::SendRequestMsg(RouteManager::Instance()->GetRouteByRandom(),psession,psession->_request_msg,
                                    cur_node_type, gamesvid);
            return EN_Handler_Succ;
        }
    }

    //请求加入比赛场
    if(cs_request_enter_table.has_game_svid() && cs_request_enter_table.game_svid() > 0 && cs_request_enter_table.postype() >= 199 && cs_request_enter_table.postype() <= 300)
    {
        if(pos.pos_type() != EN_Position_Hall)
        {
            int gamesvid = pos.gamesvrd_id();
            ENNodeType cur_node_type = PokerPBMatchTypeMapConfig::Instance()->GetNodeTypeByPosType(ENPlayerPositionType(pos.pos_type()));

            if (!GameSvrdManager::Instance()->IsGameExist(cur_node_type, gamesvid))
            {
                {
                    PBUpdateData update;
                    update.set_key(PBUserDataField::kUserInfo);
                    PBDBAtomicField& field = *update.add_field_list();
                    field.set_field(EN_DB_Field_POS);
                    PBBPlayerPositionInfo& pos = *field.mutable_pos();
                    pos.set_pos_type(EN_Position_Hall);
                    pos.set_table_id(0);
                    pos.set_gamesvrd_id(0);
                    psession->NewAddUpdateData(psession->_request_route.uid(), update);
                }
                return EN_Handler_Save;
            }
            else
            {
                //重连的服务器还存在 转发过去
                cs_request_enter_table.set_tid(pos.table_id());
                Message::SendRequestMsg(RouteManager::Instance()->GetRouteByRandom(), psession, psession->_request_msg,
                    cur_node_type, gamesvid);
                return EN_Handler_Succ;
            }
        }

        ENNodeType cur_node_type = PokerPBMatchTypeMapConfig::Instance()->GetNodeTypeByPosType((ENPlayerPositionType)cs_request_enter_table.postype());
        if (cur_node_type == EN_Node_Unknown)
        {
            PBCSMsg msg;
            CSResponseSdrEnterTable & response = *msg.mutable_cs_response_sdr_enter_table();
            response.set_result(EN_MESSAGE_SYSTEM_FAILED);
            Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
            return EN_Handler_Done;
        }

        Message::SendRequestMsg(RouteManager::Instance()->GetRouteByRandom(), psession, psession->_request_msg,
            cur_node_type, cs_request_enter_table.game_svid());
        return EN_Handler_Succ;
    }

    /*
     * 闯关场
     * 有闯关svid
    */
    if(cs_request_enter_table.has_skipmatch_game_svid() && cs_request_enter_table.skipmatch_game_svid() > 0)
    {
        const PBBPlayerPositionInfo & pbCGPos = user.skipmatch_pos();

        //重连
        if(pbCGPos.pos_type() != EN_Position_Hall)
        {
            //寻找所在的node
            int iNodeType = PokerAutoMatchRoomConfig::Instance()->GetNodeTypeByPosType(pbCGPos.pos_type());
            if(iNodeType == -1)
            {
                PBCSMsg msg;
                CSResponseSdrEnterTable & response = *msg.mutable_cs_response_sdr_enter_table();
                response.set_result(EN_MESSAGE_SYSTEM_FAILED);
                //Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
                Message::PushInnerMsg(RouteManager::Instance()->GetRouteByRandom(), psession->_request_route.uid(),msg,EN_Node_Connect,psession->_request_route.source_id());
                VLogMsg(CLIB_LOG_LEV_DEBUG,"find not node");
                return EN_Handler_Done;
            }

            int iGameSvid = pbCGPos.gamesvrd_id();
            //如果该svid不存在
            if(!GameSvrdManager::Instance()->IsGameExist(iNodeType, iGameSvid))
            {
                {
                    PBUpdateData update;
                    update.set_key(PBUserDataField::kUserInfo);
                    PBDBAtomicField& field = *update.add_field_list();
                    field.set_field(EN_DB_Field_Skipmatch_POS);
                    PBBPlayerPositionInfo& pos = *field.mutable_pos();
                    pos.set_pos_type(EN_Position_Hall);
                    pos.set_table_id(0);
                    pos.set_gamesvrd_id(0);
                    psession->NewAddUpdateData(psession->_request_route.uid(), update);
                }
                return EN_Handler_Save;
            }
            //转发给游戏服
            else
            {
                cs_request_enter_table.set_tid(pbCGPos.table_id());
                Message::SendRequestMsg(RouteManager::Instance()->GetRouteByRandom(), psession, psession->_request_msg,
                                        iNodeType, iGameSvid);
                return EN_Handler_Done;
            }
        }

        //新进入房间
        int iNodeType = PokerAutoMatchRoomConfig::Instance()->GetNodeTypeByPosType(cs_request_enter_table.pos_type());
        if(iNodeType == -1)
        {
            PBCSMsg msg;
            CSResponseSdrEnterTable & response = *msg.mutable_cs_response_sdr_enter_table();
            response.set_result(EN_MESSAGE_SYSTEM_FAILED);
            //Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
            Message::PushInnerMsg(RouteManager::Instance()->GetRouteByRandom(), psession->_request_route.uid(),msg,EN_Node_Connect,psession->_request_route.source_id());
            VLogMsg(CLIB_LOG_LEV_DEBUG,"find not node");
            return EN_Handler_Done;
        }
        Message::SendRequestMsg(RouteManager::Instance()->GetRouteByRandom(), psession, psession->_request_msg,
                                iNodeType, cs_request_enter_table.skipmatch_game_svid());
        return EN_Handler_Done;
    }

	/*
	金币场加入比赛
	*/
	if (cs_request_enter_table.has_coinmatch_game_svid() && cs_request_enter_table.coinmatch_game_svid() > 0)
	{
		//因为桌子交给game来管理，所以需要传game_svid
		//重连
		const PBBPlayerPositionInfo & pbCoinPos = user.coin_pos();

		if (pbCoinPos.pos_type() != EN_Position_Hall)
		{
			int iNodeid = PokerAutoMatchRoomConfig::Instance()->GetNodeTypeByPosType(pbCoinPos.pos_type());
			if (iNodeid == -1)
			{
				PBCSMsg msg;
				CSResponseSdrEnterTable & response = *msg.mutable_cs_response_sdr_enter_table();
				response.set_result(EN_MESSAGE_SYSTEM_FAILED);
				//Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
				Message::PushInnerMsg(RouteManager::Instance()->GetRouteByRandom(), psession->_request_route.uid(), msg, EN_Node_Connect, psession->_request_route.source_id());
				VLogMsg(CLIB_LOG_LEV_DEBUG, "find not node");
				return EN_Handler_Done;
			}

			int iGameSvid = pbCoinPos.pos_type();
			//如果没有，需要清空金币场位置
			if (!GameSvrdManager::Instance()->IsGameExist(iNodeid, iGameSvid))
			{
				{
					PBUpdateData update;
					update.set_key(PBUserDataField::kUserInfo);
					PBDBAtomicField& field = *update.add_field_list();
					field.set_field(EN_DB_Field_Coin_POS);
					PBBPlayerPositionInfo& pos = *field.mutable_pos();
					pos.set_pos_type(EN_Position_Hall);
					pos.set_table_id(0);
					pos.set_gamesvrd_id(0);
					psession->NewAddUpdateData(psession->_request_route.uid(), update);
				}
				return EN_Handler_Save;
			}
		}
		//进入新的房间
		else
		{
			int iNodeid = PokerAutoMatchRoomConfig::Instance()->GetNodeTypeByPosType(cs_request_enter_table.pos_type());
			if (iNodeid == -1)
			{
				PBCSMsg msg;
				CSResponseSdrEnterTable & response = *msg.mutable_cs_response_sdr_enter_table();
				response.set_result(EN_MESSAGE_SYSTEM_FAILED);
				//Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
				Message::PushInnerMsg(RouteManager::Instance()->GetRouteByRandom(), psession->_request_route.uid(), msg, EN_Node_Connect, psession->_request_route.source_id());
				VLogMsg(CLIB_LOG_LEV_DEBUG, "find not node");
				return EN_Handler_Done;
			}

			Message::SendRequestMsg(RouteManager::Instance()->GetRouteByRandom(), psession, psession->_request_msg,
				iNodeid, cs_request_enter_table.coinmatch_game_svid());
			return EN_Handler_Done;
		}
	}

//    //请求加入比赛场
//    if(cs_request_enter_table.has_game_svid() && cs_request_enter_table.game_svid() > 0 && cs_request_enter_table.postype() >= 199 && cs_request_enter_table.postype() <= 300)
//    {
//        if(pos.pos_type() != EN_Position_Hall)
//        {
//            int gamesvid = pos.gamesvrd_id();
//            ENNodeType cur_node_type = PokerPBMatchTypeMapConfig::Instance()->GetNodeTypeByPosType(ENPlayerPositionType(pos.pos_type()));

//            if (!GameSvrdManager::Instance()->IsGameExist(cur_node_type, gamesvid))
//            {
//                {
//                    PBUpdateData update;
//                    update.set_key(PBUserDataField::kUserInfo);
//                    PBDBAtomicField& field = *update.add_field_list();
//                    field.set_field(EN_DB_Field_POS);
//                    PBBPlayerPositionInfo& pos = *field.mutable_pos();
//                    pos.set_pos_type(EN_Position_Hall);
//                    pos.set_table_id(0);
//                    pos.set_gamesvrd_id(0);
//                    psession->NewAddUpdateData(psession->_request_route.uid(), update);
//                }
//                return EN_Handler_Save;
//            }
//            else
//            {
//                //重连的服务器还存在 转发过去
//                cs_request_enter_table.set_tid(pos.table_id());
//                Message::SendRequestMsg(RouteManager::Instance()->GetRouteByRandom(), psession, psession->_request_msg,
//                    cur_node_type, gamesvid);
//                return EN_Handler_Succ;
//            }
//        }

//        ENNodeType cur_node_type = PokerPBMatchTypeMapConfig::Instance()->GetNodeTypeByPosType((ENPlayerPositionType)cs_request_enter_table.postype());
//        if (cur_node_type == EN_Node_Unknown)
//        {
//            PBCSMsg msg;
//            CSResponseSdrEnterTable & response = *msg.mutable_cs_response_sdr_enter_table();
//            response.set_result(EN_MESSAGE_SYSTEM_FAILED);
//            Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
//            return EN_Handler_Done;
//        }

//        Message::SendRequestMsg(RouteManager::Instance()->GetRouteByRandom(), psession, psession->_request_msg,
//            cur_node_type, cs_request_enter_table.game_svid());
//        return EN_Handler_Succ;
//    }

    /*
     * 该玩家为这个房间的房主
     * 如果该桌子没有房主关联uid
     * 或者该桌房主关联uid不是该玩家
     * --说明该玩家所创建房间已超时解散（现在未再次创建或者被别人创建）--
     */
    {
        int64 iTableBindUid = -1;
        if(user.create_table_id() == tid)
        {
            if(!ZoneTableManager::Instance()->GetUidByBindTid(tid,zone_type,iTableBindUid) ||
                    (iTableBindUid != -1 && iTableBindUid != uid))
            {
                {
                    PBUpdateData update;
                    update.set_key(PBUserDataField::kUserInfo);
                    PBDBAtomicField & field = *update.add_field_list();
                    field.set_field(EN_DB_Field_Create_Table);
                    field.set_intval(0);
                    psession->NewAddUpdateData(uid, update);
                }
                {
                    PBUpdateData update;
                    update.set_key(PBUserDataField::kUserInfo);
                    PBDBAtomicField& field = *update.add_field_list();
                    field.set_field(EN_DB_Field_POS);
                    PBBPlayerPositionInfo& pos = *field.mutable_pos();
                    pos.set_pos_type(EN_Position_Hall);
                    pos.set_table_id(0);
                    pos.set_gamesvrd_id(0);
                    psession->NewAddUpdateData(psession->_request_route.uid(), update);
                }
                return EN_Handler_Save;
            }
        }
    }

    int node_type = ZoneTableManager::Instance()->GetNodeTypeForTableId(tid, zone_type);
    if (node_type == -1)
    {
        ErrMsg("failed to get node type for tid[%lld]", tid);
        PBCSMsg msg;
        CSResponseSdrEnterTable & response = *msg.mutable_cs_response_sdr_enter_table();
        response.set_result(EN_MESSAGE_TABLE_NOT_EXIST);
        Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
        return EN_Handler_Done;
    }

    //未卡房间
    //先看下是否有分配桌子的GameSvid
    int gamesvid = ZoneTableManager::Instance()->GetGamesvrdIDForTable(tid, zone_type);
    if(gamesvid<0)
    {
        if (!GameSvrdManager::Instance()->GetGameIDByRandom(node_type, gamesvid))
        {
            ErrMsg("failed to get gamesvrd id");
            PBCSMsg msg;
            CSResponseSdrEnterTable & response = *msg.mutable_cs_response_sdr_enter_table();
            response.set_result(EN_MESSAGE_SYSTEM_FAILED);
            Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
            return EN_Handler_Done;
        }
    }
    Message::SendRequestMsg(RouteManager::Instance()->GetRouteByRandom(),psession,psession->_request_msg,
                            node_type,gamesvid);
    return EN_Handler_Succ;
}

ENHandlerResult CRequestSDREnterTable::ProcessResponseMsg(CHandlerTokenBasic * ptoken,CSession * psession)
{
    const CSRequestSdrEnterTable& cs_request_enter_table = psession->_request_msg.cs_request_sdr_enter_table();
    int zone_type = cs_request_enter_table.ztype();

    const CSResponseSdrEnterTable & cs_response_enter_table = psession->_response_msg.cs_response_sdr_enter_table();
    if(int(cs_response_enter_table.pos_type()) >= 199 && int(cs_response_enter_table.pos_type()) <= 300)
    {
        VLogMsg(CLIB_LOG_LEV_DEBUG, "matach enter table return done");
        return EN_Handler_Done;
    }

    if(cs_response_enter_table.result() == EN_MESSAGE_ERROR_OK)
    {
        const CSNotifyTableInfo & table_info = cs_response_enter_table.table_info();
        int64 tid = table_info.tid();
        ZoneTableManager::Instance()->RefreshTable(tid, zone_type);
        ZoneTableManager::Instance()->SetGamesvrdIDForTable(tid, psession->_response_route.source_id(), zone_type);
    }
    Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(),psession,psession->_response_msg);
    return EN_Handler_Done;
}

ENHandlerResult CRequestSDREnterTable::ProcessUpdateSucc(CHandlerTokenBasic * ptoken,CSession * psession)
{
    PBCSMsg msg;
    CSResponseSdrEnterTable & response = *msg.mutable_cs_response_sdr_enter_table();
    response.set_result(EN_MESSAGE_TABLE_NOT_EXIST);
    Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
    return EN_Handler_Done;
//    long long uid = psession->_request_route.uid();=
//    Message::PushInnerMsg(RouteManager::Instance()->GetRouteByRandom(), uid, msg, EN_Node_Connect, psession->_request_route.source_id());
//    return EN_Handler_Done;
}

ENHandlerResult CInnerUnlock::ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession)
{
    const InterEventUnlock& inter_msg = psession->_request_msg.inter_event_unlock();
    long long uid = inter_msg.uid();
    psession->NewAddGetData(uid, PBUserDataField::kUserInfo);
    return EN_Handler_Get;
}
ENHandlerResult CInnerUnlock::ProcessGetSucc(CHandlerTokenBasic * ptoken,CSession * psession)
{
    const InterEventUnlock& inter_msg = psession->_request_msg.inter_event_unlock();
    long long uid = inter_msg.uid();
    const PBUser& user = psession->_kvdb_uid_data_map[uid].user_info();
    const PBBPlayerPositionInfo& pos = user.pos();

    //之前的牌桌的服务器已经不存在 直接重置玩家数据
    if(user.create_table_id() == pos.table_id())
    {
        PBUpdateData update;
        update.set_key(PBUserDataField::kUserInfo);
        PBDBAtomicField & field = *update.add_field_list();
        field.set_field(EN_DB_Field_Create_Table);
        field.set_intval(0);
        psession->NewAddUpdateData(uid, update);
    }
    {
        PBUpdateData update;
        update.set_key(PBUserDataField::kUserInfo);
        PBDBAtomicField& field = *update.add_field_list();
        field.set_field(EN_DB_Field_POS);
        PBBPlayerPositionInfo& pos = *field.mutable_pos();
        pos.set_pos_type(EN_Position_Hall);
        pos.set_table_id(0);
        pos.set_gamesvrd_id(0);
        psession->NewAddUpdateData(psession->_request_route.uid(), update);
    }

    VLogMsg(CLIB_LOG_LEV_DEBUG, "unlock: uid[%lld] user.create_table_id[%ld] pos.table_id[%ld] ...",
            uid, user.create_table_id(), pos.table_id());
    return EN_Handler_Save;
}

ENHandlerResult CInnerUnlock::ProcessUpdateSucc(CHandlerTokenBasic * ptoken,CSession * psession)
{
    const InterEventUnlock& inter_msg = psession->_request_msg.inter_event_unlock();
    long long uid = inter_msg.uid();
    VLogMsg(CLIB_LOG_LEV_DEBUG, "unlock[%lld] success ...", uid);

    return EN_Handler_Done;
}


ENHandlerResult CRequestTableDetail::ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession)
{
	const CSRequestTableDetail& cs_request_table_detail = psession->_request_msg.cs_request_table_detail();
	PBCSMsg msg;
	CSResponseTableDetail & response = *msg.mutable_cs_response_table_detail();
	int node_type = ZoneTableManager::Instance()->GetNodeTypeForTableId(cs_request_table_detail.tid(), EN_Zone_Common);
	int gamesvid = ZoneTableManager::Instance()->GetGamesvrdIDForTable(cs_request_table_detail.tid(), EN_Zone_Common);
	if (gamesvid < 0)
	{
		//如果是茶馆桌子，上报茶馆桌子已经不存在
		if (cs_request_table_detail.has_tbid())
		{
			PBCSMsg notify;
			SSNotifyTeaBarTableNotExist& ss_notify_tea_bar_table_not_exist = *notify.mutable_ss_notify_tea_bar_table_not_exist();
			ss_notify_tea_bar_table_not_exist.set_tbid(cs_request_table_detail.tbid());
			ss_notify_tea_bar_table_not_exist.set_tid(cs_request_table_detail.tid());
			Message::PushInnerMsg(RouteManager::Instance()->GetRouteByRandom(), 0, notify, EN_Node_TeaBar, 1);
		}

		response.set_result(EN_MESSAGE_SYSTEM_FAILED);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}
	else
	{
		if (!GameSvrdManager::Instance()->IsGameExist(node_type, gamesvid))
		{
			ErrMsg("failed to get gamesvrd id,%d", gamesvid);
			//如果是茶馆桌子，上报茶馆桌子已经不存在
			if (cs_request_table_detail.has_tbid())
			{
				PBCSMsg notify;
				SSNotifyTeaBarTableNotExist& ss_notify_tea_bar_table_not_exist = *notify.mutable_ss_notify_tea_bar_table_not_exist();
				ss_notify_tea_bar_table_not_exist.set_tbid(cs_request_table_detail.tbid());
				ss_notify_tea_bar_table_not_exist.set_tid(cs_request_table_detail.tid());
				Message::PushInnerMsg(RouteManager::Instance()->GetRouteByRandom(), 0, notify, EN_Node_TeaBar, 1);
			}
			return EN_Handler_Done;
		}
	}
	psession->_uid = psession->_request_route.uid();
	Message::SendRequestMsg(RouteManager::Instance()->GetRouteByRandom(), psession, psession->_request_msg,
        node_type, gamesvid);
	return EN_Handler_Succ;
}

ENHandlerResult CRequestTableDetail::ProcessResponseMsg(CHandlerTokenBasic * ptoken, CSession * psession)
{
	Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, psession->_response_msg);
	return EN_Handler_Done;
}


ENHandlerResult CRequestDissolveTeaBarTable::ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession)
{
    const CSRequestDissolveTeaBarTable& cs_request_dissolve_tea_bar_table = psession->_request_msg.cs_request_dissolve_tea_bar_table();
    PBCSMsg msg;
    CSResponseDissolveTeaBarTable & response = *msg.mutable_cs_response_dissolve_tea_bar_table();
    int node_type = ZoneTableManager::Instance()->GetNodeTypeForTableId(cs_request_dissolve_tea_bar_table.tid(), EN_Zone_Common);
    int gamesvid = ZoneTableManager::Instance()->GetGamesvrdIDForTable(cs_request_dissolve_tea_bar_table.tid(), EN_Zone_Common);
    if (gamesvid < 0)
    {
        ErrMsg("gamesvid error, tid[%ld]", cs_request_dissolve_tea_bar_table.tid());
        response.set_result(EN_MESSAGE_TABLE_NOT_EXIST);
        Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
        return EN_Handler_Done;
    }
    else
    {
        if (!GameSvrdManager::Instance()->IsGameExist(node_type, gamesvid))
        {
            ErrMsg("failed to get gamesvrd id,%d", gamesvid);
            return EN_Handler_Done;
        }
    }
    psession->_uid = psession->_request_route.uid();
    Message::SendRequestMsg(RouteManager::Instance()->GetRouteByRandom(), psession, psession->_request_msg,
        node_type, gamesvid);
    return EN_Handler_Succ;
}

ENHandlerResult CRequestDissolveTeaBarTable::ProcessResponseMsg(CHandlerTokenBasic * ptoken, CSession * psession)
{
    Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, psession->_response_msg);
    return EN_Handler_Done;
}


ENProcessResult CReuqestTableLog::ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession)
{
    const SSRequestTableLog& request = psession->_request_msg.ss_request_table_log();

    int64 tid = request.tid();
    int zone_type = 10; // 写死
    int node_type = ZoneTableManager::Instance()->GetNodeTypeForTableId(tid, zone_type);
    if (node_type == -1)
    {
        ErrMsg("failed to get node type for tid[%lld]", tid);
        PBCSMsg msg;
        SSResponseTableLog & response = *msg.mutable_ss_response_table_log();
        response.set_table_data("Error: failed to get node type ...");
        Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
        return EN_Process_Result_Succ;
    }
    //先看下是否有分配桌子的GameSvid
    int gamesvid = ZoneTableManager::Instance()->GetGamesvrdIDForTable(tid, zone_type);
    if(gamesvid<0)
    {
        ErrMsg("failed to get gamesvrd id");
        PBCSMsg msg;
        SSResponseTableLog & response = *msg.mutable_ss_response_table_log();
        response.set_table_data("Error: failed to get gamesvrd id ...");
        Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
        return EN_Process_Result_Succ;
    }
    Message::SendRequestMsg(RouteManager::Instance()->GetRouteByRandom(),psession,psession->_request_msg,
                            node_type,gamesvid);

    return EN_Process_Result_Succ;
}

ENProcessResult CReuqestTableLog::ProcessResponseMsg(CHandlerTokenBasic * ptoken,CSession * psession)
{
    Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, psession->_response_msg);
    return EN_Process_Result_Succ;
}


ENProcessResult CReuqestDissolveTable::ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession)
{
    const SSRequestDissolveTable& request = psession->_request_msg.ss_request_dissolve_table();

    int64 tid = request.tid();
    int zone_type = 10; // 写死
    int node_type = ZoneTableManager::Instance()->GetNodeTypeForTableId(tid, zone_type);
    if (node_type == -1)
    {
        ErrMsg("failed to get node type for tid[%lld]", tid);
        PBCSMsg msg;
        SSResponseDissolveTable & response = *msg.mutable_ss_response_dissolve_table();
        response.set_result("Error: failed to get node type ...");
        Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
        return EN_Process_Result_Succ;
    }
    //先看下是否有分配桌子的GameSvid
    int gamesvid = ZoneTableManager::Instance()->GetGamesvrdIDForTable(tid, zone_type);
    if(gamesvid<0)
    {
        ErrMsg("failed to get gamesvrd id");
        PBCSMsg msg;
        SSResponseDissolveTable & response = *msg.mutable_ss_response_dissolve_table();
        response.set_result("Error: failed to get gamesvrd id ...");
        Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
        return EN_Process_Result_Succ;
    }
    Message::SendRequestMsg(RouteManager::Instance()->GetRouteByRandom(),psession,psession->_request_msg,
                            node_type,gamesvid);

    return EN_Process_Result_Succ;
}

ENProcessResult CReuqestDissolveTable::ProcessResponseMsg(CHandlerTokenBasic * ptoken,CSession * psession)
{
    Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, psession->_response_msg);
    return EN_Process_Result_Succ;
}

ENHandlerResult CRequestTableInfo::ProcessRequestMsg(CHandlerTokenBasic *ptoken, CSession *psession)
{
    const SSRequestTableInfo & ss_request_table_info = psession->_request_msg.ss_request_table_info();
    int iGameSvrdId = ss_request_table_info.i_game_svid_id();
    int iTableId = ss_request_table_info.i64_tid();

    int iPosNode = ZoneTableManager::Instance()->GetNodeTypeForTableId(iTableId,EN_Zone_Common);
    Message::SendRequestMsg(RouteManager::Instance()->GetRouteByRandom(),psession,psession->_request_msg,
                            iPosNode,iGameSvrdId);

    return EN_Handler_Succ;
}

ENHandlerResult CRequestTableInfo::ProcessResponseMsg(CHandlerTokenBasic *ptoken, CSession *psession)
{
    Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, psession->_response_msg);
    return EN_Handler_Done;
}
ENHandlerResult CNotifyClearTableOwnerTableInfo::ProcessRequestMsg(CHandlerTokenBasic *ptoken, CSession *psession)
{
    /*
     * 查询房主的信息，看房主是否现在是在桌子中。
     */
    const SSInnerNotifyClearTableOwnerTableInfo & ss_inner_notify_clear_table_owner_table_info = psession->_request_msg.ss_inner_notify_clear_table_owner_table_info();
    int64  i64Uid = ss_inner_notify_clear_table_owner_table_info.i64_uid();
    psession->NewAddGetData(i64Uid, PBUserDataField::kUserInfo);
    return EN_Handler_Get;
}

ENHandlerResult CNotifyClearTableOwnerTableInfo::ProcessGetSucc(CHandlerTokenBasic *ptoken, CSession *psession)
{
    /*
     * 如果该房主在房间中，则不清空信息，留到之后统一解散的时候解散（防止房主被解锁了，再重新进入房间）
     * 解散的流程会延后到后面对桌子的统一成员解散
     */
    const SSInnerNotifyClearTableOwnerTableInfo & ss_inner_notify_clear_table_owner_table_info = psession->_request_msg.ss_inner_notify_clear_table_owner_table_info();
    int64  i64Uid = ss_inner_notify_clear_table_owner_table_info.i64_uid();
    const PBUser & pbUser = psession->_kvdb_uid_data_map[i64Uid].user_info();
    const PBBPlayerPositionInfo & pbPos = pbUser.pos();

    if(pbPos.pos_type() != EN_Position_Hall && pbPos.table_id() != 0)
    {
        return EN_Handler_Done;
    }

    /*
     * 房主没有在房间中，说明他已经被解锁或者没有进入房间的情况，这时需要我们提他解锁
     */
    {
        PBUpdateData update;
        update.set_key(PBUserDataField::kUserInfo);
        PBDBAtomicField & field = *update.add_field_list();
        field.set_field(EN_DB_Field_Create_Table);
        field.set_intval(0);
        psession->NewAddUpdateData(i64Uid, update);
    }
    {
        PBUpdateData update;
        update.set_key(PBUserDataField::kUserInfo);
        PBDBAtomicField& field = *update.add_field_list();
        field.set_field(EN_DB_Field_POS);
        PBBPlayerPositionInfo& pos = *field.mutable_pos();
        pos.set_pos_type(EN_Position_Hall);
        pos.set_table_id(0);
        pos.set_gamesvrd_id(0);
        psession->NewAddUpdateData(i64Uid, update);
    }
    return EN_Handler_Save;
}

ENHandlerResult CNotifyClearTableOwnerTableInfo::ProcessUpdateSucc(CHandlerTokenBasic *ptoken, CSession *psession)
{
    return EN_Handler_Done;
}

ENHandlerResult CInnerNotifyLogoutTable::ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession)
{
	const SSInnerNotifyLogoutTable & ss_notify_logout_table = psession->_request_msg.ss_inner_notify_logout_table();
	ZoneTableManager::Instance()->OnPlayerLogoutTable(ss_notify_logout_table.tid(),ss_notify_logout_table.ttype(),ss_notify_logout_table.num());
	return EN_Handler_Done;
}

//比赛场
ENHandlerResult CRequestQuickMatch::ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession)
{
    psession->NewAddGetData(psession->_request_route.uid(), PBUserDataField::kUserInfo);
    return EN_Handler_Get;
}

ENHandlerResult CRequestQuickMatch::ProcessGetSucc(CHandlerTokenBasic * ptoken, CSession * psession)
{
    long long uid = psession->_request_route.uid();
    // 封号逻辑
    {
        const PBUser & user = psession->_kvdb_uid_data_map[uid].user_info();
        if (user.limit() != 0)
        {
            PBCSMsg msg;
            CSResponseQuickMatch & response = *msg.mutable_cs_response_quick_match();
            response.set_result(EN_MESSAGE_PERMISSION_DENY);
            Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
            return EN_Handler_Done;
        }
    }

    psession->_uid = uid;
    const PBUser & user = psession->_kvdb_uid_data_map[uid].user_info();
    const PBBPlayerPositionInfo & pos = user.pos();
    if (pos.pos_type() != EN_Position_Hall && (pos.pos_type() < 199 || pos.pos_type() > 300))
    {
        //已经在房卡房间内 拒绝创建
        PBCSMsg msg;
        CSResponseQuickMatch & response = *msg.mutable_cs_response_quick_match();
        response.set_result(EN_MESSAGE_ALREADY_IN_TABLE);
        response.set_tid(pos.table_id());
        Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
        return EN_Handler_Done;
    }

    // 已经创建了房间，但是请求进入其他房间
    if (user.create_table_id() != 0)
    {
        PBCSMsg msg;
        CSResponseQuickMatch & response = *msg.mutable_cs_response_quick_match();
        response.set_result(EN_MESSAGE_ALREADY_CREATE_TABLE);
        response.set_tid(user.create_table_id());
        Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
        return EN_Handler_Done;
    }

    //已经在比赛场内
    if(pos.pos_type() != EN_Position_Hall)
    {
        PBCSMsg msg;
        CSResponseQuickMatch & response = *msg.mutable_cs_response_quick_match();
        response.set_result(EN_MESSAGE_ALREADY_IN_TABLE);
        response.set_tid(pos.table_id());
        response.set_game_svid(pos.gamesvrd_id());
        response.set_postype(pos.pos_type());
        Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
        return EN_Handler_Done;
    }

    const CSRequestQuickMatch & cs_request_quick_match = psession->_request_msg.cs_request_quick_match();

    int match_session_id = cs_request_quick_match.match_session_id();
    ENTableType ttype = ENTableType(QuickMatchManager::Instance()->_GetQuickMatchTtype(match_session_id));

    //node type
    int node_type = ZoneTableManager::Instance()->GetNodeTypeByMatchTableType(ttype, EN_Zone_Common);
    if (node_type == -1)
    {
        PBCSMsg msg;
        CSResponseQuickMatch & response = *msg.mutable_cs_response_quick_match();
        response.set_result(EN_MESSAGE_SYSTEM_FAILED);
        Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
        return EN_Handler_Done;
    }
    //svrid
    int svid = GameSvrdManager::Instance()->GetGameID(node_type);
    if (svid < 0)
    {
        PBCSMsg msg;
        CSResponseQuickMatch & response = *msg.mutable_cs_response_quick_match();
        response.set_result(EN_MESSAGE_SYSTEM_FAILED);
        Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
        return EN_Handler_Done;
    }

    //检查是否参加过比赛
    int has_join = QuickMatchManager::Instance()->_GetUserMatchInfo(match_session_id,uid);
    if(has_join > 1)
    {
        PBCSMsg msg;
        CSResponseQuickMatch & response = *msg.mutable_cs_response_quick_match();
        response.set_result(EN_MESSAGE_HAS_JOIN_PRE);
        Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
        return EN_Handler_Done;
    }

    //检查是否有其他场次比赛进行中
    int is_matching = QuickMatchManager::Instance()->_GetUserMatchState(uid);
    if(is_matching > 0)
    {
        //针对异常情况 比如game挂了
        if(QuickMatchManager::Instance()->_CheckIsAfterMatchTimeForTtype(ttype,is_matching))
        {
            //玩家显示在比赛中 但是已经超出比赛时间 重置状态
            QuickMatchManager::Instance()->_WriteUserMatchState(uid,0);
            QuickMatchManager::Instance()->_WriteUserMatchInfo(is_matching,uid,int(EN_PLAYER_MATCH_TYPE_UNKONWN));
        }
        else
        {
            PBCSMsg msg;
            CSResponseQuickMatch & response = *msg.mutable_cs_response_quick_match();
            response.set_result(EN_MESSAGE_IS_MATCHING);
            Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
            return EN_Handler_Done;
        }
    }

    //检查当前是否是开赛时间 PBUserJoinMatchInfo
    if(!QuickMatchManager::Instance()->_CheckIsInMatchTimeForTtype(ttype,match_session_id))
    {
        PBCSMsg msg;
        CSResponseQuickMatch & response = *msg.mutable_cs_response_quick_match();
        response.set_result(EN_MESSAGE_OUT_OF_MATCH_TIME);
        Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
        return EN_Handler_Done;
    }
    //检查是否需要房卡
    int match_cost = -1;
    QuickMatchManager::Instance()->_GetQuickMatchCostByTtype(match_session_id,match_cost);
    if(match_cost < 0)
    {
        PBCSMsg msg;
        CSResponseQuickMatch & response = *msg.mutable_cs_response_quick_match();
        response.set_result(EN_MESSAGE_SYSTEM_FAILED);
        Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
        return EN_Handler_Done;
    }
    if(user.chips() < match_cost)
    {
        PBCSMsg msg;
        CSResponseQuickMatch & response = *msg.mutable_cs_response_quick_match();
        response.set_result(EN_MESSAGE_NO_ENOUGH_CHIP);
        Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
        return EN_Handler_Done;
    }
    //条件符合向 比赛服发送请求
    psession->_uid = psession->_request_route.uid();
    Message::SendRequestMsg(RouteManager::Instance()->GetRouteByRandom(), psession, psession->_request_msg,ENNodeType(node_type), svid);
    VLogMsg(CLIB_LOG_LEV_DEBUG,"success to send quick match node: %d svrid: %d",int(node_type),svid);

    return EN_Handler_Succ;
}

ENHandlerResult CRequestQuickMatch::ProcessResponseMsg(CHandlerTokenBasic * ptoken, CSession * psession)
{
    //能进去比赛场后刷新用户 已经进入过得信息
    if(psession->_response_msg.cs_response_quick_match().result() == EN_MESSAGE_ERROR_OK)
    {
        Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, psession->_response_msg);
        long long uid = psession->_request_route.uid();
        int match_session_id = psession->_response_msg.cs_response_quick_match().match_session_id();
        QuickMatchManager::Instance()->_WriteUserMatchInfo(match_session_id,uid, int(EN_PLAYER_MATCH_TYPE_HAS_BAOMING));
    }

    return EN_Handler_Done;
}

ENHandlerResult CRequestSkipMatchGame::ProcessRequestMsg(CHandlerTokenBasic *ptoken, CSession *psession)
{
    //查询用户信息
    int64 iUid = psession->_request_route.uid();
    psession->NewAddGetData(iUid,PBUserDataField::kUserInfo);
    return EN_Handler_Get;
}

ENHandlerResult CRequestSkipMatchGame::ProcessGetSucc(CHandlerTokenBasic *ptoken, CSession *psession)
{
    CSRequestSkipMatchGame & cs_request_skip_match_game = *psession->_request_msg.mutable_cs_request_skip_match_game();
    int iType = (ENTableType)cs_request_skip_match_game.ttype();
    int64 iUid = psession->_request_route.uid();
    //封号逻辑
    const PBUser & pbUser = psession->_kvdb_uid_data_map[iUid].user_info();
    if(pbUser.limit() != 0)
    {
        PBCSMsg msg;
        CSResponseSkipMatchGame & response = *msg.mutable_cs_response_skip_match_game();
        response.set_result(EN_MESSAGE_PERMISSION_DENY);
        Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
        return EN_Handler_Done;
    }

    cs_request_skip_match_game.set_connect_id(psession->_request_route.source_id());
    psession->_uid = iUid;

    //对普通房间的校验
    const PBBPlayerPositionInfo & pbPos = pbUser.pos();
    {
        if(pbPos.pos_type() != EN_Position_Hall)
        {
            //已经在房卡房间内 拒绝创建
            VLogMsg(CLIB_LOG_LEV_DEBUG, "User[%ld] is not in hall",psession->_request_route.uid());
            PBCSMsg msg;
            CSResponseSkipMatchGame & response = *msg.mutable_cs_response_skip_match_game();
            response.set_result(EN_MESSAGE_ALREADY_IN_TABLE);
            response.set_tid(pbPos.table_id());
            Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
            return EN_Handler_Done;
        }

        //已经创建了房间，但是请求进入其他房间
        if(pbUser.create_table_id() != 0)
        {
            VLogMsg(CLIB_LOG_LEV_DEBUG, "User[%ld] already create table",psession->_request_route.uid());
            PBCSMsg msg;
            CSResponseSkipMatchGame & response = *msg.mutable_cs_response_skip_match_game();
            response.set_result(EN_MESSAGE_ALREADY_CREATE_TABLE);
            response.set_tid(pbUser.create_table_id());
            Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
            return EN_Handler_Done;
        }
    }

    //对闯关房间的校验
    const PBBPlayerPositionInfo & pbCGPos = pbUser.skipmatch_pos();
    {
        if(pbCGPos.pos_type() != EN_Position_Hall)
        {
            //已经在闯关房间
            int iType = PokerAutoMatchRoomConfig::Instance()->GetTableTypeByPosType(pbCGPos.pos_type());
            if(iType == -1)
            {
                VLogMsg(CLIB_LOG_LEV_DEBUG, "Can Not Find User[%ld] Pos Game Type",psession->_request_route.uid());
                PBCSMsg msg;
                CSResponseSdrEnterTable & response = *msg.mutable_cs_response_sdr_enter_table();
                response.set_result(EN_MESSAGE_SYSTEM_FAILED);
                Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
                return EN_Handler_Done;
            }

            VLogMsg(CLIB_LOG_LEV_DEBUG, "User[%ld] already in table",psession->_request_route.uid());
            PBCSMsg msg;
            CSResponseSkipMatchGame & response = *msg.mutable_cs_response_skip_match_game();
            response.set_result(EN_MESSAGE_ALREADY_IN_TABLE);
            response.set_tid(pbCGPos.table_id());
            response.set_skipmatch_game_svid(pbCGPos.gamesvrd_id());
            response.set_pos_type(pbCGPos.pos_type());
            Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
            return EN_Handler_Done;
        }
    }


    //int iLevel = cs_request_skip_match_game.level();

    //找到nodeid
    int tNodeType = PokerAutoMatchRoomConfig::Instance()->GetNodeTypeByTableType(iType);
    if(tNodeType == -1)
    {
        VLogMsg(CLIB_LOG_LEV_DEBUG, "Can Not Find Game Type,User[%ld]",psession->_request_route.uid());
        PBCSMsg msg;
        CSResponseSkipMatchGame & response = *msg.mutable_cs_response_skip_match_game();
        response.set_result(EN_MESSAGE_SYSTEM_FAILED);
        Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
        return EN_Handler_Done;
    }

    //随机获得svidid
    int iSvid = 0;
    if(!GameSvrdManager::Instance()->GetGameIDByRandom(tNodeType,iSvid))
    {
        VLogMsg(CLIB_LOG_LEV_DEBUG, "Can Not Find Game Svid,User[%ld]",psession->_request_route.uid());
        PBCSMsg msg;
        CSResponseSkipMatchGame & response = *msg.mutable_cs_response_skip_match_game();
        response.set_result(EN_MESSAGE_SYSTEM_FAILED);
        Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
        return EN_Handler_Done;
    }

    if(iSvid == -1)
    {
        VLogMsg(CLIB_LOG_LEV_DEBUG, "Can Not Find Game Svid,User[%ld]",psession->_request_route.uid());
        PBCSMsg msg;
        CSResponseSkipMatchGame & response = *msg.mutable_cs_response_skip_match_game();
        response.set_result(EN_MESSAGE_SYSTEM_FAILED);
        Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
        return EN_Handler_Done;
    }

    //向游戏服发送消息
    Message::SendRequestMsg(RouteManager::Instance()->GetRouteByRandom(), psession, psession->_request_msg, tNodeType, iSvid);
    return EN_Handler_Succ;
}

ENHandlerResult CRequestSkipMatchGame::ProcessResponseMsg(CHandlerTokenBasic *ptoken, CSession *psession)
{
    Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, psession->_response_msg);
    return EN_Handler_Done;
}

/*
金币场匹配
*/
ENHandlerResult CRequestCoinMatchGame::ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession)
{
	int iUid = psession->_request_route.uid();
	psession->NewAddGetData(iUid, PBUserDataField::kUserInfo);
	return EN_Handler_Get;
}

/*
金币场匹配
*/
ENHandlerResult CRequestCoinMatchGame::ProcessGetSucc(CHandlerTokenBasic *ptoken, CSession *psession)
{
	int iUid = psession->_request_route.uid();
	CSRequestCoinMatchGame & pbRequest = *psession->_request_msg.mutable_cs_request_coin_match_game();
	const PBUser & pbUser = psession->_kvdb_uid_data_map[iUid].user_info();
	//判定位置
	const PBBPlayerPositionInfo & pbPos = pbUser.pos();
	const PBBPlayerPositionInfo & pbCoinPos = pbUser.coin_pos();

	psession->_uid = iUid;
	//对普通房间的校验
	{
		if (pbPos.pos_type() != EN_Position_Hall)
		{
			PBCSMsg pbMsg;
			CSResponseCoinMatchGame & pbResponse = *pbMsg.mutable_cs_response_coin_match_game();
			pbResponse.set_result(EN_MESSAGE_ALREADY_IN_TABLE);
			Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, pbMsg);
			return EN_Handler_Done;
		}

		//已经创建了房间，但请求进入别人的房间
		if (pbUser.create_table_id() != 0)
		{
			PBCSMsg pbMsg;
			CSResponseCoinMatchGame & pbResponse = *pbMsg.mutable_cs_response_coin_match_game();
			pbResponse.set_result(EN_MESSAGE_ALREADY_CREATE_TABLE);
			Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, pbMsg);
			return EN_Handler_Done;
		}
	}
	//对金币场房间的校验
	{
		//如果是机器人，将它的位置清空
		if (pbUser.acc_type() == EN_Account_Robot)
		{
			PBUpdateData update;
			update.set_key(PBUserDataField::kUserInfo);
			{
				PBDBAtomicField& field = *update.add_field_list();
				field.set_field(EN_DB_Field_Coin_POS);
				PBBPlayerPositionInfo& pos = *field.mutable_pos();
				pos.set_pos_type(EN_Position_Hall);
				pos.set_table_id(0);
				pos.set_gamesvrd_id(0);
			}
			psession->NewAddUpdateData(pbUser.uid(), update);

			return EN_Handler_Save;
		}

		if (pbCoinPos.pos_type() != EN_Position_Hall)
		{
			PBCSMsg pbMsg;
			CSResponseCoinMatchGame & pbResponse = *pbMsg.mutable_cs_response_coin_match_game();
			pbResponse.set_result(EN_MESSAGE_ALREADY_IN_TABLE);
			Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, pbMsg);
			return EN_Handler_Done;
		}
	}

	//校验通过后找到nodeid（通过配置获得）
	int iNodeId = PokerAutoMatchRoomConfig::Instance()->GetNodeTypeByTableType(pbRequest.ttype());
	if (iNodeId == -1)
	{
		PBCSMsg pbMsg;
		CSResponseCoinMatchGame & pbResponse = *pbMsg.mutable_cs_response_coin_match_game();
		pbResponse.set_result(EN_MESSAGE_SYSTEM_FAILED);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, pbMsg);
		return EN_Handler_Done;
	}
	//随机获得svidid
	int iSvid = 0;
	if (!GameSvrdManager::Instance()->GetGameIDByRandom(iNodeId, iSvid))
	{
		PBCSMsg pbMsg;
		CSResponseCoinMatchGame & pbResponse = *pbMsg.mutable_cs_response_coin_match_game();
		pbResponse.set_result(EN_MESSAGE_SYSTEM_FAILED);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, pbMsg);
		return EN_Handler_Done;
	}

	if (iSvid == -1)
	{
		PBCSMsg pbMsg;
		CSResponseCoinMatchGame & pbResponse = *pbMsg.mutable_cs_response_coin_match_game();
		pbResponse.set_result(EN_MESSAGE_SYSTEM_FAILED);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, pbMsg);
		return EN_Handler_Done;
	}

	//向游戏服发送请求
	//向游戏服发送消息
	Message::SendRequestMsg(RouteManager::Instance()->GetRouteByRandom(), psession, psession->_request_msg, iNodeId, iSvid);
	return EN_Handler_Succ;
}

/*
金币场匹配
*/
ENHandlerResult CRequestCoinMatchGame::ProcessUpdateSucc(CHandlerTokenBasic *ptoken, CSession *psession)
{
	int iUid = psession->_request_route.uid();
	CSRequestCoinMatchGame & pbRequest = *psession->_request_msg.mutable_cs_request_coin_match_game();

	//校验通过后找到nodeid（通过配置获得）
	int iNodeId = PokerAutoMatchRoomConfig::Instance()->GetNodeTypeByTableType(pbRequest.ttype());
	if (iNodeId == -1)
	{
		PBCSMsg pbMsg;
		CSResponseCoinMatchGame & pbResponse = *pbMsg.mutable_cs_response_coin_match_game();
		pbResponse.set_result(EN_MESSAGE_SYSTEM_FAILED);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, pbMsg);
		return EN_Handler_Done;
	}
	//随机获得svidid
	int iSvid = 0;
	if (!GameSvrdManager::Instance()->GetGameIDByRandom(iNodeId, iSvid))
	{
		PBCSMsg pbMsg;
		CSResponseCoinMatchGame & pbResponse = *pbMsg.mutable_cs_response_coin_match_game();
		pbResponse.set_result(EN_MESSAGE_SYSTEM_FAILED);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, pbMsg);
		return EN_Handler_Done;
	}

	if (iSvid == -1)
	{
		PBCSMsg pbMsg;
		CSResponseCoinMatchGame & pbResponse = *pbMsg.mutable_cs_response_coin_match_game();
		pbResponse.set_result(EN_MESSAGE_SYSTEM_FAILED);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, pbMsg);
		return EN_Handler_Done;
	}

	//向游戏服发送请求
	//向游戏服发送消息
	Message::SendRequestMsg(RouteManager::Instance()->GetRouteByRandom(), psession, psession->_request_msg, iNodeId, iSvid);
	return EN_Handler_Succ;
}

/*
金币场匹配
*/
ENHandlerResult CRequestCoinMatchGame::ProcessResponseMsg(CHandlerTokenBasic *ptoken, CSession *psession)
{
	//返回回调
	Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, psession->_response_msg);
	return EN_Handler_Done;
}