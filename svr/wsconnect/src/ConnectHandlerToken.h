#pragma once
#include "HandlerTokenBasic.h"
#include "Timer_Handler_Base.h"
#include <string>
#include "type_def.h"

using namespace std;

#define HANDLER_HEARTBEAT_TIMER 0x1000

class ConnectHandlerToken:public CHandlerTokenBasic,public CTimerOutListener
{
public:
	ConnectHandlerToken(CTCPSocketHandler * phandler)
		:CHandlerTokenBasic(phandler)
	{
		_authorized = false;
		_uid = -1;
		_bind_account = false;
		_bind_uid = false;
		_enabled_heartbeat = false;
	}

	~ConnectHandlerToken()
	{
		_heartbeat_timer.StopTimer();
	}

	virtual int ProcessOnTimerOut(int Timerid);

	void EnableHeartBeatCheck()
	{
		_enabled_heartbeat = true;
		_heartbeat_timer.SetTimeEventObj(this,HANDLER_HEARTBEAT_TIMER);
		_heartbeat_timer.StartTimerBySecond(60*5,true);
		_last_request_stamp = time(NULL);
	}
	void ResetHeartBeatCheck()
	{
		if (_enabled_heartbeat)
		{
			_heartbeat_timer.StopTimer();
			_heartbeat_timer.StartTimerBySecond(60*5,true);
			_last_request_stamp = time(NULL);
		}
	}
	void DisableHeartBeatCheck()
	{
		if (_enabled_heartbeat)
		{
			_heartbeat_timer.StopTimer();
			_enabled_heartbeat = false;
		}
	}
	virtual void release();
public:
	bool _authorized;
	PBBPlayerPositionInfo _pos;
	bool _bind_account;
	string _account;
	bool _bind_uid;
	int64 _uid;
    time_t _login_time;
    int _acc_type;
    int _channel;
	PBBPlayerPositionInfo _requesting_pos;
	CTimer _heartbeat_timer;
	int _last_request_stamp;
	bool _enabled_heartbeat;
};

