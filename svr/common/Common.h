#pragma once
#include <ctime>
#include <algorithm>
#include "Session.h"
#include "PBConfigBasic.h"
#include "poker_data.pb.h"
using google::protobuf::RepeatedPtrField;

extern time_t g_time;

class Common
{
public:
    // 时间
    static time_t GetTime() { return g_time; }
    static int GetDiffDays(time_t time1, time_t time2);
    static int GetSecOfToday();
    static bool IsDiffWeek(time_t time1, time_t time2);
    static bool IsDiffWeekGMT(time_t time1, time_t time2);
    static int GetNaturalDayTime(time_t now, int day);
    // 合并玩家数据
    static void MergeUserData(PBUserData& des, const PBUserData& src);

	//gen new table id 
	static int64 GenerateNewtableID(int64 uid);
};
