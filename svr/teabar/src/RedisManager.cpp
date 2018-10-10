#include "RedisManager.h"
#include "poker_msg_cs.pb.h"
#include "PBConfigBasic.h"

RedisManager * RedisManager::Instance()
{
	return CSingleton<RedisManager>::Instance();
}

bool RedisManager::Init()
{
	for (int i = 0; i<PokerPBTeaBarConfig::Instance()->tea_bar_redis_size(); i++)
	{
		const PBSvrdNode & tea_bar_redis_conf = PokerPBTeaBarConfig::Instance()->tea_bar_redis(i);
		CTeaBarRedisClient* pclient = new CTeaBarRedisClient();
		if (pclient == NULL)
		{
			return false;
		}
		if (!pclient->Init(tea_bar_redis_conf.ip(), tea_bar_redis_conf.port()))
		{
        	return false;
		}
		_tea_bar_redis_map[tea_bar_redis_conf.index()] = pclient;
	}

	for (int i = 0; i < PokerPBTeaBarConfig::Instance()->tea_bar_stat_redis_size(); i++)
	{
		const PBSvrdNode & tea_bar_stat_redis_conf = PokerPBTeaBarConfig::Instance()->tea_bar_stat_redis(i);
		CTeaBarRedisClient* pclient = new CTeaBarRedisClient();
		if (pclient == NULL)
		{
			return false;
		}
		if (!pclient->Init(tea_bar_stat_redis_conf.ip(), tea_bar_stat_redis_conf.port()))
		{
			return false;
		}
		_tea_bar_stat_redis_map[tea_bar_stat_redis_conf.index()] = pclient;
	}

	return true;
}

CTeaBarRedisClient* RedisManager::GetRedisClientByTbid(int64 tbid)
{
	int index = tbid % _tea_bar_redis_map.size();
	TeaBarRedisClinetMap::iterator iter = _tea_bar_redis_map.find(index);
	if (iter != _tea_bar_redis_map.end())
	{
		return iter->second;
	}
	return NULL;
}

CTeaBarRedisClient* RedisManager::GetRedisClientByIndex(int index)
{
	TeaBarRedisClinetMap::iterator iter = _tea_bar_redis_map.find(index);
	if (iter != _tea_bar_redis_map.end())
	{
		return iter->second;
	}
	return NULL;
}

CTeaBarRedisClient* RedisManager::GetRedisClientByStatid(int64 stat_id)
{
	int index = stat_id % _tea_bar_stat_redis_map.size();
	TeaBarRedisClinetMap::iterator iter = _tea_bar_stat_redis_map.find(index);
	if (iter != _tea_bar_stat_redis_map.end())
	{
		return iter->second;
	}
	return NULL;
}

