
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
class CProcessLogMsg: public ProcesserBasic
{
    virtual ENProcessResult ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession);
private:
    void ProcessChipJournal(const PBCSMsg& msg);
    void ProcessRegist(const PBCSMsg& msg);
    void ProcessLogin(const PBCSMsg& msg);
    void ProcessLogout(const PBCSMsg& msg);
	void ProcessGameLog(const PBCSMsg& msg);
	void ProcessGameInfoLog(const PBCSMsg& msg);
	void ProcessTeaBarChipsFlow(const PBCSMsg& msg);
    void ProcessTableInfoLog(const PBCSMsg& msg);
    void ProcessTablePlayerLog(const PBCSMsg& msg);
    void ProcessCreateTableLog(const PBCSMsg& msg);
    void ProcessDiamondFlow(const PBCSMsg& a_pbmsg);
    void ProcessBonusFlow(const PBCSMsg& a_pbmsg);
    void ProcessSkipMatchLevelAndStateFlow(const PBCSMsg& a_pbMsg);
	/*
	处理金币场流水
	*/
	void ProcessCoinsFlow(const PBCSMsg& a_pbmsg);
};

