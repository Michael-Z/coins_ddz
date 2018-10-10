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
#include "FrameServer.h"

#define MAX_POLLER 102400

// 信号
#define SIG_RELOAD      SIGRTMIN
#define SIG_EXIT        SIGRTMIN+1
#define SIG_LOG_ON      SIGRTMIN+2
#define SIG_LOG_OFF     SIGRTMIN+3
#define SIG_RETIRE      SIGRTMIN+4

// 信号flag
#define FLAG_RELOAD     (1)
#define FLAG_EXIT       (2)
#define FLAG_LOG_ON     (4)
#define FLAG_LOG_OFF    (8)
#define FLAG_RETIRE		(16)

class CTimer_Handler_Base;
class CNewReactor: public CSingleton<CNewReactor>
{
public:
    void InitLog();
    void ParseArgs(int argc, char* argv[]);
    bool Init(int argc, char* argv[], FrameServer* pserver, ENNodeType type, int maxpoller = MAX_POLLER);

    bool CheckSigFlag();
    int RunEventLoop();
    int RegistClient(CTCP_Handler_Base* client);
    int RegistTimer(CTimer_Handler_Base* timer, long t);
    int AttachPoller (CPollerObject* poller);
private:
    int _maxpoller;
    CPollerUnit*    _pollerunit;
    CTimerUnit*     _timerunit;
    FrameServer*    _pserver;
};

