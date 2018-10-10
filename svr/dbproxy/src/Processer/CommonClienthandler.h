#pragma once
#include "ProcesserBasic.h"

class CRegistInnerServer : public ProcesserBasic
{
	virtual ENProcessResult ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession);
};

/*
///////////////////////////////////////////////////
// 注册、登录
class CRequestLogin : public ProcesserBasic
{
	virtual ENProcessResult ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession);
};

class CRequestCreateUid : public ProcesserBasic
{
	virtual ENProcessResult ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession);
};

class CRequestAccBindUid : public ProcesserBasic
{
	virtual ENProcessResult ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession);
};
*/

///////////////////////////////////////////////////
// 改查数据
class CRequestQueryData : public ProcesserBasic
{
	virtual ENProcessResult ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession);
};

class CRequestSaveData : public ProcesserBasic
{
	virtual ENProcessResult ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession);
};

class CRequestAccountUid : public ProcesserBasic
{
	virtual ENProcessResult ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession);
};

class CRequestUpdateRankList : public ProcesserBasic
{
	virtual ENProcessResult ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession);
};


