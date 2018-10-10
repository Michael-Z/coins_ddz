#pragma once
#include "singleton.h"
#include "type_def.h"
#include "Timer_Handler_Base.h"
#include <map>
#include <vector>
#include "PBConfigBasic.h"

using namespace std;

#define TABLE_RECYCLE_TIMER 1000

enum ENGameServiceState
{
    EN_Game_Service_State_IDEL = 1,
    EN_Game_Service_State_Connecting = 2,
    EN_Game_Service_State_Registing = 3,
    EN_Game_Service_State_Ready = 4,
    EN_Game_Service_State_Working = 5,
    EN_Game_Service_State_Retired = 6,
};

class ZoneTableManager : public CTimerOutListener
{
public:
    static ZoneTableManager * Instance();
    void OnRetire();
	bool Init();
    int64 GetGamesvrdIDForTable(int64 tid, int zone_type);
    bool SetGamesvrdIDForTable(int64 tid, int gamesvid, int zone_type);
    int64 AllocNewTableID(int table_type, int zone_type);
	int64 AllocNewPublicTableID(int table_type, int zone_type,bool & is_create);
    bool CheckAllocedTable(int64 tid, int zone_type);
    bool BindTidForUser(int64 uid,int64 tid, int zone_type,bool need_reset_gameid = true);
    bool UnBindTidForUser(int64 tid, int zone_type);
    bool RefreshTable(int64 tid, int zone_type);
    bool DissolveTable(int64 tid, int zone_type);
    bool GetUidByBindTid(int64 tid,int zone_type,int64 & uid);
    int GetNodeTypeForTableId(int64 tid, int zone_type);
    int GetNodeTypeByMatchTableType(int64 tid, int zone_type);
    int GetTableTypeForTableId(int64 tid, int zone_type);
	void OnPlayerLogoutTable(int64 tid,int ttype,int num);
	virtual int ProcessOnTimerOut(int Timerid);
public:
    CTimer _table_recycle_timer;

    vector<int64> _lz_alloced;
    map<int64,int> _lz_alloced_map;
    map<int, vector<int64> > _lz_free_maps;
    map<int, map<int64,int> > _lz_alloced_public_table_player_maps;
    ENGameServiceState _state;
};
