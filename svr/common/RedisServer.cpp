#include "RedisServer.h"
#include "global.h"

CRedisServer::CRedisServer()
{
	m_strIp = "";
	m_nPort = 0;
	m_pRedisConn = NULL;
	m_pRedisReply = NULL;
}

CRedisServer::~CRedisServer()
{
}

int CRedisServer::Connect(const string& ip, int port)
{
	m_strIp = ip;
	m_nPort = port;
	m_pRedisConn = redisConnect(ip.c_str(), port);
	if (m_pRedisConn==NULL || m_pRedisConn->err)
	{
		return -1;
	}

	return 0;
}

int CRedisServer::Reconnect()
{
	redisFree(m_pRedisConn);
	m_pRedisConn = redisConnect(m_strIp.c_str(), m_nPort);
	if (m_pRedisConn==NULL || m_pRedisConn->err)
	{
		return -1;
	}
	return 0;
}

int CRedisServer::SetExpire(const char *key, int expire)
{
	m_pRedisReply = reinterpret_cast<redisReply*>(redisCommand(m_pRedisConn, "EXPIRE %s %d", key, expire));
	if (m_pRedisReply != NULL)
	{
		freeReplyObject(m_pRedisReply);
	}
	else
	{
		//ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) SetExpire m_pRedisReply=NULL\r\n")));
	}
	return 0;
}

int CRedisServer::SetTimeout(struct timeval tv)
{
	if(m_pRedisConn!=NULL)
	{
		int ret = redisSetTimeout(m_pRedisConn,tv);
		return ret;
	}
	return REDIS_ERR;
}

redisReply * CRedisServer::RedisCommand(const char * command)
{
	m_pRedisReply = reinterpret_cast<redisReply*>(redisCommand(m_pRedisConn, command));
	if(m_pRedisReply == NULL) // lose connection , reconnect .
	{
		int ret = Reconnect();
		if(ret != 0)
		{
			return NULL;
		}
		else
		{
			m_pRedisReply = reinterpret_cast<redisReply*>(redisCommand(m_pRedisConn,command));
			if(m_pRedisReply == NULL)
			{
				return NULL;
			}
			else
			{
				return m_pRedisReply;
			}
		}
	}
	return m_pRedisReply;
}

redisReply * CRedisServer::RedisCommandArgv(vector<string> &v)
{
	//当字符串变量中含有','等特殊符号时使用.
	vector<const char *> argv( v.size());
	int j = 0;
	for ( vector<string>::const_iterator i = v.begin(); i != v.end(); ++i,++j)
	{
   		argv[j] = i->c_str();
	}
	m_pRedisReply = reinterpret_cast<redisReply*>(redisCommandArgv(m_pRedisConn,v.size(),&argv[0],NULL));
	if(m_pRedisReply == NULL) // lose connection , reconnect .
	{
		int ret = Reconnect();
		if(ret != 0)
		{
			return NULL;
		}
		else
		{
			m_pRedisReply = reinterpret_cast<redisReply*>(redisCommandArgv(m_pRedisConn,v.size(),&argv[0],NULL));
			if(m_pRedisReply == NULL)
			{
				return NULL;
			}
			else
			{
				return m_pRedisReply;
			}
		}
	}
	return m_pRedisReply;
}

redisReply * CRedisServer::RedisVCommand(char const * query,...)
{
	//redis查询
    va_list args;
    va_start(args, query);
     m_pRedisReply = (redisReply *)redisvCommand(m_pRedisConn, query, args);
    va_end(args);

	//bin log
    //char buffer[1024];
    //va_start(args, query);
    //vsnprintf(buffer, sizeof(buffer), query, args);
    //ErrMsg("redis query: %s", buffer);
    //va_end(args);

    //检查结果
    if (m_pRedisReply == NULL)
    {
        ErrMsg("[REDIS] query failed, err=%s, query=%s", m_pRedisConn->errstr, query);
    }

    //返回结果
    return m_pRedisReply;
}

