#include "AppServer.h"
#include "NewReactor.h"
#include "HallHandlerProxy.h"
#include "SessionManager.h"
#include "RouteManager.h"
#include "ProcesserManager.h"
#include "LogWriter.h"
#include "GlobalRedisClient.h"
#include "ReplayCodeManager.h"
#include "RecordRedisManager.h"


template <class T>
bool AppServer<T>::AppInit()
{
    if (!LoadConfig())
        return false;
	if (!RouteManager::Instance()->Init(HallHandlerProxy::Instance()))
        return false;
    ProcesserManager::Instance()->Init();
	SessionManager::Instance()->Init();
	if (!CLogWriter::Instance()->Init())
        return false;
	if (!GlobalRedisClient::Instance()->Init())
        return false;
    if (!ReplayCodeManager::Instance()->Init())
        return false;
    if (!RecordRedisManager::Instance()->Init())
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
    if (!RecordRedisManager::Instance()->Init())
        return false;

    return LoadConfig();
}

int main(int argc, char *argv[])
{
    AppServer<CTCPSocketHandler> server(HallHandlerProxy::Instance());

    if (!CNewReactor::Instance()->Init(argc, argv, &server, EN_Node_Hall))
        return -1;

    CNewReactor::Instance()->RunEventLoop();

	return 0;
}

