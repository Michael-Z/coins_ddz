/******************************************************************************
  文 件 名   : GlobalRedisClient.h
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
class GlobalRedisClient:public CRedisServer
{
public:
    static GlobalRedisClient * Instance();
    bool Init();
public:
    int _UpdateGlobalData(PBRedisData & data);
    bool GetHashMapValue(const string & mainkey,const string & subkey,string & value);
    bool GetHashMapValue(const string & mainkey,const string & subkey,int64 & value);
    bool SetHashMapValue(const string & mainkey,const string & subkey,const string & value);
    bool SetHashMapValue(const string & mainkey,const string & subkey,int64 value);
    bool GetAllHashMapKey(const string & mainkey,vector<int64> & tidvect);
    bool GetAllHashMapKey(const string & mainkey,vector<string> & strvect);
    bool DelHashMapKey(const string & mainkey, const string & subkey);
    void IncreaseHashObject(const string & mainkey,const string & subkey, int64_t inc);
    int64_t GetHashObject(const string & mainkey,const string & subkey);

    bool QueryHashObject(const string & mainkey,const string & subkey, PBHashDataField & pb_hash_data_field );
    bool QueryHashObjectForReply(const string & mainkey,const string & subkey, PBHashDataField & pb_hash_data_field );
    int _QueryHashObject(const string & mainkey,const string & subkey,char * buff,int & len);
    bool UpdateHashObject(const string & mainkey,const string & subkey,const PBHashDataField & pb_hash_data_field);
    int _UpdateHashObject(const string & mainkey,const string & subkey,PBRedisData & data);
    bool CheckExist(const string & mainkey,const string & subkey);
};

