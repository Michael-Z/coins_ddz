#pragma once
#include "singleton.h"
#include <map>
#include <string>
#include "poker_msg.pb.h"
#include "poker_data.pb.h"
#include "global.h"
#include "type_def.h"
#include "Timer_Handler_Base.h"
#include "singleton.h"
#include <vector>
#include "Robot.h"
#include <algorithm>

using namespace std;

#define PUT_ROBOT_TIMER 10000

typedef vector<CRobot*> RobotList;


class RobotManager : public CTimerOutListener, public CSingleton<RobotManager>
{
public:
	RobotManager() {}
	~RobotManager() {}
	bool Init();

	virtual int ProcessOnTimerOut(int Timerid);

	void InitRobot();
	void RecycleRobot(CRobot* probot);
	CRobot* GetRobot(long long uid);
	void PutRobot(int64 a_iMatchid, int a_iRobotNum, const PBSourceInfoRequestingRobot & a_pbSourceInfo);

private:
	RobotList _free_robot_list;
	RobotList _use_robot_list;
};