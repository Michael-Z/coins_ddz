#pragma once
#include "ProcesserBasic.h"
#include "NewProcessor.h"
#include "type_def.h"


class CRegistInnerServer : public ProcesserBasic
{
	virtual ENProcessResult ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession);
};

class CReportGameInfo: public NewProcessor
{
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession);

};

class CNotifyGamesvrdClosed: public NewProcessor
{
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession);
};

class CNotifyGamesvrdRetired: public NewProcessor
{
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession);
};

class CNotifyTableDissolved: public NewProcessor
{
    virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession);
};

class CRequestDelegateTableInfo : public NewProcessor
{
    virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession);
    virtual ENHandlerResult ProcessGetSucc(CHandlerTokenBasic * ptoken,CSession * psession);
    virtual ENHandlerResult ProcessUpdateSucc(CHandlerTokenBasic * ptoken,CSession * psession);
};

class CRequestDissolveDelegateTable : public NewProcessor
{
    virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession);
    virtual ENHandlerResult ProcessGetSucc(CHandlerTokenBasic * ptoken,CSession * psession);
    virtual ENHandlerResult ProcessUpdateSucc(CHandlerTokenBasic * ptoken,CSession * psession);
    virtual ENHandlerResult ProcessResponseMsg(CHandlerTokenBasic * ptoken,CSession * psession);
};

class CRequestSDRCreateTable : public NewProcessor
{
    virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession);
    virtual ENHandlerResult ProcessGetSucc(CHandlerTokenBasic* ptoken, CSession* psession);
    virtual ENHandlerResult ProcessUpdateSucc(CHandlerTokenBasic* ptoken, CSession* psession);
};

class CRequestSDREnterTable : public NewProcessor
{
    virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession);
    virtual ENHandlerResult ProcessGetSucc(CHandlerTokenBasic* ptoken, CSession* psession);
    virtual ENHandlerResult ProcessResponseMsg(CHandlerTokenBasic* ptoken, CSession* psession);
    virtual ENHandlerResult ProcessUpdateSucc(CHandlerTokenBasic* ptoken, CSession* psession);
};

class CInnerUnlock : public NewProcessor
{
    virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession);
    virtual ENHandlerResult ProcessGetSucc(CHandlerTokenBasic* ptoken, CSession* psession);
    virtual ENHandlerResult ProcessUpdateSucc(CHandlerTokenBasic* ptoken, CSession* psession);
};

class CRequestTableDetail : public NewProcessor
{
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession);
	virtual ENHandlerResult ProcessResponseMsg(CHandlerTokenBasic* ptoken, CSession* psession);
};

class CRequestDissolveTeaBarTable : public NewProcessor
{
    virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession);
    virtual ENHandlerResult ProcessResponseMsg(CHandlerTokenBasic* ptoken, CSession* psession);
};

class CReuqestTableLog : public ProcesserBasic
{
    virtual ENProcessResult ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession);
    virtual ENProcessResult ProcessResponseMsg(CHandlerTokenBasic* ptoken, CSession* psession);
};

class CReuqestDissolveTable : public ProcesserBasic
{
    virtual ENProcessResult ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession);
    virtual ENProcessResult ProcessResponseMsg(CHandlerTokenBasic* ptoken, CSession* psession);
};

class CRequestTableInfo : public NewProcessor
{
    virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession);
    virtual ENHandlerResult ProcessResponseMsg(CHandlerTokenBasic* ptoken, CSession* psession);
};

class CNotifyClearTableOwnerTableInfo : public NewProcessor
{
    virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic *ptoken, CSession *psession);
    virtual ENHandlerResult ProcessGetSucc(CHandlerTokenBasic *ptoken, CSession *psession);
    virtual ENHandlerResult ProcessUpdateSucc(CHandlerTokenBasic *ptoken, CSession *psession);
};

class CInnerNotifyLogoutTable : public NewProcessor
{
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession);
};

class CRequestQuickMatch : public NewProcessor
{
    virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic *ptoken, CSession *psession);
    virtual ENHandlerResult ProcessGetSucc(CHandlerTokenBasic *ptoken, CSession *psession);
    virtual ENHandlerResult ProcessResponseMsg(CHandlerTokenBasic* ptoken, CSession* psession);
};

class CRequestSkipMatchGame : public NewProcessor
{
    virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession);
    virtual ENHandlerResult ProcessGetSucc(CHandlerTokenBasic *ptoken, CSession *psession);
    virtual ENHandlerResult ProcessResponseMsg(CHandlerTokenBasic *ptoken, CSession *psession);
};

/*
½ð±Ò³¡Æ¥Åä
*/
class CRequestCoinMatchGame : public NewProcessor
{
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession);
	virtual ENHandlerResult ProcessGetSucc(CHandlerTokenBasic *ptoken, CSession *psession);
	virtual ENHandlerResult ProcessResponseMsg(CHandlerTokenBasic *ptoken, CSession *psession);
	virtual ENHandlerResult ProcessUpdateSucc(CHandlerTokenBasic *ptoken, CSession *psession);
};