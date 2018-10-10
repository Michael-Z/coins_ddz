
/******************************************************************************
  文 件 名   : CommonClienthandler.h
  版 本 号   : 初稿
  作    者   : BarretXia
  生成日期   : 2015年10月12日
  最近修改   :
  功能描述   : 一般游戏命令处理类
  函数列表   :
  修改历史   :
  1.日    期   : 2015年10月12日
    作    者   : BarretXia
    修改内容   : 创建文件

******************************************************************************/

/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/
#pragma once
#include "ProcesserBasic.h"
#include "type_def.h"
#include "NewProcessor.h"

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
class CRegistInnerServer : public ProcesserBasic
{
	virtual ENProcessResult ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession);
};

class CRequestGetUserData : public ProcesserBasic
{
	virtual ENProcessResult ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession);
    virtual ENProcessResult ProcessDBProxyMsg(CHandlerTokenBasic * ptoken,CSession *psession);
};

class CReuqestUpdateUserData : public ProcesserBasic
{
    enum ENUpdateDataState
    {
        EN_Wait_DB_Query_Data  = 0,
        EN_Wait_DB_Save_Data   = 1,
    };
	virtual ENProcessResult ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession);
	virtual ENProcessResult ProcessDBProxyMsg(CHandlerTokenBasic * ptoken,CSession * psession);
	ENProcessResult OnUpdateData(CHandlerTokenBasic * ptoken,CSession * psession);
 	ENProcessResult OnUpdateDataRsp(CHandlerTokenBasic * ptoken,CSession * psession);
};

