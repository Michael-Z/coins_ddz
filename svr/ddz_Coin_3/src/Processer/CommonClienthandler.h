#pragma once
#include "ProcesserBasic.h"
#include "NewProcessor.h"

class CRegistInnerServer : public ProcesserBasic
{
	virtual ENProcessResult ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession);
};

enum EN_EenterTable_State
{
	EN_EnterTable_State_ULOCK = 1,
	EN_EnterTable_State_ENTERING = 2,
	EN_EnterTable_State_ULOCK_AF_SAVE = 3,
};

/*
进入房间
1.普通进入房间
2.闯关进入房间
3.金币场进入房间
*/
class CRequestEnterTable : public NewProcessor
{
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession);
	virtual ENHandlerResult ProcessGetSucc(CHandlerTokenBasic* ptoken, CSession* psession);
	virtual ENHandlerResult ProcessUpdateSucc(CHandlerTokenBasic* ptoken, CSession* psession);
};

class CRequestReadyForGame : public NewProcessor
{
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession);
};

class CRequestDoAction : public NewProcessor
{
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession);
};

class CRequestDissolveTable : public NewProcessor
{
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession);
	virtual ENHandlerResult ProcessUpdateSucc(CHandlerTokenBasic* ptoken, CSession* psession);
	virtual ENHandlerResult ProcessUpdateFailed(CHandlerTokenBasic* ptoken, CSession* psession);
};

class CRequetSendInteractiveProp : public NewProcessor
{
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession);
};

class CRequestChat : public NewProcessor
{
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession);
};

class CReportGameRecord : public NewProcessor
{
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession);
};

class CRequestTableRecord : public NewProcessor
{
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession);
};

class CInterEventAutoDoAction : public NewProcessor
{
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession);
};

class CInterEventOnDoActionOver : public NewProcessor
{
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession);
};

class CNotifyRoomSvrd : public NewProcessor
{
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession);
};

class CInnerDissolveTable : public NewProcessor
{
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession);
	virtual ENHandlerResult ProcessUpdateSucc(CHandlerTokenBasic* ptoken, CSession* psession);
	virtual ENHandlerResult ProcessUpdateFailed(CHandlerTokenBasic* ptoken, CSession* psession);
	virtual void ProcessSessionTimeOut(CHandlerTokenBasic* ptoken, CSession* psession);
};

class CRequestLogoutTable : public NewProcessor
{
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession);
	virtual ENHandlerResult ProcessUpdateSucc(CHandlerTokenBasic* ptoken, CSession* psession);
	virtual ENHandlerResult ProcessUpdateFailed(CHandlerTokenBasic* ptoken, CSession* psession);
};

class CNotifyPlayerHandlerClose : public NewProcessor
{
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession);
};

class CInnerOnGameStart : public NewProcessor
{
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession);
	virtual ENHandlerResult ProcessGetSucc(CHandlerTokenBasic* ptoken, CSession* psession);
	virtual ENHandlerResult ProcessUpdateSucc(CHandlerTokenBasic* ptoken, CSession* psession);
};

//class CNotifyAutoReleaseTable : public NewProcessor
//{
//    virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession);
//    virtual ENHandlerResult ProcessUpdateSucc(CHandlerTokenBasic* ptoken, CSession* psession);
//    virtual ENHandlerResult ProcessUpdateFailed(CHandlerTokenBasic* ptoken, CSession* psession);
//    virtual void ProcessSessionTimeOut(CHandlerTokenBasic* ptoken, CSession* psession);
//};

class CRequestOffLine : public NewProcessor
{
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession);
};

class CRequestTableDetail : public NewProcessor
{
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession);
};

class CNotifyCreateTable : public NewProcessor
{
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession);
	virtual ENHandlerResult ProcessUpdateSucc(CHandlerTokenBasic * ptoken, CSession * psession);
	virtual ENHandlerResult ProcessUpdateFailed(CHandlerTokenBasic * ptoken, CSession * psession);
};

//class CRequestDissolveTeaBarTable : public NewProcessor
//{
//	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession);
//	virtual ENHandlerResult ProcessUpdateSucc(CHandlerTokenBasic * ptoken, CSession * psession);
//};

class CGmReuqestTableLog : public ProcesserBasic
{
	virtual ENProcessResult ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession);
};

class CGmReuqestDissolveTable : public NewProcessor
{
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession);
	virtual ENHandlerResult ProcessUpdateSucc(CHandlerTokenBasic * ptoken, CSession * psession);
	virtual ENHandlerResult ProcessUpdateFailed(CHandlerTokenBasic* ptoken, CSession* psession);
};

//class CRequestSkipMatchGame : public NewProcessor
//{
//	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession);
//	virtual ENHandlerResult ProcessGetSucc(CHandlerTokenBasic * ptoken, CSession * psession);
//	virtual ENHandlerResult ProcessUpdateSucc(CHandlerTokenBasic *ptoken, CSession *psession);
//};

/*
金币场匹配
*/
class CRequestCoinMatchGame : public NewProcessor
{
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession);
	virtual ENHandlerResult ProcessGetSucc(CHandlerTokenBasic * ptoken, CSession * psession);
	virtual ENHandlerResult ProcessUpdateSucc(CHandlerTokenBasic *ptoken, CSession *psession);
};

//class CInnerUpdateSkipMatchResult : public NewProcessor
//{
//public:
//	friend class GetState;
//	CInnerUpdateSkipMatchResult()
//	{
//		m_CNoInfoGetState.m_pCInnerUpdateSkipMatchResult = this;
//		m_CHadInfoGetState.m_pCInnerUpdateSkipMatchResult = this;
//	}
//
//	class GetState
//	{
//		friend class CInnerUpdateSkipMatchResult;
//		CInnerUpdateSkipMatchResult * m_pCInnerUpdateSkipMatchResult;
//
//	public:
//		virtual ENHandlerResult ProcessGet(CHandlerTokenBasic * ptoken, CSession * psession)
//		{
//			return EN_Handler_Done;
//		}
//	};
//
//	class GetState_NoInfo : public GetState
//	{
//	public:
//		virtual ENHandlerResult ProcessGet(CHandlerTokenBasic * ptoken, CSession * psession);
//	};
//
//	class GetState_HadInfo : public GetState
//	{
//	public:
//		virtual ENHandlerResult ProcessGet(CHandlerTokenBasic * ptoken, CSession * psession);
//	};
//
//	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession);
//	virtual ENHandlerResult ProcessUpdateSucc(CHandlerTokenBasic *ptoken, CSession *psession);
//	virtual ENHandlerResult ProcessUpdateFailed(CHandlerTokenBasic *ptoken, CSession *psession);
//	virtual ENHandlerResult ProcessGetSucc(CHandlerTokenBasic *ptoken, CSession *psession);
//
//private:
//	GetState_NoInfo m_CNoInfoGetState;       //未获得任何用户信息状态
//	GetState_HadInfo m_CHadInfoGetState;     //已获得过一次用户信息状态
//};

class CRequestTrusteeship : public NewProcessor
{
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession);
};

class CRequestSetNextRoundHandCards : public NewProcessor
{
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession);
};

class CRequestSetNextCard : public NewProcessor
{
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession);
};

/*
结果在数据库中更新
*/
class CInnerUpdateCoinMatchResult : public NewProcessor
{
public:
	friend class GetState;
	CInnerUpdateCoinMatchResult()
	{
		m_CNoInfoGetState.m_pCInnerUpdateCoinMatchResult = this;
		m_CHadInfoGetState.m_pCInnerUpdateCoinMatchResult = this;
	}

	class GetState
	{
		friend class CInnerUpdateCoinMatchResult;
		CInnerUpdateCoinMatchResult * m_pCInnerUpdateCoinMatchResult;

	public:
		virtual ENHandlerResult ProcessGet(CHandlerTokenBasic * ptoken, CSession * psession)
		{
			return EN_Handler_Done;
		}
	};

	class GetState_NoInfo : public GetState
	{
	public:
		virtual ENHandlerResult ProcessGet(CHandlerTokenBasic * ptoken, CSession * psession);
	};

	class GetState_HadInfo : public GetState
	{
	public:
		virtual ENHandlerResult ProcessGet(CHandlerTokenBasic * ptoken, CSession * psession);
	};

	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession);
	virtual ENHandlerResult ProcessUpdateSucc(CHandlerTokenBasic *ptoken, CSession *psession);
	virtual ENHandlerResult ProcessUpdateFailed(CHandlerTokenBasic *ptoken, CSession *psession);
	virtual ENHandlerResult ProcessGetSucc(CHandlerTokenBasic *ptoken, CSession *psession);

private:
	GetState_NoInfo m_CNoInfoGetState;       //未获得任何用户信息状态
	GetState_HadInfo m_CHadInfoGetState;     //已获得过一次用户信息状态
};

/*
踢出用户
*/
class CInnerNotifyKickoutUser : public NewProcessor
{
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession);
	virtual ENHandlerResult ProcessUpdateSucc(CHandlerTokenBasic* ptoken, CSession* psession);
	virtual ENHandlerResult ProcessUpdateFailed(CHandlerTokenBasic* ptoken, CSession* psession);
};


/*
通知
*/