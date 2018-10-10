#include "TeaBarManager.h"
#include "RouteManager.h"
#include "Session.h"
#include "Message.h"
#include "Common.h"
#include "TeaBarHandlerProxy.h"
#include "LogWriter.h"
#include "PBConfigBasic.h"

bool TeaBarManager::Init()
{
	_lru_timer.SetTimeEventObj(this,LRU_TIMER);
	_lru_timer.StartTimerBySecond(60,true);

	if (!RedisManager::Instance()->Init())
	{
		return false;
	}

	if (!InitMaxTbid())
	{
		return false;
	}
	return true;
}

bool TeaBarManager::InitMaxTbid()
{
	CTeaBarRedisClient* pclient = RedisManager::Instance()->GetRedisClientByIndex(0);
	if (pclient == NULL)
	{
		return false;
	}

	int iret = pclient->GetMaxTbid(_max_tbid);
	if (iret == REPLY_OK && _max_tbid == -1)
	{
		_max_tbid = 10000000;
		pclient->SetMaxTbid(_max_tbid);
	}
	else if (iret == REPLY_ERROR)
	{
		return false;
	}
	return true;
}

int TeaBarManager::ProcessOnTimerOut(int Timerid)
{
	int knock_out_time = 24 * 3600;
#ifdef _DEBUG
	knock_out_time = 120;
#endif
	int now = (int)time(NULL);
	vector<int64> vec;
	TeaBarDataMap::iterator iter = _tea_bar_map.begin();
	for (; iter != _tea_bar_map.end(); iter++)
	{
		PBTeaBarData& tbdata = iter->second;
		if (now - tbdata.last_active_time_stamp() >= knock_out_time)
		{
			vec.push_back(iter->first);
		}
	}

	for (int i = 0; i < (int)vec.size(); i++)
	{
		KnockOutTeaBarData(vec[i]);
	}

	VLogMsg(CLIB_LOG_LEV_DEBUG, "teabar statistics map size,[%d]", (int)_tea_bar_table_stat_map.size());
	VLogMsg(CLIB_LOG_LEV_DEBUG, "teabar data map size,[%d]", (int)_tea_bar_map.size());
	return 0;
}

bool TeaBarManager::AddUser(const PBUser& user, PBTeaBarData& tbdata)
{
	//if (tbdata.users_size() >= MAX_TEA_BAR_USER_NUM)
	//{
	//	return false;
	//}
	PBTeaBarUser& tbuser = *tbdata.add_users();
	tbuser.set_uid(user.uid());
	tbuser.set_name(user.nick());
	tbuser.set_url(user.pic_url());
	tbuser.set_join_time((int)time(NULL));
	//	//更新redis数据
	if (!UpdateDataToRedis(tbdata))
	{
		return false;
	}
	return true;
}

bool TeaBarManager::UpdateDataToRedis(PBTeaBarData& tbdata)
{
	CTeaBarRedisClient* pclient = RedisManager::Instance()->GetRedisClientByTbid(tbdata.tbid());
	if (pclient == NULL)
	{
		return false;
	}
	PBHashDataField field;
	field.mutable_tea_bar_data()->CopyFrom(tbdata);
	char szkey[20] = { 0 };
	snprintf(szkey, sizeof(szkey), "%ld", tbdata.tbid());
	if (!pclient->UpdateObject(szkey, field))
	{
		return false;
	}
	return true;
}

bool TeaBarManager::DeleteUser(int64 uid, PBTeaBarData& tbdata)
{
	int find_index = -1;
	for (int i = 0; i < tbdata.users_size(); i++)
	{
		const PBTeaBarUser& user = tbdata.users(i);
		if (user.uid() == uid)
		{
			find_index = i;
			break;
		}
	}
	if (find_index == -1)
	{
		return false;
	}
	
	tbdata.mutable_users()->DeleteSubrange(find_index, 1);

	//清除今日，昨日，历史记录的数据
	PBDateTeaBarUserGameRecordList* pdatelist = GetGameRecordList(tbdata, EN_TeaBar_Date_Type_Today);
	if (pdatelist != NULL)
	{
		DeleteUserGameRecord(uid, *pdatelist);
	}

	pdatelist = GetGameRecordList(tbdata, EN_TeaBar_Date_Type_Yesterday);
	if (pdatelist != NULL)
	{
		DeleteUserGameRecord(uid, *pdatelist);
	}

	pdatelist = GetGameRecordList(tbdata, EN_TeaBar_Date_Type_History);
	if (pdatelist != NULL)
	{
		DeleteUserGameRecord(uid, *pdatelist);
	}
	
	//更新redis数据
	if (!UpdateDataToRedis(tbdata))
	{
		return false;
	}
	return true;
}

PBTeaBarUser* TeaBarManager::GetUser(int64 uid, PBTeaBarData& tbdata)
{
	for (int i = 0; i < tbdata.users_size(); i++)
	{
		PBTeaBarUser& user = *tbdata.mutable_users(i);
		if (user.uid() == uid)
		{
			return &user;
		}
	}
	return NULL;
}

PBTeaBarData* TeaBarManager::CreateTeaBar(const PBUser& user, const string& tbname, const string& desc)
{
	int64 tbid = AllocNewTeaBarID();
	TeaBarDataMap::iterator iter = _tea_bar_map.find(tbid);
	if (iter != _tea_bar_map.end())
	{
		return NULL;
	}

	PBTeaBarData& tbdata = _tea_bar_map[tbid];
	tbdata.set_tbid(tbid);
	tbdata.set_tbname(tbname);
	tbdata.set_master_uid(user.uid());
	tbdata.set_master_name(user.nick());
	tbdata.set_master_url(user.pic_url());
	tbdata.set_desc(desc);

	//插入历史记录，时间搓0
	PBDateTeaBarUserGameRecordList& datelist = *tbdata.add_record_list();
	datelist.set_time_stamp(0);
	datelist.set_create_table_num(0);
	datelist.set_cost_chips(0);

	//测试代码
//#ifdef _DEBUG
//	int test_id_start = 100000000;
//	for (int i = test_id_start; i < test_id_start + 499; i++)
//	{
//		PBTeaBarUser& tbuser = *tbdata.add_users();
//		tbuser.set_uid(i);
//		char sznick[100] = { 0 };
//		snprintf(sznick, sizeof(sznick), "robot%d", i - test_id_start);
//		tbuser.set_name(sznick);
//		tbuser.set_url(user.pic_url());
//		tbuser.set_join_time((int)time(NULL));
//	}
//	//测试代码--end
//#endif

	if (!AddUser(user, tbdata))
	{
		ErrMsg("add user failed,uid[%ld],tbid[%ld]", user.uid(), tbdata.tbid());
		return NULL;
	}
	return &tbdata;
}


PBTeaBarData* TeaBarManager::GetTeaBar(int64 tbid)
{
	TeaBarDataMap::iterator iter = _tea_bar_map.find(tbid);
	if (iter == _tea_bar_map.end())
	{
		//查询redis数据
		return QueryDataFromRedis(tbid);
	}
	iter->second.set_last_active_time_stamp((int)time(NULL));
	if (!iter->second.has_pay_type())
	{
		iter->second.set_pay_type(EN_TeaBar_Pay_Type_Master);
	}
	return &(iter->second);
}

PBTeaBarData* TeaBarManager::QueryDataFromRedis(int64 tbid)
{
	CTeaBarRedisClient* pclient = RedisManager::Instance()->GetRedisClientByTbid(tbid);
	if (pclient == NULL)
	{
		return NULL;
	}
	char szkey[20] = { 0 };
	snprintf(szkey, sizeof(szkey), "%lld", tbid);
	PBHashDataField field;
	field.mutable_tea_bar_data();
	if (!pclient->QueryObject(szkey, field))
	{
		return NULL;
	}
	//空数据
	if (!field.mutable_tea_bar_data()->has_tbid())
	{
		return NULL;
	}
	PBTeaBarData& tbdata = _tea_bar_map[tbid];
	tbdata.CopyFrom(field.tea_bar_data());
	tbdata.set_last_active_time_stamp((int)time(NULL));
	if (!tbdata.has_pay_type())
	{
		tbdata.set_pay_type(EN_TeaBar_Pay_Type_Master);
	}
	return &tbdata;
}

bool TeaBarManager::DeleteTeaBar(int64 tbid)
{
	TeaBarDataMap::iterator iter = _tea_bar_map.find(tbid);
	if (iter == _tea_bar_map.end())
	{
		return false;
	}
	//删除所有牌局统计记录
	PBTeaBarData& tbdata = iter->second;
	for (int i = 0; i < tbdata.table_record_list_size(); i++)
	{
		const PBDateTeaBarTableRecordList& date_list = tbdata.table_record_list(i);
		for (int j = 0; j < date_list.tables_size(); j++)
		{
			const PBTeaBarTable& tbtable = date_list.tables(j);
			DeleteTableStatistics(tbtable.statistics_id());
		}
	}
	_tea_bar_map.erase(iter);
	//删除redis数据
	if (!DelteDataFromRedis(tbid))
	{
		return false;
	}
	return true;
}

bool TeaBarManager::DelteDataFromRedis(int64 tbid)
{
	CTeaBarRedisClient* pclient = RedisManager::Instance()->GetRedisClientByTbid(tbid);
	if (pclient == NULL)
	{
		return NULL;
	}
	char szkey[20] = { 0 };
	snprintf(szkey, sizeof(szkey), "%lld", tbid);
	if (!pclient->DeleteObject(szkey))
	{
		return false;
	}
	return true;
}

int64 TeaBarManager::AllocNewTeaBarID()
{
	_max_tbid++;
	CTeaBarRedisClient* pclient = RedisManager::Instance()->GetRedisClientByIndex(0);
	if (pclient != NULL)
	{
		pclient->SetMaxTbid(_max_tbid);
	}
	return _max_tbid;
}

int TeaBarManager::GetTodayZeroTimeStamp()
{
	time_t now = time(NULL);
	tm *temptm;
	temptm = localtime(&now);
	temptm->tm_hour = 0;
	temptm->tm_min = 0;
	temptm->tm_sec = 0;
	return mktime(temptm);
}

int TeaBarManager::GetWeekDay()
{
	time_t now = time(NULL);
	tm *temptm;
	temptm = localtime(&now);
	if (temptm->tm_wday == 0)
	{
		return 7;
	}
	return temptm->tm_wday;
}

void TeaBarManager::GetMonthAndDay(int& month, int& day)
{
	time_t now = time(NULL);
	tm *temptm;
	temptm = localtime(&now);
	month = temptm->tm_mon + 1;
	day = temptm->tm_mday;
}

int TeaBarManager::GetMonth(int timestamp)
{
	time_t stamp = timestamp;
	tm *temptm;
	temptm = localtime(&stamp);
	return temptm->tm_mon + 1;
}

PBDateTeaBarUserGameRecordList* TeaBarManager::GetTodayGameRecordList(PBTeaBarData& tbdata)
{
	PBDateTeaBarUserGameRecordList* ptodayrecord = NULL;
	int zerostamp = GetTodayZeroTimeStamp();
	int record_list_size = tbdata.record_list_size();
	if (record_list_size > 0)
	{
		PBDateTeaBarUserGameRecordList& record = *tbdata.mutable_record_list(record_list_size - 1);
		if (record.time_stamp() == zerostamp)
		{
			ptodayrecord = &record;
		}
	}

	if (ptodayrecord == NULL)
	{
		if (tbdata.record_list_size() >= 60)
		{
			tbdata.mutable_record_list()->DeleteSubrange(1, 1);//防止出错
		}
		ptodayrecord = tbdata.add_record_list();
		ptodayrecord->set_time_stamp(zerostamp);
		ptodayrecord->set_cost_chips(0);
		ptodayrecord->set_create_table_num(0);
	}

	//累积生成新的历史记录
	GenerateHistoryRecord(tbdata);

	return ptodayrecord;
}

PBDateTeaBarUserGameRecordList* TeaBarManager::GetGameRecordList(PBTeaBarData& tbdata, int date_type)
{
	switch (date_type)
	{
	case EN_TeaBar_Date_Type_Today:
		return GetTodayGameRecordList(tbdata);
	case EN_TeaBar_Date_Type_Yesterday:
		return GetYesterdayGameRecordList(tbdata);
	case EN_TeaBar_Date_Type_History:
		return GetHistoryGameRecordList(tbdata);
	}
	return NULL;
}

PBDateTeaBarUserGameRecordList* TeaBarManager::GetHistoryGameRecordList(PBTeaBarData& tbdata)
{
	GenerateHistoryRecord(tbdata);
	return tbdata.mutable_record_list(0);
}


PBDateTeaBarUserGameRecordList* TeaBarManager::GetYesterdayGameRecordList(PBTeaBarData& tbdata)
{
	PBDateTeaBarUserGameRecordList* pyesterdayrecord = NULL;
	int yesterzerostamp = GetTodayZeroTimeStamp() - 24*60*60;
	int record_list_size = tbdata.record_list_size();
	if (record_list_size == 0)
	{
		return NULL;
	}

	for (int i = record_list_size - 1; i >= 0; i--)
	{
		PBDateTeaBarUserGameRecordList& datelist = *tbdata.mutable_record_list(i);
		if (datelist.time_stamp() == yesterzerostamp)
		{
			pyesterdayrecord = &datelist;
			break;
		}
	}
	return pyesterdayrecord;
}

void TeaBarManager::GenerateHistoryRecord(PBTeaBarData& tbdata)
{
	int zerostamp = GetTodayZeroTimeStamp();
	int start_index = -1;
	int delete_num = 0;
	PBDateTeaBarUserGameRecordList& historylist = *tbdata.mutable_record_list(0);
	for (int i = 1; i < tbdata.record_list_size(); i++)
	{
		const PBDateTeaBarUserGameRecordList& datelist = tbdata.record_list(i);
		if (datelist.time_stamp() >= zerostamp - 24 * 60 * 60)//今天和昨天除外属于历史记录
		{
			break;
		}

		if (start_index == -1)
		{
			start_index = i;
		}
		delete_num++;
		historylist.set_cost_chips(historylist.cost_chips() + datelist.cost_chips());
		historylist.set_create_table_num(historylist.create_table_num()+ datelist.create_table_num());
		for (int j = 0; j < datelist.user_list_size(); j++)
		{
			const PBTeaBarUserGameRecord& userrecord = datelist.user_list(j);
			PBTeaBarUserGameRecord* phistoryrecord = GetUserGameRecord(userrecord.uid(), historylist);
			if (phistoryrecord == NULL)
			{
				//新增记录
				PBTeaBarUserGameRecord& newrecord = *historylist.add_user_list();
				newrecord.CopyFrom(userrecord);
			}
			else
			{
				//累积记录
				phistoryrecord->set_create_table_num(phistoryrecord->create_table_num()+userrecord.create_table_num());
				phistoryrecord->set_play_num(phistoryrecord->play_num() + userrecord.play_num());
				phistoryrecord->set_best_score_num(phistoryrecord->best_score_num() + userrecord.best_score_num());
				phistoryrecord->set_settle_num(phistoryrecord->settle_num() + userrecord.settle_num());
			}
		}
	}

	//删除淘汰数据
	if (start_index != -1)
	{
		tbdata.mutable_record_list()->DeleteSubrange(start_index, delete_num);
	}
}

PBTeaBarUserGameRecord* TeaBarManager::GetUserGameRecord(int64 uid, PBDateTeaBarUserGameRecordList& datelist)
{
	PBTeaBarUserGameRecord* puserrecord = NULL;
	for (int i = 0; i < datelist.user_list_size(); i++)
	{
		PBTeaBarUserGameRecord& record = *datelist.mutable_user_list(i);
		if (record.uid() == uid)
		{
			puserrecord = &record;
			break;
		}
	}

	if (puserrecord == NULL)
	{
		puserrecord = datelist.add_user_list();
		puserrecord->set_uid(uid);
		puserrecord->set_create_table_num(0);
		puserrecord->set_best_score_num(0);
		puserrecord->set_play_num(0);
		puserrecord->set_settle_num(0);
	}
	return puserrecord;
}

bool TeaBarManager::DeleteUserGameRecord(int64 uid, PBDateTeaBarUserGameRecordList& datelist)
{
	int find_index = -1;
	for (int i = 0; i < datelist.user_list_size(); i++)
	{
		const PBTeaBarUserGameRecord& record = datelist.user_list(i);
		if (record.uid() == uid)
		{
			find_index = i;
			break;
		}
	}

	if (find_index != -1)
	{
		datelist.mutable_user_list()->DeleteSubrange(find_index, 1);
	}
	return true;
}

void TeaBarManager::GetGameRecordList(map<int64, PBTeaBarUserGameRecord>& m, int& create_table_num, int64& cost_chips, PBTeaBarData& tbdata, int date_type)
{
	switch (date_type)
	{
	case EN_TeaBar_Date_Type_Today:
		GetTodayGameRecordList(m, create_table_num, cost_chips, tbdata);
		break;
	case EN_TeaBar_Date_Type_Yesterday:
		GetYesterdayGameRecordList(m, create_table_num, cost_chips, tbdata);
		break;
		/*case EN_TeaBar_Date_Type_Week:
			GetWeekGameRecordList(m, create_table_num, cost_chips, tbdata);
			break;
			case EN_TeaBar_Date_Type_Last_Week:
			GetLastWeekGameRecordList(m, create_table_num, cost_chips, tbdata);
			break;
			case EN_TeaBar_Date_Type_Month:
			GetMonthGameRecordList(m, create_table_num, cost_chips, tbdata);
			break;
			case EN_TeaBar_Date_Type_Last_Month:
			GetLastMonthGameRecordList(m, create_table_num, cost_chips, tbdata);
			break;*/
	case EN_TeaBar_Date_Type_History:
		GetHistoryGameRecordList(m, create_table_num, cost_chips, tbdata);
		break;
	default:
		break;
	}
}

void TeaBarManager::GetTodayGameRecordList(map<int64, PBTeaBarUserGameRecord>& m, int& create_table_num, int64& cost_chips, PBTeaBarData& tbdata)
{
	PBDateTeaBarUserGameRecordList* pdatelist = GetTodayGameRecordList(tbdata);
	if (pdatelist == NULL)
	{
		ErrMsg("precordlist == NULL,%ld", tbdata.tbid());
		return;
	}
	create_table_num = pdatelist->create_table_num();
	cost_chips = pdatelist->cost_chips();
	for (int i = 0; i < pdatelist->user_list_size(); i++)
	{
		const PBTeaBarUserGameRecord& record = pdatelist->user_list(i);
		m[record.uid()] = record;
	}
}

void TeaBarManager::GetYesterdayGameRecordList(map<int64, PBTeaBarUserGameRecord>& m, int& create_table_num, int64& cost_chips, PBTeaBarData& tbdata)
{
	int zerostamp = GetTodayZeroTimeStamp();
	int yesterdayzerostamp = zerostamp - 24 * 60 * 60;
	int record_list_size = tbdata.record_list_size();
	if (record_list_size == 0)
	{
		return;
	}
	for (int i = record_list_size - 1; i >= 0; i--)
	{
		const PBDateTeaBarUserGameRecordList& datelist = tbdata.record_list(i);
		if (datelist.time_stamp() == yesterdayzerostamp)
		{
			create_table_num = datelist.create_table_num();
			cost_chips = datelist.cost_chips();
			for (int i = 0; i < datelist.user_list_size(); i++)
			{
				const PBTeaBarUserGameRecord& record = datelist.user_list(i);
				m[record.uid()] = record;
			}
			break;
		}

		if (datelist.time_stamp() < yesterdayzerostamp)
		{
			break;
		}
	}
	GenerateHistoryRecord(tbdata);
}

void TeaBarManager::GetWeekGameRecordList(map<int64, PBTeaBarUserGameRecord>& m, int& create_table_num, int64& cost_chips, PBTeaBarData& tbdata)
{
	int weekday = GetWeekDay();
	int zerostamp = GetTodayZeroTimeStamp();
	int weekbeginstamp = zerostamp - (weekday - 1) * 24 * 60 * 60;
	int record_list_size = tbdata.record_list_size();
	if (record_list_size == 0)
	{
		return;
	}
	for (int i = record_list_size - 1; i >= 0; i--)
	{
		const PBDateTeaBarUserGameRecordList& datelist = tbdata.record_list(i);
		if (datelist.time_stamp() < weekbeginstamp)
		{
			break;
		}
		for (int i = 0; i < datelist.user_list_size(); i++)
		{
			const PBTeaBarUserGameRecord& record = datelist.user_list(i);
			map<int64, PBTeaBarUserGameRecord>::iterator iter = m.find(record.uid());
			if (iter != m.end())
			{
				PBTeaBarUserGameRecord& statistics = iter->second;
				statistics.set_best_score_num(statistics.best_score_num() + record.best_score_num());
				statistics.set_create_table_num(statistics.create_table_num() + record.create_table_num());
				statistics.set_play_num(statistics.play_num() + record.play_num());
				statistics.set_settle_num(statistics.settle_num() + record.settle_num());
			}
			else
			{
				PBTeaBarUserGameRecord& statistics = m[record.uid()];
				statistics.CopyFrom(record);
			}
		}
	}
}

void TeaBarManager::GetLastWeekGameRecordList(map<int64, PBTeaBarUserGameRecord>& m, int& create_table_num, int64& cost_chips, PBTeaBarData& tbdata)
{
	int weekday = GetWeekDay();
	int zerostamp = GetTodayZeroTimeStamp();
	int weekbeginstamp = zerostamp - (weekday - 1) * 24 * 60 * 60;
	int lastweekbeginstamp = weekbeginstamp - 7 * 24 * 60 * 60;
	int record_list_size = tbdata.record_list_size();
	if (record_list_size == 0)
	{
		return;
	}
	for (int i = record_list_size - 1; i >= 0; i--)
	{
		const PBDateTeaBarUserGameRecordList& datelist = tbdata.record_list(i);
		if (datelist.time_stamp() >= weekbeginstamp || datelist.time_stamp() < lastweekbeginstamp)
		{
			break;
		}
		for (int i = 0; i < datelist.user_list_size(); i++)
		{
			const PBTeaBarUserGameRecord& record = datelist.user_list(i);
			map<int64, PBTeaBarUserGameRecord>::iterator iter = m.find(record.uid());
			if (iter != m.end())
			{
				PBTeaBarUserGameRecord& statistics = iter->second;
				statistics.set_best_score_num(statistics.best_score_num() + record.best_score_num());
				statistics.set_create_table_num(statistics.create_table_num() + record.create_table_num());
				statistics.set_play_num(statistics.play_num() + record.play_num());
				statistics.set_settle_num(statistics.settle_num() + record.settle_num());
			}
			else
			{
				PBTeaBarUserGameRecord& statistics = m[record.uid()];
				statistics.CopyFrom(record);
			}
		}
	}
}

void TeaBarManager::GetMonthGameRecordList(map<int64, PBTeaBarUserGameRecord>& m, int& create_table_num, int64& cost_chips, PBTeaBarData& tbdata)
{
	int month = 0;
	int day = 0;
	GetMonthAndDay(month, day);
	int zerostamp = GetTodayZeroTimeStamp();
	int monthbeginstamp = zerostamp - (day - 1) * 24 * 60 * 60;
	int record_list_size = tbdata.record_list_size();
	if (record_list_size == 0)
	{
		return;
	}
	for (int i = record_list_size - 1; i >= 0; i--)
	{
		const PBDateTeaBarUserGameRecordList& datelist = tbdata.record_list(i);
		if (datelist.time_stamp() < monthbeginstamp)
		{
			break;
		}
		for (int i = 0; i < datelist.user_list_size(); i++)
		{
			const PBTeaBarUserGameRecord& record = datelist.user_list(i);
			map<int64, PBTeaBarUserGameRecord>::iterator iter = m.find(record.uid());
			if (iter != m.end())
			{
				PBTeaBarUserGameRecord& statistics = iter->second;
				statistics.set_best_score_num(statistics.best_score_num() + record.best_score_num());
				statistics.set_create_table_num(statistics.create_table_num() + record.create_table_num());
				statistics.set_play_num(statistics.play_num() + record.play_num());
				statistics.set_settle_num(statistics.settle_num() + record.settle_num());
			}
			else
			{
				PBTeaBarUserGameRecord& statistics = m[record.uid()];
				statistics.CopyFrom(record);
			}
		}
	}
}

void TeaBarManager::GetLastMonthGameRecordList(map<int64, PBTeaBarUserGameRecord>& m, int& create_table_num, int64& cost_chips, PBTeaBarData& tbdata)
{
	int month = 0;
	int day = 0;
	int lastmonth = 0;
	GetMonthAndDay(month, day);
	if (month == 1)
	{
		lastmonth = 12;
	}
	else
	{
		lastmonth = month - 1;
	}
	int zerostamp = GetTodayZeroTimeStamp();
	int monthbeginstamp = zerostamp - (day - 1) * 24 * 60 * 60;
	int record_list_size = tbdata.record_list_size();
	for (int i = 0; i < record_list_size; i++)
	{
		const PBDateTeaBarUserGameRecordList& datelist = tbdata.record_list(i);
		if(datelist.time_stamp() >= monthbeginstamp)
		{
			break;
		}

		int mon = GetMonth(datelist.time_stamp());
		if (mon == lastmonth)
		{
			for (int i = 0; i < datelist.user_list_size(); i++)
			{
				const PBTeaBarUserGameRecord& record = datelist.user_list(i);
				map<int64, PBTeaBarUserGameRecord>::iterator iter = m.find(record.uid());
				if (iter != m.end())
				{
					PBTeaBarUserGameRecord& statistics = iter->second;
					statistics.set_best_score_num(statistics.best_score_num() + record.best_score_num());
					statistics.set_create_table_num(statistics.create_table_num() + record.create_table_num());
					statistics.set_play_num(statistics.play_num() + record.play_num());
					statistics.set_settle_num(statistics.settle_num() + record.settle_num());
				}
				else
				{
					PBTeaBarUserGameRecord& statistics = m[record.uid()];
					statistics.CopyFrom(record);
				}
			}
		}
	}
}

void TeaBarManager::GetHistoryGameRecordList(map<int64, PBTeaBarUserGameRecord>& m, int& create_table_num, int64& cost_chips, PBTeaBarData& tbdata)
{
	//累积生成最新的历史记录
	GenerateHistoryRecord(tbdata);

	const PBDateTeaBarUserGameRecordList& historylist = tbdata.record_list(0);
	create_table_num = historylist.create_table_num();
	cost_chips = historylist.cost_chips();
	for (int i = 0; i < historylist.user_list_size(); i++)
	{
		const PBTeaBarUserGameRecord& record = historylist.user_list(i);
		m[record.uid()] = record;
	}
}


void TeaBarManager::RefreshTableRecordList(PBTeaBarData& tbdata)
{
	int todayzerotime = GetTodayZeroTimeStamp();
	int start_index = -1;
	int delete_num = 0;
	vector<int> vec;
	for (int i = 0; i < tbdata.table_record_list_size(); i++)
	{
		const PBDateTeaBarTableRecordList& table_list = tbdata.table_record_list(i);
		if (table_list.time_stamp() >= todayzerotime - 24 * 60 * 60)//保留昨天和今天的桌子记录
		{
			break;
		}
		if (start_index == -1)
		{
			start_index = i;
		}
		delete_num++;
		for (int k = 0; k < table_list.tables_size(); k++)
		{
			const PBTeaBarTable& tbtable = table_list.tables(k);
			if (!DeleteTableStatistics(tbtable.statistics_id()))
			{
				ErrMsg("DeleteTableStatisticsFromRedis error, %ld", tbtable.statistics_id());
			}
		}
	}

	if (start_index != -1)
	{
		tbdata.mutable_table_record_list()->DeleteSubrange(start_index, delete_num);
	}
}

PBDateTeaBarTableRecordList* TeaBarManager::GetTodayTableRecordList(PBTeaBarData& tbdata)
{
	PBDateTeaBarTableRecordList* precordlist = NULL;
	RefreshTableRecordList(tbdata);
	int todayzerotime = GetTodayZeroTimeStamp();
	for (int i = 0; i < tbdata.table_record_list_size(); i++)
	{
		PBDateTeaBarTableRecordList& table_list = *tbdata.mutable_table_record_list(i);
		if (table_list.time_stamp() == todayzerotime)
		{
			precordlist = &table_list;
			break;
		}
	}

	if (precordlist == NULL)
	{
		//新增今日桌子记录
		precordlist = tbdata.add_table_record_list();
		precordlist->set_time_stamp(todayzerotime);
	}
	return precordlist;
}


bool TeaBarManager::UpdateTableStatisticsToRedis(int64 statistics_id, PBTeaBarTableDetailStatistics& statistics)
{
	CTeaBarRedisClient* pclient = RedisManager::Instance()->GetRedisClientByStatid(statistics_id);
	if (pclient == NULL)
	{
		return false;
	}
	PBHashDataField field;
	field.mutable_tea_bar_table_detail_statistics()->CopyFrom(statistics);
	char szkey[100] = { 0 };
	snprintf(szkey, sizeof(szkey), "%lld", statistics_id);
	if (!pclient->UpdateObject(szkey, field))
	{
		return false;
	}
	return true;
}

PBTeaBarTableDetailStatistics* TeaBarManager::GetTableStatisticsFromRedis(int64 statistics_id)
{
	CTeaBarRedisClient* pclient = RedisManager::Instance()->GetRedisClientByStatid(statistics_id);
	if (pclient == NULL)
	{
		return NULL;
	}
	PBHashDataField field;
	field.mutable_tea_bar_table_detail_statistics();
	char szkey[100] = { 0 };
	snprintf(szkey, sizeof(szkey), "%lld", statistics_id);
	if (!pclient->QueryObject(szkey, field))
	{
		return NULL;
	}
	if (!field.tea_bar_table_detail_statistics().has_statistics())
	{
		return NULL;
	}
	PBTeaBarTableDetailStatistics& statistics = _tea_bar_table_stat_map[statistics_id];
	statistics.CopyFrom(field.tea_bar_table_detail_statistics());
	return &statistics;
}

bool TeaBarManager::DeleteTableStatisticsFromRedis(int64 statistics_id)
{
	CTeaBarRedisClient* pclient = RedisManager::Instance()->GetRedisClientByStatid(statistics_id);
	if (pclient == NULL)
	{
		return NULL;
	}
	char szkey[100] = { 0 };
	snprintf(szkey, sizeof(szkey), "%lld", statistics_id);
	if (!pclient->DeleteObject(szkey))
	{
		return false;
	}
	return true;
}

PBTeaBarTableDetailStatistics* TeaBarManager::GetTableStatistics(int64 statistics_id)
{
	TeaBarTableDetailStatisticsMap::iterator iter = _tea_bar_table_stat_map.find(statistics_id);
	if (iter == _tea_bar_table_stat_map.end())
	{
		return GetTableStatisticsFromRedis(statistics_id);
	}
	return &(iter->second);
}

bool TeaBarManager::DeleteTableStatistics(int64 statistics_id)
{
	TeaBarTableDetailStatisticsMap::iterator iter = _tea_bar_table_stat_map.find(statistics_id);
	if (iter != _tea_bar_table_stat_map.end())
	{
		_tea_bar_table_stat_map.erase(iter);
	}
	//删除redis数据
	if (!DeleteTableStatisticsFromRedis(statistics_id))
	{
		return false;
	}
	return true;
}

bool TeaBarManager::AddTableStatistics(int64 statistics_id, PBTeaBarTableDetailStatistics& statistics)
{
	TeaBarTableDetailStatisticsMap::iterator iter = _tea_bar_table_stat_map.find(statistics_id);
	if (iter != _tea_bar_table_stat_map.end())
	{
		return false;
	}
	_tea_bar_table_stat_map[statistics_id] = statistics;

	if (!UpdateTableStatisticsToRedis(statistics_id, statistics))
	{
		ErrMsg("UpdateTableStatisticsToRedis error, %lld", statistics_id);
		return false;
	}
	return true;
}

void TeaBarManager::GetAllTableStatistics(vector<PBTeaBarTable>& vec, PBTeaBarData& tbdata)
{
	RefreshTableRecordList(tbdata);
	int size = tbdata.table_record_list_size();
	for (int i = size - 1; i >= 0; i--)
	{
		PBDateTeaBarTableRecordList& date_table_list = *tbdata.mutable_table_record_list(i);
		int table_size = date_table_list.tables_size();
		for (int k = table_size - 1; k >= 0; k--)
		{
			PBTeaBarTable& tbtable = *date_table_list.mutable_tables(k);
			vec.push_back(tbtable);
		}
	}
}

void TeaBarManager::GetSortTables(PBTeaBarData& tbdata, vector<PBTeaBarTable>& vec)
{
	vector<PBTeaBarTable> tmp;
	for (int i = 0; i < tbdata.tables_size(); i++)
	{
		PBTeaBarTable& tbtable = *tbdata.mutable_tables(i);
		//开局或者人满的房间往后排
		if (tbtable.if_start() || tbtable.player_num() == tbtable.conf().seat_num())
		{
			tmp.push_back(tbtable);
		}
		else
		{
			vec.push_back(tbtable);
		}
	}
	vec.insert(vec.end(), tmp.begin(), tmp.end());
}

void TeaBarManager::KnockOutTeaBarData(int64 tbid)
{
	TeaBarDataMap::iterator iter = _tea_bar_map.find(tbid);
	if (iter == _tea_bar_map.end())
	{
		return;
	}

	PBTeaBarData& tbdata = iter->second;
	//淘汰内存中的统计记录
	for (int i = 0; i < tbdata.table_record_list_size(); i++)
	{
		const PBDateTeaBarTableRecordList& date_list = tbdata.table_record_list(i);
		for (int j = 0; j < date_list.tables_size(); j++)
		{
			const PBTeaBarTable& tbtable = date_list.tables(j);
			TeaBarTableDetailStatisticsMap::iterator it = _tea_bar_table_stat_map.find(tbtable.statistics_id());
			if (it != _tea_bar_table_stat_map.end())
			{
				_tea_bar_table_stat_map.erase(it);
			}
			VLogMsg(CLIB_LOG_LEV_DEBUG, "knock out statistics, [%ld]", tbtable.statistics_id());
		}
	}
	//淘汰内存中的茶馆数据
	_tea_bar_map.erase(iter);
	VLogMsg(CLIB_LOG_LEV_DEBUG, "knock out data, [%ld]", tbdata.tbid());
}