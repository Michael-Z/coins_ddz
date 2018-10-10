#include "QuickMatchManager.h"
#include <map>
#include <sstream>
#include <algorithm>
#include "clib_time.h"
#include "ActivityRedisClient.h"
using namespace std;

const char* kQuickMatchTimeStampKey = "QUICKMATCH_CONFIG";
const char* kQuickMatchStartTimeStampKey = "STARTTIMESTAMP";
const char* kQuickMatchStopTimeStampKey = "STOPTIMESTAMP";

QuickMatchManager::QuickMatchManager()
{

}

QuickMatchManager::~QuickMatchManager()
{

}

int QuickMatchManager::_GetQuickMatchCostByTtype(int match_session_id,int& cost)
{
    cost = -1;
    char phpMsg[1024];
    if(!ActivityRedisClient::Instance()->GetHashMapValueForMatch(kQuickMatchTimeStampKey, match_session_id, phpMsg))
    {
        return -1;
    }

    std::string json_msg = phpMsg;
    Json::Value root;
    Json::Reader reader;

    if (!reader.parse(json_msg, root))
        return -1;

    cost = root["COST"].asInt();
    return 0;
}

int QuickMatchManager::_GetQuickMatchTtype(int match_session_id)
{
    char phpMsg[1024];
    if(!ActivityRedisClient::Instance()->GetHashMapValueForMatch(kQuickMatchTimeStampKey, match_session_id, phpMsg))
    {
        return -1;
    }

    std::string json_msg = phpMsg;
    Json::Value root;
    Json::Reader reader;

    if (!reader.parse(json_msg, root))
        return -1;

    int ttype = root["MATCH_TABLE_TYPE"].asInt();

    return ttype;
}

int64 QuickMatchManager::_GetQuickMatchTimeByTtype(int ttype,int64& starttimestamp, int64& stoptimestamp,
                                                   int64& starttimestamp_day, int64& stoptimestamp_day,int& match_session)
{
    char phpMsg[1024];
    if(!ActivityRedisClient::Instance()->GetHashMapValueForMatch(kQuickMatchTimeStampKey, match_session, phpMsg))
    {
        return false;
    }

    std::string json_msg = phpMsg;
    Json::Value root;
    Json::Reader reader;

    if (!reader.parse(json_msg, root))
        return false;

    starttimestamp = root["STARTTIMESTAMP"].asInt64();
    stoptimestamp = root["STOPTIMESTAMP"].asInt64();

    int64 lingcheng_for_today;
    {
        time_t t = time(NULL);
        struct tm *tm = localtime(&t);
        tm->tm_hour = 0;
        tm->tm_min = 0;
        tm->tm_sec = 0;
        lingcheng_for_today = mktime(tm);
    }

    starttimestamp_day = (root["STARTTIMESTAMP_DAY"].asInt64() * 60) + lingcheng_for_today;
    stoptimestamp_day = (root["STOPTIMESTAMP_DAY"].asInt64() * 60) + lingcheng_for_today;

    return true;
}

bool QuickMatchManager::_CheckIsInMatchTimeForTtype(int ttype, int match_session)
{
    int64 start_time = 0;
    int64 stop_time = 0;
    int64 start_time_day = 0;
    int64 stop_time_day = 0;

    if(!_GetQuickMatchTimeByTtype(ttype,start_time,stop_time,start_time_day,stop_time_day,match_session))
    {
        return false;
    }

    if(start_time == 0 || stop_time == 0)
    {
        return false;
    }

    int64 current_time = time(NULL);
    if(current_time < start_time || current_time > stop_time)
    {
        VLogMsg(CLIB_LOG_LEV_DEBUG,"is not match time current_time[%lld] start_time[%lld] stop_time[%lld]",current_time,start_time,stop_time);
        return false;
    }

    if(current_time < start_time_day || current_time > stop_time_day)
    {
        VLogMsg(CLIB_LOG_LEV_DEBUG,"is not match time current_time[%lld] start_time_day[%lld] stop_time_day[%lld]",current_time,start_time_day,stop_time_day);
        return false;
    }

    return true;
}

//玩家是否在比赛中 包含所有比赛
void QuickMatchManager::_WriteUserMatchState(long long uid, int user_state)
{
    ActivityRedisClient::Instance()->WriteUserMatchState(uid,user_state);
}

//检查是否比赛时间已经结束
bool QuickMatchManager::_CheckIsAfterMatchTimeForTtype(int ttype, int match_session)
{
    int64 start_time = 0;
    int64 stop_time = 0;
    int64 start_time_day = 0;
    int64 stop_time_day = 0;

    if(!_GetQuickMatchTimeByTtype(ttype,start_time,stop_time,start_time_day,stop_time_day,match_session))
    {
        return false;
    }

    if(start_time == 0 || stop_time == 0)
    {
        return false;
    }

    int64 current_time = time(NULL);
    if(current_time > stop_time)
    {
        VLogMsg(CLIB_LOG_LEV_DEBUG,"is not match time current_time[%lld] start_time[%lld] stop_time[%lld]",current_time,start_time,stop_time);
        return true;
    }

    if(current_time > stop_time_day)
    {
        VLogMsg(CLIB_LOG_LEV_DEBUG,"is not match time current_time[%lld] start_time_day[%lld] stop_time_day[%lld]",current_time,start_time_day,stop_time_day);
        return true;
    }

    return false;
}

void QuickMatchManager::_WriteUserMatchInfo(int match_session_id, long long uid, int user_state)
{
    ActivityRedisClient::Instance()->WriteUserMatchInfo(match_session_id,uid,user_state);
}


int QuickMatchManager::_GetUserMatchInfo(int match_session_id, long long uid)
{
    return ActivityRedisClient::Instance()->GetUserMatchInfo(match_session_id,uid);
}
//检查所有比赛是否有在进行中的 托管也算
int QuickMatchManager::_GetUserMatchState(long long uid)
{
    return ActivityRedisClient::Instance()->GetUserMatchState(uid);
}
