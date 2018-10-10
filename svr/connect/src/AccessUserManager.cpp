#include "AccessUserManager.h"
#include "Message.h"
#include "RouteManager.h"
#include "LogWriter.h"

AccessUserManager * AccessUserManager::Instance()
{
	return CSingleton<AccessUserManager>::Instance();
}

void AccessUserManager::Init()
{
    timer.SetTimeEventObj(this, LOG_TIMER);
    timer.StartTimerBySecond(60, true);
}

ConnectHandlerToken * AccessUserManager::GetHandlerTokenByAccount(const string & account)
{
	ConnectHandlerToken * ptoken = NULL;
	AccountMap::iterator iter = _accountmap.find(account);
	if (iter != _accountmap.end()){
		ptoken = iter->second;
	}
	return ptoken;
}

ConnectHandlerToken * AccessUserManager::GetHandlerTokenByUid(long long uid)
{
	ConnectHandlerToken * ptoken = NULL;
	TokenMap::iterator iter = _tokenmap.find(uid);
	if (iter != _tokenmap.end()){
		ptoken = iter->second;
	}
	return ptoken;
}

void AccessUserManager::UnBindAccountHandler(const string & account)
{
	_accountmap.erase(account);
}

void AccessUserManager::BindAccountHandlerToken(const string & account,ConnectHandlerToken * ptoken)
{
	_accountmap[account] = ptoken;
	ptoken->_bind_account = true;
	ptoken->_account= account;
}

void AccessUserManager::BindUserHandlerToken(long long uid, int acc_type, int channel, ConnectHandlerToken * ptoken)
{
    if (_tokenmap.find(uid) == _tokenmap.end())
        _online_map[acc_type]++;
        
    _tokenmap[uid] = ptoken;
    ptoken->_authorized = true;
    ptoken->_bind_uid = true;
    ptoken->_uid = uid;
    ptoken->_login_time = time(NULL);
    ptoken->_acc_type = acc_type;
    ptoken->_channel = channel;
}

void AccessUserManager::UnBindUserHandlerToken(const ConnectHandlerToken* token)
{
    if (_tokenmap.find(token->_uid) != _tokenmap.end())
        _online_map[token->_acc_type]--;

	_tokenmap.erase(token->_uid);
}

void AccessUserManager::OnHandlerClosed(ConnectHandlerToken * token)
{
	if (token->_bind_account)
	{
		UnBindAccountHandler(token->_account);
        VLogMsg(CLIB_LOG_LEV_DEBUG,"unbind account[%s] for token[%lx]",token->_account.c_str(),(long)token);
	}
	else
	{
        VLogMsg(CLIB_LOG_LEV_DEBUG,"closed token[%lx], unbind account yet.",(long)token);
	}
	if (token->_bind_uid)
	{
		const PBBPlayerPositionInfo & pos = token->_pos;
		UnBindUserHandlerToken(token);
		PBCSMsg msg;
		msg.mutable_ss_notify_player_handler_close()->set_uid(token->_uid);

        if (PokerPBConnectSvrdConfig::Instance()->CheckPosition(pos.pos_type()))
        {
            int node_type = PokerPBConnectSvrdConfig::Instance()->GetNode(pos.pos_type());
            Message::PushInnerMsg(RouteManager::Instance()->GetRouteByRandom(),token->_uid,msg,node_type,pos.gamesvrd_id());
            VLogMsg(CLIB_LOG_LEV_DEBUG,"unbind uid[%lld] at node[%d] game[%d] for token",token->_uid, node_type, pos.gamesvrd_id());
        }
        else
        {
            VLogMsg(CLIB_LOG_LEV_DEBUG,"user[%lld]: pos[%d] has no node matched ...", token->_uid, pos.pos_type());
        }

		PBCSMsg log_msg;
        LogLogout& log = *log_msg.mutable_log_logout();
        log.set_uid(token->_uid);
        log.set_online_time(time(NULL) - token->_login_time);
        log.set_acc_type(token->_acc_type);
        log.set_channel(token->_channel);
        CLogWriter::Instance()->Send(log_msg);
		/*
		PBCSMsg ss_cid_notify;
        SSRequestUpdateUserData& ss_update = *ss_cid_notify.mutable_ss_request_update_user_data();
        ss_update.set_uid(token->_uid);
        PBUpdateData& user_key = *ss_update.add_key_list();
		user_key.set_key(PBUserDataField::kUserInfo);
		PBDBAtomicField& hallid_field = *user_key.add_field_list();
        hallid_field.set_field(EN_DB_Field_Hsvid);
        hallid_field.set_strategy(EN_Update_Strategy_Replace);
        hallid_field.set_intval(0);        
        Message::PushInnerMsg(RouteManager::Instance()->GetRouteByRandom(), token->_uid, ss_cid_notify, EN_Node_User,1,EN_Route_hash);
        */
	}
}

void AccessUserManager::OnHandlerRepeatedLogin(ConnectHandlerToken * token, CSession * psession)
{
	PBCSMsg msg;
	const CSNotifyRepeatedLogin & cs_notify_repeated_login = *msg.mutable_cs_notify_repeated_login();
	(void)cs_notify_repeated_login;
	if(token->_phandler)
	{
		if ( Message::SendMsgToClient(token->_phandler, psession, msg) <0 )
		{
			//发送失败 说明链接已经断掉了 此时token已经在OnClose中释放
			return;
		}
	}
	PBBPlayerPositionInfo & pos = token->_pos;
    if (PokerPBConnectSvrdConfig::Instance()->CheckPosition(pos.pos_type()))
    {
        int node_type = PokerPBConnectSvrdConfig::Instance()->GetNode(pos.pos_type());
        PBCSMsg msg;
        msg.mutable_ss_notify_player_handler_close()->set_uid(token->_uid);
        Message::PushInnerMsg(RouteManager::Instance()->GetRouteByRandom(),token->_uid,msg,node_type,pos.gamesvrd_id());
        VLogMsg(CLIB_LOG_LEV_DEBUG,"notify node[%d] gamesvrd[%d] user[%lld] handler closed for repeated login", node_type, pos.gamesvrd_id(),token->_uid);
    }
    else
    {
        VLogMsg(CLIB_LOG_LEV_DEBUG,"user[%lld]: pos[%d] has no node matched ...", token->_uid, pos.pos_type());
    }

	VLogMsg(CLIB_LOG_LEV_DEBUG,"user[%s,%lld] repeated login",token->_account.c_str(),token->_uid);
	token->_authorized = false;
	token->_phandler->Disconnect();
	delete token->_phandler;
	token->_phandler = NULL;
	if (token->_bind_account)
	{
		UnBindAccountHandler(token->_account);
	}
	if (token->_bind_uid)
	{
		UnBindUserHandlerToken(token);
	}
	token->_bind_account = false;
	token->_bind_uid = false;
	token->release();
}

void AccessUserManager::OnHandlerRepeatedLogin(int64 uid)
{
	ConnectHandlerToken * ptoken = GetHandlerTokenByUid(uid);
	if(ptoken == NULL)
	{
		VLogMsg(CLIB_LOG_LEV_DEBUG,"user[%lld] repeated login,but not found yet.",uid);
		return;
	}
	VLogMsg(CLIB_LOG_LEV_DEBUG,"user[%s,%lld] repeated login",ptoken->_account.c_str(),ptoken->_uid);
	PBCSMsg msg;
	const CSNotifyRepeatedLogin & cs_notify_repeated_login = *msg.mutable_cs_notify_repeated_login();
	(void)cs_notify_repeated_login;
	if(ptoken->_phandler)
	{
		CPBOutputPacket output;
		PBHead head;
		head.set_cmd(PBCSMsg::kCsNotifyRepeatedLogin);
		output.EncodeProtoHead(head);
		output.EncodeProtoMsg(msg);
		output.End();
		if ( ptoken->_phandler->Send(output.packet_buf(),output.packet_size()) >= 0 )
		{
			//这个包有可能没有发出去.但是这里还是需要断链接
			ptoken->_phandler->Disconnect();
			delete ptoken->_phandler;
			ptoken->_phandler = NULL;
		}
		else
		{
			ptoken->_phandler = NULL;
		}
	}
	ptoken->_authorized = false;
	if (ptoken->_bind_account)
	{
		UnBindAccountHandler(ptoken->_account);
	}
	if (ptoken->_bind_uid)
	{
        PBBPlayerPositionInfo & pos = ptoken->_pos;
        if (PokerPBConnectSvrdConfig::Instance()->CheckPosition(pos.pos_type()))
        {
            int node_type = PokerPBConnectSvrdConfig::Instance()->GetNode(pos.pos_type());
            PBCSMsg msg;
            msg.mutable_ss_notify_player_handler_close()->set_uid(ptoken->_uid);
            Message::PushInnerMsg(RouteManager::Instance()->GetRouteByRandom(),ptoken->_uid,msg,node_type,pos.gamesvrd_id());
            VLogMsg(CLIB_LOG_LEV_DEBUG,"notify node[%d] gamesvrd[%d] user[%lld] handler closed for repeated login",node_type,pos.gamesvrd_id(),ptoken->_uid);
        }
        else
        {
            VLogMsg(CLIB_LOG_LEV_DEBUG,"user[%lld]: pos[%d] has no node matched ...", ptoken->_uid, pos.pos_type());
        }

		UnBindUserHandlerToken(ptoken);
	}
	ptoken->_bind_account = false;
	ptoken->_bind_uid = false;
	ptoken->release();
}

void AccessUserManager::BroadCastMessage(CPBOutputPacket & output)
{
	TokenMap::iterator iter = _tokenmap.begin();
	for(;iter!=_tokenmap.end();iter++)
	{
		ConnectHandlerToken * pUserToken = iter->second;
		if(pUserToken!=NULL && pUserToken->_phandler)
		{
			pUserToken->_phandler->Send(output.packet_buf(),output.packet_size());
		}
	}
}

int AccessUserManager::ProcessOnTimerOut(int Timerid)
{
	PBCSMsg msg;
    LogOnline& log = *msg.mutable_log_online();
    log.set_svr_id(TGlobal::_svid);
    log.set_online(_tokenmap.size());
    CLogWriter::Instance()->Send(msg);
	return 0;
}

