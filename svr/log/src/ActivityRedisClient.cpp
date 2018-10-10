#include "ActivityRedisClient.h"
#include "PBConfigBasic.h"
#include "Common.h"

ActivityRedisClient * ActivityRedisClient::Instance()
{
	return CSingleton<ActivityRedisClient>::Instance();
}

bool ActivityRedisClient::Init()
{
	if (Connect(PokerPBActivityConfig::Instance()->activity_redis_config().ip(), PokerPBActivityConfig::Instance()->activity_redis_config().port()) != 0)
	{
		VLogMsg(CLIB_LOG_LEV_ERROR,"failed to connect to global redis[ip:%s,port:%d]",
			PokerPBActivityConfig::Instance()->activity_redis_config().ip().c_str(), PokerPBActivityConfig::Instance()->activity_redis_config().port());
		return false;
	}
	return true;
}

void ActivityRedisClient::IncreaseHashObject(const string & mainkey, const string & subkey, int inc)
{
	m_pRedisReply = (redisReply *)RedisVCommand("HINCRBY %s %s %d", mainkey.c_str(), subkey.c_str(), inc);
	if (m_pRedisReply)
	{
		freeReplyObject(m_pRedisReply);
	}
	else
	{
		ErrMsg("IncreaseHashObject error,mainkey[%s], subkey[%s], inc[%d]", mainkey.c_str(), subkey.c_str(), inc);
	}
}