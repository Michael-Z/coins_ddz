#pragma once
#include "singleton.h"
#include "type_def.h"
#include "Timer_Handler_Base.h"
#include <map>
#include <vector>

using namespace std;

#define LZMJ_TABLE_RECYCLE_TIMER 1000

class TableManager : public CTimerOutListener
{
public:
	static TableManager * Instance();
	bool Init();
    int64 GetGamesvrdIDForTable(int64 tid, int game_type = 4);  // game_type 默认为EN_Node_Game
    bool SetGamesvrdIDForTable(int64 tid,int gamesvid, int game_type = 4);
    int64 AllocNewTableID(int game_type = 4);
	bool CheckAllocedTable(int64 tid);
    bool BindTidForUser(int64 uid,int64 tid, int game_type = 4);
    bool UnBindTidForUser(int64 uid,int64 tid, int game_type = 4);
    bool RefreshTable(int64 tid, int game_type = 4);
    bool DissolveTable(int64 tid, int game_type = 4);
	virtual int ProcessOnTimerOut(int Timerid);
public:
	vector<int64> _alloced;
	map<int64,int> _alloced_map;
	vector<int64> _free;
	CTimer _lzmj_table_recycle_timer;
    // fpf
    vector<int64> _fpf_alloced;
    map<int64,int> _fpf_alloced_map;
    vector<int64> _fpf_free;
    // daer
    vector<int64> _daer_alloced;
    map<int64,int> _daer_alloced_map;
    vector<int64> _daer_free;
};


