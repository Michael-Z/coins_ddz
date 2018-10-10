/******************************************************************************
  文 件 名   : RoomHandlerProxy.h
  版 本 号   : 初稿
  作    者   : BarretXia
  生成日期   : 2016年1月25日
  最近修改   :
  功能描述   : 链接服句柄代理
  函数列表   :
  修改历史   :
  1.日    期   : 2016年1月25日
    作    者   : BarretXia
    修改内容   : 创建文件

******************************************************************************/

/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/
#pragma once
#include "singleton.h"
#include "HandlerProxyBasic.h"
#include "PacketDecoder.h"
#include "RoomHandlerToken.h"
#include <queue>

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

class RoomHandlerProxy : public HandlerProxyBasic,public CSingleton<RoomHandlerProxy>
{
	public:
		virtual int OnAttachPoller(CTCPSocketHandler * phandler);
		virtual int OnConnected(CTCPSocketHandler * phandler);
		virtual int OnConnectedByRequest(CTCPSocketHandler * phandler);
		virtual int OnClose(CTCPSocketHandler * phandler);
		virtual int OnPacketComplete(CTCPSocketHandler * phandler,const char * data,int len);
		virtual int OnProcessOnTimerOut(CTCPSocketHandler * phandler,int Timerid);
		virtual void OnProcessInnerMessage(const PBCSMsg & innermsg);
	public:
		int ProcessPacket(RoomHandlerToken * ptoken,const PBHead & head,const PBCSMsg & msg,const PBRoute & route);
	public:
	 	virtual void ProcessInnerMessage()
		{
			int flag = 0;
			while(flag<100 && _inner_msg_queue.size() > 0)
			{
				OnProcessInnerMessage(_inner_msg_queue.front());
				flag++;
				_inner_msg_queue.pop();
			}
		}
		virtual void PushInnerMsg(const PBCSMsg & msg)
		{
			_inner_msg_queue.push(msg);
		}
	public:
		queue<PBCSMsg>  _inner_msg_queue;
	public:
		static CPacketDecoder  _decoder;
};


