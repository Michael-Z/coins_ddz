#include "SessionManager.h"
#include "global.h"

SessionManager * SessionManager::Instance(void)
{
	return CSingleton<SessionManager>::Instance();
}

void SessionManager::Init()
{
	for(int i=1;i<MAX_SESSION_ID;i++)
	{
		_free_id_queue.push(i);
	}

    _deal_session_count = 0;

    _timer.SetTimeEventObj(this, 1);
    _timer.StartTimerBySecond(60, true);
}

int SessionManager::AllocSessionID()
{
	unsigned int free_size = _free_id_queue.size();
	if (free_size <= 0)
	{
		ErrMsg("failed to alloc session id.no free id left");
		return -1;	
	}
	int id = _free_id_queue.front();
	_free_id_queue.pop();
	return id;
}

CSession * SessionManager::AllocSession()
{
	int session_id = AllocSessionID();
	if (session_id < 0)
	{
		return NULL;
	}
	CSession * psession = new CSession(session_id);
	_alloc_id_map[session_id] = psession;

    // stats
    gettimeofday(&psession->_start_time, NULL);

	return psession;
}

int SessionManager::ReleaseSession(int id)
{
	map<int,CSession *>::iterator iter = _alloc_id_map.find(id);
	if (iter == _alloc_id_map.end())
	{
		ErrMsg("failed to release the session id[%d].not found.",id);
		return -1;
	}

	CSession* psession = iter->second;

    // stats
    _deal_session_count++;

    struct timeval start_time = psession->_start_time;
    struct timeval end_time;
    gettimeofday(&end_time, NULL);
    long long diff_time = (end_time.tv_sec - start_time.tv_sec) *1000000 + (end_time.tv_usec - start_time.tv_usec);

    stStatsInfo& info = _stats_map[psession->_head.cmd()];
    info.avg_time = (info.avg_time *info.count + diff_time) /++info.count;

    UnlockProcess(psession);

    // release
    delete psession;
	_alloc_id_map.erase(iter);
	_free_id_queue.push(id);

	return 0;
}

CSession * SessionManager::GetSession(int id)
{
	map<int,CSession *>::iterator iter = _alloc_id_map.find(id);
	if (iter == _alloc_id_map.end())
	{
		ErrMsg("failed to get the session id[%d].not found.",id);
		return NULL;
	}
	return iter->second;
}

int SessionManager::ProcessOnTimerOut(int Timerid)
{
    // stats log
    string str;
    if (_deal_session_count)
    {
        str = "\nDeal Session Count:\t";
        char count[32] = {0};
        snprintf(count, sizeof(count), "%d", _deal_session_count);
        str += count;
        str += "\nDeal Session Detail:\n";
        for (map<int, stStatsInfo>::iterator iter = _stats_map.begin(); iter != _stats_map.end(); ++iter)
        {
            const stStatsInfo& info = iter->second;
            str += "msg id: ";
            char msg[32] = {0};
            snprintf(msg, sizeof(msg), "0x%x", iter->first);
            str += msg;

            str += "\t\tdeal count: ";
            char count[32] = {0};
            snprintf(count, sizeof(count), "%10d", info.count);
            str += count;

            str += "\t\tdeal time: ";
            char time[32] = {0};
            snprintf(time, sizeof(time), "%lld", info.avg_time);
            str += time;
            str += "\n";
        }
    }
    if (_alloc_id_map.size())
    {
        str += "\nCurrent Sessions Count:\t";
        char count2[32] = {0};
        snprintf(count2, sizeof(count2), "%lu", _alloc_id_map.size());
        str += count2;
        str += "\nCurrent Sessions Detail:\n";
        map<int, int> session_count;
        for (map<int, CSession*>::iterator iter = _alloc_id_map.begin(); iter != _alloc_id_map.end(); ++iter)
        {
            CSession* psession = iter->second;
            if (!psession) continue;
            session_count[psession->_head.cmd()]++;
        }
        for (map<int, int>::iterator iter = session_count.begin(); iter != session_count.end(); ++iter)
        {
            str += "msg id: ";
            char msg[32] = {0};
            snprintf(msg, sizeof(msg), "0x%x", iter->first);
            str += msg;

            str += "\t\ttotal count: ";
            char count[32] = {0};
            snprintf(count, sizeof(count), "%d", iter->second);
            str += count;
            str += "\n";
        }
    }
    if (str.size())
    {
        StatsMsg("%s", str.c_str());
    }

    // reset
    _deal_session_count = 0;
    _stats_map.clear();

	return 0;
}

bool SessionManager::LockProcess(CSession* psession)
{
    int cmd = psession->_request_msg.msg_union_case();
    long long uid = psession->_request_route.uid();

    map<int, map<long long, int> >::iterator iter = msg_map.find(cmd);
    if (iter != msg_map.end())
    {
        map<long long, int>& uid_map = iter->second;
        map<long long, int>::iterator iter2 = uid_map.find(uid);
        if (iter2 != uid_map.end())
        {
            return false;
        }
    }

    msg_map[cmd][uid] = psession->sessionid();
    psession->set_need_unlock(true);
    return true;
}

void SessionManager::UnlockProcess(const CSession* psession)
{
    if (!psession->need_unlock()) return;

    int cmd = psession->_request_msg.msg_union_case();
    long long uid = psession->_request_route.uid();

    if (msg_map[cmd][uid] == psession->sessionid())
    {
        msg_map[cmd].erase(uid);
        if (!msg_map[cmd].size())
        {
            msg_map.erase(cmd);
        }
    }
}

