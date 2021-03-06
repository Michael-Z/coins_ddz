/******************************************************************************
  文 件 名   : RecordRedisManager.h
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
#include "RecordRedisServer.h"
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
typedef map<int, CRecordRedisServer*> RecordRedisMap;
class RecordRedisManager
{
public:
    static RecordRedisManager * Instance(void);
    bool Init();
public:
    bool CheckExist(const string & mainkey,const string & subkey);
    bool QueryHashObject(const string & mainkey,const string & subkey, PBHashDataField & pb_hash_data_field );

public:
    RecordRedisMap _record_redis_map;
};

