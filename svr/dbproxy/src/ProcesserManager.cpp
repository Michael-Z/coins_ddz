#include "ProcesserManager.h"
#include "CommonClienthandler.h"

ProcesserManager * ProcesserManager::Instance()
{
	return CSingleton<ProcesserManager>::Instance();
}

void ProcesserManager::Init()
{
    REGIST_MSG_HANDLER(PBCSMsg::kSsNotifyInnerServer,       CRegistInnerServer);
	//REGIST_MSG_HANDLER(PBCSMsg::kSsRequestLogin,            CRequestLogin);
    //REGIST_MSG_HANDLER(PBCSMsg::kSsRequestCreateUid,        CRequestCreateUid);
    //REGIST_MSG_HANDLER(PBCSMsg::kSsRequestAccBindUid,       CRequestAccBindUid);
    REGIST_MSG_HANDLER(PBCSMsg::kSsRequestQueryData,        CRequestQueryData);
    REGIST_MSG_HANDLER(PBCSMsg::kSsRequestSaveData,         CRequestSaveData);
	REGIST_MSG_HANDLER(PBCSMsg::kSsRequestAccUid,			CRequestAccountUid);
	REGIST_MSG_HANDLER(PBCSMsg::kSsRequestUpdateRankList,   CRequestUpdateRankList);
	//REGIST_MSG_HANDLER(PBCSMsg::kCsRequestRankListInfo,     CRequestRankListInfo);
}

ProcessorBase * ProcesserManager::GetProcesser(int cmd)
{
    HandlerMap::iterator iter = _handler_map.find(cmd);
    if (iter != _handler_map.end())
        return iter->second;

    return NULL;
}


