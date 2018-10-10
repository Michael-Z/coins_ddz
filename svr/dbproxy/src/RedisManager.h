/******************************************************************************
  文 件 名   : RedisManager.h
  版 本 号   : 初稿
  作    者   : BarretXia
  生成日期   : 2015年10月13日
  最近修改   :
  功能描述   : redis客户端
  函数列表   :
  修改历史   :
  1.日    期   : 2015年10月13日
    作    者   : BarretXia
    修改内容   : 创建文件

******************************************************************************/

/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/
#pragma once
#include "singleton.h"
#include "UserRedisServer.h"
#include "poker_msg.pb.h"
#include <map>

using namespace std;
/*----------------------------------------------*
 * 外部变量说明                                 *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 外部函数原型说明                             *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 内部函数原型说明                             *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 全局变量                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 模块级变量                                   *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 常量定义                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 宏定义                                       *
 *----------------------------------------------*/
typedef map<int,CUserRedisServer *> UserRedisMap;
class RedisManager
{
	public:
		static RedisManager * Instance(void);
		bool Init();
	public:

        bool CheckAccountToken(string account, int acc_type, string token);
		bool QueryAccountUID(string account, int acc_type, long long& uid);
		bool CreateNewUID(long long& uid);
		bool SetAccountUID(const string & account, int acc_type, long long uid);
        bool QueryUserInfo(long long uid, PBRedisData& redis_data);

        void QueryDataField(long long uid, PBRedisData& redis_data);
        void SaveDataField(long long uid, PBRedisData& redis_data);
		CUserRedisServer * GetUserRedisByID(long long uid);

		void UpdateRankList(int rank_id, long long rank_key, long long rank_score);
		
	public:
		CUserRedisServer _account;
		CUserRedisServer _rank;
		UserRedisMap _user_redis_map;
		CUserRedisServer _ssdb;
};

