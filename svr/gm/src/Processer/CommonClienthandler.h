
/******************************************************************************
  文 件 名   : CommonClienthandler.h
  版 本 号   : 初稿
  作    者   : BarretXia
  生成日期   : 2015年10月12日
  最近修改   :
  功能描述   : 一般游戏命令处理类
  函数列表   :
  修改历史   :
  1.日    期   : 2015年10月12日
    作    者   : BarretXia
    修改内容   : 创建文件

******************************************************************************/

/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/
#pragma once
#include "ProcesserBasic.h"
#include "NewProcessor.h"
#include <map>
/*----------------------------------------------*
 * 外部变量说明                                 *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 外部函数原型说明                             *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 内部函数原型说明                             *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 全局变量                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 模块级变量                                   *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 常量定义                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 宏定义                                       *
 *----------------------------------------------*/
class CRegistInnerServer : public ProcesserBasic
{
	virtual ENProcessResult ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession);
};

class CProcessPHPMsg: public ProcesserBasic
{
    virtual ENProcessResult ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession);
    virtual ENProcessResult ProcessResponseMsg(CHandlerTokenBasic * ptoken,CSession * psession);
};

class CUpdateChipsMulit: public NewProcessor
{
    enum UpdateState
    {
        Wait_Del    = 0,
        Wait_Update = 1,
    };
    virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession);
    virtual ENHandlerResult ProcessUpdateSucc(CHandlerTokenBasic* ptoken, CSession * psession);
    virtual ENHandlerResult ProcessUpdateFailed(CHandlerTokenBasic* ptoken, CSession* psession);
};

class CPushMessageMulit: public NewProcessor
{
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession);
	virtual ENHandlerResult ProcessGetSucc(CHandlerTokenBasic* ptoken, CSession * psession);
};

class CRequestUserActivityGameInfo : public NewProcessor
{
    virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession);
    virtual ENHandlerResult ProcessGetSucc(CHandlerTokenBasic* ptoken, CSession * psession);
    virtual ENHandlerResult ProcessUpdateSucc(CHandlerTokenBasic* ptoken, CSession * psession);

    void FileResponseMsgBySkipInfo(const PBUserSkipMatchInfo & a_pbUserSkipMatchInfo,std::string & a_szJsonMsg,int a_iGameType);
};

class CRequestNotifyShareSuccess : public NewProcessor
{
    virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession);
    virtual ENHandlerResult ProcessGetSucc(CHandlerTokenBasic* ptoken, CSession * psession);
    virtual ENHandlerResult ProcessUpdateSucc(CHandlerTokenBasic* ptoken, CSession * psession);
};
