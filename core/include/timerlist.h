/******************************************************************************
  文 件 名   : timerlist.h
  版 本 号   : v 0.0.1
  作    者   : BarretXia
  生成日期   : 2015年8月28日
  最近修改   :
  功能描述   : 定时器列表
  函数列表   :
  修改历史   :
  1.日    期   : 2015年8月28日
    作    者   : BarretXia
    修改内容   : 创建文件
******************************************************************************/

/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/

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

#pragma once

#include <list.h>
#include <timestamp.h>

class CTimerObject;
class CTimerUnit;

class CTimerList {
private:
	CListObject<CTimerObject> tlist;
	int timeout;
	CTimerList *next;

public:
	friend class CTimerUnit;
	friend class CTimerObject;
	CTimerList(int t) : timeout(t), next(NULL) { }
	~CTimerList(void) { tlist.FreeList(); }
	int CheckExpired(int64_t now=0);
};

class CTimerUnit {
private:
	CTimerList pending;
	CTimerList *next;
public:
	friend class CTimerObject;
	CTimerUnit(void);
	~CTimerUnit(void);

	CTimerList *GetTimerList(int);
	int ExpireMicroSeconds(int);
	int CheckExpired(int64_t now=0);
	int CheckPending(void);
};

class CTimerObject: private CListObject<CTimerObject> {
private:
	int64_t objexp;

public:
	friend class CTimerList;
	friend class CTimerUnit;
	CTimerObject() { }
	virtual ~CTimerObject(void);
	virtual void TimerNotify(void);
	void DisableTimer(void) { ResetList(); }
	void AttachTimer(class CTimerList *o);
	void AttachZeroTimer(class CTimerUnit *o) { ListMoveTail(o->pending.tlist); }
};

