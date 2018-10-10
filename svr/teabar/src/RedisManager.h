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
#include "TeaBarRedisClient.h"
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
typedef map<int, CTeaBarRedisClient*> TeaBarRedisClinetMap;
typedef map<int, CTeaBarRedisClient*> TeaBarStatRedisClinetMap;
class RedisManager
{
public:
	static RedisManager * Instance(void);
	bool Init();
	CTeaBarRedisClient* GetRedisClientByTbid(int64 tbid);
	CTeaBarRedisClient* GetRedisClientByIndex(int index);
	CTeaBarRedisClient* GetRedisClientByStatid(int64 stat_id);
public:
		TeaBarRedisClinetMap _tea_bar_redis_map;
		TeaBarStatRedisClinetMap _tea_bar_stat_redis_map;
};

