#include "CostManager.h"

#include <map>
#include <algorithm>

#include "clib_time.h"
#include "ActivityRedisClient.h"

using namespace std;

const char* kSdrDiscountTable = "DISCOUNT_INFO";
const char* kDiscountTimeStampKey = "DISCOUNT_TIMESTAMP";
const char* kDiscountInfo = "DISCOUNT_INFO_%d_%d"; /*DISCOUNT_INFO_tabletype_round*/
const char* kDiscountStartTime = "DISCOUNT_START_TIME";
const char* kDiscountEndTime = "DISCOUNT_END_TIME";
const char* kDiscountStartMin = "DISCOUNT_START_MIN";
const char* kDiscountEndMin = "DISCOUNT_END_MIN";

CostManager::CostManager()
{
}

CostManager::~CostManager()
{
}

bool CostManager::Init()
{
    int64 discount_timestamp = _GetDiscountTimestamp();
    last_timestamp_ = discount_timestamp;
    _UpdateDiscountTimestamp();
    return true;
}

bool CostManager::_NeedUpdate()
{
    return last_timestamp_ != _GetDiscountTimestamp();
}

int64 CostManager::_GetDiscountTimestamp()
{
    int64 discount_timestamp = -1;
    ActivityRedisClient::Instance()->GetValueOfKey(kDiscountTimeStampKey, discount_timestamp);
    return discount_timestamp;
}

void CostManager::_UpdateDiscountTimestamp()
{
    ActivityRedisClient::Instance()->GetHashMapValue(kSdrDiscountTable, kDiscountStartTime, start_timestamp_);
    ActivityRedisClient::Instance()->GetHashMapValue(kSdrDiscountTable, kDiscountEndTime, end_timestamp_);
    ActivityRedisClient::Instance()->GetHashMapValue(kSdrDiscountTable, kDiscountStartMin, start_min_);
    ActivityRedisClient::Instance()->GetHashMapValue(kSdrDiscountTable, kDiscountEndMin, end_min_);
}

bool CostManager::_IsInDiscountTime()
{
    int64 cur_timestamp = time(0);
    if (cur_timestamp > start_timestamp_ && cur_timestamp < end_timestamp_)
    {
        int64 start_min_of_today = CTimeHelper::GetCurrentDayStartStamp() + start_min_ * 60;
        int64 end_min_of_today = CTimeHelper::GetCurrentDayStartStamp() + end_min_ * 60;
        if (cur_timestamp > start_min_of_today && cur_timestamp < end_min_of_today)
        {
            return true;
        }
    }
    return false;
}

void CostManager::_UpdateLastTimestamp()
{
    last_timestamp_ = _GetDiscountTimestamp();
}

int CostManager::_GetDiscountVal(int table_type, int round)
{
    int64 discount_val = 0;
    snprintf(discount_val_key_, sizeof(discount_val_key_), kDiscountInfo, table_type, round);
    //如果没有找到，说明不做活动，直接返回100
    if (!ActivityRedisClient::Instance()->GetHashMapValue(kSdrDiscountTable, discount_val_key_, discount_val))
    {
        return 100;
    }
    return discount_val;
}

int CostManager::GetDiscountVal(int table_type, int round)
{
    TableAndRoundPair key = make_pair(table_type, round);
    if (_NeedUpdate())
    {
        _UpdateLastTimestamp();
        _UpdateDiscountTimestamp();

        //不在活动时间内，直接返回100
        if (!_IsInDiscountTime())
        {
            return 100;
        }

        int64 discount_val = _GetDiscountVal(table_type, round);
        //table_2_discount_map_.erase(table_2_discount_map_.begin(), table_2_discount_map_.end());
        Table2DiscountMap::iterator it = table_2_discount_map_.find(key);
        if (it != table_2_discount_map_.end())
        {
            it->second = discount_val;
        }
        else
        {
            table_2_discount_map_.insert(make_pair(key, discount_val));
        }
        updated_vec.erase(updated_vec.begin(), updated_vec.end());
        updated_vec.push_back(key);
        return discount_val;
    }

    //不需要更新，也先确认是否在活动时间内
    if (!_IsInDiscountTime())
    {
        return 100;
    }

    //没有在updated_vec中找到说明从上次更新到现在，该游戏类型的房间没有玩家创建过，则需重新去redis中取值
    if (find(updated_vec.begin(), updated_vec.end(), key) == updated_vec.end())
    {
        int64 discount_val = _GetDiscountVal(table_type, round);
        Table2DiscountMap::iterator it = table_2_discount_map_.find(key);
        if (it != table_2_discount_map_.end())
        {
            it->second = discount_val;
        }
        else
        {
            table_2_discount_map_.insert(make_pair(key, discount_val));
        }
        updated_vec.push_back(pair<int, int>(table_type, round));
        return discount_val;
    }
    //找到了，则说明从上次更新到现在，该类型的玩法已经有玩家创建、并且更新过，直接从Table2DiscountMap中查找就行
    else
    {
        Table2DiscountMap::iterator it = table_2_discount_map_.find(key);
        if (it != table_2_discount_map_.end())
        {
            return it->second;
        }
        //理论上不会进这个分支
        else
        {
            return _GetDiscountVal(table_type, round);
        }
    }

    return 0;
}
