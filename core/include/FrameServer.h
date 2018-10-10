#pragma once
#include "net.h"
#include "poller.h"
#include "TCP_Handler_Base.h"

class FrameServer: public CPollerObject
{
public:
    FrameServer(HandlerProxyBasic* handler_porxy, int acceptcnt, int backlog);
    virtual ~FrameServer();
    virtual int OnAttachPoller();
    virtual int InputNotify();
    virtual void ProcessInnerMessage();

    bool FrameInit(const char* ip, uint16_t port);
    virtual bool AppInit() = 0;
    virtual bool Reload() = 0;
	virtual bool Retire(){return false;}
    virtual void Exit() = 0;
private:
    int proc_accept(struct sockaddr * peer, socklen_t* peerSize);
    int proc_request(struct sockaddr * peer);
protected:
    virtual CTCP_Handler_Base* CreateHandler(int netfd, struct sockaddr* peer) = 0;

public:
    HandlerProxyBasic* _handler_proxy;
protected:
    int     _accept_cnt;
    int     _newfd_cnt;
    int*    _fd_array;
    uint16_t    _bindPort;
    char        _bindAddr[128];
    int         _backlog;
    unsigned int    _flag;
    struct sockaddr *  _peer_array;
};

