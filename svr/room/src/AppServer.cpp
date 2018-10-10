#include "AppServer.h"
#include "NewReactor.h"
#include "RoomHandlerProxy.h"
#include "SessionManager.h"
#include "RouteManager.h"
#include "ProcesserManager.h"
#include "GlobalRedisClient.h"
#include "TableManager.h"
#include "PBConfigBasic.h"
#include "ZoneTableManager.h"
#include "CostManager.h"
#include "ActivityRedisClient.h"
#include "LogWriter.h"

template <class T>
bool AppServer<T>::AppInit()
{
    if (!LoadConfig())
        return false;
	if (!RouteManager::Instance()->Init(RoomHandlerProxy::Instance()))
        return false;

    ProcesserManager::Instance()->Init();
	SessionManager::Instance()->Init();

    if (!CLogWriter::Instance()->Init())
        return false;

	if(!GlobalRedisClient::Instance()->Init())
	{
		ErrMsg("failed to load global redis client");
	}
    if (!ActivityRedisClient::Instance()->Init())
    {
        ErrMsg("failed to load activity redis client");
    }
    bool ret = false;
    if (PokerPBTableMgrConfig::Instance()->need_zone_manager())
    {
        LogMsg("ZoneTableManager init ...");
        ret = ZoneTableManager::Instance()->Init();
        if (!ret)
        {
            return false;
        }
    }
    if (PokerPBTableMgrConfig::Instance()->need_common_manager())
    {
        LogMsg("TableManager init ...");
        ret = TableManager::Instance()->Init();
        if (!ret)
        {
            return false;
        }
    }
    if (!ActivityRedisClient::Instance()->Init())
    {
        return false;
    }
    CostManager::Instance()->Init();
    return true;
}

template <class T>
bool AppServer<T>::LoadConfig()
{
	if (!PokerPBCostConfig::Instance()->Init("../../conf/cost.CFG"))
	{
		ErrMsg("failed to ini cost.CFG");
		return false;
	}
    if (!PokerPBTableMgrConfig::Instance()->Init("../../conf/table_mgr_config.CFG"))
    {
        ErrMsg("failed to ini table_mgr.CFG");
        return false;
    }
    if (!PokerPBRoomSvrdConfig::Instance()->Init("../../conf/room_svrd_config.CFG"))
    {
        ErrMsg("failed to ini room_svrd_config .CFG");
        return false;
    }
	if (!PokerPBFreeGameConfig::Instance()->Init("../../conf/free_game_config.CFG"))
    {
        ErrMsg("failed to ini free_game_config.CFG");
    }
    if (!PokerPBMatchTableMgrConfig::Instance()->Init("../../conf/match_table_mgr_config.CFG"))
    {
        ErrMsg("failed to ini match_table_mgr_config.CFG");
        return false;
    }
    if(!PokerPBMatchTypeMapConfig::Instance()->Init("../../conf/match_game_type_map_config.CFG"))
    {
        ErrMsg("failed to ini match_game_type_map_config.CFG");
        return false;
    }
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

    if (PokerPBTableMgrConfig::Instance()->need_zone_manager())
    {
        LogMsg("ZoneTableManager init ...");
        bool ret = ZoneTableManager::Instance()->Init();
        if (!ret)
        {
            return false;
        }
    }

    return true;
}

template <class T>
bool AppServer<T>::Retire()
{
    ZoneTableManager::Instance()->OnRetire();
    return true;
}

int main(int argc, char *argv[])
{
    AppServer<CTCPSocketHandler> server(RoomHandlerProxy::Instance());

    if (!CNewReactor::Instance()->Init(argc, argv, &server, EN_Node_Room))
        return -1;

    CNewReactor::Instance()->RunEventLoop();

	return 0;
}

