#include "TableManager.h"
#include "GlobalRedisClient.h"
#include "GameSvrdManager.h"
#include "Message.h"

TableManager * TableManager::Instance (void)
{
	return CSingleton<TableManager>::Instance();
}


int TableManager::ProcessOnTimerOut(int Timerid)
{
	if (Timerid == LZMJ_TABLE_RECYCLE_TIMER)
	{
		int stamp = time(NULL);
		VLogMsg(CLIB_LOG_LEV_DEBUG,"process lzmj recycle table timer time out.");
		vector<int64> need_erase_vect;
		map<int64,int>::iterator iter = _alloced_map.begin();
		for(;iter!=_alloced_map.end();iter++)
		{
			int last_refresh_time = iter->second;
			if(stamp-last_refresh_time > 24*60*60)
			//if(stamp-last_refresh_time > 30)
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
			int gamesvid = GetGamesvrdIDForTable(need_erase_vect[i]);
			VLogMsg(CLIB_LOG_LEV_ERROR,"get game svid[%d] table[%lld] ...",gamesvid,tid);
			if (GameSvrdManager::Instance()->IsGameExist(EN_Node_Game,gamesvid))
			{
				VLogMsg(CLIB_LOG_LEV_ERROR,"find game svid[%d] for table[%lld] ... push inner msg for game",gamesvid,need_erase_vect[i]);
				PBCSMsg msg;
				SSInnerNotifyAutoReleaseTable & ss_inner_notify_auto_release_table = *msg.mutable_ss_inner_notify_auto_release_table();
				ss_inner_notify_auto_release_table.set_tid(tid);
				Message::PushInnerMsg(RouteManager::Instance()->GetRouteByRandom(),0,msg,EN_Node_Game,gamesvid);
			}
			else
			{
				VLogMsg(CLIB_LOG_LEV_ERROR,"not found game svid[%d] for table[%lld] ...",gamesvid,need_erase_vect[i]);
			}
			DissolveTable(tid);
		}

        // fpf 牌桌回收
        {
            int stamp = time(NULL);
            VLogMsg(CLIB_LOG_LEV_DEBUG,"process fpf recycle table timer time out.");
            vector<int64> need_erase_vect;
            map<int64,int>::iterator iter = _fpf_alloced_map.begin();
            for(;iter!=_fpf_alloced_map.end();iter++)
            {
                int last_refresh_time = iter->second;
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
                int gamesvid = GetGamesvrdIDForTable(need_erase_vect[i],EN_Node_FPF);
                VLogMsg(CLIB_LOG_LEV_ERROR,"get fpf svid[%d] table[%lld] ...",gamesvid,tid);
                if (GameSvrdManager::Instance()->IsGameExist(EN_Node_FPF,gamesvid))
                {
                    VLogMsg(CLIB_LOG_LEV_ERROR,"find fpf svid[%d] for table[%lld] ... push inner msg for game",gamesvid,need_erase_vect[i]);
                    PBCSMsg msg;
                    SSInnerNotifyAutoReleaseTable & ss_inner_notify_auto_release_table = *msg.mutable_ss_inner_notify_auto_release_table();
                    ss_inner_notify_auto_release_table.set_tid(tid);
                    Message::PushInnerMsg(RouteManager::Instance()->GetRouteByRandom(),0,msg,EN_Node_FPF,gamesvid);
                }
                else
                {
                    VLogMsg(CLIB_LOG_LEV_ERROR,"not found fpf svid[%d] for table[%lld] ...",gamesvid,need_erase_vect[i]);
                }
                DissolveTable(tid, EN_Node_FPF);
            }
        }

        // daer 牌桌回收
        {
            int stamp = time(NULL);
            VLogMsg(CLIB_LOG_LEV_DEBUG,"process daer recycle table timer time out.");
            vector<int64> need_erase_vect;
            map<int64,int>::iterator iter = _daer_alloced_map.begin();
            for(;iter!=_daer_alloced_map.end();iter++)
            {
                int last_refresh_time = iter->second;
                if(stamp-last_refresh_time > 24*60*60)
                {
                    need_erase_vect.push_back(iter->first);
                }
            }
            for(unsigned int i=0;i<need_erase_vect.size();i++)
            {
                DissolveTable(need_erase_vect[i], EN_Node_DAER);
                //to do 查找一下 gamesvid
                //如果svid还存在 通知一下game 释放掉这个桌子
            }
        }
	}

	return 0;
}



bool TableManager::Init()
{
	_lzmj_table_recycle_timer.SetTimeEventObj(this,LZMJ_TABLE_RECYCLE_TIMER);
	_lzmj_table_recycle_timer.StartTimerBySecond(60,true);
	//init alloc room map
	_alloced.clear();
	_alloced_map.clear();
	_free.clear();
    // fpf
    _fpf_alloced.clear();
    _fpf_alloced_map.clear();
    _fpf_free.clear();
    // daer
    _daer_alloced.clear();
    _daer_alloced_map.clear();
    _daer_free.clear();

	if(!GlobalRedisClient::Instance()->GetAllHashMapKey("LZMJ_TID_UID_MAP",_alloced))
	{
		ErrMsg("failed to get sub keys for hash[LZMJ_TID_MAP]");
		return false;
	}
    // fpf
    if(!GlobalRedisClient::Instance()->GetAllHashMapKey("FPF_TID_UID_MAP",_fpf_alloced))
    {
        ErrMsg("failed to get sub keys for hash[FPF_TID_MAP]");
        return false;
    }
    // daer
    if(!GlobalRedisClient::Instance()->GetAllHashMapKey("DAER_TID_UID_MAP",_daer_alloced))
    {
        ErrMsg("failed to get sub keys for hash[DAER_TID_MAP]");
        return false;
    }

	int64 curstamp = time(NULL);
	for(unsigned int i = 0;i<_alloced.size();i++)
	{
		_alloced_map[_alloced[i]] = curstamp;
	}
	for(int i=0;i<800000;i++)
	{
		int tid = i+100000;
		if(_alloced_map.find(tid) == _alloced_map.end())
		{
			_free.push_back(tid);
		}
	}

    // fpf
    for(unsigned int i = 0;i<_fpf_alloced.size();i++)
    {
        _fpf_alloced_map[_fpf_alloced[i]] = curstamp;
    }
    for(int i=0;i<800000;i++)
    {
        int tid = i+100000;
        if(_fpf_alloced_map.find(tid) == _fpf_alloced_map.end())
        {
            _fpf_free.push_back(tid);
        }
    }

    // daer
    for(unsigned int i = 0;i<_daer_alloced.size();i++)
    {
        _daer_alloced_map[_daer_alloced[i]] = curstamp;
    }
    for(int i=0;i<800000;i++)
    {
        int tid = i+100000;
        if(_daer_alloced_map.find(tid) == _daer_alloced_map.end())
        {
            _daer_free.push_back(tid);
        }
    }

	return true;
}

int64 TableManager::GetGamesvrdIDForTable(int64 tid, int game_type /*= 4*/)
{
    // fpf
    if (game_type == 11)  // EN_Node_FPF = 11;
    {
        char subkey[64] = {0};
        snprintf(subkey,sizeof(subkey),"%lld",tid);
        int64 value = -1;
        GlobalRedisClient::Instance()->GetHashMapValue("FPF_TID_SVID_MAP",subkey,value);
        return value;
    }
    // daer
    if (game_type == 12)
    {
        char subkey[64] = {0};
        snprintf(subkey,sizeof(subkey),"%lld",tid);
        int64 value = -1;
        GlobalRedisClient::Instance()->GetHashMapValue("DAER_TID_SVID_MAP",subkey,value);
        return value;
    }
    // lzmj
	char subkey[64] = {0};
	snprintf(subkey,sizeof(subkey),"%lld",tid);
	int64 value = -1;
	GlobalRedisClient::Instance()->GetHashMapValue("LZMJ_TID_SVID_MAP",subkey,value);
	return value;
}

bool TableManager::SetGamesvrdIDForTable(int64 tid,int gamesvid, int game_type /*= 4*/)
{
    // fpf
    if (game_type == 11)  // EN_Node_FPF = 11;
    {
        char subkey[64] = {0};
        snprintf(subkey,sizeof(subkey),"%lld",tid);
        return GlobalRedisClient::Instance()->SetHashMapValue("FPF_TID_SVID_MAP",subkey,gamesvid);
    }
    // daer
    if (game_type == 12)
    {
        char subkey[64] = {0};
        snprintf(subkey,sizeof(subkey),"%lld",tid);
        return GlobalRedisClient::Instance()->SetHashMapValue("DAER_TID_SVID_MAP",subkey,gamesvid);
    }
    // lzmj
	char subkey[64] = {0};
	snprintf(subkey,sizeof(subkey),"%lld",tid);
	return GlobalRedisClient::Instance()->SetHashMapValue("LZMJ_TID_SVID_MAP",subkey,gamesvid);
}


int64 TableManager::AllocNewTableID(int game_type /*= 4*/)
{
    // fpf
    if (game_type == 11)  // EN_Node_FPF = 11;
    {
        if(_fpf_free.size() == 0)
        {
            ErrMsg("failed to alloc new table id . no free to use");
            return -1;
        }
        int randomkey = random()%_fpf_free.size(); //从0-(size-1)开始
        //
        int64 tid =  _fpf_free[randomkey];
        _fpf_alloced_map[tid] = time(NULL);
        _fpf_free.erase(_fpf_free.begin()+randomkey);
        return tid;
    }
    // daer
    if (game_type == 12)
    {
        if(_daer_free.size() == 0)
        {
            ErrMsg("failed to alloc new table id . no free to use");
            return -1;
        }
        int randomkey = random()%_daer_free.size(); //从0-(size-1)开始
        //
        int64 tid =  _daer_free[randomkey];
        _daer_alloced_map[tid] = time(NULL);
        _daer_free.erase(_daer_free.begin()+randomkey);
        return tid;
    }
    // lzmj
	if(_free.size() == 0)
	{
		ErrMsg("failed to alloc new table id . no free to use");
		return -1;
	}
	int randomkey = random()%_free.size(); //从0-(size-1)开始
	//
	int64 tid =  _free[randomkey];
	_alloced_map[tid] = time(NULL);
	_free.erase(_free.begin()+randomkey);
	return tid;
}

bool TableManager::CheckAllocedTable(int64 tid)
{
	map<int64,int>::iterator iter = _alloced_map.find(tid);
	if(iter==_alloced_map.end())
	{
		return false;
	}
	return true;
}

bool TableManager::BindTidForUser(int64 uid,int64 tid, int game_type /*= 4*/)
{
    // fpf
    if (game_type == 11)  // EN_Node_FPF = 11;
    {
        char subkey[64] = {0};
        snprintf(subkey,sizeof(subkey),"%lld",tid);
        if(!GlobalRedisClient::Instance()->SetHashMapValue("FPF_TID_UID_MAP",subkey,uid))
        {
            return false;
        }
		TableManager::Instance()->SetGamesvrdIDForTable(tid,-1,game_type);
        return true;
    }
    // daer
    if (game_type == 12)
    {
        char subkey[64] = {0};
        snprintf(subkey,sizeof(subkey),"%lld",tid);
        if(!GlobalRedisClient::Instance()->SetHashMapValue("DAER_TID_UID_MAP",subkey,uid))
        {
            return false;
        }
        TableManager::Instance()->SetGamesvrdIDForTable(tid,-1,game_type);
        return true;
    }
    // lzmj
	char subkey[64] = {0};
	snprintf(subkey,sizeof(subkey),"%lld",tid);
	if(!GlobalRedisClient::Instance()->SetHashMapValue("LZMJ_TID_UID_MAP",subkey,uid))
	{
		return false;
	}
	TableManager::Instance()->SetGamesvrdIDForTable(tid,-1);
	return true;
}

bool TableManager::UnBindTidForUser(int64 uid,int64 tid, int game_type /*= 4*/)
{
    // fpf
    if (game_type == 11)  // EN_Node_FPF = 11;
    {
        char subkey[64] = {0};
        snprintf(subkey,sizeof(subkey),"%lld",tid);
        if(!GlobalRedisClient::Instance()->DelHashMapKey("FPF_TID_UID_MAP",subkey))
        {
            return false;
        }
        return true;
    }
    // daer
    if (game_type == 12)
    {
        char subkey[64] = {0};
        snprintf(subkey,sizeof(subkey),"%lld",tid);
        if(!GlobalRedisClient::Instance()->DelHashMapKey("DAER_TID_UID_MAP",subkey))
        {
            return false;
        }
        return true;
    }
    // lzmj
	char subkey[64] = {0};
	snprintf(subkey,sizeof(subkey),"%lld",tid);
	if(!GlobalRedisClient::Instance()->DelHashMapKey("LZMJ_TID_UID_MAP",subkey))
	{
		return false;
	}
	return true;
}

bool TableManager::RefreshTable(int64 tid, int game_type /*= 4*/)
{
    // fpf
    if (game_type == 11)  // EN_Node_FPF = 11;
    {
        if(_fpf_alloced_map.find(tid) == _fpf_alloced_map.end())
        {
            return false;
        }
        _fpf_alloced_map[tid] = time(NULL);
        return true;
    }
    // daer
    if (game_type == 12)
    {
        if(_daer_alloced_map.find(tid) == _daer_alloced_map.end())
        {
            return false;
        }
        _daer_alloced_map[tid] = time(NULL);
        return true;
    }
    // lzmj
	if(_alloced_map.find(tid) == _alloced_map.end())
	{
		return false;
	}
	_alloced_map[tid] = time(NULL);
	return true;
}

bool TableManager::DissolveTable(int64 tid, int game_type /*= 4*/)
{
    // fpf
    if (game_type == 11)  // EN_Node_FPF = 11;
    {
        if(_fpf_alloced_map.find(tid) != _fpf_alloced_map.end())
        {
            _fpf_alloced_map.erase(tid);
            char subkey[64] = {0};
            snprintf(subkey,sizeof(subkey),"%lld",tid);
            if (!GlobalRedisClient::Instance()->DelHashMapKey("FPF_TID_UID_MAP",subkey))
            {
                //释放redis中的uid失败 下次重读还会认为这个桌子id已经被分配了.
                //暂不影响逻辑
                ErrMsg("failed to release fpf table[%lld]",tid);
            }
        }
        else
        {
            //因为某种原因 已经释放了桌子id
            //例如定时器
            return false;
        }
        _fpf_free.push_back(tid);
        TableManager::Instance()->SetGamesvrdIDForTable(tid,-1, game_type);
        return true;
    }
    // daer
    if (game_type == 12)
    {
        if(_daer_alloced_map.find(tid) != _daer_alloced_map.end())
        {
            _daer_alloced_map.erase(tid);
            char subkey[64] = {0};
            snprintf(subkey,sizeof(subkey),"%lld",tid);
            if (!GlobalRedisClient::Instance()->DelHashMapKey("DAER_TID_UID_MAP",subkey))
            {
                //释放redis中的uid失败 下次重读还会认为这个桌子id已经被分配了.
                //暂不影响逻辑
                ErrMsg("failed to release fpf table[%lld]",tid);
            }
        }
        else
        {
            //因为某种原因 已经释放了桌子id
            //例如定时器
            return false;
        }
        _daer_free.push_back(tid);
        TableManager::Instance()->SetGamesvrdIDForTable(tid,-1, game_type);
        return true;
    }
    // lzmj
	if(_alloced_map.find(tid) != _alloced_map.end())
	{
		_alloced_map.erase(tid);
		char subkey[64] = {0};
		snprintf(subkey,sizeof(subkey),"%lld",tid);
		if (!GlobalRedisClient::Instance()->DelHashMapKey("LZMJ_TID_UID_MAP",subkey))
		{
			//释放redis中的uid失败 下次重读还会认为这个桌子id已经被分配了.
			//暂不影响逻辑
			ErrMsg("failed to release table[%lld]",tid);
		}
		VLogMsg(CLIB_LOG_LEV_ERROR,"dissolve lzmj table[%lld]",tid);
	}
	else
	{
		//因为某种原因 已经释放了桌子id
		//例如定时器
		return false;
	}
	TableManager::Instance()->SetGamesvrdIDForTable(tid,-1);
	_free.push_back(tid);
	return true;
}



