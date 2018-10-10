#include "ProcesserManager.h"
#include "CommonClienthandler.h"

ProcesserManager * ProcesserManager::Instance()
{
    return CSingleton<ProcesserManager>::Instance();
}

void ProcesserManager::Init()
{
    REGIST_MSG_HANDLER(PBCSMsg::kSsNotifyInnerServer, CRegistInnerServer);
    REGIST_MSG_HANDLER(PBCSMsg::kCsRequestLogin, CLoginProcesser);
    REGIST_MSG_HANDLER(PBCSMsg::kCsRequestUserRecord, CRequestUserRecord);
    /*
    REGIST_MSG_HANDLER(PBCSMsg::kCsRequestGetUserInfo,  CRequestGetUserInfo);
    REGIST_MSG_HANDLER(PBCSMsg::kCsRequestUpdateConfig, CRequestUpdateConfig);
    */
    REGIST_MSG_HANDLER(PBCSMsg::kCsRequestUpdateUserInfo, CRequestUpdateUserInfo);
    REGIST_MSG_HANDLER(PBCSMsg::kCsRequestTableFlowRecord, CRequestTableFlowRecord);

    REGIST_MSG_HANDLER(PBCSMsg::kCsRequestReplayCode, CRequestReplayCode);
    REGIST_MSG_HANDLER(PBCSMsg::kCsRequestReplayCodeData, CRequestReplayCodeData);
    REGIST_MSG_HANDLER(PBCSMsg::kCsRequestOnClickAvatar, CRequestOnClickAvatar);

    REGIST_MSG_HANDLER(PBCSMsg::kCsRequestUserActivityGameInfo, CRequestUserActivityGameInfo);// 请求活动游戏信息
    REGIST_MSG_HANDLER(PBCSMsg::kCsRequestGiveUpSkipMatchLevel, CRequestGiveUpSkipMatchLevel);
}

ProcessorBase * ProcesserManager::GetProcesser(int cmd)
{
    HandlerMap::iterator iter = _handler_map.find(cmd);
    if (iter != _handler_map.end())
        return iter->second;

    return NULL;
}