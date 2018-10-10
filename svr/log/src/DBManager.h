#pragma once
#include "singleton.h"
#include "DataBaseHandler.h"
#include "PBConfigBasic.h"
#include "Timer_Handler_Base.h"
#include <map>
#include <vector>
#include "type_def.h"

#define  LOG_TIMER 1

class CThreadPara
{
public:
    vector<string> sql_vec;
    PBConfigBasic<PBLogsvrdConfig> log_config;
};

class CDBManager: public CTimerOutListener
{
public:
	static CDBManager* Instance(void);
    bool Init();
    void AddSql(string sql);
private:
    CDataBaseHandler db_handler;
    PBConfigBasic<PBLogsvrdConfig> log_config;
    vector<string> sql_vec;

public:
    // 在线在玩
    int play;
    int play_robot;
    std::map<int, int> online_map;
    std::map<int, int> robot_map;
	std::map<int64,int> match_map;
	int bj_play;
    std::map<int, std::map<int, int> > atplay_map;  // <game_type, <svrd_id, play> >
private:
   	virtual int ProcessOnTimerOut(int Timerid);
	CTimer timer;
};

