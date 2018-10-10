#include "AppServer.h"
#include "NewReactor.h"
#include "DBProxyHandlerProxy.h"
#include "SessionManager.h"
#include "RedisManager.h"
#include "RouteManager.h"
#include "ProcesserManager.h"

template <class T>
bool AppServer<T>::AppInit()
{
    if (!LoadConfig())
        return false;
	if (!RedisManager::Instance()->Init())
        return false;
	if (!RouteManager::Instance()->Init(DBProxyHandlerProxy::Instance()))
        return false;

    ProcesserManager::Instance()->Init();
	SessionManager::Instance()->Init();

    return true;
}

template <class T>
bool AppServer<T>::LoadConfig()
{
    return true;
}

template <class T>
bool AppServer<T>::Reload()
{
    return LoadConfig();
}

int main(int argc, char *argv[])
{
    AppServer<CTCPSocketHandler> server(DBProxyHandlerProxy::Instance());

    if (!CNewReactor::Instance()->Init(argc, argv, &server, EN_Node_DBProxy))
        return -1;

    CNewReactor::Instance()->RunEventLoop();

	return 0;
}

