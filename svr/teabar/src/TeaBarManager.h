#pragma once
#include "singleton.h"
#include <map>
#include <string>
#include "poker_msg.pb.h"
#include "poker_data.pb.h"
#include "global.h"
#include "type_def.h"
#include "Timer_Handler_Base.h"
#include "singleton.h"
#include <vector>
#include "RedisManager.h"

const int MAX_TEA_BAR_USER_NUM = 500;
const int MAX_TEA_BAR_APPLY_USER_NUM = 100;
const int MAX_USER_TEA_BAR_MUM = 5;

using namespace std;

class CSocketHandler;
class CSession;

typedef map<int64, PBTeaBarData> TeaBarDataMap;
typedef map<string,int> TeaBarNameMap;
typedef map<int64, PBTeaBarTableDetailStatistics> TeaBarTableDetailStatisticsMap;

#define LRU_TIMER 1000

class TeaBarManager : public CTimerOutListener, public CSingleton<TeaBarManager>
{
public:
	TeaBarManager() {}
	~TeaBarManager() {}
	bool Init();
	
	virtual int ProcessOnTimerOut(int Timerid);

	bool AddUser(const PBUser& user, PBTeaBarData& tbdata);
	bool DeleteUser(int64 uid, PBTeaBarData& tbdata);
	PBTeaBarUser* GetUser(int64 uid, PBTeaBarData& tbdata);

	PBTeaBarData* CreateTeaBar(const PBUser& user, const string& tbname, const string& desc);
	PBTeaBarData* GetTeaBar(int64 tbid);
	bool DeleteTeaBar(int64 tbid);

	int64 AllocNewTeaBarID();

	PBDateTeaBarUserGameRecordList* GetGameRecordList(PBTeaBarData& tbdata, int date_type);
	
	PBTeaBarUserGameRecord* GetUserGameRecord(int64 uid, PBDateTeaBarUserGameRecordList& datelist);
	bool DeleteUserGameRecord(int64 uid, PBDateTeaBarUserGameRecordList& datelist);

	void GetGameRecordList(map<int64, PBTeaBarUserGameRecord>& m, int& create_table_num, int64& cost_chips, PBTeaBarData& tbdata, int date_type);

	int GetTodayZeroTimeStamp();
	int GetWeekDay();
	void GetMonthAndDay(int& month, int& day);
	int GetMonth(int timestamp);

	//����redis����
	bool UpdateDataToRedis(PBTeaBarData& tbdata);
	//ɾ��redis����
	bool DelteDataFromRedis(int64 tbid);
	//��ѯredis����
	PBTeaBarData* QueryDataFromRedis(int64 tbid);

	//ˢ�����Ӽ�¼
	void RefreshTableRecordList(PBTeaBarData& tbdata);

	//��ȡ�������Ӽ�¼
	PBDateTeaBarTableRecordList* GetTodayTableRecordList(PBTeaBarData& tbdata);

	//�������Ӵ�����¼��redis
	bool UpdateTableStatisticsToRedis(int64 statistics_id, PBTeaBarTableDetailStatistics& statistics);
	//��ȡ���Ӵ�����¼��redis
	PBTeaBarTableDetailStatistics* GetTableStatisticsFromRedis(int64 statistics_id);
	//ɾ��ͳ�����Ӽ�¼
	bool DeleteTableStatisticsFromRedis(int64 statistics_id);

	//��ȡ����ͳ�Ƽ�¼
	PBTeaBarTableDetailStatistics* GetTableStatistics(int64 statistics_id);
	//ɾ������ͳ�Ƽ�¼
	bool DeleteTableStatistics(int64 statistics_id);
	//�������ͳ�Ƽ�¼
	bool AddTableStatistics(int64 statistics_id, PBTeaBarTableDetailStatistics& statistics);
	//��ȡ����ͳ�����Ӽ�¼
	void GetAllTableStatistics(vector<PBTeaBarTable>& vec, PBTeaBarData& tbdata);
	//��ȡ����������б�
	void GetSortTables(PBTeaBarData& tbdata, vector<PBTeaBarTable>& vec);
private:
	bool InitMaxTbid();
	void GetTodayGameRecordList(map<int64, PBTeaBarUserGameRecord>& m, int& create_table_num, int64& cost_chips, PBTeaBarData& tbdata);
	void GetYesterdayGameRecordList(map<int64, PBTeaBarUserGameRecord>& m, int& create_table_num, int64& cost_chips, PBTeaBarData& tbdata);
	void GetWeekGameRecordList(map<int64, PBTeaBarUserGameRecord>& m, int& create_table_num, int64& cost_chips, PBTeaBarData& tbdata);
	void GetLastWeekGameRecordList(map<int64, PBTeaBarUserGameRecord>& m, int& create_table_num, int64& cost_chips, PBTeaBarData& tbdata);
	void GetMonthGameRecordList(map<int64, PBTeaBarUserGameRecord>& m, int& create_table_num, int64& cost_chips, PBTeaBarData& tbdata);
	void GetLastMonthGameRecordList(map<int64, PBTeaBarUserGameRecord>& m, int& create_table_num, int64& cost_chips, PBTeaBarData& tbdata);
	void GetHistoryGameRecordList(map<int64, PBTeaBarUserGameRecord>& m, int& create_table_num, int64& cost_chips, PBTeaBarData& tbdata);
	void GenerateHistoryRecord(PBTeaBarData& tbdata);
	PBDateTeaBarUserGameRecordList* GetTodayGameRecordList(PBTeaBarData& tbdata);
	PBDateTeaBarUserGameRecordList* GetYesterdayGameRecordList(PBTeaBarData& tbdata);
	PBDateTeaBarUserGameRecordList* GetHistoryGameRecordList(PBTeaBarData& tbdata);
	void KnockOutTeaBarData(int64 tbid);
private:
	TeaBarDataMap _tea_bar_map;
	TeaBarNameMap _tea_bar_name_map;
	int64 _max_tbid;
	TeaBarTableDetailStatisticsMap _tea_bar_table_stat_map;
public:
	CTimer _lru_timer;
};

