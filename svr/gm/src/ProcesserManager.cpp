#include "ProcesserManager.h"
#include "CommonClienthandler.h"

ProcesserManager * ProcesserManager::Instance()
{
	return CSingleton<ProcesserManager>::Instance();
}

void ProcesserManager::Init()
{
    REGIST_MSG_HANDLER(PBCSMsg::kSsNotifyInnerServer,   CRegistInnerServer);
    REGIST_MSG_HANDLER(PBCSMsg::kGmUpdateChipsMulti,    CUpdateChipsMulit);
	REGIST_MSG_HANDLER(PBCSMsg::kGmPushMessageMulti,    CPushMessageMulit);
    REGIST_MSG_HANDLER(PBCSMsg::kCsRequestUserActivityGameInfo,    CRequestUserActivityGameInfo);
    REGIST_MSG_HANDLER(PBCSMsg::kGmRequestNotifyShareSuccess,    CRequestNotifyShareSuccess);
    REGIST_MSG_HANDLER(-1,   CProcessPHPMsg);
}

ProcessorBase * ProcesserManager::GetProcesser(int cmd)
{
    HandlerMap::iterator iter = _handler_map.find(cmd);
    if (iter != _handler_map.end())
        return iter->second;

    iter = _handler_map.find(-1);
    if (iter != _handler_map.end())
        return iter->second;

    return NULL;
}

