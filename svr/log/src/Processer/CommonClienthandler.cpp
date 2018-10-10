#include "CommonClienthandler.h"
#include "DBManager.h"
#include "ActivityMgr.h"
#include "ActivityRedisClient.h"

ENProcessResult CProcessLogMsg::ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession)
{
    const PBCSMsg& msg = psession->_request_msg;
    switch (msg.msg_union_case())
    {
        case PBCSMsg::kLogOnline:
            CDBManager::Instance()->online_map[msg.log_online().svr_id()] = msg.log_online().online();
            break;
        case PBCSMsg::kLogChipJournal:
            ProcessChipJournal(msg);
            break;
        case PBCSMsg::kLogRegist:
            ProcessRegist(msg);
            break;
        case PBCSMsg::kLogLogin:
            ProcessLogin(msg);
            break;
        case PBCSMsg::kLogLogout:
            ProcessLogout(msg);
            break;
		case PBCSMsg::kLogGameLog:
            ProcessGameLog(msg);
            break;
		case PBCSMsg::kLogGameInfoLog:
            ProcessGameInfoLog(msg);
            break;
		case PBCSMsg::kLogTeaBarChipsFlow:
			ProcessTeaBarChipsFlow(msg);
			break;
        case PBCSMsg::kLogAtPlay:
            CDBManager::Instance()->atplay_map[msg.log_at_play().game_type()][msg.log_at_play().svr_id()] = msg.log_at_play().paly();
            break;
        case PBCSMsg::kLogTableInfoLog:
            ProcessTableInfoLog(msg);
            break;
        case PBCSMsg::kLogTablePlayerLog:
            ProcessTablePlayerLog(msg);
            break;
        case PBCSMsg::kLogCreateTable:
            ProcessCreateTableLog(msg);
            break;
        case PBCSMsg::kLogDiamondFlow:
            ProcessDiamondFlow(msg);
            break;
        case PBCSMsg::kLogBonusFlow:
            ProcessBonusFlow(msg);
            break;
        case PBCSMsg::kLogSkipmatchLevelAndStateFlow:
            ProcessSkipMatchLevelAndStateFlow(msg);
            break;
		case PBCSMsg::kLogCoinsFlow:
			ProcessCoinsFlow(msg);

        default: break;
    }
    return EN_Process_Result_Completed;
}

void CProcessLogMsg::ProcessChipJournal(const PBCSMsg& msg)
{
    const LogChipJournal& log = msg.log_chip_journal();

    time_t t_now = log.time_stamp();
	struct tm *p;
	p = localtime(&t_now);

	char szSql[10240] = {0};
	sprintf(szSql,
        "INSERT INTO lzmj_chip_%04d%02d%02d(uid, num, total, reason, sign, time_stamp, acc_type, channel)"
        "VALUES(%ld, %ld, %ld, %d, %d, %d, %d, %d)",
        p->tm_year + 1900, p->tm_mon + 1, p->tm_mday,
		log.uid(), log.act_num(), log.total_num(), log.reason(), (log.act_num() > 0 ? 1 : 0),
		log.time_stamp(), log.acc_type(), log.channel());
	string sql = szSql;

    CDBManager::Instance()->AddSql(sql);
	{
		char szSql[10240] = {0};
		int stamp = time(NULL);
		sprintf(szSql,
	        "INSERT INTO lzmj_user_info_%03d(uid, chip, acc_type, channel,time_stamp)"
	        "VALUES(%ld, %ld, %d, %d ,%d) ON DUPLICATE KEY UPDATE chip = %ld, acc_type = %d, channel = %d, time_stamp = %d",
	        0,
	        log.uid(), log.total_num(), log.acc_type(), log.channel(),stamp,
	        log.total_num(), log.acc_type(), log.channel(),stamp);
		string sql = szSql;

	    CDBManager::Instance()->AddSql(sql);
    }
}

void CProcessLogMsg::ProcessRegist(const PBCSMsg& msg)
{
    const LogRegist& log = msg.log_regist();

    time_t t_now = log.time_stamp();
	struct tm *p;
	p = localtime(&t_now);

	char szSql[10240] = {0};
	sprintf(szSql,
        "INSERT INTO lzmj_regist_%04d%02d%02d(uid, time_stamp, acc_type, channel,device_name,band)"
        "VALUES(%ld, %d, %d, %d,'%s','%s')",
        p->tm_year + 1900, p->tm_mon + 1, p->tm_mday,
		log.uid(), log.time_stamp(), log.acc_type(), log.channel(),log.device_name().c_str(),log.band().c_str());
	string sql = szSql;

    CDBManager::Instance()->AddSql(sql);

	char szSql2[10240] = {0};
	sprintf(szSql2,
        "INSERT INTO lzmj_user_info_%03d set uid = %ld, acc_type = %d, name = '%s', channel = %d , device_name ='%s', band='%s', time_stamp = %d , regist_stamp = %d",
        0, log.uid(), log.acc_type(), log.name().c_str(), log.channel(),log.device_name().c_str(),log.band().c_str(),(int)time(NULL),(int)time(NULL));
	string sql2 = szSql2;

    CDBManager::Instance()->AddSql(sql2);
}

void CProcessLogMsg::ProcessLogin(const PBCSMsg& msg)
{
    const LogLogin& log = msg.log_login();

    time_t t_now = log.time_stamp();
	struct tm *p;
	p = localtime(&t_now);

	char szSql[10240] = {0};
	sprintf(szSql,
        "INSERT INTO lzmj_login_%04d%02d%02d(uid, time_stamp, acc_type, channel, device_name , band)"
        "VALUES(%ld, %d, %d, %d,'%s','%s') ON DUPLICATE KEY UPDATE login_times=login_times+1",
        p->tm_year + 1900, p->tm_mon + 1, p->tm_mday,
		log.uid(), log.time_stamp(), log.acc_type(), log.channel(),log.device_name().c_str(),
		log.band().c_str());
	string sql = szSql;

    CDBManager::Instance()->AddSql(sql);

	char szSql2[10240] = {0};
	int stamp = time(NULL);
	sprintf(szSql2,
        "INSERT INTO lzmj_user_info_%03d(uid, is_online,time_stamp)"
        "VALUES(%ld, true,%d) ON DUPLICATE KEY UPDATE is_online = true,time_stamp=%d",
        0, log.uid(),stamp,stamp);
	string sql2 = szSql2;

    CDBManager::Instance()->AddSql(sql2);
}

void CProcessLogMsg::ProcessLogout(const PBCSMsg& msg)
{
    const LogLogout& log = msg.log_logout();

    time_t t_now = time(NULL);
	struct tm *p;
	p = localtime(&t_now);

	char szSql[10240] = {0};
	sprintf(szSql,
        "INSERT INTO lzmj_logout_%04d%02d%02d(uid, online_time, acc_type, channel)"
        "VALUES(%ld, %d, %d, %d) ON DUPLICATE KEY UPDATE online_time = online_time + %d",
        p->tm_year + 1900, p->tm_mon + 1, p->tm_mday,
		log.uid(), log.online_time(), log.acc_type(), log.channel(), log.online_time());
	string sql = szSql;

    CDBManager::Instance()->AddSql(sql);

	char szSql2[10240] = {0};
	sprintf(szSql2,
        "INSERT INTO lzmj_user_info_%03d(uid, is_online)"
        "VALUES(%ld, false) ON DUPLICATE KEY UPDATE is_online = false",
        0, log.uid());
	string sql2 = szSql2;

    CDBManager::Instance()->AddSql(sql2);
}

void CProcessLogMsg::ProcessGameLog(const PBCSMsg& msg)
{
    const LogGameLog& log = msg.log_game_log();
	
	// 德州牌局日志
    time_t t_now = log.begin_time();
	struct tm *p;
	p = localtime(&t_now);

	char szSql[409600] = {0};
	sprintf(szSql,
            "INSERT INTO lzmj_game_table_%04d%02d%02d(game_id,table_id,table_log,begin_time,end_time,seat_num)"
            "VALUES(%ld,%ld,'%s',%d,%d,%d)",
            p->tm_year + 1900, p->tm_mon + 1, p->tm_mday,
            log.game_id(), log.table_id(), log.table_log().c_str(), log.begin_time(), log.end_time(), log.seat_num());
	string sql = szSql;

    CDBManager::Instance()->AddSql(sql);

    for (int i = 0; i < log.players_info_size(); ++i)
    {
		int64 player = log.players(i);
        const PBLogPlayersInfo & pbPlayersInfo = log.players_info(i);
        int64 iPlayerUid = pbPlayersInfo.player_uid();
        int iChannel = pbPlayersInfo.channel();

    	char szSql[409600] = {0};
    	sprintf(szSql,
            "INSERT INTO lzmj_game_player_%04d%02d%02d(uid,game_id,table_id,time_stamp, acc_type, seat_num, game_type,is_free_game,is_finished_game,channel)"
            "VALUES(%lld,%ld,%ld,%d,%d,%d,%d,%d,%d,%d)",
            p->tm_year + 1900, p->tm_mon + 1, p->tm_mday,
            player, log.game_id(), log.table_id(),log.begin_time(), 0, log.seat_num(), log.game_type(),log.is_free_game(),log.is_finished_game(),iChannel);
    	string sql = szSql;
		
		//
        CDBManager::Instance()->AddSql(sql);
    }

	//定点玩牌统计
	if (CActivityMgr::Instance()->IsInActPeriod(EN_Activity_ID_DING_DIAN_WAN_PAI))
	{
		char szmainkey[100] = { 0 };
		snprintf(szmainkey, sizeof(szmainkey), "ddwp_%04d%02d%02d", p->tm_year + 1900, p->tm_mon + 1, p->tm_mday);
		for (int i = 0; i < log.players_size(); i++)
		{
			if (!CActivityMgr::Instance()->IsInWhiteList(log.players(i)))
			{
				continue;
			}
			char szsubkey[100] = { 0 };
			snprintf(szsubkey, sizeof(szsubkey), "%ld", log.players(i));
			ActivityRedisClient::Instance()->IncreaseHashObject(szmainkey, szsubkey, 1);
		}
	}

	//一段时间内玩牌活动
	if (CActivityMgr::Instance()->IsInActPeriod(EN_Activity_ID_PERIOD_WAN_PAI))
	{
		for (int i = 0; i < log.players_size(); i++)
		{
			if (!CActivityMgr::Instance()->IsInWhiteList(log.players(i)))
			{
				continue;
			}
			char szsubkey[100] = { 0 };
			snprintf(szsubkey, sizeof(szsubkey), "%ld", log.players(i));
			ActivityRedisClient::Instance()->IncreaseHashObject("playactivity", szsubkey, 1);
		}
	}
}

void CProcessLogMsg::ProcessGameInfoLog(const PBCSMsg& msg)
{
    const LogGameInfoLog& log = msg.log_game_info_log();

    // 德州牌局日志
    time_t t_now = time(NULL);
    struct tm *p;
    p = localtime(&t_now);

    char szSql[409600] = {0};
    sprintf(szSql,
        "INSERT INTO lzmj_game_info_%04d%02d%02d(game_id,game_type,conf_round,real_round,seat_num,create_uid,master_uid)"
        "VALUES(%ld,%d,%d,%d,%d,%ld,%ld)",
        p->tm_year + 1900, p->tm_mon + 1, p->tm_mday,
        log.game_id(), log.game_type(), log.conf_round(), log.real_round(), log.seat_num(),log.creator_uid(),log.master_uid());
    string sql = szSql;

    CDBManager::Instance()->AddSql(sql);
}


void CProcessLogMsg::ProcessTeaBarChipsFlow(const PBCSMsg& msg)
{
	const LogTeaBarChipsFlow& log = msg.log_tea_bar_chips_flow();

	time_t t_now = log.time_stamp();
	struct tm *p;
	p = localtime(&t_now);

	char szSql[10240] = { 0 };
	sprintf(szSql,
		"INSERT INTO lzmj_tea_bar_chips_%04d%02d%02d(tbid, master_uid, num, total, reason, time_stamp)"
		"VALUES(%ld, %ld, %ld, %ld, %d, %d)",
		p->tm_year + 1900, p->tm_mon + 1, p->tm_mday,
		log.tbid(), log.master_uid(), log.act_num(), log.total_num(), log.reason(),
		log.time_stamp());
	string sql = szSql;

	CDBManager::Instance()->AddSql(sql);
}

void CProcessLogMsg::ProcessTableInfoLog(const PBCSMsg& msg)
{
    const LogTableInfoLog& log = msg.log_table_info_log();

    // 打点数据 牌桌数据
    time_t t_now = time(NULL);
    struct tm *p;
    p = localtime(&t_now);

    char szSql[409600] = {0};
    sprintf(szSql,
        "INSERT INTO lzmj_table_info_%04d%02d%02d(game_id,table_id,game_type,conf_round,real_round,seat_num,detail,creator_uid,tbid,master_uid,time_used,time_stamp)"
        "VALUES(%ld,%d,%d,%d,%d,%d,'%s',%ld,%ld,%ld,%d,%d)",
        p->tm_year + 1900, p->tm_mon + 1, p->tm_mday,
        log.game_id(), log.table_id(), log.game_type(), log.conf_round(), log.real_round(), log.seat_num(),
            log.detail().c_str(), log.creator_uid(), log.tbid(), log.master_uid(), log.time_used(), log.time_stamp());
    string sql = szSql;

    CDBManager::Instance()->AddSql(sql);
}

void CProcessLogMsg::ProcessTablePlayerLog(const PBCSMsg& msg)
{
    const LogTablePlayerLog& log = msg.log_table_player_log();

    // 打点数据 玩家数据
    time_t t_now = time(NULL);
    struct tm *p;
    p = localtime(&t_now);

    char szSql[409600] = {0};
    sprintf(szSql,
        "INSERT INTO lzmj_table_player_%04d%02d%02d(uid,game_id,win_round,score,cost,is_winner,time_stamp)"
        "VALUES(%ld,%ld,%d,%d,%d,%d,%d)",
        p->tm_year + 1900, p->tm_mon + 1, p->tm_mday,
        log.uid(), log.game_id(), log.win_round(), log.score(), log.cost(), log.is_winner(), log.time_stamp());
    string sql = szSql;

    CDBManager::Instance()->AddSql(sql);
}

void CProcessLogMsg::ProcessCreateTableLog(const PBCSMsg& msg)
{
    const LogCreateTable& log = msg.log_create_table();

    // 创建房间日志
    time_t t_now = time(NULL);
    struct tm *p;
    p = localtime(&t_now);

    char szSql[409600] = {0};
    sprintf(szSql,
        "INSERT INTO lzmj_create_table_%04d%02d%02d(creator_uid,master_uid,game_type,table_id)"
        "VALUES(%ld,%ld,%d,%d)",
        p->tm_year + 1900, p->tm_mon + 1, p->tm_mday,
        log.creator_uid(), log.master_uid(), log.game_type(), log.table_id());
    string sql = szSql;

    CDBManager::Instance()->AddSql(sql);
}

void CProcessLogMsg::ProcessDiamondFlow(const PBCSMsg& a_pbmsg)
{
    const LogDiamondFlow & pbLog = a_pbmsg.log_diamond_flow();

    time_t tNow = pbLog.time_stamp();
    struct tm *p;
    p = localtime(&tNow);

    char szSql[10240] = { 0 };
    sprintf(szSql,
        "INSERT INTO lzmj_diamond_flow_%03d(uid,num,total,reason,time_stamp,acc_type,channel)"
        "VALUES(%ld,%ld,%ld,%d,%d,%d,%d)",
        0,
        pbLog.uid(),pbLog.act_num(),pbLog.total_num(),pbLog.reason(),pbLog.time_stamp(),pbLog.acc_type(),pbLog.channel());
    string sql = szSql;

    CDBManager::Instance()->AddSql(sql);

    //用户表修改
    {
        char szSql[10240] = {0};
        int stamp = time(NULL);
        sprintf(szSql,
            "INSERT INTO lzmj_user_info_%03d(uid, diamond, acc_type ,time_stamp)"
            "VALUES(%ld, %ld, %d ,%d) ON DUPLICATE KEY UPDATE diamond = %ld, acc_type = %d, time_stamp = %d",
            0,
            pbLog.uid(), pbLog.total_num(), pbLog.acc_type() ,stamp,
            pbLog.total_num(), pbLog.acc_type() ,stamp);
        string sql = szSql;

        CDBManager::Instance()->AddSql(sql);
    }
}

/*
处理金币场流水
*/
void CProcessLogMsg::ProcessCoinsFlow(const PBCSMsg& a_pbmsg)
{
	const LogCoinsFlow & pbLog = a_pbmsg.log_coins_flow();

	time_t tNow = pbLog.time_stamp();
	struct tm *p;
	p = localtime(&tNow);

	char szSql[10240] = { 0 };
	sprintf(szSql,
		"INSERT INTO lzmj_coin_%04d%02d%02d(uid,num,total,reason,time_stamp,acc_type,channel)"
		"VALUES(%ld, %ld, %ld, %d, %d, %d, %d)",
		p->tm_year + 1900, p->tm_mon + 1, p->tm_mday,
		pbLog.uid(), pbLog.act_num(), pbLog.total_num(), pbLog.reason(), pbLog.time_stamp(), pbLog.acc_type(), pbLog.channel());
	string sql = szSql;

	CDBManager::Instance()->AddSql(sql);

	//用户表修改
	{
		char szSql[10240] = { 0 };
		int stamp = time(NULL);
		sprintf(szSql,
			"INSERT INTO lzmj_user_info_%03d(uid, coins, acc_type ,time_stamp)"
			"VALUES(%ld, %d %d ,%d) ON DUPLICATE KEY UPDATE coins = %ld, acc_type = %d , time_stamp = %d",
			0,
			pbLog.uid(), pbLog.total_num(), pbLog.acc_type(), stamp,
			pbLog.total_num(), pbLog.acc_type(), stamp);
		string sql = szSql;

		CDBManager::Instance()->AddSql(sql);
	}
}

void CProcessLogMsg::ProcessBonusFlow(const PBCSMsg& a_pbmsg)
{
    const LogBonusFlow & pbLog = a_pbmsg.log_bonus_flow();

    time_t tNow = pbLog.time_stamp();
    struct tm *p;
    p = localtime(&tNow);

    char szSql[10240] = { 0 };
    sprintf(szSql,
        "INSERT INTO lzmj_bonus_flow_%03d(uid,num,total,reason,time_stamp,acc_type,channel)"
        "VALUES(%ld,%f,%f,%d,%d,%d,%d)",
        0,
        pbLog.uid(),pbLog.act_num(),pbLog.total_num(),pbLog.reason(),pbLog.time_stamp(),pbLog.acc_type(),pbLog.channel());
    string sql = szSql;

    CDBManager::Instance()->AddSql(sql);

    //用户表修改
    {
        char szSql[10240] = {0};
        int stamp = time(NULL);
        sprintf(szSql,
            "INSERT INTO lzmj_user_info_%03d(uid, bonus, acc_type ,time_stamp)"
            "VALUES(%ld, %f %d ,%d) ON DUPLICATE KEY UPDATE bonus = %f, acc_type = %d , time_stamp = %d",
            0,
            pbLog.uid(), pbLog.total_num(), pbLog.acc_type() ,stamp,
            pbLog.total_num(), pbLog.acc_type() ,stamp);
        string sql = szSql;

        CDBManager::Instance()->AddSql(sql);
    }
}

void CProcessLogMsg::ProcessSkipMatchLevelAndStateFlow(const PBCSMsg& a_pbMsg)
{
    const LogSkipMatchLevelAndStateFlow & pbLog = a_pbMsg.log_skipmatch_level_and_state_flow();

    time_t tNow = pbLog.time_stamp();
    struct tm *p;
    p = localtime(&tNow);

    char szSql[10240] = { 0 };
    sprintf(szSql,
        "INSERT INTO lzmj_skip_match_info_%04d%02d%02d(uid,level_change_num,level_total,state,game_type,reason,time_stamp,is_finished_skip,channel)"
        "VALUES(%ld,%d,%d,%d,%d,%d,%d,%d,%d)",
        p->tm_year + 1900, p->tm_mon + 1, p->tm_mday,
        pbLog.uid(),pbLog.level_act_val(),pbLog.level_after_change(),pbLog.state_after_change(),pbLog.game_type(),pbLog.reason(),pbLog.time_stamp(),pbLog.is_finished_skip(),pbLog.channel());
    string sql = szSql;

    CDBManager::Instance()->AddSql(sql);
}
