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
class CTeaBarRedisClient:public CRedisServer
{
public:
	CTeaBarRedisClient();
	~CTeaBarRedisClient();
	bool Init(const string& ip, int port);
	bool QueryObject(const string & key, PBHashDataField & pb_hash_data_field);
	int _QueryObject(const string & key,char * buff,int & len);
	bool UpdateObject(const string & key,const PBHashDataField & pb_hash_data_field);
	int _UpdateObject(const string & key,PBRedisData & data);
	bool DeleteObject(const string & key);
	int GetMaxTbid(int64& max_tbid);
	int SetMaxTbid(int64 max_tbid);
};

