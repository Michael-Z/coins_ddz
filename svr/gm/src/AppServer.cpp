#include "AppServer.h"
#include "NewReactor.h"
#include "GMHandlerProxy.h"
#include "SessionManager.h"
#include "RouteManager.h"
#include "ProcesserManager.h"
#include "PBConfigBasic.h"

template <class T>
bool AppServer<T>::AppInit()
{
    if (!LoadConfig())
        return false;
	if (!RouteManager::Instance()->Init(GMHandlerProxy::Instance()))
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
    AppServer<CTCPSocketHandler> server(GMHandlerProxy::Instance());

    if (!CNewReactor::Instance()->Init(argc, argv, &server, EN_Node_GM))
        return -1;

    CNewReactor::Instance()->RunEventLoop();

	return 0;
}

