#pragma once

#include "singleton.h"
#include <map>
#include "poker_msg.pb.h"
#include "Table.h"
#include "type_def.h"

using namespace std;

typedef map<int64,CPBGameTable> TableMap;
typedef map<int64,int64> UserTableMap;
typedef std::set<int64> TableSet;
typedef map<int, TableSet > TablePlayerNumMap;

/*
 * key : 房间号
 * value : 人数
*/
typedef map<int,int> TableActiveMap;

enum ENGameServiceState
{
    EN_Game_Service_State_IDEL	= 1,
    EN_Game_Service_State_Connecting = 2,
    EN_Game_Service_State_Registing = 3,
    EN_Game_Service_State_Ready = 4,
    EN_Game_Service_State_Working = 5,
    EN_Game_Service_State_Retired = 6,
};

class TableManager : public CTimerOutListener
{

#define  REPORT_AT_PLAY_TIMER 10000
#define  TABLES_RECYCLE_TIMER 10001

public:
	static TableManager * Instance(void);
	CPBGameTable * FindTable(int64 tid);
    CPBGameTable * CreateTable(int64 tid,const PBSDRTableConfig & conf);
	void OnPlayerEnterTable(int64 uid,int64 tid,bool a_bIsReConnect = false);
	void OnPlayerLeaveTable(int64 uid, int64 tid);
	int64 GetPlayerTableID(int64 uid);
    void DissolveTable(int64 a_iTid);
    void OnRetire();
    void ReportGameInfo();
    void RefreshArgv();
    virtual int ProcessOnTimerOut(int Timerid);
    void Init();
    /*
     * 通过a_ppbConf指针创建房间
     * 该函数会修改m_TablePlayerNumMap的值，使人数是1的房间+1
    */
    CPBGameTable * CreateTableByP(int64 a_iTid,int a_ilevel,PBSDRTableConfig * a_ppbConf = NULL);
    int64 GetTableForUser(int a_iLevel, int iAccType,string a_stNick,int a_iExclude_tid,PBSDRTableConfig * a_ppbConf);

public:
	TableMap _tablemap;
	UserTableMap _usertablemap;
    ENGameServiceState _state;
    CTimer timer;

    TablePlayerNumMap  m_TablePlayerNumMap;
    int64 m_StartAllocTid;
};




