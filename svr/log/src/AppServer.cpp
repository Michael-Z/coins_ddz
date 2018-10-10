#include "AppServer.h"
#include "NewReactor.h"
#include "LogHandlerProxy.h"
#include "SessionManager.h"
#include "DBManager.h"
#include "ProcesserManager.h"
#include "ActivityRedisClient.h"

template <class T>
bool AppServer<T>::AppInit()
{
    if (!LoadConfig())
        return false;
    if (!CDBManager::Instance()->Init())
        return false;
	if (!ActivityRedisClient::Instance()->Init())
	{
		return false;
	}

    ProcesserManager::Instance()->Init();
	SessionManager::Instance()->Init();

    return true;
}

template <class T>
bool AppServer<T>::LoadConfig()
{
	if (!PokerPBActivityConfig::Instance()->Init("../../conf/activity.CFG"))
	{
		VLogMsg(CLIB_LOG_LEV_ERROR, "failed to load activity redis config");
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
    AppServer<CTCPSocketHandler> server(LogHandlerProxy::Instance());

    if (!CNewReactor::Instance()->Init(argc, argv, &server, EN_Node_Log))
        return -1;

    CNewReactor::Instance()->RunEventLoop();

	return 0;
}

