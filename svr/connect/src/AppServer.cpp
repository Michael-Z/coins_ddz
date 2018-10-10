#include "AppServer.h"
#include "NewReactor.h"
#include "ConnectHandlerProxy.h"
#include "ClientHandlerProxy.h"
#include "SessionManager.h"
#include "RouteManager.h"
#include "AccessUserManager.h"
#include "ProcesserManager.h"
#include "LogWriter.h"

template <class T>
bool AppServer<T>::AppInit()
{
    if (!LoadConfig())
        return false;
	if (!RouteManager::Instance()->Init(ConnectHandlerProxy::Instance()))
        return false;

    ProcesserManager::Instance()->Init();
	SessionManager::Instance()->Init();
    AccessUserManager::Instance()->Init();
	if (!CLogWriter::Instance()->Init())
        return false;
    return true;
}

template <class T>
bool AppServer<T>::LoadConfig()
{
    if (!PokerPBConnectSvrdConfig::Instance()->Init("../../conf/connect_svrd_config.CFG"))
    {
        ErrMsg("failed to ini connect_svrd_config.CFG");
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
    AppServer<CTCPSocketHandler> server(ClientHandlerProxy::Instance());

    if (!CNewReactor::Instance()->Init(argc, argv, &server, EN_Node_Connect))
        return -1;

    CNewReactor::Instance()->RunEventLoop();

	return 0;
}

