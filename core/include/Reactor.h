/******************************************************************************
  文 件 名   : Reactor.h
  版 本 号   : v 0.0.1
  作    者   : BarretXia
  生成日期   : 2015年8月28日
  最近修改   :
  功能描述   : Reactor 负责主循环的控制以及epoll,timer单元管理
  函数列表   :
  修改历史   :
  1.日    期   : 2015年8月28日
    作    者   : BarretXia
    修改内容   : 创建文件
******************************************************************************/

/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/

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

#pragma once

#include "timerlist.h"
#include "poller.h"
#include "singleton.h"
#include "TCP_Server_Base.h"
#include "TCP_Handler_Base.h"
#include "global.h"

#define MAX_POLLER 102400
class CTimer_Handler_Base;
 
class CReactor: public CSingleton<CReactor>
{
	public:
		int Init(int maxpoller=MAX_POLLER);
		int RunEventLoop(); 
		int RegistServer(CTCP_Server_Base* server) ;		  
		int RegistClient(CTCP_Handler_Base* client);		
		int RegistTimer(CTimer_Handler_Base* timer, long t);		 
 	    int AttachPoller (CPollerObject* poller);  
		void InitLog();
	private:

		CPollerUnit*    _pollerunit;
		CTimerUnit*    _timerunit;
		int _maxpoller;
		CTCP_Server_Base * _pserver;
};


