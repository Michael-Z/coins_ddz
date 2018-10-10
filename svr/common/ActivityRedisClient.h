/******************************************************************************
  文 件 名   : ActivityRedisClient.h
  版 本 号   : 初稿
  作    者   : BarretXia
  生成日期   : 2015年12月14日
  最近修改   :
  功能描述   : 全局Redi缓存客户端
  函数列表   :
  修改历史   :
  1.日    期   : 2015年12月14日
    作    者   : BarretXia
    修改内容   : 创建文件

******************************************************************************/

/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/
#pragma once
#include "RedisServer.h"
#include "poker_msg.pb.h"
#include "global.h"
#include "singleton.h"
#include "map"
#include "type_def.h"
using google::protobuf::RepeatedPtrField;
using google::protobuf::RepeatedField;
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
class ActivityRedisClient :public CRedisServer
{
public:
	static ActivityRedisClient * Instance();
	bool Init();
public:
    void IncreaseHashObject(const string & mainkey, const string & subkey, int inc);
    bool GetValueOfKey(const string & key, int64 & value);
    bool GetHashMapValue(const string & mainkey, const string & subkey, int64 & value);
    //比赛场
    bool GetHashMapValueForMatch(const string & mainkey, int & subkey, char* value);
    void WriteUserMatchInfo(int match_session_id, long long uid, int user_state);
    void WriteUserMatchState(long long uid, int user_state);
    void WriteUserMatchResult(int match_session_id, long long uid, int rank);
    int GetUserMatchInfo(int match_session_id, long long uid);
    int GetUserMatchState(long long uid);
    void RfreshMatchPersonNum(int match_session_id,int num);
};

