#pragma once
#include "ProcesserBasic.h"
#include "NewProcessor.h"

class CRegistInnerServer : public ProcesserBasic
{
    virtual ENProcessResult ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession);
};

class CLoginProcesser : public NewProcessor
{
    enum ENLoginState
    {
        EN_Login_State_Request_Uid = 0,
        EN_Login_State_Request_Create_Uid = 1,
        EN_Login_State_Wait_Save = 2,
        EN_Login_State_Wait_Bind = 3,
        EN_Login_State_Request_User_Data = 4,
    };
public:
    ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession);
    ENHandlerResult ProcessResponseMsg(CHandlerTokenBasic* ptoken, CSession* psession);
    ENHandlerResult OnResponseAccountUid(CHandlerTokenBasic * ptoken, CSession * psession);
    ENHandlerResult OnResponseCreateUid(CHandlerTokenBasic * ptoken, CSession * psession);
    ENHandlerResult OnResponseSave(CHandlerTokenBasic * ptoken, CSession * psession);
    ENHandlerResult OnResponseBind(CHandlerTokenBasic * ptoken, CSession * psession);
    ENHandlerResult ProcessGetSucc(CHandlerTokenBasic* ptoken, CSession* psession);
    ENHandlerResult ProcessGetFailed(CHandlerTokenBasic * ptoken, CSession * psession);
};

class CRequestUserRecord : public NewProcessor
{
    ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession);
    ENHandlerResult ProcessGetSucc(CHandlerTokenBasic* ptoken, CSession* psession);
};

/*
class CRequestGetUserInfo: public ProcesserBasic
{
    virtual ENProcessResult ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession);
    virtual ENProcessResult ProcessUserMsg(CHandlerTokenBasic * ptoken,CSession * psession);
    void CopyUserInfo(CSUserInfo& user_info, const PBUserData user_data);
};

class CRequestUpdateConfig : public ProcesserBasic
{
    virtual ENProcessResult ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession);
};
*/

class CRequestUpdateUserInfo : public ProcesserBasic
{
    virtual ENProcessResult ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession);
    virtual ENProcessResult ProcessUserMsg(CHandlerTokenBasic * ptoken, CSession * psession);
};

class CRequestTableFlowRecord : public NewProcessor
{
    virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession);
};

// 请求回放码
class CRequestReplayCode : public NewProcessor
{
    virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession);
};
// 请求回放码数据
class CRequestReplayCodeData : public NewProcessor
{
    virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession);
};

class CRequestOnClickAvatar : public NewProcessor
{
    virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession);
    virtual ENHandlerResult ProcessGetSucc(CHandlerTokenBasic * ptoken, CSession * psession);
};

class CRequestUserActivityGameInfo : public NewProcessor
{
    virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession);
    virtual ENHandlerResult ProcessGetSucc(CHandlerTokenBasic * ptoken,CSession * psession);
};

class CRequestGiveUpSkipMatchLevel : public NewProcessor
{
public:
    friend class GetState;
    CRequestGiveUpSkipMatchLevel()
    {
        m_CNoInfoGetState.m_pCRequestGiveUpSkipMatchLevel = this;
        m_CHadInfoGetState.m_pCRequestGiveUpSkipMatchLevel = this;
    }

    class GetState
    {
        friend class CRequestGiveUpSkipMatchLevel;
        CRequestGiveUpSkipMatchLevel * m_pCRequestGiveUpSkipMatchLevel;

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

