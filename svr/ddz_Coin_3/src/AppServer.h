#pragma once
#include "FrameServer.h"

template <class T>
class AppServer: public FrameServer
{
public:
    AppServer(HandlerProxyBasic* handler_porxy, int acceptcnt = 256, int backlog = 256):
        FrameServer(handler_porxy, acceptcnt, backlog) {};
    virtual ~AppServer() {};

    bool AppInit();
    bool LoadConfig();
    bool Reload();
	bool Retire();
    void Exit() {};
protected:
	 virtual CTCP_Handler_Base* CreateHandler(int netfd, struct sockaddr* peer)
	 {
		 return new T;
	 }
};

