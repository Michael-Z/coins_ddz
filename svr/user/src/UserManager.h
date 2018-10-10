#pragma once
#include "singleton.h"
#include <map>
#include <string>
#include "poker_msg.pb.h"
#include "poker_data.pb.h"
#include "global.h"
#include "type_def.h"
#include "Timer_Handler_Base.h"

using namespace std;

class CSocketHandler;
class CSession;

#define LRU_TIMER 1000

class UserManager : public CTimerOutListener
{
    typedef map<string, int64> AccountMap;
    typedef map<string, int> AccountLruMap;
    typedef map<int64, PBUserData> UserDataMap;
    typedef map<int64, int> UserDataLruMap;
public:
    UserManager() { _accountmap.clear(); _userdatamap.clear(); _account_lru_map.clear(); _userdata_lru_map.clear(); }
    static UserManager * Instance(void);
    void Init();
    void OnUserLogin(const string & account, const PBUserData & user_data);

    PBUser* GetUserInfo(long long uid);
    PBUserRecord * GetUserRecord(long long uid);
    PBUserTableInfo * GetUserTableInfo(long long uid);
    PBUserTeaBarData* GetUserTeaBarData(long long uid);
    void UpdateUserData(long long uid, const PBUserData& user_data);
    PBUserData* GetUserData(long long uid);
    PBUserData* GetUserDataByAcc(string account);
    ENMessageError OnParseUserData(const PBDataSet& data_set);

    void UpdateUserLastWeekGameInfo(long long uid, const PBGameRecord& game_record, PBUserRecord* user_record);

    bool OnUpdateUserInfo(int64 uid, const PBUpdateData& update_key, PBRedisData& redis_data);
    bool OnUpdateUserRecord(int64 uid, const PBUpdateData & update_key, PBRedisData & redis_data);
    bool OnUpdateUserTableInfo(int64 uid, const PBUpdateData & update_key, PBRedisData & redis_data);
    bool OnUpdateUserTeaBarData(int64 uid, const PBUpdateData& update_key, PBRedisData& redis_data);
    void SendChipJournalLog(const PBUser& user_info, long long act_num, int reason);
    void SendChipChangeMsg(const PBUser& user_info);
    void SendUpdateRankMsg(int rank_id, long long rank_key, long long rank_score);
    virtual int ProcessOnTimerOut(int Timerid);
    void SendTeaBarChipLog(int64 uid, TeaBarBriefData& brief, const TeaBarChipsFlow& flow);
    void SendUserRemoveOutTeaBar(int64 uid, const TeaBarBriefData& brief);
    void SendDiamondChangeMsg(const PBUser& user_info);
    void SendDiamondFlowLog(const PBUser& user_info, long long act_num, int reason);
    void SendBonusChangeMsg(const PBUser& user_info);
    void SendBonusFlowLog(const PBUser& user_info, float act_num, int reason);
    void SendToTalBonusLog(const PBUser& pbUserInfo, long long a_iActNum, int a_iReason);

    /*
     * 修改用户游戏数据
    */
    bool OnUpdateUserGameData(int64 a_iUid, const PBUpdateData& a_pbUpdateKey, PBRedisData& a_pbRedisData);

    /*
     * 获得用户的游戏数据
     * 返回值为指针
    */
    PBUserGameData * GetUserGameData(long long a_iUid);

    /*
     * 初始化某个闯关游戏的记录
    */
    bool InitSkipMatchInfoItemByType(PBUserSkipMatchInfoItem * a_ppbInfoItem,int a_iSkipmatchType);

    /*
     * 获得某个游戏的闯关信息
     * 从中的闯关信息中查询
    */
    PBUserSkipMatchInfoItem * GetItemFromSkipMatchInfo(int a_iGameType,PBUserSkipMatchInfo & a_pbUserSkipMatcahInfo);

    /*
     * 尝试增加游戏的次数
    */
    void TryAddSkipMatchGameNum(PBUserSkipMatchInfoItem * a_ppbInfoItem,const PBDBAtomicField & a_pbFieldInfo);

    /*
     * 发送闯关状态变更
    */
    void SendSkipMatchLevelAndStateFlowLog(const PBUser & user_info,const PBUserSkipMatchInfoItem & a_pbUserSkipInfoItem,int a_iLevelActNum,int a_iReason,int a_iTableType);

    /*
     * 更新结果操作流
    */
    void UpdateUserSkipMatchResultFlow(PBUserSkipMatchInfoItem & a_pbUserSkipInfoItem,int64 a_iSessionId,int a_iLevelChangeVal,int a_iState
                                                    ,int a_iReason);
    /*
     * 删除多余的流
    */
    bool RemoveNeedlessResultFlow(PBUserSkipMatchInfoItem & a_pbUserSkipInfoItem);

    /*
     * 发送总的获胜局数变更信息
    */
    void SendToTalWinNumLog(int64 a_iUid,const PBUserSkipMatchInfo & a_pbUserSkipInfo,int a_iActNum,int a_iReason);

	/*
	推送金币改变
	*/
	void SendCoinsChangeMsg(const PBUser& user_info);

	/*
	发送金币日志流水
	*/
	void SendCoinsFlowLog(const PBUser& user_info, int64 act_num, int reason);

private:
    AccountMap _accountmap;
    UserDataMap _userdatamap;
    AccountLruMap _account_lru_map;
    UserDataLruMap _userdata_lru_map;
public:
    CTimer _lru_timer;
};

template<typename SET_TYPE>
bool CheckUserData(const SET_TYPE& set, PBDataSet& data_set)
{
    long long uid = set.uid();
    for (int i = 0; i < set.key_list_size(); ++i)
    {
        int key = set.key_list(i).key();
        switch (key)
        {
        case PBUserDataField::kUserInfo:
            if (!UserManager::Instance()->GetUserInfo(uid))
                data_set.add_key_list()->set_key(key);
            break;
        case PBUserDataField::kUserRecord:
            if (!UserManager::Instance()->GetUserRecord(uid))
                data_set.add_key_list()->set_key(key);
            break;
        case PBUserDataField::kUserTableInfo:
            if (!UserManager::Instance()->GetUserTableInfo(uid))
                data_set.add_key_list()->set_key(key);
            break;
        case PBUserDataField::kUserTeaBarData:
            if (!UserManager::Instance()->GetUserTeaBarData(uid))
                data_set.add_key_list()->set_key(key);
            break;
            case PBUserDataField::kUserGameData:
                if (!UserManager::Instance()->GetUserGameData(uid))
                    data_set.add_key_list()->set_key(key);
                break;
            default: return true;
        }
    }

    if (data_set.key_list_size())
    {
        data_set.set_uid(uid);
        return false;
    }

    return true;
}

template<typename SET_TYPE>
void FillResponseUserData(const SET_TYPE& set, PBUserData& user_data)
{
    PBUserData* puser_data = UserManager::Instance()->GetUserData(set.uid());
    for (int j = 0; j < set.key_list_size(); ++j)
    {
        switch (set.key_list(j).key())
        {
        case PBUserDataField::kUserInfo:
            user_data.mutable_user_info()->CopyFrom(puser_data->user_info());
            break;
        case PBUserDataField::kUserRecord:
            user_data.mutable_user_record()->CopyFrom(puser_data->user_record());
            break;
        case PBUserDataField::kUserTableInfo:
            user_data.mutable_user_table_info()->CopyFrom(puser_data->user_table_info());
            break;
        case PBUserDataField::kUserTeaBarData:
            user_data.mutable_user_tea_bar_data()->CopyFrom(puser_data->user_tea_bar_data());
            break;
            case PBUserDataField::kUserGameData:
                user_data.mutable_user_game_data()->CopyFrom(puser_data->user_game_data());
                break;
            default : break;
        }
    }
}

template<typename DATA_TYPE>
bool SerializeUserData(const DATA_TYPE& data, int64 uid, int key, PBRedisData& redis_data)
{
    google::protobuf::uint8 buff[102400] = { 0 };
    unsigned int buflen = data.ByteSize();
    if (buflen > sizeof(buff))
    {
        ErrMsg("user[%lld] serialize key[%d] exceed max length = %u.", uid, key, buflen);
        return false;
    }
    if (!data.SerializeWithCachedSizesToArray(buff))
    {
        ErrMsg("user[%lld] serialize key[%d] falied.", uid, key);
        return false;
    }

    redis_data.set_key(key);
    redis_data.set_buff(buff, buflen);
    return true;
}
