#pragma once

#include "TCP_Server_Base.h"
#include "TCP_Handler_Base.h"
using namespace std;
 
class CTCPSocketServerBase  : public CTCP_Server_Base
{
	public:
		CTCPSocketServerBase(HandlerProxyBasic * handler_porxy,const char* bindIp, uint16_t port, int acceptcnt, int backlog);
		virtual ~CTCPSocketServerBase();
 
		virtual int Init();
		virtual int OnAttachPoller();
		virtual int InputNotify();
		virtual void ProcessInnerMessage();
	protected:
		int	_accept_cnt;
		int	_newfd_cnt;
		int*	_fd_array;
		struct sockaddr *  _peer_array;

		uint16_t	_bindPort;
		char	_bindAddr[128];
		int	_backlog;
		unsigned int	_flag;

		virtual CTCP_Handler_Base * CreateHandler(int netfd, struct sockaddr* peer)=0;

		int proc_accept (struct sockaddr* peer, socklen_t* peerSize);
		int proc_request (struct sockaddr* peer);
		
};



