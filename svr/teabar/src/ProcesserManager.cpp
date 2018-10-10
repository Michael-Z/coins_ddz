#include "ProcesserManager.h"
#include "CommonClienthandler.h"

ProcesserManager * ProcesserManager::Instance()
{
	return CSingleton<ProcesserManager>::Instance();
}

void ProcesserManager::Init()
{
    REGIST_MSG_HANDLER(PBCSMsg::kSsNotifyInnerServer, CRegistInnerServer);
	REGIST_MSG_HANDLER(PBCSMsg::kCsRequestCreateTeaBar, CRequestCreateTeaBar);
	REGIST_MSG_HANDLER(PBCSMsg::kCsRequestEnterTeaBar, CRequestEnterTeaBar);
	REGIST_MSG_HANDLER(PBCSMsg::kCsRequestApplyJoinTeaBar, CRequestApplyJoinTeaBar);
	REGIST_MSG_HANDLER(PBCSMsg::kCsRequestAgreeUserJoinTeaBar, CRequestAgreeUserJoinTeaBar);
	REGIST_MSG_HANDLER(PBCSMsg::kCsRequestApplyDropTeaBar, CRequestApplyDropTeaBar);
	REGIST_MSG_HANDLER(PBCSMsg::kCsRequestAgreeUserDropTeaBar, CRequestAgreeUserDropTeaBar);
	REGIST_MSG_HANDLER(PBCSMsg::kCsRequestTeaBarList, CRequestTeaBarList);
	REGIST_MSG_HANDLER(PBCSMsg::kSsNotifyCreateTable, CNotifyCreateTable);
	REGIST_MSG_HANDLER(PBCSMsg::kSsNotifyTeabarTableGameOver, CNotifyTeabarTableGameOver);
	REGIST_MSG_HANDLER(PBCSMsg::kSsNotifyTeabarTableGameStart, CNotifyTeabarTableGameStart);
	REGIST_MSG_HANDLER(PBCSMsg::kSsNotifyTeabarTablePlayerNum, CNotifyTeabarTablePlayerNum);
	REGIST_MSG_HANDLER(PBCSMsg::kCsRequestGetTeaBarUserList, CRequestGetTeaBarUserList);
	REGIST_MSG_HANDLER(PBCSMsg::kCsRequestTeaBarInfo, CRequestTeaBarInfo);
	REGIST_MSG_HANDLER(PBCSMsg::kCsRequestTeaBarMessage, CRequestTeaBarMessage);
	REGIST_MSG_HANDLER(PBCSMsg::kCsRequestPutChipsToTeaBar, CRequestPutChipsToTeaBar);
	REGIST_MSG_HANDLER(PBCSMsg::kCsRequestModifySettleNum, CRequestModifySettleNum);
	REGIST_MSG_HANDLER(PBCSMsg::kCsRequestRemoveUser, CRequestRemoveUser);
	REGIST_MSG_HANDLER(PBCSMsg::kSsNotifyTeaBarTableNotExist, CNotifyTeaBarTableNotExist);
	REGIST_MSG_HANDLER(PBCSMsg::kCsRequestModifyTeaBarDesc, CRequestModifyTeaBarDesc);
	REGIST_MSG_HANDLER(PBCSMsg::kCsRequestFreeTeaBar, CRequestFreeTeaBar);
	REGIST_MSG_HANDLER(PBCSMsg::kCsRequestStatisticsTableRecordList, CRequestStatisticsTableRecordList);
	REGIST_MSG_HANDLER(PBCSMsg::kCsRequestTeaBarStatistics, CRequestTeaBarStatistics);
	REGIST_MSG_HANDLER(PBCSMsg::kCsRequestTeaBarTableSettle, CRequestTeaBarTableSettle);
	REGIST_MSG_HANDLER(PBCSMsg::kLogTeaBarChipsFlow, CLogTeaBarChipsFlow);
	REGIST_MSG_HANDLER(PBCSMsg::kSsRequestTransferTeaBar, CRequestTransferTeaBar);
    REGIST_MSG_HANDLER(PBCSMsg::kGmRequestQueryTeabarUserList,CGMRequestQueryTeaBarUserList);

    REGIST_MSG_HANDLER(PBCSMsg::kCsRequestChangeTeabarCreateInfo,CRequestChangeTeabarCreateInfo);
}

ProcessorBase * ProcesserManager::GetProcesser(int cmd)
{
    HandlerMap::iterator iter = _handler_map.find(cmd);
    if (iter != _handler_map.end())
        return iter->second;

    return NULL;
}

