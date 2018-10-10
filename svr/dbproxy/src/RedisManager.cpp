#include "RedisManager.h"
#include "poker_msg_cs.pb.h"
#include "PBConfigBasic.h"

RedisManager * RedisManager::Instance()
{
	return CSingleton<RedisManager>::Instance();
}

bool RedisManager::Init()
{
	if (!PokerPBDBProxySvrdConfig::Instance()->Init("../conf/dbproxy.CFG"))
	{
		ErrMsg("failed to ini dbproxy.CFG");
		return false;
	}
	const PBSvrdNode & account_redis_conf = PokerPBDBProxySvrdConfig::Instance()->account();
	if (0 != _account.Connect(account_redis_conf.ip().c_str(),account_redis_conf.port()))
	{
		ErrMsg("failed to ini account redis[%s,%d]",account_redis_conf.ip().c_str(),account_redis_conf.port());
        return false;
	}
	const PBSvrdNode & rank_redis_conf = PokerPBDBProxySvrdConfig::Instance()->rank();
	if (0 != _rank.Connect(rank_redis_conf.ip().c_str(),rank_redis_conf.port()))
	{
		ErrMsg("failed to ini rank redis[%s,%d]",rank_redis_conf.ip().c_str(),rank_redis_conf.port());
        return false;
	}
	const PBSvrdNode & ssdb_conf = PokerPBDBProxySvrdConfig::Instance()->ssdb();
	if(0 != _ssdb.Connect(ssdb_conf.ip().c_str(),ssdb_conf.port()))
	{
		ErrMsg("failed to ini ssdb[%s,%d]",ssdb_conf.ip().c_str(),ssdb_conf.port());
		return false;
	}
	for(int i=0;i<PokerPBDBProxySvrdConfig::Instance()->users_size();i++)
	{
		const PBSvrdNode & user_redis_conf = PokerPBDBProxySvrdConfig::Instance()->users(i);
		CUserRedisServer * puser = new CUserRedisServer();
		if (0 != puser->Connect(user_redis_conf.ip().c_str(),user_redis_conf.port()))
		{
			ErrMsg("failed to ini user redis[%s,%d]",user_redis_conf.ip().c_str(),user_redis_conf.port());
        	return false;
		}
		_user_redis_map[user_redis_conf.index()] = puser;
	}
	return true;
}

bool RedisManager::CheckAccountToken(string account, int acc_type, string token)
{
    if (REPLY_OK != _account.CheckAccountToken(account, acc_type, token))
	{
		return (REPLY_OK == _ssdb.CheckAccountToken(account, acc_type, token));
	}
	return true;
}

bool RedisManager::QueryAccountUID(string account, int acc_type, long long& uid)
{
	if (REPLY_OK != _account.QueryAccountUid(account, acc_type, uid))
	{
		return (REPLY_OK == _ssdb.QueryAccountUid(account, acc_type, uid));
	}
	return true;
}

bool RedisManager::CreateNewUID(long long& uid)
{
    //return (REPLY_OK == _account.GetNewUID(uid)) ? true : false;
    if(_ssdb.GetNewUID(uid) == REPLY_OK)
	{
		return true;
	}
	return false;
}

bool RedisManager::SetAccountUID(const string & account, int acc_type, long long uid)
{
	//return (REPLY_OK == _account.SetAccountUID(account, acc_type, uid)) ? true : false;
	if(_ssdb.SetAccountUID(account,acc_type,uid) != REPLY_OK)
	{
		return false;
	}
	_account.SetAccountUID(account, acc_type, uid);
	return true;
}

CUserRedisServer * RedisManager::GetUserRedisByID(long long uid)
{
	CUserRedisServer * predis = NULL;
	if(_user_redis_map.size() == 0)
	{
		return predis;
	}
	int slot = (uid%_user_redis_map.size()) + 1 ;
	if(_user_redis_map.find(slot) != _user_redis_map.end())
	{
		predis = _user_redis_map[slot];
	}
	return predis;
}


bool RedisManager::QueryUserInfo(long long uid, PBRedisData& redis_data)
{
	char buff[102400] = {0};
	int len = 0;
	CUserRedisServer * predis = GetUserRedisByID(uid);
	if(predis==NULL)
	{
		redis_data.set_result(false);
		ErrMsg("fatal error. failed to find redis for uid[%lld]",uid);
        return false;
	}
    if (REPLY_OK == predis->_QueryDataField(uid, PBUserDataField::kUserInfo, buff, len))
    {
        redis_data.set_result(true);
        redis_data.set_buff(buff, len);
        return true;
    }
    else
    {
    	/*
    	if(REPLY_OK == _ssdb._QueryDataField(uid,PBUserDataField::kUserInfo,buff,len))
		{
			redis_data.set_result(true);
	        redis_data.set_buff(buff, len);
	        return true;
		}
		else
		*/
		{
	        redis_data.set_result(false);
	        return false;
		}
    }
}

void RedisManager::QueryDataField(long long uid, PBRedisData& redis_data)
{
	char buff[102400] = {0};
	int len = 0;
	CUserRedisServer * predis = GetUserRedisByID(uid);
	if(predis==NULL)
	{
		redis_data.set_result(false);
		ErrMsg("fatal error. failed to find redis for uid[%lld]",uid);
        return;
	}
	int iret = predis->_QueryDataField(uid, redis_data.key(), buff, len);
	/*
	if(iret == REPLY_NULL)
	{
		iret = _ssdb._QueryDataField(uid,redis_data.key(),buff,len);
	}
	*/
    if (iret == REPLY_OK || iret == REPLY_NULL)
    {
        redis_data.set_result(true);
        if (iret == REPLY_OK)
        {
            redis_data.set_buff(buff, len);
        }
    }
    else
    {
        redis_data.set_result(false);
    }
}

void RedisManager::SaveDataField(long long uid, PBRedisData& redis_data)
{
	/*
	int iret = _ssdb._SaveDataField(uid, redis_data.key(), redis_data.buff().c_str(), redis_data.buff().size());
	if (iret!=REPLY_OK)
	{
		redis_data.set_result(false);
		return;
	}
	*/
	CUserRedisServer * predis = GetUserRedisByID(uid);
	if(predis==NULL)
	{
		redis_data.set_result(false);
		ErrMsg("fatal error. failed to find redis for uid[%lld]",uid);
        return;
	}
	predis->_SaveDataField(uid, redis_data.key(), redis_data.buff().c_str(), redis_data.buff().size());
	redis_data.set_result(true);
}

void RedisManager::UpdateRankList(int rank_id, long long rank_key, long long rank_score)
{
    _rank._UpdateRankList(rank_id, rank_key, rank_score);
}


