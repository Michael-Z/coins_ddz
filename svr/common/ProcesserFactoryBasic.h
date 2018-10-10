/******************************************************************************
  文 件 名   : ProcesserFactoryBasic.h
  版 本 号   : 初稿
  作    者   : BarretXia
  生成日期   : 2015年10月9日
  最近修改   :
  功能描述   : 
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
#include "HandlerTokenBasic.h"
#include "Session.h"
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

class ProcessorBase
{
public:
    ProcessorBase() {};
    virtual ~ProcessorBase() {};
    virtual void Process(CHandlerTokenBasic* ptoken, CSession* psession) = 0;
};

#define REGIST_MSG_HANDLER(msg_id, handler) \
do { \
	ProcessorBase* phandler = new handler(); \
	_handler_map[msg_id] = phandler; \
} while(0)

typedef map<int, ProcessorBase*> HandlerMap;

class ProcesserFactoryBasic 
{
public :
	virtual int Process(CHandlerTokenBasic * ptoken, CSession * psession);
	virtual int ProcessInnerMsg(CSession * psession);
	virtual ProcessorBase * GetProcesser(int cmd){return NULL;};

protected:
    HandlerMap _handler_map;
};


