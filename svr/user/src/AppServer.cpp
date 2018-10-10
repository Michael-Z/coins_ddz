#include "AppServer.h"
#include "NewReactor.h"
#include "UserHandlerProxy.h"
#include "SessionManager.h"
#include "RouteManager.h"
#include "ProcesserManager.h"
#include "LogWriter.h"
#include "UserManager.h"

template <class T>
bool AppServer<T>::AppInit()
{
    if (!LoadConfig())
        return false;
	if (!RouteManager::Instance()->Init(UserHandlerProxy::Instance()))
        return false;

    ProcesserManager::Instance()->Init();
	SessionManager::Instance()->Init();
	UserManager::Instance()->Init();
	if (!CLogWriter::Instance()->Init())
        return false;
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
    AppServer<CTCPSocketHandler> server(UserHandlerProxy::Instance());

    if (!CNewReactor::Instance()->Init(argc, argv, &server, EN_Node_User))
        return -1;

    CNewReactor::Instance()->RunEventLoop();

	return 0;
}

