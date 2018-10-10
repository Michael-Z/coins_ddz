#include "clib_time.h"

const int kSecondsOfDay = 86400;
const int kSecondsOfWeek = 604800;

time_t CTimeHelper::GetCurrentDayStartStamp()
{
    return GetStampOnClock(0, time(NULL));
}

time_t CTimeHelper::GetDayStartStampByStamp(int stamp)
{
    return GetStampOnClock(0, stamp);
}

bool CTimeHelper::IsInSameWeekBeginOfMon(time_t dest_timestamp, int current_timestamp)
{
    struct tm * p_tm_of_dest_timestamp = localtime(&dest_timestamp);
    int day_of_week = p_tm_of_dest_timestamp->tm_wday;
    day_of_week = day_of_week == 0 ? 7 : day_of_week;
    const int begin_stamp_of_week_of_dest_timestamp = GetDayStartStampByStamp(dest_timestamp) - (day_of_week - 1) * kSecondsOfDay;

    return current_timestamp - begin_stamp_of_week_of_dest_timestamp < kSecondsOfWeek;
}

time_t CTimeHelper::GetStampOnClock(int clock, time_t stamp)
{
    struct tm * p_tm_of_stamp = localtime(&stamp);
    struct tm tm_of_clock;
    SetClockForTm(p_tm_of_stamp, &tm_of_clock, clock);
    return mktime(&tm_of_clock);
}

void CTimeHelper::SetClockForTm(struct tm * tm_in, struct tm * tm_out, int clock)
{
    tm_out->tm_sec = 0;
    tm_out->tm_min = 0;
    tm_out->tm_hour = clock;
    tm_out->tm_mday = tm_in->tm_mday;
    tm_out->tm_mon = tm_in->tm_mon;
    tm_out->tm_year = tm_in->tm_year;
    tm_out->tm_wday = tm_in->tm_wday;
    tm_out->tm_yday = tm_in->tm_yday;
    tm_out->tm_isdst = tm_in->tm_isdst;
    tm_out->tm_gmtoff = tm_in->tm_gmtoff;
}

int CTimeHelper::GetDiffDay(time_t stamp_left, time_t stamp_right)
{
    int abs_diff = abs(stamp_right - stamp_left);
    return abs_diff / (24 * 60 * 60);
}