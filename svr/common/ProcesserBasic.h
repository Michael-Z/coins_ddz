/******************************************************************************
  文 件 名   : ProcesserBasic.h
  版 本 号   : 初稿
  作    者   : BarretXia
  生成日期   : 2015年10月9日
  最近修改   :
  功能描述   : 句柄处理基类
  函数列表   :
  修改历史   :
  1.日    期   : 2015年10月9日
    作    者   : BarretXia
    修改内容   : 创建文件

******************************************************************************/

/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/
#pragma once
#include "Session.h"
#include "global.h"
#include "SessionManager.h"
#include "ProcesserFactoryBasic.h"
#include "Message.h"
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
enum ENPreProcessResult
{
	EN_Pre_Process_Result_Query = 1,
	EN_Pre_Process_Result_Ready = 2,
	EN_Pre_Process_Result_Done = 3,
};

class ProcesserBasic: public ProcessorBase
{
public:
	virtual ENProcessResult ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession){VLogMsg(CLIB_LOG_LEV_DEBUG,"process request msg by default");return EN_Process_Result_Completed;};
	virtual ENProcessResult ProcessUpdateUserDataCompleted(CHandlerTokenBasic * ptoken,CSession * psession);
	virtual ENProcessResult ProcessPushMsg(CHandlerTokenBasic * ptoken,CSession * psession){VLogMsg(CLIB_LOG_LEV_DEBUG,"process push msg by default");return EN_Process_Result_Completed;};
  	virtual ENProcessResult ProcessResponseMsg(CHandlerTokenBasic * ptoken,CSession * psession);

	virtual ENProcessResult ProcessHallMsg(CHandlerTokenBasic * ptoken,CSession * psession){VLogMsg(CLIB_LOG_LEV_DEBUG,"process hall msg by default");return EN_Process_Result_Completed;};
	virtual ENProcessResult ProcessRoomMsg(CHandlerTokenBasic * ptoken,CSession * psession){VLogMsg(CLIB_LOG_LEV_DEBUG,"process room msg by default");return EN_Process_Result_Completed;};
	virtual ENProcessResult ProcessUserMsg(CHandlerTokenBasic * ptoken,CSession * psession){VLogMsg(CLIB_LOG_LEV_DEBUG,"process user msg by default");return EN_Process_Result_Completed;};
	virtual ENProcessResult ProcessGameMsg(CHandlerTokenBasic * ptoken,CSession *psession){VLogMsg(CLIB_LOG_LEV_DEBUG,"process game msg by default");return EN_Process_Result_Completed;};
	virtual ENProcessResult ProcessRouteMsg(CHandlerTokenBasic * ptoken,CSession *psession){VLogMsg(CLIB_LOG_LEV_DEBUG,"process route msg by default");return EN_Process_Result_Completed;};
	virtual ENProcessResult ProcessDBProxyMsg(CHandlerTokenBasic * ptoken,CSession *psession){VLogMsg(CLIB_LOG_LEV_DEBUG,"process dbproxy msg by default");return EN_Process_Result_Completed;};
	virtual ENProcessResult ProcessOtherMsg(CHandlerTokenBasic * ptoken,CSession * psession){VLogMsg(CLIB_LOG_LEV_DEBUG,"process other msg by default");return EN_Process_Result_Completed;}

	virtual ENPreProcessResult PreProcessRequest(CHandlerTokenBasic * ptoken,CSession * psession);
	virtual ENProcessResult ProcessResponseQueryKVDBData(CHandlerTokenBasic * ptoken,CSession * psession);
	virtual ENProcessResult ProcessResponseUpdateKVDBData(CHandlerTokenBasic * ptoken,CSession * psession);
	virtual void ProcessQueryUserDataFailed(CHandlerTokenBasic * ptoken,CSession * psession);
	virtual void ProcessUpdateUserDataFailed(CHandlerTokenBasic * ptoken,CSession * psession);
	
	virtual void Process(CHandlerTokenBasic * ptoken,CSession * psession);
private:
	//为了避免重复的调用导致逻辑混乱 限制只能由框架层来掉用.
	virtual ENProcessResult ProcessQueryUserData(CHandlerTokenBasic * ptoken,CSession * psession);
	virtual ENProcessResult ProcessUpdateUserData(CHandlerTokenBasic * ptoken,CSession * psession);

protected:
    void AddGetData(CSession* psession, long long uid, int key);
};

