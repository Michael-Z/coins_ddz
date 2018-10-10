#include "ActivityRedisClient.h"
#include "PBConfigBasic.h"
#include "Common.h"

ActivityRedisClient * ActivityRedisClient::Instance()
{
	return CSingleton<ActivityRedisClient>::Instance();
}

bool ActivityRedisClient::Init()
{
    if (!PokerPBActivityConfig::Instance()->Init("../../conf/activity.CFG"))
    {
        VLogMsg(CLIB_LOG_LEV_ERROR, "failed to load activity redis config");
        return false;
    }
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

bool ActivityRedisClient::GetValueOfKey(const string & key, int64 & value)
{
    m_pRedisReply = RedisVCommand("get %s", key.c_str());
    if (!m_pRedisReply)
    {
        if (m_pRedisReply) freeReplyObject(m_pRedisReply);
        return false;
    }
    if (m_pRedisReply->type == REDIS_REPLY_INTEGER)
    {
        value = m_pRedisReply->integer;
    }
    else if (m_pRedisReply->type == REDIS_REPLY_STRING)
    {
        value = atoll(m_pRedisReply->str);
    }
    else
    {
        if (m_pRedisReply) freeReplyObject(m_pRedisReply);
        return false;
    }
    if (m_pRedisReply) freeReplyObject(m_pRedisReply);
    return true;
}

bool ActivityRedisClient::GetHashMapValue(const string & mainkey, const string & subkey, int64 & value)
{
    m_pRedisReply = RedisVCommand("hget %s %s", mainkey.c_str(), subkey.c_str());
    if (!m_pRedisReply)
    {
        if (m_pRedisReply) freeReplyObject(m_pRedisReply);
        return false;
    }
    if (m_pRedisReply->type == REDIS_REPLY_STRING)
    {
        value = atoll(m_pRedisReply->str);
    }
    else if (m_pRedisReply->type == REDIS_REPLY_INTEGER)
    {
        value = m_pRedisReply->integer;
    }
    else
    {
        if (m_pRedisReply) freeReplyObject(m_pRedisReply);
        return false;
    }
    if (m_pRedisReply) freeReplyObject(m_pRedisReply);
    return true;
}

bool ActivityRedisClient::GetHashMapValueForMatch(const string & mainkey, int & subkey, char* value)
{
    // key[100];
    //snprintf(key,sizeof(key),"%d",subkey);
    m_pRedisReply = RedisVCommand("hget %s %d", mainkey.c_str(), subkey);
    if (!m_pRedisReply)
    {
        if (m_pRedisReply) freeReplyObject(m_pRedisReply);
        VLogMsg(CLIB_LOG_LEV_ERROR,"failed to GetHashMapValueForMatch");
        return false;
    }
    if (m_pRedisReply->type == REDIS_REPLY_STRING && strcmp(m_pRedisReply->str,"") != 0)
    {
        strcpy(value,(m_pRedisReply->str));
    }
    else
    {
        if (m_pRedisReply) freeReplyObject(m_pRedisReply);
        return false;
    }
    if (m_pRedisReply) freeReplyObject(m_pRedisReply);
    return true;
}

int ActivityRedisClient::GetUserMatchState(long long uid)
{
    int has_join = -1;
    char user_match_state[1024];
    snprintf(user_match_state,sizeof(user_match_state),"%lld_GAMING_STATE",uid);
    char user_is_matching[1024];
    snprintf(user_is_matching,sizeof(user_is_matching),"IS_MATCH_ING");

    m_pRedisReply = RedisVCommand("HGET %s %s", user_match_state, user_is_matching);

    if(m_pRedisReply == NULL)
    {
        VLogMsg(CLIB_LOG_LEV_ERROR,"failed to GetUserMatchSate");
        return -1;
    }

    VLogMsg(CLIB_LOG_LEV_ERROR,"m_pRedisReply : %d",m_pRedisReply->type);
    if(m_pRedisReply->type == REDIS_REPLY_INTEGER)
    {
            VLogMsg(CLIB_LOG_LEV_DEBUG,"set user[%lld] data successed",uid);
            has_join = m_pRedisReply->integer;
            freeReplyObject(m_pRedisReply);
            return has_join;
    }
    else if(m_pRedisReply->type == REDIS_REPLY_STRING)
    {
        VLogMsg(CLIB_LOG_LEV_DEBUG,"set user[%lld] data successed",uid);
        has_join = atoi(m_pRedisReply->str);
        freeReplyObject(m_pRedisReply);
        return has_join;
    }
    else
    {
            VLogMsg(CLIB_LOG_LEV_ERROR,"failed to get user[%lld] data.reply type:%d,str:%s",uid,m_pRedisReply->type,m_pRedisReply->str);
            VLogMsg(CLIB_LOG_LEV_ERROR,"HGET %s %s", user_match_state, user_is_matching);
    }

    if (m_pRedisReply) freeReplyObject(m_pRedisReply);
    return -1;
}

int ActivityRedisClient::GetUserMatchInfo(int match_session_id, long long uid)
{
    int has_join = -1;
    char user_match_state[1024];
    snprintf(user_match_state,sizeof(user_match_state),"%lld_MATCH_STATE",uid);

    m_pRedisReply = RedisVCommand("HGET %s %d", user_match_state, match_session_id);

    if(m_pRedisReply == NULL)
    {
        VLogMsg(CLIB_LOG_LEV_ERROR,"failed to GetUserMatchInfo");
        return -1;
    }

    VLogMsg(CLIB_LOG_LEV_ERROR,"m_pRedisReply : %d",m_pRedisReply->type);
    if(m_pRedisReply->type == REDIS_REPLY_INTEGER)
    {
            VLogMsg(CLIB_LOG_LEV_DEBUG,"set user[%lld] data successed",uid);
            has_join = m_pRedisReply->integer;
            freeReplyObject(m_pRedisReply);
            return has_join;
    }
    else if(m_pRedisReply->type == REDIS_REPLY_STRING)
    {
        VLogMsg(CLIB_LOG_LEV_DEBUG,"set user[%lld] data successed",uid);
        has_join = atoi(m_pRedisReply->str);
        freeReplyObject(m_pRedisReply);
        return has_join;
    }
    else
    {
            VLogMsg(CLIB_LOG_LEV_ERROR,"failed to get user[%lld] data.reply type:%d,str:%s",uid,m_pRedisReply->type,m_pRedisReply->str);
            VLogMsg(CLIB_LOG_LEV_ERROR,"HGET %s %d", user_match_state, match_session_id);
    }

    if (m_pRedisReply) freeReplyObject(m_pRedisReply);
    return -1;
}

void ActivityRedisClient::WriteUserMatchState(long long uid, int user_state)
{
    char user_match_state[1024];
    snprintf(user_match_state,sizeof(user_match_state),"%lld_GAMING_STATE",uid);
    char user_is_matching[1024];
    snprintf(user_is_matching,sizeof(user_is_matching),"IS_MATCH_ING");
    //0代表有比赛进行中 1表示没有
    m_pRedisReply = RedisVCommand("HSET %s %s %d", user_match_state, user_is_matching, user_state);

    int iret = REPLY_OK;
    if(m_pRedisReply == NULL)
    {
            iret = REPLY_ERROR;
    }
    if(m_pRedisReply->type == REDIS_REPLY_INTEGER)
    {
            VLogMsg(CLIB_LOG_LEV_DEBUG,"set user[%lld] data successed",uid);
            iret = REPLY_OK;
    }
    else
    {
            VLogMsg(CLIB_LOG_LEV_ERROR,"failed to set user[%lld] data.reply type:%d,str:%s",uid,m_pRedisReply->type,m_pRedisReply->str);
            VLogMsg(CLIB_LOG_LEV_ERROR,"HSET %s %s %d", user_match_state, user_is_matching, user_state);
            iret = REPLY_ERROR;
    }

    if (m_pRedisReply) freeReplyObject(m_pRedisReply);
    return;
}

void ActivityRedisClient::WriteUserMatchInfo(int match_session_id, long long uid, int user_state)
{
    char user_match_state[1024];
    snprintf(user_match_state,sizeof(user_match_state),"%lld_MATCH_STATE",uid);

    m_pRedisReply = RedisVCommand("HSET %s %d %d", user_match_state, match_session_id, user_state);

    int iret = REPLY_OK;
    if(m_pRedisReply == NULL)
    {
            iret = REPLY_ERROR;
    }
    if(m_pRedisReply->type == REDIS_REPLY_INTEGER)
    {
            VLogMsg(CLIB_LOG_LEV_DEBUG,"set user[%lld] data successed",uid);
            iret = REPLY_OK;
    }
    else
    {
            VLogMsg(CLIB_LOG_LEV_ERROR,"failed to set user[%lld] data.reply type:%d,str:%s",uid,m_pRedisReply->type,m_pRedisReply->str);
            VLogMsg(CLIB_LOG_LEV_ERROR,"HSET %s %d %d", user_match_state, match_session_id, user_state);
            iret = REPLY_ERROR;
    }

    if (m_pRedisReply) freeReplyObject(m_pRedisReply);
    return;
}

void ActivityRedisClient::WriteUserMatchResult(int match_session_id, long long uid, int rank)
{
    char user_match_state[1024];
    snprintf(user_match_state,sizeof(user_match_state),"%lld_MATCH_RESULT",uid);
    int64 cur_time= time(NULL);
    char user_match_time[1024];
    snprintf(user_match_time,sizeof(user_match_time),"%d_%lld",rank,cur_time);

    m_pRedisReply = RedisVCommand("HSET %s %s %d", user_match_state, user_match_time, match_session_id);

    int iret = REPLY_OK;
    if(m_pRedisReply == NULL)
    {
            iret = REPLY_ERROR;
    }
    if(m_pRedisReply->type == REDIS_REPLY_INTEGER)
    {
            VLogMsg(CLIB_LOG_LEV_DEBUG,"set user[%lld] RANK data successed",uid);
            iret = REPLY_OK;
    }
    else
    {
            VLogMsg(CLIB_LOG_LEV_ERROR,"failed to set user[%lld]  RANK data.reply type:%d,str:%s",uid,m_pRedisReply->type,m_pRedisReply->str);
            VLogMsg(CLIB_LOG_LEV_ERROR,"HSET %s %s %d", user_match_state, user_match_time, match_session_id);
            iret = REPLY_ERROR;
    }

    if (m_pRedisReply) freeReplyObject(m_pRedisReply);
    return;
}

void ActivityRedisClient::RfreshMatchPersonNum(int match_session_id,int num)
{
    char user_match_num[1024];
    snprintf(user_match_num,sizeof(user_match_num),"MATCH_TOTAL_NUM_OF_PEOPLE");

    m_pRedisReply = RedisVCommand("HSET %s %d %d", user_match_num, match_session_id, num);

    int iret = REPLY_OK;
    if(m_pRedisReply == NULL)
    {
            iret = REPLY_ERROR;
    }
    if(m_pRedisReply->type == REDIS_REPLY_INTEGER)
    {
            VLogMsg(CLIB_LOG_LEV_DEBUG,"set match[%d] num[%d] data successed",match_session_id,num);
            iret = REPLY_OK;
    }
    else
    {
            VLogMsg(CLIB_LOG_LEV_ERROR,"failed to set match[%d] num[%d]",match_session_id,num);
            VLogMsg(CLIB_LOG_LEV_ERROR,"HSET %s %d %d", user_match_num, match_session_id, num);
            iret = REPLY_ERROR;
    }

    if (m_pRedisReply) freeReplyObject(m_pRedisReply);
    return;
}
