/******************************************************************************
  文 件 名   : HandlerManager.h
  版 本 号   : 初稿
  作    者   : BarretXia
  生成日期   : 2016年1月26日
  最近修改   :
  功能描述   : 句柄管理器
  函数列表   :
  修改历史   :
  1.日    期   : 2016年1月26日
    作    者   : BarretXia
    修改内容   : 创建文件

******************************************************************************/

/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/
#pragma once
#include "singleton.h"
#include <map>
#include "TCPSocketHandler.h"
#include "HandlerTokenBasic.h"
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
typedef map<CTCPSocketHandler *,CHandlerTokenBasic *> HandlerTokenMap;
class HandlerManager : public CSingleton<HandlerManager>
{
	public:
		void BindHandlerToken(CTCPSocketHandler * phandler,CHandlerTokenBasic * ptoken);
		void UnBindHandlerToken(CTCPSocketHandler * phandler);
		CHandlerTokenBasic * GetHandlerToken(CTCPSocketHandler * phandler);
	public:
		HandlerTokenMap _handler_token_map;
};

