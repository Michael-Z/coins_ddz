#pragma once
#include "Timer_Handler_Base.h"
#include "type_def.h"
#include "poker_msg.pb.h"
#include "TCPSocketHandler.h"
#include <map>
#include <string>
using google::protobuf::RepeatedField;
using namespace std;

#define HEART_BEAT_TIMER 10000

enum ENRobotState
{
	EN_Robot_State_Idle = 0,
	EN_Robot_State_Wait_Login_Response = 1,
	EN_Robot_State_Wait_Enter_Match = 2,
	EN_Robot_State_Enter_Match_Succ = 3,
	EN_Robot_State_Wait_Skip_Match = 4,
	EN_Robot_State_Wait_Coin_Match = 5,
	EN_Robot_State_Error = 100,
};

class CRobot : public CTimerOutListener
{
public:
	CRobot();
	~CRobot();

	void Login(int64 a_iMatchid, const PBSourceInfoRequestingRobot & a_pbSourceInfo);
	void Logout();

	void SetInfo(string account, string url, string nick, int play_num, int win_time);
	bool Init();
	void ProcessState();

	virtual int ProcessOnTimerOut(int TimerID);

	void OnHandlerClosed();
	void Recycle();
	long long GetUid() { return _uid; }
	void SetUid(long long uid);
	void SetChips(long long chips);
private:
	void OnResponseLogin();
	void OnResponseEnterMatch();
	void OnResponseSkipMatch();
	void OnResponseCoinMatch();
public:
	string _account;
	PBCSMsg _response_msg;
	int _play_num;
	int _win_num;
private:
	string _url;
	string _nick;
	long long _uid;
	long long _chips;
	CTCPSocketHandler* _phandler;
	ENRobotState _state;
	CTimer _heartbeat_timer;
	int _last_active_time_stamp;
	int _pos_type;
	int _gamesvid;
	int64 _tid;
	int64 _matchid;
	int32 _iConenctId;
	PBSourceInfoRequestingRobot _source_info;
};