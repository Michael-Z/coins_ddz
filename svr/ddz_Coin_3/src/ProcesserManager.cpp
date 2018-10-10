#include "ProcesserManager.h"
#include "CommonClienthandler.h"
#include "RouteManager.h"
#include "LocalInfo.h"

ProcesserManager * ProcesserManager::Instance()
{
	return CSingleton<ProcesserManager>::Instance();
}

void ProcesserManager::Init()
{
    REGIST_MSG_HANDLER(PBCSMsg::kSsNotifyInnerServer,   CRegistInnerServer);
    REGIST_MSG_HANDLER(PBCSMsg::kCsRequestSdrEnterTable,	CRequestEnterTable);
	REGIST_MSG_HANDLER(PBCSMsg::kCsRequestReadyForGame, CRequestReadyForGame);
    REGIST_MSG_HANDLER(PBCSMsg::kCsRequestSdrDoAction, CRequestDoAction);
//	REGIST_MSG_HANDLER(PBCSMsg::kCsRequestDissolveTable, CRequestDissolveTable);    //不允许解散
	REGIST_MSG_HANDLER(PBCSMsg::kCsRequestSendInteractiveProp, CRequetSendInteractiveProp);
	REGIST_MSG_HANDLER(PBCSMsg::kCsRequestChat, CRequestChat);
	REGIST_MSG_HANDLER(PBCSMsg::kSsInnerReportGameRecord,CReportGameRecord);
	REGIST_MSG_HANDLER(PBCSMsg::kCsRequestTableRecord,CRequestTableRecord);
    REGIST_MSG_HANDLER(PBCSMsg::kInterEventAutoAction, CInterEventAutoDoAction);
    REGIST_MSG_HANDLER(PBCSMsg::kInterEventOnDoActionOver, CInterEventOnDoActionOver);
    REGIST_MSG_HANDLER(PBCSMsg::kSsInnerDissolveTable, CInnerDissolveTable);
    REGIST_MSG_HANDLER(PBCSMsg::kCsRequestLogoutTable, CRequestLogoutTable);
    REGIST_MSG_HANDLER(PBCSMsg::kSsNotifyPlayerHandlerClose,CNotifyPlayerHandlerClose);
    REGIST_MSG_HANDLER(PBCSMsg::kSsNotifyRoomSvrd,CNotifyRoomSvrd);
    REGIST_MSG_HANDLER(PBCSMsg::kInterEventOnGameStart, CInnerOnGameStart);
//    REGIST_MSG_HANDLER(PBCSMsg::kSsInnerNotifyAutoReleaseTable,CNotifyAutoReleaseTable);
    REGIST_MSG_HANDLER(PBCSMsg::kCsRequestOffLine, CRequestOffLine);
    REGIST_MSG_HANDLER(PBCSMsg::kCsRequestTableDetail, CRequestTableDetail);
//    REGIST_MSG_HANDLER(PBCSMsg::kCsRequestDissolveTeaBarTable, CRequestDissolveTeaBarTable);
    REGIST_MSG_HANDLER(PBCSMsg::kSsNotifyCreateTable, CNotifyCreateTable);
    REGIST_MSG_HANDLER(PBCSMsg::kSsRequestTableLog, CGmReuqestTableLog);
    REGIST_MSG_HANDLER(PBCSMsg::kSsRequestDissolveTable, CGmReuqestDissolveTable);
 //   REGIST_MSG_HANDLER(PBCSMsg::kCsRequestSkipMatchGame, CRequestSkipMatchGame);
 //   REGIST_MSG_HANDLER(PBCSMsg::kSsInnerUpdateSkipMatchResult, CInnerUpdateSkipMatchResult);
    REGIST_MSG_HANDLER(PBCSMsg::kCsRequestTrusteeship, CRequestTrusteeship);
	REGIST_MSG_HANDLER(PBCSMsg::kCsRequestCoinMatchGame, CRequestCoinMatchGame);
	REGIST_MSG_HANDLER(PBCSMsg::kSsInnerUpdateMatchResult, CInnerUpdateCoinMatchResult);
	REGIST_MSG_HANDLER(PBCSMsg::kSsInnerNotifyKickoutUser, CInnerNotifyKickoutUser);

    //写牌    只能在测试服中添加
    if(!LocalInfo::Instance()->Init())
    {
        return ;
    }

    if(LocalInfo::Instance()->JudInTestEnv())
    {
        REGIST_MSG_HANDLER(PBCSMsg::kCsRequestSetNextRoundHandCards, CRequestSetNextRoundHandCards);
        REGIST_MSG_HANDLER(PBCSMsg::kCsRequestSetNextCard, CRequestSetNextCard);
    }
}

ProcessorBase * ProcesserManager::GetProcesser(int cmd)
{
    HandlerMap::iterator iter = _handler_map.find(cmd);
    if (iter != _handler_map.end())
        return iter->second;

    return NULL;
}

