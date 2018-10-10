#include "ProcesserManager.h"
#include "CommonClienthandler.h"

ProcesserManager * ProcesserManager::Instance()
{
	return CSingleton<ProcesserManager>::Instance();
}

void ProcesserManager::Init()
{
    REGIST_MSG_HANDLER(PBCSMsg::kSsNotifyInnerServer,           CRegistInnerServer);
    REGIST_MSG_HANDLER(PBCSMsg::kSsReportGameInfo,              CReportGameInfo);
    REGIST_MSG_HANDLER(PBCSMsg::kSsNotifyGamesvrdClosed,        CNotifyGamesvrdClosed);
    REGIST_MSG_HANDLER(PBCSMsg::kSsNotifyGamesvrdRetired,       CNotifyGamesvrdRetired);
    REGIST_MSG_HANDLER(PBCSMsg::kSsNotifyTableDissolved,        CNotifyTableDissolved);
    REGIST_MSG_HANDLER(PBCSMsg::kCsRequestDelegateTableInfo,    CRequestDelegateTableInfo);
    REGIST_MSG_HANDLER(PBCSMsg::kCsRequestDissolveDelegateTable,    CRequestDissolveDelegateTable);
    REGIST_MSG_HANDLER(PBCSMsg::kInterEventUnlock,       CInnerUnlock);
    REGIST_MSG_HANDLER(PBCSMsg::kCsRequestSdrCreateTable,      CRequestSDRCreateTable);
    REGIST_MSG_HANDLER(PBCSMsg::kCsRequestSdrEnterTable,       CRequestSDREnterTable);

	REGIST_MSG_HANDLER(PBCSMsg::kCsRequestTableDetail, CRequestTableDetail);
    REGIST_MSG_HANDLER(PBCSMsg::kCsRequestDissolveTeaBarTable, CRequestDissolveTeaBarTable);
    REGIST_MSG_HANDLER(PBCSMsg::kSsRequestTableLog, CReuqestTableLog);
    REGIST_MSG_HANDLER(PBCSMsg::kSsRequestDissolveTable, CReuqestDissolveTable);
    REGIST_MSG_HANDLER(PBCSMsg::kSsRequestTableInfo, CRequestTableInfo);
    REGIST_MSG_HANDLER(PBCSMsg::kSsInnerNotifyClearTableOwnerTableInfo, CNotifyClearTableOwnerTableInfo);
    REGIST_MSG_HANDLER(PBCSMsg::kSsInnerNotifyLogoutTable, CInnerNotifyLogoutTable);

    //比赛场CSRequestQuickMatch CSRequestCreateTable
    REGIST_MSG_HANDLER(PBCSMsg::kCsRequestQuickMatch,           CRequestQuickMatch);
	//闯关
    REGIST_MSG_HANDLER(PBCSMsg::kCsRequestSkipMatchGame, CRequestSkipMatchGame);// 匹配
	//金币场
	REGIST_MSG_HANDLER(PBCSMsg::kCsRequestCoinMatchGame, CRequestCoinMatchGame);
}

ProcessorBase * ProcesserManager::GetProcesser(int cmd)
{
    HandlerMap::iterator iter = _handler_map.find(cmd);
    if (iter != _handler_map.end())
        return iter->second;

    return NULL;
}

