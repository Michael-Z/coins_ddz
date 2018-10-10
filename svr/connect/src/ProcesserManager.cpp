#include "ProcesserManager.h"
#include "CommonClienthandler.h"

ProcesserManager * ProcesserManager::Instance()
{
	return CSingleton<ProcesserManager>::Instance();
}

void ProcesserManager::Init()
{
    REGIST_MSG_HANDLER(PBCSMsg::kSsNotifyInnerServer,CRegistInnerServer);
    REGIST_MSG_HANDLER(PBCSMsg::kCsRequestLogin, CLoginProcesser);
    REGIST_MSG_HANDLER(PBCSMsg::kCsRequestHeartBeat,    CRequestHeartBeat);
	REGIST_MSG_HANDLER(PBCSMsg::kGmPushMessageMulti, CGMPushMessageMulti);
	REGIST_MSG_HANDLER(PBCSMsg::kSsNotifyPlayerRepeatedLogin, CNotifyPlayerRepeatedLogin);
    REGIST_MSG_HANDLER(PBCSMsg::kCsRequestEcho, CRequestEcho);
    REGIST_MSG_HANDLER(PBCSMsg::kCsRequestSdrEnterTable, CRequestSdrEnterTable);
    REGIST_MSG_HANDLER(PBCSMsg::kCsResponseSdrEnterTable, CResponseSdrEnterTable);
    //REGIST_MSG_HANDLER(PBCSMsg::kSsNotifyPlayerSkipMatchPosChange, CNotifySkipMatchPosChange);
	REGIST_MSG_HANDLER(PBCSMsg::kSsNotifyPlayerPosChange, CNotifyPosChange);
}

ProcessorBase * ProcesserManager::GetProcesser(int cmd)
{
    HandlerMap::iterator iter = _handler_map.find(cmd);
    if (iter != _handler_map.end())
        return iter->second;

    return NULL;
}

