#include "Table.h"
#include "global.h"
#include "TableModle.h"

CPBGameTable::CPBGameTable()
{
}

CPBGameTable::~CPBGameTable()
{
    StopTimer();
    StopCheckUserOffTimer();
    StopActionWaitingTimer();
}

void CPBGameTable::Init(int64 tid,const PBSDRTableConfig & conf)
{
    start_stamp = time(NULL);
    set_tid(tid);
    set_state(EN_TABLE_STATE_WAIT);
	set_creator_uid(conf.creator_uid());

    // 定随机庄
    int dealer_index = start_stamp%conf.seat_num();
    set_dealer_index(dealer_index);

    set_last_winner(-1);

    for(int i = 0; i < conf.seat_num(); i++)
    {
        PBSDRTableSeat& seat = *add_seats();
        seat.set_index(i);
        seat.set_state(EN_SEAT_STATE_NO_PLAYER);
    }

    mutable_config()->CopyFrom(conf);
    timer.SetTimeEventObj(this, DISSOLVE_TABLE_TIMER);
    m_ActionWaitingTimer.SetTimeEventObj(this,ACTION_WAITING_TIMER);
    m_CheckUserOffTimer.SetTimeEventObj(this,CHECK_USER_OFF_TIMER);
    m_ReadyTimer.SetTimeEventObj(this,READY_TIMER);
	m_RobotTimer.SetTimeEventObj(this, ROBOT_TIMER);
	m_WaitingOtherTimer.SetTimeEventObj(this, WAITING_OTHER_TIMER);
}

/*
 * 解散定时器（弃用）
*/
void CPBGameTable::StartTimer(int inteval,bool loop)
{
    timer.StartTimerBySecond(inteval,loop);
}

void CPBGameTable::StopTimer()
{
    timer.StopTimer();
}

/*
 * 检测玩家是否离开定时器
*/
void CPBGameTable::StartCheckUserOffTimer(int inteval,bool loop)
{
    m_CheckUserOffTimer.StartTimerBySecond(inteval,loop);
}

void CPBGameTable::StopCheckUserOffTimer()
{
    m_CheckUserOffTimer.StopTimer();
}

/*
 * 某个动作执行后，后续等待的定时器
*/
void CPBGameTable::StartActionWaitingTimerByMillis(int a_iIntval,bool a_bLoop)
{
    m_ActionWaitingTimer.StartTimer(a_iIntval,a_bLoop);
}

void CPBGameTable::StopActionWaitingTimer()
{
    m_ActionWaitingTimer.StopTimer();
}

/*
 * 注册动作等待时间
*/
void CPBGameTable::RegisterActionWaitingTime(int a_iType,int a_iMilliSecond)
{
    m_mpActionWaitingTimeMap[a_iType] = a_iMilliSecond;
}

/*
 * 获得动作等待时间
*/
int CPBGameTable::GetWaitingTime(int a_iType)
{
    return m_mpActionWaitingTimeMap[a_iType];
}

/*
 * 准备定时器
*/
void CPBGameTable::StartReadyTimer(int a_iIntval,bool a_bLoop)
{
    m_ReadyTimer.StartTimerBySecond(a_iIntval,a_bLoop);
}

void CPBGameTable::StopReadyTimer()
{
    m_ReadyTimer.StopTimer();
}

/*
 * 机器人定时器
*/
void CPBGameTable::StartRobotTimer(int a_iIntval, bool a_bLoop)
{
	m_RobotTimer.StartTimerBySecond(a_iIntval, a_bLoop);
}

void CPBGameTable::StopRobotTimer()
{
	m_RobotTimer.StopTimer();
}

/*
   等待其他人定时器
*/
void CPBGameTable::StartWaitingOhterTimer(int a_iIntval, bool a_bLoop)
{
	m_WaitingOtherTimer.StartTimerBySecond(a_iIntval, a_bLoop);
}

void CPBGameTable::StopWaitingOhterTimer()
{
	m_WaitingOtherTimer.StopTimer();
}

int CPBGameTable::ProcessOnTimerOut(int Timerid)
{
    VLogMsg(CLIB_LOG_LEV_DEBUG,"process sdr table[%ld] time out.",this->tid());
    switch(Timerid)
    {
        //解散定时器(弃用)
        case DISSOLVE_TABLE_TIMER :
            TableLogic::OnDissolveTableTimeOut(*this);
            break;
        //动作等待定时器(等待某个动作做完)(没有用到，暂时保留)
        case ACTION_WAITING_TIMER:
            TableLogic::OnActionWaitingTimerOut(*this);
            break;
        //检测用户是否离线定时器
        case CHECK_USER_OFF_TIMER:
            TableLogic::OnCheckUserOffTimerOut(*this);
            break;
        //未准备玩家准备
        case READY_TIMER:
            //TableLogic::OnReadTimerTimeOut(*this);
            break;
		//机器人操作定时器
		case ROBOT_TIMER:
			TableLogic::OnRobotTimerTimeOut(*this);
			break;
		//等待其他玩家定时器
		case WAITING_OTHER_TIMER:
			//TableLogic::OnWaitinOtherTimeOut(*this);
			break;

        default:
            break;
    }
    return 0;
}




