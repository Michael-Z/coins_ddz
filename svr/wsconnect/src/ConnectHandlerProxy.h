#pragma once
#include "singleton.h"
#include "HandlerProxyBasic.h"
#include "PacketDecoder.h"
#include "ConnectHandlerToken.h"
#include "PBPacket.h"
#include "Session.h"

class ConnectHandlerProxy : public HandlerProxyBasic,public CSingleton<ConnectHandlerProxy>
{
	public:
		virtual int OnAttachPoller(CTCPSocketHandler * phandler);
		virtual int OnConnected(CTCPSocketHandler * phandler);
		virtual int OnConnectedByRequest(CTCPSocketHandler * phandler);
		virtual int OnClose(CTCPSocketHandler * phandler);
		virtual int OnPacketComplete(CTCPSocketHandler * phandler,const char * data,int len);
		virtual int OnProcessOnTimerOut(CTCPSocketHandler * phandler,int Timerid);
	public:
		int ProcessPacket(ConnectHandlerToken * ptoken,CSession * psession);
		bool IsProcessInLocal(int cmd);
		int DoTransfer(CSession * psession,CPBInputPacket & input);
	public:
		static CPacketDecoder  _decoder;
};


