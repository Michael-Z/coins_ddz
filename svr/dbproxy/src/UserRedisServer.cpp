#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>
#include "UserRedisServer.h"
#include "global.h"

CUserRedisServer::CUserRedisServer()
{
}

CUserRedisServer::~CUserRedisServer()
{
}

int CUserRedisServer::GetNewUID(long long & uid)
{
	char c_command[100] = {0};
	snprintf(c_command,sizeof(c_command),"HINCRBY D_TEXAS_INDEX UID 1");
	m_pRedisReply = RedisCommand(c_command);
	if(m_pRedisReply == NULL)
	{
		return REPLY_ERROR;
	}
	int iret = REPLY_OK;
	switch(m_pRedisReply->type)
	{			
		case REDIS_REPLY_NIL:
			{
				iret = REPLY_NULL;
			}
			break;
		case REDIS_REPLY_ARRAY:
		case REDIS_REPLY_STATUS:
			{
				iret = REPLY_TYPE_ERROR;
				break;
			}
		case REDIS_REPLY_ERROR:
			{
				iret = REPLY_ERROR;
				break;
			}
		case REDIS_REPLY_STRING:
			{
				uid = atoll(m_pRedisReply->str);
				iret = REPLY_OK;
				break;
			}
		case REDIS_REPLY_INTEGER:
			{
				uid = m_pRedisReply->integer;
				iret = REPLY_OK;
				break;
			}
		default :
			{
				iret = REPLY_ERROR;
				break;
			}
	}

	freeReplyObject(m_pRedisReply);
	return iret;
}

int CUserRedisServer::CheckAccountToken(string account, int acc_type, string token)
{
	//char c_command[100] = {0};
	//snprintf(c_command,sizeof(c_command),"HGET php_login_%d %s", acc_type, account.c_str());
	m_pRedisReply = RedisVCommand("HGET php_login_%d %s", acc_type, account.c_str());

    if (!m_pRedisReply || m_pRedisReply->type != REDIS_REPLY_STRING)
    {
        return REPLY_ERROR;
    }

    string reply_str(m_pRedisReply->str);
    if (token != reply_str)
    {
        return REPLY_ERROR;
    }

	freeReplyObject(m_pRedisReply);
    return REPLY_OK;
}

int CUserRedisServer::QueryAccountUid(string account, int acc_type, long long & uid)
{
	//char c_command[100] = {0};
	//snprintf(c_command,sizeof(c_command),"HGET TEXAS_ACCOUNT_%d %s", acc_type, account.c_str());
	m_pRedisReply = RedisVCommand("HGET TEXAS_ACCOUNT_%d %s", acc_type, account.c_str());

	if(m_pRedisReply == NULL)
	{
		return REPLY_ERROR;
	}
	int iret = REPLY_OK;
	switch(m_pRedisReply->type)
	{			
		case REDIS_REPLY_NIL:
			{
				iret = REPLY_NULL;
			}
			break;
		case REDIS_REPLY_ARRAY:
		case REDIS_REPLY_STATUS:
			{
				iret = REPLY_TYPE_ERROR;
				break;
			}
		case REDIS_REPLY_ERROR:
			{
				iret = REPLY_ERROR;
				break;
			}
		case REDIS_REPLY_STRING:
			{
				uid = atoll(m_pRedisReply->str);
				iret = REPLY_OK;
				break;
			}
		case REDIS_REPLY_INTEGER:
			{
				uid = m_pRedisReply->integer;
				iret = REPLY_OK;
				break;
			}
		default :
			{
				iret = REPLY_ERROR;
				break;
			}
	}

	freeReplyObject(m_pRedisReply);
	return iret;
}

int CUserRedisServer::SetAccountUID(const string & account, int acc_type, long long uid)
{
	//char c_command[200] = {0};
	//snprintf(c_command,sizeof(c_command),"HSET TEXAS_ACCOUNT_%d %s %lld", acc_type, account.c_str(), uid);
	m_pRedisReply = RedisVCommand("HSET TEXAS_ACCOUNT_%d %s %lld", acc_type, account.c_str(), uid);

	int iret = REPLY_OK;
	if(m_pRedisReply == NULL)
	{
		iret = REPLY_ERROR;
		return iret;
	}
	if(m_pRedisReply->type == REDIS_REPLY_INTEGER)
	{
		VLogMsg(CLIB_LOG_LEV_DEBUG,"set account[%s] uid[%lld] successed",account.c_str(),uid);
		iret = REPLY_OK;
	}
	else
	{
		VLogMsg(CLIB_LOG_LEV_ERROR,"failed to set account[%s] uid[%lld].reply type:%d",
			account.c_str(),uid,m_pRedisReply->type);
		iret = REPLY_ERROR;
	}

	freeReplyObject(m_pRedisReply);
	return iret;
}

int CUserRedisServer::_QueryDataField(long long uid, int key, char* buff, int & len)
{
	char c_command[100] = {0};
	snprintf(c_command,sizeof(c_command),"HGET %lld %d",uid, key);
    m_pRedisReply = (redisReply *)RedisCommand(c_command);
	if(m_pRedisReply == NULL)
	{
		return REPLY_ERROR;
	}
	int iret = REPLY_OK;
	switch(m_pRedisReply->type)
	{
		case REDIS_REPLY_NIL:
			{
				iret = REPLY_NULL;
			}
			break;
		case REDIS_REPLY_STRING:
			{
				if(m_pRedisReply->len == 0)
				{
					iret = REPLY_NULL;
				}
				if(m_pRedisReply->len > 102400)
				{
					iret = REPLY_TYPE_STR_EXCEED_LEN;
				}
				else
				{
					memcpy(buff,m_pRedisReply->str,m_pRedisReply->len);
					len = m_pRedisReply->len;
					iret = REPLY_OK;
				}
			}
			break;
		case REDIS_REPLY_ARRAY:
		case REDIS_REPLY_INTEGER:
		case REDIS_REPLY_STATUS:
			{
				iret = REPLY_TYPE_ERROR;
			}
		case REDIS_REPLY_ERROR:
			{
				iret = REPLY_ERROR;
			}
	}

	freeReplyObject(m_pRedisReply);
	return iret;
}

int CUserRedisServer::_SaveDataField(long long uid, int key, const char* buff, int len)
{
	m_pRedisReply = RedisVCommand("HSET %lld %d %b", uid, key, buff, len);
	
	int iret = REPLY_OK;
	if(m_pRedisReply == NULL)
	{
		iret = REPLY_ERROR;
		return iret;
	}
	if(m_pRedisReply->type == REDIS_REPLY_INTEGER && (m_pRedisReply->integer == 0 || m_pRedisReply->integer == 1))
	{
		VLogMsg(CLIB_LOG_LEV_DEBUG,"set user[%lld] data successed",uid);
		iret = REPLY_OK;
	}
	else
	{
		VLogMsg(CLIB_LOG_LEV_ERROR,"failed to set user[%lld] data.reply type:%d,str:%s",uid,m_pRedisReply->type,m_pRedisReply->str);
		iret = REPLY_ERROR;
	}

	freeReplyObject(m_pRedisReply);
	return iret;
}

void CUserRedisServer::_UpdateRankList(int rank_id, long long rank_key, long long inc_rank_score)
{
    time_t t_now = time(NULL);
    struct tm *p;
    p = localtime(&t_now);

    char key[125] = {0};
    if (rank_id == EN_Rank_List_Type_Day_Round)
    {
        snprintf(key,sizeof(key),"LZMJ_ROUND_DAYLY_%04d_%02d_%02d", p->tm_year + 1900, p->tm_mon + 1, p->tm_mday);
    }
    else if(rank_id == EN_Rank_List_Type_Week_Round)
    {
        char time_key[125] = {0};
        strftime(time_key,sizeof(time_key),"%W",p);
        snprintf(key,sizeof(key),"LZMJ_ROUND_WEEKLY_%04d_%s", p->tm_year + 1900, time_key);
    }
    else if(rank_id == EN_Rank_List_Type_Month_Max)
    {
        snprintf(key,sizeof(key),"LZMJ_ROUND_MONTHLY_%04d_%d", p->tm_year + 1900, p->tm_mon + 1);
    }
    else
    {
        VLogMsg(CLIB_LOG_LEV_ERROR,"invalid rank id[%d]",rank_id);
        return;
    }

    m_pRedisReply = RedisVCommand("ZINCRBY %s %lld %lld", key, inc_rank_score, rank_key);

    if (m_pRedisReply)
    {
        freeReplyObject(m_pRedisReply);
    }
}



