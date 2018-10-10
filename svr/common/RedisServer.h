#pragma once
#include <string>
#include <string.h>
#include <vector>
using std::string;
using namespace std;
#include "hiredis.h"

enum
{
	REPLY_OK = 0,
	REPLY_NULL,
	REPLY_ELEMENT_NULL,
	REPLY_ELEMENTCOUNT_ERROR,
	REPLY_KEY_ERROR,
	REPLY_KEY_TYPEERROR,
	REPLY_COMMAND_ERROR,
	REPLY_TYPE_ERROR,
	REPLY_TYPE_STR_EXCEED_LEN,
	REPLY_ERROR,
};

#define REDIS_CONNECT_TIMER 1

#define RANK_REDIS_EXPIRE  2592000        //排名redis 30天过期
#define SCORE_REDIS_EXPIRE 10800          //玩家个人积分redis记录3小时过期

class CRedisServer
{
public:
	CRedisServer();
	~CRedisServer();
	int Connect(const string & ip, int port);
	int Reconnect();
	int SetExpire(const char* key, int expire);
	redisReply * RedisCommand(const char * command);
	redisReply * RedisCommandArgv(vector<string> &v);
	redisReply * RedisVCommand(char const * query,...);
	int SetTimeout(struct timeval tv);

public:
	string m_strIp;
	int m_nPort;
	redisContext *m_pRedisConn;		//连接hiredis
	redisReply *m_pRedisReply;		//回复hiredis 
};

