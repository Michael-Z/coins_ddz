#include "AppServer.h"
#include "NewReactor.h"
#include "GameHandlerProxy.h"
#include "SessionManager.h"
#include "RouteManager.h"
#include "ProcesserManager.h"
#include "PBConfigBasic.h"
#include "TableManager.h"
#include "GlobalRedisClient.h"
#include "LogWriter.h"
#include <stdlib.h>
#include "RecordRedisClient.h"
#include "ActionBehaviorManager.h"

template <class T>
bool AppServer<T>::AppInit()
{
    if (!LoadConfig())
        return false;
	if (!RouteManager::Instance()->Init(GameHandlerProxy::Instance()))
        return false;
	
    ProcesserManager::Instance()->Init();
	SessionManager::Instance()->Init();

    if (!CLogWriter::Instance()->Init())
        return false;

    if (!GlobalRedisClient::Instance()->Init())
        return false;

    if (!RecordRedisClient::Instance()->Init())
        return false;

	if (!BehaviorManager::Instance()->init())
		return false;

    struct timeval cur_time;
    gettimeofday(&cur_time, NULL);
    long long usectime = (cur_time.tv_sec) * 1000000 + cur_time.tv_usec;
    srand(usectime);

    TableManager::Instance()->Init();

    return true;
}

template <class T>
bool AppServer<T>::LoadConfig()
{
	if (!PokerAutoMatchRoomConfig::Instance()->Init("../../conf/auto_match_room.CFG"))
	{
		ErrMsg("failed to ini auto_match_room.CFG");
		return false;
	}

    return true;
}

template <class T>
bool AppServer<T>::Reload()
{
    if (!LoadConfig())
        return false;

    if (!RecordRedisClient::Instance()->Init())
        return false;

    return true;
}

template <class T>
bool AppServer<T>::Retire()
{
    TableManager::Instance()->OnRetire();
	return true; 
}

int main(int argc, char *argv[])
{
    AppServer<CTCPSocketHandler> server(GameHandlerProxy::Instance());

    if (!CNewReactor::Instance()->Init(argc, argv, &server, EN_Node_3Ren_DDZ_COIN))
        return -1;

    CNewReactor::Instance()->RunEventLoop();

	return 0;
}

