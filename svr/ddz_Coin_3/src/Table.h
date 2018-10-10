#pragma once
#include "type_def.h"
#include "poker_msg.pb.h"
#include "Timer_Handler_Base.h"
#include "poker_table_log.h"

#define  DISSOLVE_TABLE_TIMER 10000
#define  OPERATION_TABLE_TIMER 10001
#define  ACTION_WAITING_TIMER 10002
#define  CHECK_USER_OFF_TIMER 10003
#define  TABLE_RECYCLE_TIMER 10004
#define  READY_TIMER    10005
#define  ROBOT_TIMER	10006
#define  WAITING_OTHER_TIMER 10007

class CPBGameTable : public PBSDRGameTable , public CTimerOutListener
{
public:
    //显示得写出复制构造函数是必须的
    CPBGameTable(const CPBGameTable & table)
    {
        CopyFrom(table);
    }
    CPBGameTable();
    ~CPBGameTable();
    void Init(int64 tid,const PBSDRTableConfig & conf);
    void StartTimer(int inteval,bool loop = false);
    void StopTimer();

    void StartActionWaitingTimerByMillis(int a_iIntval,bool a_bLoop = false);
    void StopActionWaitingTimer();
    void RegisterActionWaitingTime(int a_iType,int a_iMilliSecond);
    int GetWaitingTime(int a_iType);

    void StartCheckUserOffTimer(int inteval,bool loop = false);
	void StopCheckUserOffTimer();

    void StartReadyTimer(int a_iIntval,bool a_bLoop = false);
    void StopReadyTimer();

	void StartRobotTimer(int a_iIntval,bool a_bLoop = false);
	void StopRobotTimer();

	void StartWaitingOhterTimer(int a_iIntval, bool a_bLoop = false);
	void StopWaitingOhterTimer();


    virtual int ProcessOnTimerOut(int Timerid);
public:
    int64 start_stamp;
    CTimer timer;
    PBSDRTableFlowRecordItem flow_record_item;
    TableLog<1000*1024> table_log;

    /*
     * 动作类型和等待时间map<type,time>   其中time为毫秒级
    */
    map<int,int> m_mpActionWaitingTimeMap;

    /*
     * 动作后延长定时器
    */
    CTimer m_ActionWaitingTimer;

    /*
     * 验证用户离开定时器
    */
    CTimer m_CheckUserOffTimer;

    /*
     * 准备定时器，将未准备的玩家自动准备
    */
    CTimer m_ReadyTimer;

	/*
	 * 机器人定时器
	*/
	CTimer m_RobotTimer;

	/*
	 * 等待其他玩家定时器
	*/
	CTimer m_WaitingOtherTimer;

    /*
     * 暂存未发送的InnerMsg
    */
    vector<PBCSMsg> m_vtInnerMsg;

};


