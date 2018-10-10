#include "ReplayCodeManager.h"
#include "GlobalRedisClient.h"
#include "Message.h"

ReplayCodeManager * ReplayCodeManager::Instance (void)
{
    return CSingleton<ReplayCodeManager>::Instance();
}


int ReplayCodeManager::ProcessOnTimerOut(int Timerid)
{
    if (Timerid == _RECYCLE_TIMER)
	{
		int stamp = time(NULL);
        VLogMsg(CLIB_LOG_LEV_DEBUG, "process replay code recycle timer time out.");
		vector<int64> need_erase_vect;
		map<int64,int>::iterator iter = _alloced_map.begin();
        for( ; iter != _alloced_map.end(); iter++)
		{
			int last_refresh_time = iter->second;
            if(stamp - last_refresh_time > 2*24*60*60)
			{
				need_erase_vect.push_back(iter->first);
			}
		}
        for(unsigned int i = 0; i < need_erase_vect.size(); i++)
		{
            int64 code = need_erase_vect[i];
            VLogMsg(CLIB_LOG_LEV_ERROR,"try auto release replay code[%lld] ...", code);
            // 回收回放码 ...
            ReleaseCode(code);
        }
	}

	return 0;
}

bool ReplayCodeManager::Init()
{
    _recycle_timer.SetTimeEventObj(this, _RECYCLE_TIMER);
    _recycle_timer.StartTimerBySecond(60, true);

	_alloced.clear();
	_alloced_map.clear();
	_free.clear();
    _key_map.clear();

    if(!GlobalRedisClient::Instance()->GetAllHashMapKey("REPLAY_CODE_ALLOCED", _alloced))
	{
        ErrMsg("failed to get sub keys for hash[REPLAY_CODE_ALLOCED]");
		return false;
	}

	int64 curstamp = time(NULL);
    for(unsigned int i = 0; i < _alloced.size(); i++)
	{
		_alloced_map[_alloced[i]] = curstamp;
	}
    for(int i = 0; i < 899999; i++)
	{
        int code = i + 100000;
        if(_alloced_map.find(code) == _alloced_map.end())
		{
            _free.push_back(code);
		}
    }

	return true;
}


int64 ReplayCodeManager::AllocNewCode(const string& mainkey, const string& subkey)
{
    // 检测是否已经分配code，如果已经分配了，就不再分配新的code
    map<string, int>::iterator it = _key_map.find(subkey);
    if (it != _key_map.end())
    {
        return it->second;
    }

	if(_free.size() == 0)
	{
        ErrMsg("failed to alloc new replay code . no free to use");
		return -1;
	}
	int randomkey = random()%_free.size(); //从0-(size-1)开始

    int64 code =  _free[randomkey];
    // 绑定code
    if (!BindCodeForReplay(code, mainkey, subkey))
    {
        ErrMsg("failed to bind new replay code .");
        return -1;
    }
    _alloced_map[code] = time(NULL);
    _free.erase(_free.begin() + randomkey);

    return code;
}

bool ReplayCodeManager::CheckAllocedCode(int64 code)
{
    map<int64,int>::iterator iter = _alloced_map.find(code);
    if(iter == _alloced_map.end())
	{
		return false;
	}
	return true;
}

bool ReplayCodeManager::BindCodeForReplay(int64 code, const string& mainkey, const string& subkey)
{
    // 合成val
    char val[128] = {0};
    snprintf(val, sizeof(val), "%s,%s", mainkey.c_str(), subkey.c_str());
    {
        char key[64] = {0};
        snprintf(key, sizeof(key), "%lld", code);
        if(!GlobalRedisClient::Instance()->SetHashMapValue("REPLAY_CODE_ALLOCED", key, val))
        {
            return false;
        }
        _key_map[subkey] = code;
    }

	return true;
}

bool ReplayCodeManager::UnBindCodeForReplay(int64 code)
{
    char subkey[64] = {0};
    snprintf(subkey, sizeof(subkey), "%lld", code);
    if(!GlobalRedisClient::Instance()->DelHashMapKey("REPLAY_CODE_ALLOCED", subkey))
    {
        return false;
    }

    for (map<string, int>::iterator it = _key_map.begin();
         it != _key_map.end(); ++it)
    {
        if (it->second == code)
        {
            _key_map.erase(it);
            break;
        }
    }
	return true;
}

bool ReplayCodeManager::GetReplayForCode(int64 code, string& mainkey, string& subkey)
{
    string val;
    {
        char subkey[64] = {0};
        snprintf(subkey, sizeof(subkey), "%lld", code);
        if(!GlobalRedisClient::Instance()->GetHashMapValue("REPLAY_CODE_ALLOCED", subkey, val))
        {
            return false;
        }
    }

    // 分解val
    {
        size_t pos = val.find_first_of(',', 0);
        if (pos != std::string::npos)
        {
            mainkey = val.substr(0, pos);
            subkey = val.substr(pos+1, val.length());
        }
    }

    return true;
}

bool ReplayCodeManager::ReleaseCode(int64 code)
{
    if (!UnBindCodeForReplay(code))
    {
        VLogMsg(CLIB_LOG_LEV_ERROR,"failed to release replay code[%lld] ...", code);
        return false;
    }
    VLogMsg(CLIB_LOG_LEV_ERROR,"succed to release replay code[%lld] ...", code);
    _alloced_map.erase(code);
    _free.push_back(code);

    return true;
}
