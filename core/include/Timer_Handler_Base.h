/******************************************************************************
  文 件 名   : Timer_Handler_Base.h
  版 本 号   : v 0.0.1
  作    者   : 定时器
  生成日期   : 2015年8月30日
  最近修改   :
  功能描述   : 定时器功能
  函数列表   :
  修改历史   :
  1.日    期   : 2015年8月30日
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

#include "timerlist.h"
#include "NewReactor.h"

class CTimerOutListener
{
public:
	virtual int ProcessOnTimerOut(int Timerid)=0;
};

class CTimer_Handler_Base: public CTimerObject
{
	public:
		virtual void StartTimer(long interval, bool isloop=false)=0;
		virtual void StartTimerBySecond(long ssecond, bool isloop=false)=0;
		virtual void StopTimer(void)=0;
};

class CTimer: public CTimer_Handler_Base
{
	public:
		CTimer(){_isloop = false;}
	public:
		virtual void StartTimer(long millisecond, bool isloop=false);
		virtual void StartTimerBySecond(long ssecond, bool isloop=false);
		virtual void StopTimer(void);
		void SetTimeEventObj(CTimerOutListener * obj, int id=0);
		virtual void TimerNotify(void);
	private:
		int _nId;
		CTimerOutListener * _TimeListener;
		bool _isloop;
		long _interval;
};


