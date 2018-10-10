/******************************************************************************
  文 件 名   : SessionManager.h
  版 本 号   : 初稿
  作    者   : BarretXia
  生成日期   : 2015年10月28日
  最近修改   :
  功能描述   : 会话管理
  函数列表   :
  修改历史   :
  1.日    期   : 2015年10月28日
    作    者   : BarretXia
    修改内容   : 创建文件

******************************************************************************/

/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/
#pragma once
#include "singleton.h"
#include "Session.h"
#include <queue>
#include <map>
using namespace std;
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
#define MAX_SESSION_ID 0x10000

struct stStatsInfo
{
    int count;
    long long avg_time;
};

class SessionManager: public CTimerOutListener
{
public:
	static SessionManager * Instance(void);
	void Init();
	int AllocSessionID();
	CSession * AllocSession();
	int ReleaseSession(int id);
	CSession * GetSession(int id);
public:
	map<int,CSession *> _alloc_id_map;
	queue<int> _free_id_queue;

private:
   	virtual int ProcessOnTimerOut(int Timerid);
    CTimer _timer;

    int _deal_session_count;
    map<int, stStatsInfo> _stats_map; // deal msg info by msg id

public:
    bool LockProcess(CSession* psession);
    void UnlockProcess(const CSession* psession);
    map<int, map<long long, int> > msg_map; // msg正在处理的uid集合
};

