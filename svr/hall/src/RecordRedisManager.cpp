#include "RecordRedisManager.h"
#include "poker_msg_cs.pb.h"
#include "PBConfigBasic.h"

RecordRedisManager * RecordRedisManager::Instance()
{
    return CSingleton<RecordRedisManager>::Instance();
}

bool RecordRedisManager::Init()
{
    _record_redis_map.clear();
    if (!PokerPBHallSvrdConfig::Instance()->Init("../../conf/hallsvrd.CFG"))
    {
        ErrMsg("failed to ini hallsvrd.CFG");
        return false;
    }
    for(int i=0;i<PokerPBHallSvrdConfig::Instance()->records_size();i++)
    {
        const PBSvrdNode & redis_conf = PokerPBHallSvrdConfig::Instance()->records(i);
        CRecordRedisServer * puser = new CRecordRedisServer();
        if (0 != puser->Connect(redis_conf.ip().c_str(),redis_conf.port()))
        {
            ErrMsg("failed to ini record redis[%s,%d]",redis_conf.ip().c_str(),redis_conf.port());
            return false;
        }
        _record_redis_map[redis_conf.index()] = puser;
    }
    return true;
}

bool RecordRedisManager::QueryHashObject(const string & mainkey,const string & subkey,PBHashDataField & pb_hash_data_field)
{
    for (RecordRedisMap::iterator it = _record_redis_map.begin();
         it != _record_redis_map.end(); ++it)
    {
        CRecordRedisServer* redis_svr = it->second;
        if (redis_svr != NULL && redis_svr->QueryHashObject(mainkey, subkey, pb_hash_data_field))
        {
            return true;
        }
    }

    return false;
}

bool RecordRedisManager::CheckExist(const string & mainkey,const string & subkey)
{
    for (RecordRedisMap::iterator it = _record_redis_map.begin();
         it != _record_redis_map.end(); ++it)
    {
        CRecordRedisServer* redis_svr = it->second;
        if (redis_svr != NULL && redis_svr->CheckExist(mainkey, subkey))
        {
            return true;
        }
    }

    return false;
}


