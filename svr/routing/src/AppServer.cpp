#include "AppServer.h"
#include "NewReactor.h"
#include "RouteHandlerProxy.h"
#include "SessionManager.h"
#include "ProcesserManager.h"

template <class T>
bool AppServer<T>::AppInit()
{
    if (!LoadConfig())
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
    AppServer<CTCPSocketHandler> server(RouteHandlerProxy::Instance());

    if (!CNewReactor::Instance()->Init(argc, argv, &server, EN_Node_Route))
        return -1;

    CNewReactor::Instance()->RunEventLoop();

	return 0;
}

