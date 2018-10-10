#include "AppServer.h"
#include "NewReactor.h"
#include "TeaBarHandlerProxy.h"
#include "SessionManager.h"
#include "RouteManager.h"
#include "ProcesserManager.h"
#include "LogWriter.h"
#include "TeaBarManager.h"
#include "GlobalRedisClient.h"

template <class T>
bool AppServer<T>::AppInit()
{
    if (!LoadConfig())
        return false;
	if (!RouteManager::Instance()->Init(TeaBarHandlerProxy::Instance()))
        return false;

    ProcesserManager::Instance()->Init();
	SessionManager::Instance()->Init();
	if (!TeaBarManager::Instance()->Init())
	{
		return false;
	}
	if (!CLogWriter::Instance()->Init())
        return false;
    return true;
}

template <class T>
bool AppServer<T>::LoadConfig()
{
	if (!PokerPBTeaBarConfig::Instance()->Init("../conf/teabar.CFG"))
	{
		return false;
	}

    return true;
}

template <class T>
bool AppServer<T>::Reload()
{
    return LoadConfig();
}

int main(int argc, char *argv[])
{
    AppServer<CTCPSocketHandler> server(TeaBarHandlerProxy::Instance());

    if (!CNewReactor::Instance()->Init(argc, argv, &server, EN_Node_TeaBar))
        return -1;

    CNewReactor::Instance()->RunEventLoop();

	return 0;
}

