#include "TableManager.h"
#include "global.h"
#include "Message.h"
#include "RouteManager.h"
#include "LogWriter.h"
#include "TableModle.h"
#include "LocalInfo.h"

TableManager * TableManager::Instance(void)
{
	return CSingleton<TableManager>::Instance();
}

CPBGameTable * TableManager::FindTable(int64 tid)
{
	if (_tablemap.find(tid) == _tablemap.end())
	{
		return NULL;
	}
	CPBGameTable & table = _tablemap[tid];
	return &table;
}

CPBGameTable * TableManager::CreateTable(int64 tid, const PBSDRTableConfig & conf)
{
	CPBGameTable & table = _tablemap[tid];
	table.Init(tid, conf);
	RefreshArgv();

	return &table;
}

void TableManager::OnPlayerEnterTable(int64 uid, int64 a_iTid, bool a_bIsReConnect)
{
	if (_tablemap.find(a_iTid) == _tablemap.end())
	{
		return;
	}
	CPBGameTable & CTable = _tablemap[a_iTid];
	int iSeatsNum = 0;
	for (auto iter = CTable.seats().begin(); iter != CTable.seats().end(); iter++)
	{
		if (iter->state() == EN_SEAT_STATE_NO_PLAYER)
		{
			continue;
		}

		iSeatsNum++;
	}

	//删除老的
	for (int i = 0; i <= 3; i++)
	{
		if (m_TablePlayerNumMap.find(i) == m_TablePlayerNumMap.end())
		{
			continue;
		}

		TableSet & DelTableSet = m_TablePlayerNumMap[i];
		if (DelTableSet.find(a_iTid) != DelTableSet.end())
		{
			DelTableSet.erase(a_iTid);
		}
	}

	//都需要检查，进行刷新操作
	{
		TableSet & AddTableSet = m_TablePlayerNumMap[iSeatsNum];
		AddTableSet.insert(a_iTid);
	}
	_usertablemap[uid] = a_iTid;

	RefreshArgv();
}

void TableManager::OnPlayerLeaveTable(int64 a_iUid, int64 a_iTid)
{
	if (_tablemap.find(a_iTid) == _tablemap.end())
	{
		return;
	}

	CPBGameTable & CTable = _tablemap[a_iTid];
	int iPlayerNum = TableLogic::GetPlayerNum(CTable);

	UserTableMap::iterator iter = _usertablemap.find(a_iUid);
	if (iter != _usertablemap.end())
	{
		_usertablemap.erase(iter);
	}

	//如果桌子的状态是准备状态，或者是结束状态
	if (CTable.state() == EN_TABLE_STATE_WAIT || CTable.state() == EN_TABLE_STATE_SINGLE_OVER)
	{
		if (iPlayerNum - 1 >= 0)
		{
			TableSet & PreTableSet = m_TablePlayerNumMap[iPlayerNum - 1];
			PreTableSet.insert(a_iTid);
		}
	}

	{
		TableSet & PreTableSet = m_TablePlayerNumMap[iPlayerNum];
		PreTableSet.erase(a_iTid);
	}

	RefreshArgv();
}

int64 TableManager::GetPlayerTableID(int64 uid)
{
	int64 tid = -1;
	if (_usertablemap.find(uid) != _usertablemap.end())
	{
		tid = _usertablemap[uid];
	}
	return tid;
}

void TableManager::DissolveTable(int64 a_iTid)
{
	if (_tablemap.find(a_iTid) == _tablemap.end())
	{
		return;
	}
	CPBGameTable & CTable = _tablemap[a_iTid];
	CTable.StopTimer();
	CTable.StopCheckUserOffTimer();
	CTable.StopActionWaitingTimer();

	//让所以玩家都离开桌子
	for (int i = 0; i < CTable.seats_size(); i++)
	{
		PBSDRTableSeat & pbSeat = *CTable.mutable_seats(i);
		if (pbSeat.state() == EN_SEAT_STATE_NO_PLAYER)
		{
			continue;
		}

		//通知玩家离开座子
		PBCSMsg notify;
		CSNotifyLogoutTable & cs_notify_logout_table = *notify.mutable_cs_notify_logout_table();
		cs_notify_logout_table.set_seat_index(pbSeat.index());
		if (!CTable.is_trusteeship_error())
		{
			cs_notify_logout_table.set_reason(EN_LOGOUT_TABOLE_REASON_FINISH_GAME);
		}
		else
		{
			cs_notify_logout_table.set_reason(EN_LOGOUT_TABOLE_REASON_TRUSTEESHIP_ERROR);
		}
		TableLogic::BroadcastTableMsg(CTable, notify);

		const PBTableUser & pbUser = pbSeat.user();
		OnPlayerLeaveTable(pbUser.uid(), a_iTid);

		//这里需要清空玩家信息
		pbSeat.clear_user();
		pbSeat.clear_action_choice();
		pbSeat.clear_hand_cards();
		pbSeat.set_state(EN_SEAT_STATE_NO_PLAYER);
		pbSeat.set_action_token(0);
		pbSeat.set_final_score(0);
		pbSeat.set_multiple(0);
		pbSeat.set_need_check_double(false);
		pbSeat.set_double_times(-1);
		pbSeat.set_bomb_num(0);
		pbSeat.set_bao_type(0);
		pbSeat.clear_win_info();
		pbSeat.set_is_buy(false);
		pbSeat.set_soft_bomb_num(0);
		pbSeat.set_is_lai_zhuang(false);
		pbSeat.set_is_baozi_zhuang(false);
		pbSeat.set_qiang_di_zhu_times(0);
		pbSeat.set_is_mingpai_after_deal_cards(-1);
		if (pbSeat.is_mingpai() == 1)
		{
			pbSeat.set_is_mingpai_after_deal_cards(2);
		}
		pbSeat.set_is_jiao_dizhu(false);
		pbSeat.clear_is_trusteeship();
		pbSeat.clear_trusteeship_time();
		pbSeat.clear_end_trusteeship_time();
	}

	//金币场桌子打完之后房间不解散
	RefreshArgv();
}

void TableManager::OnRetire()
{
	_state = EN_Game_Service_State_Retired;
	PBCSMsg msg;
	SSNotifyGameSvrdRetire & ss_notify_gamesvrd_retired = *msg.mutable_ss_notify_gamesvrd_retired();
	ss_notify_gamesvrd_retired.set_gameid(TGlobal::_svid);
	ss_notify_gamesvrd_retired.set_gtype(TGlobal::_svrd_type);
	Message::PushInnerMsg(RouteManager::Instance()->GetRouteByRandom(), 0, msg, EN_Node_Room, 1);

	RefreshArgv();
}

void TableManager::ReportGameInfo()
{
	if (_state == EN_Game_Service_State_Retired)
	{
		return;
	}
	PBCSMsg msg;
	SSReportGameSvrdInfo & ss_report_gamesvrd_info = *msg.mutable_ss_report_game_info();
	ss_report_gamesvrd_info.set_gameid(TGlobal::_svid);
	ss_report_gamesvrd_info.set_gtype(TGlobal::_svrd_type);
	Message::PushInnerMsg(RouteManager::Instance()->GetRouteByRandom(), 0, msg, EN_Node_Room, 1);
}

void TableManager::RefreshArgv()
{
	char szbuf[100] = { 0 };
	snprintf(szbuf, sizeof(szbuf), "us:%04d,ts:%04d,r:%s", (int)_usertablemap.size(), (int)_tablemap.size(), _state == EN_Game_Service_State_Retired ? "T" : "F");
	memset(TGlobal::_pargv[TGlobal::_argc - 1], 0, strlen(TGlobal::_pargv[TGlobal::_argc - 1]));
	strcpy(TGlobal::_pargv[TGlobal::_argc - 1], szbuf);
}

int TableManager::ProcessOnTimerOut(int Timerid)
{
	switch (Timerid)
	{
	case REPORT_AT_PLAY_TIMER:
	{
		PBCSMsg msg;
		LogAtPlay & log = *msg.mutable_log_at_play();
		log.set_paly(_usertablemap.size());
		log.set_game_type(TGlobal::_svrd_type);
		log.set_svr_id(TGlobal::_svid);
		CLogWriter::Instance()->Send(msg);
		break;
	}

	default:
		break;
	}
	return 0;
}

void TableManager::Init()
{
	m_StartAllocTid = 1000000 * TGlobal::_svid;

	timer.SetTimeEventObj(this, REPORT_AT_PLAY_TIMER);
	timer.StartTimerBySecond(30, true);
}

CPBGameTable * TableManager::CreateTableByP(int64 a_iTid, int a_ilevel, PBSDRTableConfig * a_ppbConf)
{
	PBSDRTableConfig pbTableConfig;
	if (a_ppbConf != NULL)
	{
		pbTableConfig.CopyFrom(*a_ppbConf);
		pbTableConfig.set_round(1);
		pbTableConfig.set_skipmatch_level(a_ilevel);
		pbTableConfig.set_seat_num(3);
	}
	else
	{
		pbTableConfig.set_round(1);
		pbTableConfig.set_ttype(EN_Table_3Ren_DDZ_COIN);
		pbTableConfig.set_level(a_ilevel);
		pbTableConfig.set_pay_type(0);
		pbTableConfig.set_seat_num(3);
		pbTableConfig.set_max_line(32);
		pbTableConfig.set_count_way(0);
	}

	TableSet & AddTableSet = m_TablePlayerNumMap[1];
	AddTableSet.insert(a_iTid);

	return CreateTable(a_iTid, pbTableConfig);
}

int64 TableManager::GetTableForUser(int a_iLevel, int iAccType, string a_stNick, int a_iExclude_tid, PBSDRTableConfig * a_ppbConf)
{
	//set的特性是，所有元素都会根据元素的键值自动排序，set的元素不像map那样可以同时拥有实值(value)和键值(key),set元素的键值就是实值，实值就是键值。set不允许两个元素有相同的键值。
	//先找2人的,再找1人的
	for (int i = 2; i >= 0; i--)
	{
		//机器人不进入人数为0的房间
		if (iAccType == EN_Account_Robot && i == 0)
		{
			break;
		}

		TableSet & DelTableSet = m_TablePlayerNumMap[i];
		if (DelTableSet.size() > 0)
		{
			TableSet::iterator Iter = DelTableSet.begin();
			for (; Iter != DelTableSet.end();)
			{
				int64 iTid = *Iter;
				CPBGameTable * pCTable = FindTable(iTid);

				if (iTid == a_iExclude_tid)
				{
					Iter++;
					continue;
				}

				if (!pCTable)
				{
					DelTableSet.erase(Iter++);
					continue;
				}

				if (pCTable->state() != EN_TABLE_STATE_WAIT && pCTable->state() != EN_TABLE_STATE_SINGLE_OVER && pCTable->state() != EN_TABLE_STATE_WAIT_ROBOT)
				{
					DelTableSet.erase(Iter++);
					continue;
				}

				int iPlayNum = TableLogic::GetPlayerNum(*pCTable);
				if (iPlayNum != i)
				{
					VLogMsg(CLIB_LOG_LEV_DEBUG, "table[%lld] error,error value:[%d],value:[%d]", *Iter, i,iPlayNum);
					DelTableSet.erase(Iter++);
					TableSet & table_set = m_TablePlayerNumMap[iPlayNum];
					table_set.insert(iTid);
					continue;
				}

				if(iPlayNum >= pCTable->config().seat_num())
				{
					DelTableSet.erase(Iter++);
					continue;
				}

				if (iAccType == EN_Account_Robot)
				{
					if (!TableLogic::CheckRobotAbleEnterTable(*pCTable, a_stNick))
					{
						Iter++;
						continue;
					}
				}

				DelTableSet.erase(Iter);
				TableSet & AddTableSet = m_TablePlayerNumMap[i + 1];
				AddTableSet.insert(iTid);
				return iTid;
			}
		}
	}

	//机器人不自己创建桌子
	if (iAccType == EN_Account_Robot)
	{
		return -1;
	}

	//如果没有找到房间
	while (true)
	{
		m_StartAllocTid++;
		//如果分配的房间到头了
		if (m_StartAllocTid == 1000000 * (TGlobal::_svid + 1))
		{
			m_StartAllocTid = 1000000 * TGlobal::_svid;
		}

		if (_tablemap.find(m_StartAllocTid) == _tablemap.end())
		{
			break;
		}
	}

	VLogMsg(CLIB_LOG_LEV_DEBUG,"user create table[%lld] ,acc_type[%d]", m_StartAllocTid, iAccType);
	CreateTableByP(m_StartAllocTid, a_iLevel, a_ppbConf);
	return m_StartAllocTid;
}
