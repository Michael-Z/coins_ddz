#pragma once
#include "singleton.h"
#include "HandlerProxyBasic.h"
#include "PacketDecoder.h"
#include "GameHandlerToken.h"
#include <queue>
using namespace std;


class GameHandlerProxy : public HandlerProxyBasic,public CSingleton<GameHandlerProxy>
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
		int ProcessPacket(GameHandlerToken * ptoken,const PBHead & head,const PBCSMsg & msg,const PBRoute & route);
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


