#pragma once
#include "NewProcessor.h"
#include "ProcesserBasic.h"
#include "type_def.h"


class CRegistInnerServer : public NewProcessor
{
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession);
};

class CLoginProcesser : public NewProcessor
{
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession);
	virtual ENHandlerResult ProcessResponseMsg(CHandlerTokenBasic * ptoken,CSession * psession);
};

class CRequestEnterTable : public NewProcessor
{
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession);
    virtual ENHandlerResult ProcessResponseMsg(CHandlerTokenBasic* ptoken, CSession* psession);
};

class CRequestFPFEnterTable : public NewProcessor
{
    virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession);
    virtual ENHandlerResult ProcessResponseMsg(CHandlerTokenBasic* ptoken, CSession* psession);
};

class CRequestDaerEnterTable : public NewProcessor
{
    virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession);
    virtual ENHandlerResult ProcessResponseMsg(CHandlerTokenBasic* ptoken, CSession* psession);
};

class CRequestDssEnterTable : public NewProcessor
{
    virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession);
    virtual ENHandlerResult ProcessResponseMsg(CHandlerTokenBasic* ptoken, CSession* psession);
};

class CRequestHeartBeat: public NewProcessor
{
    virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession);
};

class CGMPushMessageMulti : public NewProcessor
{
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession);
};

class CNotifyPlayerRepeatedLogin: public ProcesserBasic
{
	virtual ENProcessResult ProcessPushMsg(CHandlerTokenBasic* ptoken, CSession* psession);
};

class CRequestEcho : public NewProcessor
{
    virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession);
};

class CResponseDssEnterTable : public NewProcessor
{
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession);
};

class CRequestSdrEnterTable : public NewProcessor
{
    virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession);
    virtual ENHandlerResult ProcessResponseMsg(CHandlerTokenBasic* ptoken, CSession* psession);
};

class CNotifyPosChange :public NewProcessor
{
    virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession);
};
