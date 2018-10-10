#include "ZoneTableManager.h"
#include "GlobalRedisClient.h"
#include "GameSvrdManager.h"
#include "Message.h"
#include "RoomHandlerProxy.h"
#include <algorithm>

ZoneTableManager * ZoneTableManager::Instance (void)
{
    return CSingleton<ZoneTableManager>::Instance();
}

void ZoneTableManager::OnRetire()
{
    _state = EN_Game_Service_State_Retired;
}

void ZoneTableManager::OnPlayerLogoutTable(int64 tid,int ttype,int num)
{
	
	map<int64,int> & tablePlayerNumMap = _lz_alloced_public_table_player_maps[ttype];
	tablePlayerNumMap[tid] = num;
}

int ZoneTableManager::ProcessOnTimerOut(int Timerid)
{
    if (Timerid == TABLE_RECYCLE_TIMER)
    {
        // EN_Zone_Common
        {
            int stamp = time(NULL);
            VLogMsg(CLIB_LOG_LEV_DEBUG,"process recycle table timer time out.");
            vector<int64> need_erase_vect;
            map<int64,int>::iterator iter = _lz_alloced_map.begin();
            for(;iter!=_lz_alloced_map.end();iter++)
            {
                int last_refresh_time = iter->second;
#ifdef _DEBUG
                //测试用，将超时解散时间变更为1分钟
                if(stamp-last_refresh_time > 60)
                {
                    need_erase_vect.push_back(iter->first);
                }
                continue;
#endif
                if(stamp-last_refresh_time > 24*60*60)
                {
                    need_erase_vect.push_back(iter->first);
                }
            }
            for(unsigned int i=0;i<need_erase_vect.size();i++)
            {
                int64 tid = need_erase_vect[i];
                //to do 查找一下 gamesvid
                //如果svid还存在 通知一下game 释放掉这个桌子
                VLogMsg(CLIB_LOG_LEV_ERROR,"try auto release table[%lld] ...",tid);
                int gamesvid = GetGamesvrdIDForTable(need_erase_vect[i], EN_Zone_Common);
                VLogMsg(CLIB_LOG_LEV_ERROR,"get game svid[%d] table[%lld] ...",gamesvid,tid);

                int node_type = GetNodeTypeForTableId(tid, EN_Zone_Common);
                if (node_type == -1)
                {
                    return 0;
                }
                if (GameSvrdManager::Instance()->IsGameExist(node_type,gamesvid))
                {
                    VLogMsg(CLIB_LOG_LEV_ERROR,"find game svid[%d] for table[%lld] ... push inner msg for game",gamesvid,need_erase_vect[i]);
                    PBCSMsg msg;
                    SSInnerNotifyAutoReleaseTable & ss_inner_notify_auto_release_table = *msg.mutable_ss_inner_notify_auto_release_table();
                    ss_inner_notify_auto_release_table.set_tid(tid);
                    Message::PushInnerMsg(RouteManager::Instance()->GetRouteByRandom(),0,msg, node_type,gamesvid);   
                }
                else
                {
                    VLogMsg(CLIB_LOG_LEV_ERROR,"not found game svid[%d] for table[%lld] ...",gamesvid,need_erase_vect[i]);
                }

                //如果这个房间关联一个房主用户，也要清空这个用户信息
                //不管这个game服是否存在
                {
                    int64 iTableBindUid = -1;
                    if(GetUidByBindTid(tid,EN_Zone_Common,iTableBindUid) && iTableBindUid != -1)
                    {
                        PBCSMsg pbNotify;
                        //超时解散时，清空用户状态信息
                        SSInnerNotifyClearTableOwnerTableInfo & ss_inner_notify_clear_table_owner_table_info = *pbNotify.mutable_ss_inner_notify_clear_table_owner_table_info();
                        ss_inner_notify_clear_table_owner_table_info.set_i64_uid(iTableBindUid);
                        RoomHandlerProxy::Instance()->PushInnerMsg(pbNotify);
                    }
                }

                DissolveTable(tid, EN_Zone_Common);
            }
        }
    }

    return 0;
}

int ZoneTableManager::GetNodeTypeByMatchTableType(int64 ttype, int zone_type)
{
    const RepeatedPtrField<PBTableZone>& zones = PokerPBMatchTableMgrConfig::Instance()->zones();
    for (int i = 0; i < zones.size(); i++)
    {
        if (zones.Get(i).zone_type() == zone_type)
        {
            const RepeatedPtrField<PBTableGame>& games = zones.Get(i).games();
            for (int j = 0; j < games.size(); j++)
            {
                if(games.Get(j).table_type() == ttype)
                {
                    return games.Get(j).node_type();
                }
            }
        }
    }
    return -1;
}

bool ZoneTableManager::Init()
{
    _table_recycle_timer.SetTimeEventObj(this, TABLE_RECYCLE_TIMER);
    _table_recycle_timer.StartTimerBySecond(60, true);

    // EN_Zone_Common
    {
        _lz_alloced.clear();
        _lz_alloced_map.clear();
        _lz_free_maps.clear();

		char mainkey[100] = {0};
        snprintf(mainkey, sizeof(mainkey), "%d_TID_UID_MAP", EN_Zone_Common);
		if (!GlobalRedisClient::Instance()->GetAllHashMapKey(mainkey, _lz_alloced))
        {
			ErrMsg("failed to get sub keys for hash[%s]", mainkey);
            return false;
        }

        int64 curstamp = time(NULL);
        for(unsigned int i = 0;i<_lz_alloced.size();i++)
        {
            _lz_alloced_map[_lz_alloced[i]] = curstamp;
        }
        for(int i=0;i<899999;i++)
        {
            int tid = i+100000;
            if(_lz_alloced_map.find(tid) == _lz_alloced_map.end())
            {
                int table_type = GetTableTypeForTableId(tid, EN_Zone_Common);
                if (table_type != -1)
                {
                    _lz_free_maps[table_type].push_back(tid);
                }
            }
        }
    }

    return true;
}

int64 ZoneTableManager::GetGamesvrdIDForTable(int64 tid, int zone_type)
{
    // EN_Zone_Common
    if (zone_type == EN_Zone_Common)
    {
        char subkey[64] = {0};
        snprintf(subkey,sizeof(subkey),"%lld",tid);
        char mainkey[64] = {0};
        snprintf(mainkey,sizeof(mainkey),"%d_TID_SVID_MAP",zone_type);
        int64 value = -1;
        GlobalRedisClient::Instance()->GetHashMapValue(mainkey,subkey,value);
        return value;
    }
    return false;
}

bool ZoneTableManager::SetGamesvrdIDForTable(int64 tid, int gamesvid, int zone_type)
{
    // EN_Zone_Common
    if (zone_type == EN_Zone_Common)
    {
        char subkey[64] = {0};
        snprintf(subkey,sizeof(subkey),"%lld",tid);
        char mainkey[64] = {0};
        snprintf(mainkey,sizeof(mainkey),"%d_TID_SVID_MAP",zone_type);
        return GlobalRedisClient::Instance()->SetHashMapValue(mainkey,subkey,gamesvid);
    }
    return false;
}

int64 ZoneTableManager::AllocNewPublicTableID(int table_type, int zone_type,bool & is_create){
	// 泸州地区
    if (zone_type == EN_Zone_Common)
    {
        // 
        if(_lz_alloced_public_table_player_maps.find(table_type) != _lz_alloced_public_table_player_maps.end())		
		{
			map<int64,int> & tablePlayerNumMap = _lz_alloced_public_table_player_maps[table_type];
			const PBFreeGameConfItem * pconfItem = PokerPBFreeGameConfig::Instance()->GetConfItem(table_type);
			if(pconfItem == NULL) return -1;
			map<int64,int>::iterator iter = tablePlayerNumMap.begin();
			for(;iter!=tablePlayerNumMap.end();iter++){
				int64 tid = iter->first;
				if(iter->second < pconfItem->conf().seat_num()){
					tablePlayerNumMap[tid] = tablePlayerNumMap[tid] + 1;
					is_create = false;
					return tid;
				}
			}
		}
    	{
    		std::random_shuffle(_lz_free_maps[table_type].begin(), _lz_free_maps[table_type].end());

	        vector<int64>& free = _lz_free_maps[table_type];
	        if(free.size() == 0)
	        {
	            ErrMsg("failed to alloc new table id . no free to use");
	            return -1;
	        }
	        int64 tid = free[free.size()-1];
	        free.resize(free.size()-1);
	        _lz_alloced_map[tid] = time(NULL);

			map<int64,int> & tablePlayerNumMap = _lz_alloced_public_table_player_maps[table_type];

			tablePlayerNumMap[tid] = 1;
			is_create = true;
			return tid;
		}
    }

    return -1;
}


int64 ZoneTableManager::AllocNewTableID(int table_type, int zone_type)
{
    // 泸州地区
    if (zone_type == EN_Zone_Common)
    {
        // 乱序移到这里
        std::random_shuffle(_lz_free_maps[table_type].begin(), _lz_free_maps[table_type].end());

        vector<int64>& free = _lz_free_maps[table_type];
        if(free.size() == 0)
        {
            ErrMsg("failed to alloc new table id . no free to use");
            return -1;
        }
        int64 tid = free[free.size()-1];
        free.resize(free.size()-1);
        _lz_alloced_map[tid] = time(NULL);
        return tid;
    }

    return -1;
}

//bool ZoneTableManager::CheckAllocedTable(int64 tid)
//{
//    map<int64,int>::iterator iter = _lz_alloced_map.find(tid);
//    if(iter==_lz_alloced_map.end())
//	{
//		return false;
//	}
//	return true;
//}

bool ZoneTableManager::GetUidByBindTid(int64 tid,int zone_type,int64 & uid)
{
    if(zone_type == EN_Zone_Common)
    {
        char szSubKey[64] = {0};
        snprintf(szSubKey,sizeof(szSubKey),"%lld",tid);

        char szMainKey[64] = {0};
        snprintf(szMainKey,sizeof(szMainKey),"%d_TID_UID_MAP",zone_type);

        if(!GlobalRedisClient::Instance()->GetHashMapValue(szMainKey,szSubKey,uid))
        {
            return false;
        }
        return true;
    }

    return false;
}

bool ZoneTableManager::BindTidForUser(int64 uid,int64 tid, int zone_type,bool need_reset_gameid)
{
    // EN_Zone_Common
    if (zone_type == EN_Zone_Common)
    {
        char subkey[64] = {0};
        snprintf(subkey,sizeof(subkey),"%lld",tid);

        char mainkey[64] = {0};
        snprintf(mainkey,sizeof(mainkey),"%d_TID_UID_MAP",zone_type);

        if(!GlobalRedisClient::Instance()->SetHashMapValue(mainkey,subkey,uid))
        {
            return false;
        }
		if(need_reset_gameid)
		{
        	ZoneTableManager::Instance()->SetGamesvrdIDForTable(tid,-1,zone_type);
		}
        return true;
    }

    return false;
}

bool ZoneTableManager::UnBindTidForUser(int64 tid, int zone_type)
{
    // EN_Zone_Common
    if (zone_type == EN_Zone_Common)
    {
        char subkey[64] = {0};
        snprintf(subkey,sizeof(subkey),"%lld",tid);
        char mainkey[64] = {0};
        snprintf(mainkey,sizeof(mainkey),"%d_TID_UID_MAP",zone_type);
        if(!GlobalRedisClient::Instance()->DelHashMapKey(mainkey,subkey))
        {
            ErrMsg("failed to release fpf table[%lld]",tid);
            return false;
        }
        return true;
    }
    return false;
}

bool ZoneTableManager::RefreshTable(int64 tid, int zone_type)
{
    // EN_Zone_Common
    if (zone_type == EN_Zone_Common)
    {
        if(_lz_alloced_map.find(tid) == _lz_alloced_map.end())
        {
            return false;
        }
        _lz_alloced_map[tid] = time(NULL);
        return true;
    }

    return false;
}

bool ZoneTableManager::DissolveTable(int64 tid, int zone_type)
{
    // EN_Zone_Common
    if (zone_type == EN_Zone_Common)
    {
        if(_lz_alloced_map.find(tid) != _lz_alloced_map.end())
        {
            _lz_alloced_map.erase(tid);
            UnBindTidForUser(tid,zone_type);
        }
        else
        {
            //因为某种原因 已经释放了桌子id
            //例如定时器
            return false;
        }

        int table_type = GetTableTypeForTableId(tid, zone_type);
        if (table_type == -1)
        {
            ErrMsg("failed to release fpf table[%lld]",tid);
            return false;
        }
        vector<int64>& lz_free = _lz_free_maps[table_type];
        lz_free.push_back(tid);
        std::random_shuffle(lz_free.begin(), lz_free.end());
        ZoneTableManager::Instance()->SetGamesvrdIDForTable(tid,-1, zone_type);

		map<int64,int> playerNumMap = _lz_alloced_public_table_player_maps[table_type];
		playerNumMap.erase(tid);
        return true;
    }

    return false;
}


int ZoneTableManager::GetNodeTypeForTableId(int64 tid, int zone_type)
{
    const RepeatedPtrField<PBTableZone>& zones = PokerPBTableMgrConfig::Instance()->zones();
    for (int i = 0; i < zones.size(); i++)
    {
        if (zones.Get(i).zone_type() == zone_type)
        {
            const RepeatedPtrField<PBTableGame>& games = zones.Get(i).games();
            for (int j = 0; j < games.size(); j++)
            {
                if (tid >= games.Get(j).start() && tid <= games.Get(j).end())
                {
                    return games.Get(j).node_type();
                }
            }
        }
    }
    return -1;
}

int ZoneTableManager::GetTableTypeForTableId(int64 tid, int zone_type)
{
    const RepeatedPtrField<PBTableZone>& zones = PokerPBTableMgrConfig::Instance()->zones();
    for (int i = 0; i < zones.size(); i++)
    {
        if (zones.Get(i).zone_type() == zone_type)
        {
            const RepeatedPtrField<PBTableGame>& games = zones.Get(i).games();
            for (int j = 0; j < games.size(); j++)
            {
                if (tid >= games.Get(j).start() && tid <= games.Get(j).end())
                {
                    return games.Get(j).table_type();
                }
            }
        }
    }
    return -1;
}

bool ZoneTableManager::CheckAllocedTable(int64 tid, int zone_type)
{
    if (zone_type == EN_Zone_Common)
    {
        map<int64,int>::iterator iter = _lz_alloced_map.find(tid);
        if(iter==_lz_alloced_map.end())
        {
            return false;
        }
        return true;
    }
    return false;
}


