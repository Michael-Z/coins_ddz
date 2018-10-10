#pragma once
#include "global.h"
#include "SessionManager.h"
#include "ProcesserFactoryBasic.h"
#include "Message.h"
#include "PBConfigBasic.h"
#include "RouteManager.h"

enum ENHandlerResult
{
    EN_Handler_Done     = 1,
    EN_Handler_Succ     = 2,
    EN_Handler_Get      = 3,
    EN_Handler_Save     = 4,
};

enum ENSessionState
{
    EN_Session_Idle             = 1,
    EN_Session_Wait_Get_Data    = 2,
    EN_Session_Wait_Update_Data = 3,
};

class NewProcessor: public ProcessorBase
{
public:
    void Process(CHandlerTokenBasic* ptoken, CSession* psession);
    void EndProcess(CHandlerTokenBasic* ptoken, CSession* psession);
    ENHandlerResult OnGetData(CHandlerTokenBasic* ptoken, CSession* psession);
    ENHandlerResult OnGetResponse(CHandlerTokenBasic* ptoken, CSession* psession);
    ENHandlerResult OnUpdateData(CHandlerTokenBasic* ptoken, CSession* psession);
    ENHandlerResult OnUpdateResponse(CHandlerTokenBasic* ptoken, CSession* psession);

    virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession);
    virtual ENHandlerResult ProcessResponseMsg(CHandlerTokenBasic* ptoken, CSession* psession);
    virtual ENHandlerResult ProcessGetSucc(CHandlerTokenBasic* ptoken, CSession* psession);
    virtual ENHandlerResult ProcessGetFailed(CHandlerTokenBasic* ptoken, CSession* psession);
    virtual ENHandlerResult ProcessUpdateSucc(CHandlerTokenBasic* ptoken, CSession* psession);
    virtual ENHandlerResult ProcessUpdateFailed(CHandlerTokenBasic* ptoken, CSession* psession);
	virtual void ProcessSessionTimeOut(CHandlerTokenBasic* ptoken, CSession* psession);
};

