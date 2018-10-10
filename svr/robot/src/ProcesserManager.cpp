#include "ProcesserManager.h"
#include "CommonClienthandler.h"

ProcesserManager * ProcesserManager::Instance()
{
	return CSingleton<ProcesserManager>::Instance();
}

void ProcesserManager::Init()
{
    REGIST_MSG_HANDLER(PBCSMsg::kSsNotifyInnerServer, CRegistInnerServer);
	//REGIST_MSG_HANDLER(PBCSMsg::kSsRequestRobotJoinMatch, CRequestRobotJoinMatch);
	REGIST_MSG_HANDLER(PBCSMsg::kCsResponseLogin, CResponseLogin);
	//REGIST_MSG_HANDLER(PBCSMsg::kCsResponseEnterMatch, CResponseEnterMatch);

	REGIST_MSG_HANDLER(PBCSMsg::kSsRequestRobotJoinMatch,CRequestRobotJoinMatch);
	REGIST_MSG_HANDLER(PBCSMsg::kCsResponseSdrEnterTable, CResponseSdrEnterTable);
	REGIST_MSG_HANDLER(PBCSMsg::kCsResponseSkipMatchGame,CResponseSkipMatchGame);
	REGIST_MSG_HANDLER(PBCSMsg::kCsResponseCoinMatchGame, CResponseCoinMatchGame);
}

ProcessorBase * ProcesserManager::GetProcesser(int cmd)
{
    HandlerMap::iterator iter = _handler_map.find(cmd);
    if (iter != _handler_map.end())
        return iter->second;

    return NULL;
}

