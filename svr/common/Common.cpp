#include "Common.h"
using google::protobuf::RepeatedField;

int Common::GetDiffDays(time_t time1, time_t time2)
{
    if (time1 < time2) std::swap(time1, time2);

    struct tm date1, date2;
    localtime_r(&time1, &date1);
    localtime_r(&time2, &date2);
    return (date1.tm_year - date2.tm_year) *365 + (date1.tm_yday - date2.tm_yday);
}

int Common::GetSecOfToday()
{
	time_t tnow = GetTime();
	struct tm *p = localtime(&tnow);

	return p->tm_hour* 3600 + p->tm_min* 60 + p->tm_sec;
}

bool Common::IsDiffWeek(time_t time1, time_t time2)
{
    if (time1 < time2) std::swap(time1, time2);

    struct tm date1, date2;
    localtime_r(&time1, &date1);
    localtime_r(&time2, &date2);

    int wday1 = date1.tm_wday == 0 ? 7 : date1.tm_wday;
    int wday2 = date2.tm_wday == 0 ? 7 : date2.tm_wday;

    if (((date1.tm_year - date2.tm_year)*365 + (date1.tm_yday - date2.tm_yday)) >= 7)
    {
        return true;
    }
    else if (wday1 < wday2)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool Common::IsDiffWeekGMT(time_t time1, time_t time2)
{
    time_t t1 = time1 + 3 *24 *60 *60;
    time_t t2 = time2 + 3 *24 *60 *60;
    int week1 = t1 /(7 *24 *60 *60);
    int week2 = t2 /(7 *24 *60 *60);
    if (week1 != week2) return true;
    return false;
}

int Common::GetNaturalDayTime(time_t time, int day)
{
    struct tm date;
    localtime_r(&time, &date);

    date.tm_hour = 0;
    date.tm_min = 0;
    date.tm_sec = 0;
    date.tm_mday += (day + 1);

    return mktime(&date) - 1;
}

void Common::MergeUserData(PBUserData& des, const PBUserData& src)
{
    if (src.has_user_info())    des.mutable_user_info()->CopyFrom(src.user_info());
	if (src.has_user_record())  des.mutable_user_record()->CopyFrom(src.user_record());
	if (src.has_user_table_info())  des.mutable_user_table_info()->CopyFrom(src.user_table_info());
	if (src.has_user_tea_bar_data()) des.mutable_user_tea_bar_data()->CopyFrom(src.user_tea_bar_data());
    if (src.has_user_game_data()) des.mutable_user_game_data()->CopyFrom(src.user_game_data());
}

int64 Common::GenerateNewtableID(int64 uid)
{
	return uid;
}
