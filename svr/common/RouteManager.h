/******************************************************************************
  文 件 名   : RouteManager.h
  版 本 号   : 初稿
  作    者   : BarretXia
  生成日期   : 2015年10月12日
  最近修改   :
  功能描述   : 路由服务器通讯句柄管理
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
#include "singleton.h"
#include "TCPSocketHandler.h"
#include "poker_msg.pb.h"
#include "PBPacket.h"
#include "Timer_Handler_Base.h"

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
#define NODE_MONITOR_TIMER 1000

class HandlerProxyBasic;
class RouteManager : public CTimerOutListener
{
typedef map<int,CTCPSocketHandler *> RouteMap;

public:
	enum ConHeatSendState
    {
        Con_Heat_Send_State_Wait_Send = 0,
        Con_Heat_Send_State_Sending  = 1,
    };

	static RouteManager * Instance(void);
	bool Init(HandlerProxyBasic * pproxy);
	void Send(CPBOutputPacket & output);
	virtual int ProcessOnTimerOut(int Timerid);
	int HeartBeatTimerStart();
	int HeartBeatTimerStop();
	CTCPSocketHandler * GetRouteByRandom();
public:
	//游戏服务器链接句柄
	//当有多个服务器时 需要重构
	RouteMap _route_map;
	CTimer _monitor_timer;
	int _state_attribute;
};

