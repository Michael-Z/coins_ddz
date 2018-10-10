#pragma once
#include "singleton.h"
#include "HandlerProxyBasic.h"
#include "PacketDecoder.h"
#include "HallHandlerToken.h"

class HallHandlerProxy : public HandlerProxyBasic,public CSingleton<HallHandlerProxy>
{
	public:
		virtual int OnAttachPoller(CTCPSocketHandler * phandler);
		virtual int OnConnected(CTCPSocketHandler * phandler);
		virtual int OnConnectedByRequest(CTCPSocketHandler * phandler);
		virtual int OnClose(CTCPSocketHandler * phandler);
		virtual int OnPacketComplete(CTCPSocketHandler * phandler,const char * data,int len);
		virtual int OnProcessOnTimerOut(CTCPSocketHandler * phandler,int Timerid);
	public:
		int ProcessPacket(HallHandlerToken * ptoken,const PBHead & head,const PBCSMsg & msg,const PBRoute & route);
	public:
		static CPacketDecoder  _decoder;
};


