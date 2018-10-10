/******************************************************************************




  文 件 名   : Session.h
  版 本 号   : 初稿
  作    者   : BarretXia
  生成日期   : 2015年10月28日
  最近修改   :
  功能描述   : 会话
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
#include "poker_msg.pb.h"
#include "Timer_Handler_Base.h"
#include "type_def.h"
#include "TCPSocketHandler.h"
#include <map>
#include "PBPacket.h"
#include "HandlerTokenBasic.h"

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

enum ENSessionKVDBState
{
	EN_Session_KVDB_State_Idle = 0,
	EN_Session_KVDB_State_Wait_Query_Response = 1,
	EN_Session_KVDB_State_Wait_Request_Update = 2,
	EN_Session_KVDB_State_Wait_Update_Response = 3,
};

typedef map<int,PBCSMsg> ResponseMsgMap;
typedef map<int64,bool> KVDBUidFlagMap;
typedef map<int64,PBUserData> KVDBUidDataMap;
typedef map<int64,PBCSMsg> KVDBUidRequestMsgMap;
class NewProcessor;
class CHandlerTokenBasic;
class CSession :public PBSession, public CTimerOutListener
{
public:
	PBHead _head;//协议头
	PBCSMsg _request_msg;//请求
	PBCSMsg _response_msg;//请求
	PBCSMsg _tmp_msg;
	CPBInputPacket * _input_packet;
	PBRoute _request_route;//请求路由信息
	PBRoute _response_route;//响应路由信息
	CTimer _timer;
	long long _uid;//请求链接句柄
	ENMessageType _message_logic_type;
	ResponseMsgMap _response_msg_map;
	int _fsm_state;
	int _kvdb_state;
	KVDBUidFlagMap _kvdb_uid_flag_map;
	KVDBUidDataMap _kvdb_uid_data_map;
	KVDBUidRequestMsgMap _kvdb_uid_query_msg_map;
	KVDBUidRequestMsgMap _kvdb_uid_update_msg_map;
    struct timeval _start_time;
	PBCSMsg _notify_msg;
	long long _tbid;
    long long m_iTidTmp;
    vector<PBTableUser> m_iTableUserTmp;
    void* m_pVoidTmp;

public:
	CSession(int session_id);
	~CSession();
	virtual int ProcessOnTimerOut(int Timerid);
	virtual void AddResponseMsg(const PBCSMsg & msg);
	virtual void CreateHead();

    void StartSessionTimer(long second);
    void StopSessionTimer();
public:
	bool IsAllUserDataReady();
    void AddGetData(long long uid, int key);
	void QueryUserData(CTCPSocketHandler * phandler);
	void AddUpdateData(const SSRequestUpdateUserData & data);
	void UpdateUserData(CTCPSocketHandler * phandler);

public:
    void NewAddGetData(int64 uid, int key);
    void NewAddUpdateData(int64 uid, const PBUpdateData& update_data);
    vector<PBCSMsg> m_TmpMsgVect;
    map<int64, set<int> > _get_data_map;
    map<int64, vector<PBUpdateData> > _update_data_map;
public:
    void BindProcessor(NewProcessor * processor, CHandlerTokenBasic * ptoken);
public:
	NewProcessor * _processor;
	CHandlerTokenBasic * _ptoken;
};

