/******************************************************************************
  文 件 名   : poller.h
  版 本 号   : v 0.0.1
  作    者   : BarretXia
  生成日期   : 2015年8月28日
  最近修改   :
  功能描述   : epoll 
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

#include <arpa/inet.h>
#include <myepoll.h>
#include <list.h>
#include "global.h"


#define EPOLL_DATA_SLOT(x)	((x)->data.u64 & 0xFFFFFFFF)
#define EPOLL_DATA_SEQ(x)	((x)->data.u64 >> 32)

enum TPollerState
{
    POLLER_FAIL     = -1,
    POLLER_SUCC,
    POLLER_COMPLETE
};

class CPollerUnit;
class CPollerObject;

struct CEpollSlot {
	uint32_t seq;
	CPollerObject *poller;
	struct CEpollSlot *freeList;
};

class CPollerObject 
{
public:
	CPollerObject (CPollerUnit *thread=NULL, int fd=0);
	virtual ~CPollerObject ();
	
	virtual int OnAttachPoller() = 0;	//加入到epoll中时被调用.(断线重连会调用)
	virtual int InputNotify (void);
	virtual int OutputNotify (void);
	virtual int HangupNotify (void);
	
	void EnableInput(void) 
	{
		newEvents |= EPOLLIN;
	}
	void EnableOutput(void) 
	{
		newEvents |= EPOLLOUT;
	}
	void DisableInput(void) 
	{
		newEvents &= ~EPOLLIN;
	}
	void DisableOutput(void) 
	{
        VBootMsg(CLIB_LOG_LEV_DEBUG,"netfd[%d] DisableOutput!!!", netfd);
		newEvents &= ~EPOLLOUT;
	}

	void EnableInput(bool i) 
	{
		if(i)
			newEvents |= EPOLLIN;
		else
			newEvents &= ~EPOLLIN;
	}
	void EnableOutput(bool o) 
	{
		if(o)
			newEvents |= EPOLLOUT;
		else
			newEvents &= ~EPOLLOUT;
	}

	int AttachPoller (CPollerUnit *thread=NULL);
	int DetachPoller (void);
	int ApplyEvents ();
	uint64_t GetPollerEventId();

	friend class CPollerUnit;

protected:
	int netfd;
	CPollerUnit *ownerUnit;
	int newEvents;
	int oldEvents;
	struct CEpollSlot *epslot;
};

class CPollerUnit 
{
public:
	friend class CPollerObject;
	CPollerUnit(int mp);
	~CPollerUnit();

	int SetMaxPollers(int mp);
	int GetMaxPollers(void) const { return maxPollers; }
	int InitializePollerUnit(void);
	void WaitPollerEvents(int);
	void ProcessPollerEvents(void);
	int GetFD(void) { return epfd; }

private:
	int VerifyEvents(struct epoll_event *);
	int Epctl (int op, int fd, struct epoll_event *events);
	int GetSlotId (CEpollSlot *p) {return ((char*)p - (char*)pollerTable) / sizeof (CEpollSlot);}

	void FreeEpollSlot (CEpollSlot *p);
	struct CEpollSlot *AllocEpollSlot ();

	struct epoll_event *ep_events;
	int epfd;
	int maxPollers;
	int usedPollers;
	struct CEpollSlot *freeSlotList;	
	struct CEpollSlot *pollerTable;

	int nrEvents;
};

