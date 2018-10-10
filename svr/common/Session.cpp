#include "Session.h"
#include "Message.h"
#include "SessionManager.h"
#include "NewProcessor.h"

CSession::CSession(int session_id)
{
	PBSession::set_sessionid(session_id);
	_message_logic_type = EN_Message_Unknown;
	_fsm_state = -1;
	_uid = -1;
	_kvdb_state = EN_Session_KVDB_State_Idle;
	_input_packet = NULL;
    _timer.SetTimeEventObj(this, 1);
	_tbid = -1;
	_processor = NULL;
	_ptoken = NULL;
}

CSession::~CSession()
{
	_timer.StopTimer();
}

int CSession::ProcessOnTimerOut(int Timerid)
{
	ErrMsg("session [cmd:0x%lx,id:%d] timeout",request_cmd(),sessionid());
    SessionManager::Instance()->ReleaseSession(sessionid());
	if (_processor != NULL)
	{
		_processor->ProcessSessionTimeOut(_ptoken, this);
	}
	return -1;
}

void CSession::AddResponseMsg(const PBCSMsg & msg)
{
	_response_msg_map[_fsm_state] = msg;
}

void CSession::CreateHead()
{
	_head.set_proto_version(0);
}

bool CSession::IsAllUserDataReady()
{
	KVDBUidFlagMap::iterator iter = _kvdb_uid_flag_map.begin();
	for(;iter!=_kvdb_uid_flag_map.end();iter++)
	{
		if(iter->second != true)
		{
			return false;
		}
	}
	_kvdb_state = EN_Session_KVDB_State_Idle;
	return true;
}

void CSession::AddGetData(long long uid, int key)
{		
	if(_kvdb_state != EN_Session_KVDB_State_Wait_Query_Response)
	{
		//第一次调用时清空flag
		_kvdb_uid_flag_map.clear();
        _kvdb_uid_query_msg_map.clear();
		VLogMsg(CLIB_LOG_LEV_DEBUG,"invoke add get data for user[%lld] ,key[%d],session[%d]",uid,key,sessionid());
	}
	PBCSMsg & msg = _kvdb_uid_query_msg_map[uid];
	SSRequestGetUserData& request = *msg.mutable_ss_request_get_user_data();
    PBDataSet& data_set = *request.mutable_data_set();
    data_set.set_uid(uid);
    bool has_find_key = false;
    for (int i = 0; i < data_set.key_list_size(); ++i)
    {
        const PBRedisData& redis_data = data_set.key_list(i);
        if (redis_data.key() == key)
        {
            has_find_key = true;
            break;
        }
    }
    if (!has_find_key)
    {
        data_set.add_key_list()->set_key(key);
    }	
	_kvdb_uid_flag_map[uid] = false;
	_kvdb_state = EN_Session_KVDB_State_Wait_Query_Response;
}

void CSession::AddUpdateData(const SSRequestUpdateUserData & data)
{
	if(_kvdb_state != EN_Session_KVDB_State_Wait_Request_Update)
	{
		//第一次调用时清空flag
		_kvdb_uid_flag_map.clear();
        _kvdb_uid_update_msg_map.clear();
		VLogMsg(CLIB_LOG_LEV_DEBUG,"invoke add get data for user[%ld] ,session[%d]",data.uid(),sessionid());
	}

	PBCSMsg & msg = _kvdb_uid_update_msg_map[data.uid()];
	SSRequestUpdateUserData & request = * msg.mutable_ss_request_update_user_data();
	request.set_uid(data.uid());
	for(int i=0;i<data.key_list_size();i++)
	{
		request.add_key_list()->CopyFrom(data.key_list(i));
	}
	_kvdb_uid_flag_map[data.uid()] = false;
	_kvdb_state = EN_Session_KVDB_State_Wait_Request_Update;
}

void CSession::QueryUserData(CTCPSocketHandler * phandler)
{
	KVDBUidRequestMsgMap::iterator iter=_kvdb_uid_query_msg_map.begin();
	for(;iter!=_kvdb_uid_query_msg_map.end();iter++)
	{
		PBCSMsg & msg = iter->second;
		this->_head.set_cmd(msg.msg_union_case());
		::Message::SendRequestToUser(phandler,this,msg,iter->first);
	}
}

void CSession::UpdateUserData(CTCPSocketHandler * phandler)
{
	KVDBUidRequestMsgMap::iterator iter = _kvdb_uid_update_msg_map.begin();
	for(;iter!=_kvdb_uid_update_msg_map.end();iter++)
	{
		PBCSMsg & msg = iter->second;
		this->_head.set_cmd(msg.msg_union_case());
		::Message::SendRequestToUser(phandler,this,msg,iter->first);
	}
	_kvdb_state = EN_Session_KVDB_State_Wait_Update_Response;
}

void CSession::StartSessionTimer(long second)
{
    _timer.StartTimerBySecond(second);
}

void CSession::StopSessionTimer()
{
    _timer.StopTimer();
}

void CSession::NewAddGetData(int64 uid, int key)
{
    _get_data_map[uid].insert(key);
}

void CSession::NewAddUpdateData(int64 uid, const PBUpdateData& update_data)
{
    _update_data_map[uid].push_back(update_data);
}

void CSession::BindProcessor(NewProcessor * processor , CHandlerTokenBasic * ptoken)
{
	_processor = processor;
	_ptoken = ptoken;
	return;
}