
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
#include "type_def.h"
#include "NewProcessor.h"

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

class CRequestCreateTeaBar : public NewProcessor
{
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession);
	virtual ENHandlerResult ProcessGetSucc(CHandlerTokenBasic* ptoken, CSession* psession);
	virtual ENHandlerResult ProcessUpdateSucc(CHandlerTokenBasic* ptoken, CSession* psession);
};

class CRequestEnterTeaBar : public NewProcessor
{
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession);
	virtual ENHandlerResult ProcessGetSucc(CHandlerTokenBasic* ptoken, CSession* psession);
};

class CRequestApplyJoinTeaBar : public NewProcessor
{
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession);
	virtual ENHandlerResult ProcessGetSucc(CHandlerTokenBasic* ptoken, CSession* psession);
};

class CRequestAgreeUserJoinTeaBar : public NewProcessor
{
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession);
	virtual ENHandlerResult ProcessGetSucc(CHandlerTokenBasic* ptoken, CSession* psession);
	virtual ENHandlerResult ProcessUpdateSucc(CHandlerTokenBasic* ptoken, CSession* psession);
};

class CRequestApplyDropTeaBar : public NewProcessor
{
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession);
	virtual ENHandlerResult ProcessGetSucc(CHandlerTokenBasic* ptoken, CSession* psession);
	virtual ENHandlerResult ProcessUpdateSucc(CHandlerTokenBasic* ptoken, CSession* psession);
    virtual ENHandlerResult ProcessResponseMsg(CHandlerTokenBasic *ptoken, CSession *psession);
};

class CRequestAgreeUserDropTeaBar : public NewProcessor
{
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession);
	virtual ENHandlerResult ProcessGetSucc(CHandlerTokenBasic* ptoken, CSession* psession);
	virtual ENHandlerResult ProcessUpdateSucc(CHandlerTokenBasic* ptoken, CSession* psession);
};

class CRequestTeaBarList : public NewProcessor
{
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession);
	virtual ENHandlerResult ProcessGetSucc(CHandlerTokenBasic* ptoken, CSession* psession);
};

class CNotifyCreateTable : public NewProcessor
{
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession);
	virtual ENHandlerResult ProcessUpdateSucc(CHandlerTokenBasic* ptoken, CSession* psession);
};

class CNotifyTeabarTableGameStart : public NewProcessor
{
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession);
};

class CNotifyTeabarTableGameOver : public NewProcessor
{
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession);
};

class CNotifyTeabarTablePlayerNum : public NewProcessor
{
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession);
	virtual ENHandlerResult ProcessUpdateSucc(CHandlerTokenBasic* ptoken, CSession* psession);
};

class CRequestGetTeaBarUserList : public NewProcessor
{
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession);
};

class CRequestTeaBarInfo : public NewProcessor
{
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession);
};

class CRequestTeaBarMessage : public NewProcessor
{
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession);
};

class CRequestPutChipsToTeaBar : public NewProcessor
{
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession);
	virtual ENHandlerResult ProcessGetSucc(CHandlerTokenBasic* ptoken, CSession* psession);
	virtual ENHandlerResult ProcessUpdateSucc(CHandlerTokenBasic* ptoken, CSession* psession);
};

class CRequestModifySettleNum : public NewProcessor
{
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession);
};

class CRequestRemoveUser : public NewProcessor
{
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession);
	virtual ENHandlerResult ProcessGetSucc(CHandlerTokenBasic* ptoken, CSession* psession);
	virtual ENHandlerResult ProcessUpdateSucc(CHandlerTokenBasic* ptoken, CSession* psession);
};

class CNotifyTeaBarTableNotExist : public NewProcessor
{
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession);
};

class CRequestModifyTeaBarDesc : public NewProcessor
{
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession);
	virtual ENHandlerResult ProcessUpdateSucc(CHandlerTokenBasic* ptoken, CSession* psession);
};

class CRequestFreeTeaBar : public NewProcessor
{
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession);
	virtual ENHandlerResult ProcessGetSucc(CHandlerTokenBasic* ptoken, CSession* psession);
	virtual ENHandlerResult ProcessUpdateSucc(CHandlerTokenBasic* ptoken, CSession* psession);
	virtual ENHandlerResult ProcessUpdateFailed(CHandlerTokenBasic* ptoken, CSession* psession);
};


class CRequestStatisticsTableRecordList : public NewProcessor
{
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession);
};

class CRequestTeaBarStatistics : public NewProcessor
{
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession);
};

class CRequestTeaBarTableSettle : public NewProcessor
{
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession);
};

class CLogTeaBarChipsFlow : public NewProcessor
{
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession);
};

class CRequestTransferTeaBar : public NewProcessor
{
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession);
	virtual ENHandlerResult ProcessGetSucc(CHandlerTokenBasic* ptoken, CSession* psession);
	virtual ENHandlerResult ProcessUpdateSucc(CHandlerTokenBasic* ptoken, CSession* psession);
	virtual ENHandlerResult ProcessUpdateFailed(CHandlerTokenBasic* ptoken, CSession* psession);
};

class CGMRequestQueryTeaBarUserList : public NewProcessor
{
    virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic *ptoken, CSession *psession);
};

class CRequestChangeTeabarCreateInfo : public NewProcessor
{
    virtual ENHandlerResult ProcessGetSucc(CHandlerTokenBasic* ptoken, CSession* psession);
    virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession);
};
