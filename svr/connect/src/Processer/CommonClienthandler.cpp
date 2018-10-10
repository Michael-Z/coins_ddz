#include "CommonClienthandler.h"
#include "RouteManager.h"
#include "AccessUserManager.h"
#include "ConnectHandlerToken.h"
#include "Common.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

ENHandlerResult CRegistInnerServer::ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession)
{
	const SSNotifyInnerServer ss_notify_inner_server = psession->_request_msg.ss_notify_inner_server();
	int routeid = ss_notify_inner_server.routeid();
	RouteManager::Instance()->_route_map[routeid] = ptoken->_phandler;
    VLogMsg(CLIB_LOG_LEV_DEBUG,"receive noitfy from route[%d]",routeid);
	return EN_Handler_Done;
}

ENHandlerResult CLoginProcesser::ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession)
{
	const CSRequestLogin & request = psession->_request_msg.cs_request_login();
	//检查该链接是否登录过其他的用户.
	string account = request.account();
	ConnectHandlerToken * pconntoken = (ConnectHandlerToken*)ptoken;
	if(pconntoken->_bind_account && pconntoken->_account != account)
	{
		//同一个链接登陆不同的帐号
		//取消之前绑定的帐号
		AccessUserManager::Instance()->UnBindAccountHandler(pconntoken->_account);
		pconntoken->_bind_account = false;
        VLogMsg(CLIB_LOG_LEV_DEBUG,"unbind token[0x%lx] pre account[%s]",(long)pconntoken,account.c_str());
	}
	if(pconntoken->_bind_uid)
	{
		if((!request.has_uid() || request.uid()!=pconntoken->_uid) || pconntoken->_account != account)
		{
			AccessUserManager::Instance()->UnBindUserHandlerToken(pconntoken);
			pconntoken->_bind_uid = false;	
            VLogMsg(CLIB_LOG_LEV_DEBUG,"unbind token[0x%lx] pre uid[%lld]",(long)pconntoken,pconntoken->_uid);
		}
	}
	//检查本地是否有该用户的登录信息.
    VLogMsg(CLIB_LOG_LEV_DEBUG,"User[%s] Request Login",account.c_str());
	ConnectHandlerToken * preusertoken = AccessUserManager::Instance()->GetHandlerTokenByAccount(account);
	if (preusertoken && preusertoken != ptoken){	
		//已经登录 需要通知上一个链接 重复登录
		//断开链接
		AccessUserManager::Instance()->OnHandlerRepeatedLogin(preusertoken, psession);
	}
	//更新帐号与链接的索引关系
	AccessUserManager::Instance()->BindAccountHandlerToken(account,(ConnectHandlerToken*)ptoken);
	//发送请给用户服务器查询用户状态
	Message::SendRequestToHall(RouteManager::Instance()->GetRouteByRandom(), psession, psession->_request_msg);
	return EN_Handler_Succ;
}

ENHandlerResult CLoginProcesser::ProcessResponseMsg(CHandlerTokenBasic * ptoken,CSession * psession)
{
	const CSRequestLogin & request = psession->_request_msg.cs_request_login();
	string account = request.account();
	ConnectHandlerToken * pUserToken = AccessUserManager::Instance()->GetHandlerTokenByAccount(account);
	if (pUserToken == NULL)
	{
        VLogMsg(CLIB_LOG_LEV_DEBUG,"handler for account[%s] not exist,maybe lost connection.", account.c_str());
		return EN_Handler_Done;
	}
	char s[1204] = {0};
	
	struct sockaddr_in6 peer ;
	socklen_t addr_size = sizeof(peer);
	int iret = getpeername(pUserToken->_phandler->GetNetfd(),(sockaddr*)&peer,&addr_size);
	switch (peer.sin6_family)
	{
		case AF_INET6:
			inet_ntop(AF_INET6,&peer.sin6_addr,s, 128);
			VLogMsg(CLIB_LOG_LEV_DEBUG,"user[%s] request login,iret:%d",s,iret);
			break;
	}
	
	const SSResponseLogin & ss_response = psession->_response_msg.ss_response_login();
	PBCSMsg msg;
	CSResponseLogin& response = *msg.mutable_cs_response_login();
	response.set_result(ss_response.result());

	if (ss_response.result() == EN_MESSAGE_ERROR_OK)
	{
		// 设置uid与socket的映射关系 并设置链接验证合法
		long long uid = ss_response.data_set().user_data().user_info().uid();
        VLogMsg(CLIB_LOG_LEV_DEBUG,"account[%s,%lld] login successed.", account.c_str(),uid);
		AccessUserManager::Instance()->BindUserHandlerToken(uid, request.acc_type(), psession->_head.channel_id(), pUserToken);

		const PBUser& user_info = ss_response.data_set().user_data().user_info();	
    	response.mutable_user()->CopyFrom(user_info);	
		response.mutable_user()->set_last_login_ip(s);
        response.set_is_created(ss_response.is_created());
		if(ss_response.data_set().user_data().has_user_game_data())
		{
			const PBUserGameData& user_game_data = ss_response.data_set().user_data().user_game_data();
			response.mutable_user_game_data()->CopyFrom(user_game_data);
		}

		// 更新玩家位置信息
		pUserToken->_pos.CopyFrom(user_info.pos());
		Message::SendMsgToClient(pUserToken->_phandler, psession, msg);
		// 更新用户IP
        // 通知用户状态
        PBUpdateData update;
		update.set_key(PBUserDataField::kUserInfo);
		{
			PBDBAtomicField& field = *update.add_field_list();
			field.set_field(EN_DB_Field_IP);
			field.set_strvalue(s);
		}
		psession->NewAddUpdateData(uid , update);
		return EN_Handler_Save;
	}
	else
	{
		//没有登录成功
        VLogMsg(CLIB_LOG_LEV_DEBUG,"account[%s] login faild[%d].", account.c_str(),ss_response.result());
		if (pUserToken->_bind_account)
		{
			AccessUserManager::Instance()->UnBindAccountHandler(pUserToken->_account);
		}
	}

    Message::SendMsgToClient(pUserToken->_phandler, psession, msg);

	return EN_Handler_Done;
}

ENHandlerResult CRequestHeartBeat::ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession)
{
    PBCSMsg msg;
    CSResponseHeartBeat & response = *msg.mutable_cs_response_heart_beat();
    response.set_time(Common::GetTime());

    CPBOutputPacket output;
    output.EncodeProtoHead(psession->_head);
    output.EncodeProtoMsg(msg);
    output.End();
    if (ptoken->_phandler)
        ptoken->_phandler->Send(output.packet_buf(),output.packet_size());

    return EN_Handler_Done;
}

ENHandlerResult CGMPushMessageMulti::ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession)
{
	const GMPushMessageMulti & gm_push_message_multi = psession->_request_msg.gm_push_message_multi();
	int64 stamp = time(NULL);
	PBCSMsg notify;
	CSNotifyPushMessage & cs_notify_push_message = *notify.mutable_cs_notify_push_message();
	cs_notify_push_message.set_message(gm_push_message_multi.message());
	cs_notify_push_message.set_stamp(stamp);
	CPBOutputPacket output;
    output.EncodeProtoHead(psession->_head);
    output.EncodeProtoMsg(notify);
    output.End();
	if (gm_push_message_multi.all())
	{
		//全服推送
		//
		AccessUserManager::Instance()->BroadCastMessage(output);
		return EN_Handler_Done;
	}
	for(int i=0;i<gm_push_message_multi.uid_list_size();i++)
	{
		long long uid = gm_push_message_multi.uid_list(i);
		ConnectHandlerToken * pUserToken = AccessUserManager::Instance()->GetHandlerTokenByUid(uid);
		if (pUserToken != NULL)
		{
			pUserToken->_phandler->Send(output.packet_buf(),output.packet_size());
		}
	}
	return EN_Handler_Done;
}

ENProcessResult CNotifyPlayerRepeatedLogin::ProcessPushMsg(CHandlerTokenBasic * ptoken,CSession * psession)
{
	const SSNotifyPlayerRepeatedLogin & notify = psession->_request_msg.ss_notify_player_repeated_login();
	int64 uid = notify.uid();
	AccessUserManager::Instance()->OnHandlerRepeatedLogin(uid);
	return EN_Process_Result_Completed;
}

ENHandlerResult CRequestSdrEnterTable::ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession)
{
    // todo
    //请求roomsvrd 获取当前牌桌的gamesvrd id
    int64 uid = psession->_uid;
    ConnectHandlerToken * pUserToken = AccessUserManager::Instance()->GetHandlerTokenByUid(uid);
    if(pUserToken)
    {
        const CSRequestSdrEnterTable & cs_request_enter_table = psession->_request_msg.cs_request_sdr_enter_table();
        Message::SendRequestMsg(RouteManager::Instance()->GetRouteByRandom(),psession,psession->_request_msg,
                                EN_Node_Room,1);
        VLogMsg(CLIB_LOG_LEV_DEBUG,"user[%lld] request enter sdr table[%d] of server[%d].",
            uid,cs_request_enter_table.tid(),1);
        return EN_Handler_Succ;
    }
    else
    {
        ErrMsg("fatal error,cannot find handle by uid[%lld]",uid);
        return EN_Handler_Done;
    }
}

ENHandlerResult CRequestSdrEnterTable::ProcessResponseMsg(CHandlerTokenBasic* ptoken, CSession* psession)
{
    long long uid = psession->_uid;
    ConnectHandlerToken * pUserToken = AccessUserManager::Instance()->GetHandlerTokenByUid(uid);
    if (pUserToken != NULL)
    {
        CPBOutputPacket output;
        output.EncodeProtoHead(psession->_head);
        output.EncodeProtoMsg(psession->_response_msg);
        output.End();
        pUserToken->_phandler->Send(output.packet_buf(),output.packet_size());
        const CSResponseSdrEnterTable & cs_response_enter_table = psession->_response_msg.cs_response_sdr_enter_table();
        if(cs_response_enter_table.result() == EN_MESSAGE_ERROR_OK)
        {
            pUserToken->_pos.set_pos_type(cs_response_enter_table.pos_type());
            pUserToken->_pos.set_gamesvrd_id(cs_response_enter_table.gamesvrd_id());
            VLogMsg(CLIB_LOG_LEV_DEBUG,"user[%lld] enter table of sdr game[%d] succ,session[%d]",
                uid,cs_response_enter_table.gamesvrd_id(),psession->sessionid());
        }
    }
    return EN_Handler_Done;
}

ENHandlerResult CRequestEcho::ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession)
{
    const CSRequestEcho& request = psession->_request_msg.cs_request_echo();
    PBCSMsg msg;
    msg.mutable_cs_response_echo()->set_ip(request.ip());
    msg.mutable_cs_response_echo()->set_port(request.port());
    msg.mutable_cs_response_echo()->set_session_id(request.session_id());
	msg.mutable_cs_response_echo()->set_gtype(request.gtype());
	msg.mutable_cs_response_echo()->set_main_key(request.main_key());
    CPBOutputPacket output;
    output.EncodeProtoHead(psession->_head);
    output.EncodeProtoMsg(msg);
    output.End();
    if (ptoken->_phandler)
        ptoken->_phandler->Send(output.packet_buf(),output.packet_size());

    VLogMsg(CLIB_LOG_LEV_DEBUG,"Monitor: process echo request ...");

    return EN_Handler_Done;
}

ENHandlerResult CResponseSdrEnterTable::ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession)
{
    long long uid = psession->_uid;
    ConnectHandlerToken * pUserToken = AccessUserManager::Instance()->GetHandlerTokenByUid(uid);
    VLogMsg(CLIB_LOG_LEV_DEBUG," connect recevie PushInnerMsg from user[%lld] for enter table",uid);
    if (pUserToken != NULL)
    {
        CPBOutputPacket output;
        output.EncodeProtoHead(psession->_head);
        output.EncodeProtoMsg(psession->_request_msg);
        output.End();
        pUserToken->_phandler->Send(output.packet_buf(),output.packet_size());
        const CSResponseSdrEnterTable & cs_response_enter_table = psession->_request_msg.cs_response_sdr_enter_table();
        if(cs_response_enter_table.result() == EN_MESSAGE_ERROR_OK)
        {
            pUserToken->_pos.set_pos_type(cs_response_enter_table.pos_type());
            pUserToken->_pos.set_gamesvrd_id(cs_response_enter_table.gamesvrd_id());
            VLogMsg(CLIB_LOG_LEV_DEBUG,"user[%lld] enter table of es_match game[%d] succ,session[%d]",
                uid,cs_response_enter_table.gamesvrd_id(),psession->sessionid());
        }
    }
    return EN_Handler_Done;
}

ENHandlerResult CNotifyPosChange::ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession)
{
    long long uid = psession->_uid;
    ConnectHandlerToken * pUserToken = AccessUserManager::Instance()->GetHandlerTokenByUid(uid);
    if (pUserToken != NULL)
    {
        const SSNotifyPlayerPosChange & ss_notify_player_pos_change = psession->_request_msg.ss_notify_player_pos_change();
        pUserToken->_pos.CopyFrom(ss_notify_player_pos_change.pos());
    }
    return EN_Handler_Done;
}
