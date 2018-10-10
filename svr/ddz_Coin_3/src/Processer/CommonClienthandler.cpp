#include "CommonClienthandler.h"
#include "RouteManager.h"
#include "clib_time.h"
#include "PBConfigBasic.h"
#include "GameHandlerProxy.h"
#include "TableManager.h"
#include "TableModle.h"
#include <google/protobuf/unknown_field_set.h>
//#include "command_line_interface.h"

//using namespace google::protobuf;

ENProcessResult CRegistInnerServer::ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession)
{
	const SSNotifyInnerServer ss_notify_inner_server = psession->_request_msg.ss_notify_inner_server();
	int routeid = ss_notify_inner_server.routeid();
	RouteManager::Instance()->_route_map[routeid] = ptoken->_phandler;
	VLogMsg(CLIB_LOG_LEV_ERROR, "receive noitfy from route[%d]", routeid);
	TableManager::Instance()->ReportGameInfo();
	return EN_Process_Result_Completed;
}

ENHandlerResult CRequestReadyForGame::ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession)
{
	const CSRequestReadyForGame & cs_request_ready_for_game = psession->_request_msg.cs_request_ready_for_game();
	PBCSMsg msg;
	CSResponseReadyForGame & response = *msg.mutable_cs_response_ready_for_game();
	long long uid = psession->_request_route.uid();
	//根据ID获取玩家的房间ID
	int tid = TableManager::Instance()->GetPlayerTableID(uid);
	CPBGameTable * ptable = TableManager::Instance()->FindTable(tid);
	if (ptable == NULL)
	{
		response.set_result(EN_MESSAGE_TABLE_NOT_EXIST);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}
	int index = TableLogic::FindIndexByUid(*ptable, uid);
	if (index < 0 || index > ptable->seats_size() - 1)
	{
		response.set_result(EN_MESSAGE_INVALID_SEAT_INDEX);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}
	if (ptable->state() != EN_TABLE_STATE_SINGLE_OVER && ptable->state() != EN_TABLE_STATE_WAIT && ptable->state() != EN_TABLE_STATE_WAIT_ROBOT)
	{
		return EN_Handler_Done;
	}
	PBSDRTableSeat & seat = *ptable->mutable_seats(index);
	if (seat.state() == EN_SEAT_STATE_WAIT_FOR_NEXT_ONE_GAME)
	{
		if (cs_request_ready_for_game.state() == true)
		{
			seat.set_state(EN_SEAT_STATE_READY_FOR_NEXT_ONE_GAME);
			response.set_result(EN_MESSAGE_ERROR_OK);
			response.set_state(true);
			Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);

			PBCSMsg notify;
			CSNotifyReadyForGame & notify_ready_for_game = *notify.mutable_cs_notify_ready_for_game();
			notify_ready_for_game.set_seat_index(index);
			notify_ready_for_game.set_state(true);
			TableLogic::BroadcastTableMsg(*ptable, notify);

			if (TableLogic::GetPlayerNumByState(*ptable, EN_SEAT_STATE_READY_FOR_NEXT_ONE_GAME) == ptable->seats_size())
			{
				ptable->set_state(EN_TABLE_STATE_READY_TO_START);
			}
			TableLogic::ProcessTable(*ptable);
			return EN_Handler_Done;
		}
		else
		{
			response.set_result(EN_MESSAGE_ERROR_OK);
			response.set_state(false);
			Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		}
	}
	else if (seat.state() == EN_SEAT_STATE_READY_FOR_NEXT_ONE_GAME)
	{
		if (cs_request_ready_for_game.state() == false)
		{
			seat.set_state(EN_SEAT_STATE_WAIT_FOR_NEXT_ONE_GAME);
			response.set_result(EN_MESSAGE_ERROR_OK);
			response.set_state(false);
			Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);

			PBCSMsg notify;
			CSNotifyReadyForGame & notify_ready_for_game = *notify.mutable_cs_notify_ready_for_game();
			notify_ready_for_game.set_seat_index(index);
			notify_ready_for_game.set_state(false);
			TableLogic::BroadcastTableMsg(*ptable, notify);
			return EN_Handler_Done;
		}
		else
		{
			response.set_result(EN_MESSAGE_ERROR_OK);
			response.set_state(true);
			Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		}
	}
	else
	{
		response.set_result(EN_MESSAGE_ERROR_OK);
		response.set_state(true);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
	}
	return EN_Handler_Done;
}

ENHandlerResult CRequestDoAction::ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession)
{
	const CSRequestSDRDoAction & cs_request_do_action = psession->_request_msg.cs_request_sdr_do_action();
	PBCSMsg msg;
	CSResponseSDRDoAction & response = *msg.mutable_cs_response_sdr_do_action();
	long long uid = psession->_request_route.uid();
	//根据ID获取玩家的房间ID
	int64 tid = TableManager::Instance()->GetPlayerTableID(uid);
	CPBGameTable * ptable = TableManager::Instance()->FindTable(tid);
	if (ptable == NULL)
	{
		response.set_result(EN_MESSAGE_TABLE_NOT_EXIST);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}
	//    if(ptable->state() != EN_TABLE_STATE_PLAYING)
	//    {
	//        response.set_result(EN_MESSAGE_TABLE_NOT_EXIST);
	//        Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
	//        return EN_Handler_Done;
	//    }

	// 如果处于等待解散状态 所有doaction请求，全部忽略
	if (ptable->state() == EN_TABLE_STATE_WAIT_DISSOLVE)
	{
		return EN_Handler_Done;
	}

	PBSDRAction request_action;
	request_action.set_tid(tid);
	request_action.set_seat_index(cs_request_do_action.seat_index());
	if (cs_request_do_action.act_type() < ENSDRActionType_MIN || cs_request_do_action.act_type() > ENSDRActionType_MAX)
	{
		return EN_Handler_Done;
	}
	request_action.set_act_type((ENSDRActionType)cs_request_do_action.act_type());
	//    request_action.set_dest_card(cs_request_do_action.dest_card());
	request_action.mutable_col_info()->mutable_cards()->CopyFrom(cs_request_do_action.cards());
	// 透传数据
	request_action.set_cardtype(cs_request_do_action.cardtype());
	request_action.set_real(cs_request_do_action.real());
	request_action.set_num(cs_request_do_action.num());
	request_action.mutable_cards()->CopyFrom(cs_request_do_action.cards());

	int token = cs_request_do_action.token();

	int index = TableLogic::FindIndexByUid(*ptable, uid);
	if (index < 0 || index > ptable->seats_size() - 1)
	{
		response.set_result(EN_MESSAGE_INVALID_SEAT_INDEX);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}

	PBSDRTableSeat * p_user_seat = ptable->mutable_seats(index);

	if (!TableLogic::IsPlayerAbleDoAction(*p_user_seat, request_action, token))
	{
		response.set_result(EN_MESSAGE_INVALID_ACTION);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		VLogMsg(CLIB_LOG_LEV_ERROR, "player[%lld] disable to do action[%d] on table[%lld]", uid, request_action.act_type(), tid);
		return EN_Handler_Done;
	}
	// 检验action合法性
	if (TableLogic::DetermineAction(*ptable, *p_user_seat, request_action) == -1)
	{
		response.set_result(EN_MESSAGE_INVALID_ACTION);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		VLogMsg(CLIB_LOG_LEV_ERROR, "player[%lld] fialed to do action[%d] on table[%lld]", uid, request_action.act_type(), tid);
		return EN_Handler_Done;
	}

	response.set_result(EN_MESSAGE_ERROR_OK);
	response.mutable_action()->CopyFrom(request_action);
	Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);

	if (request_action.act_type() == EN_SDR_ACTION_CHUPAI)
	{
		const PBSDRAction* p_action = TableLogic::GetLastDeterminedActionChuPai(*ptable);
		if (p_action != NULL)
		{
			request_action.CopyFrom(*p_action);
		}
	}

	std::string strProto;
	google::protobuf::TextFormat::PrintToString(request_action, &strProto);
	VLogMsg(CLIB_LOG_LEV_DEBUG, "\nrequest_action:\n%s", strProto.c_str());
	// 开始处理action
	TableLogic::PlayerDoAction(*ptable, *p_user_seat, request_action);

	return EN_Handler_Done;
}

ENHandlerResult CRequestDissolveTable::ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession)
{
	const CSRequestDissolveTable & request = psession->_request_msg.cs_request_dissolve_table();
	PBCSMsg msg;
	CSResponseDissolveTable & response = *msg.mutable_cs_response_dissolve_table();
	long long uid = psession->_request_route.uid();
	//根据ID获取玩家的房间ID
	int tid = TableManager::Instance()->GetPlayerTableID(uid);
	CPBGameTable * ptable = TableManager::Instance()->FindTable(tid);
	if (ptable == NULL)
	{
		response.set_result(EN_MESSAGE_TABLE_NOT_EXIST);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}

	bool is_tea_bar_table = false;
	if (ptable->config().has_tbid() && ptable->config().has_master_uid())
	{
		is_tea_bar_table = true;
	}

	int index = TableLogic::FindIndexByUid(*ptable, uid);
	if (index < 0 || index > ptable->seats_size() - 1)
	{
		response.set_result(EN_MESSAGE_INVALID_SEAT_INDEX);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}
	if (ptable->state() == EN_TABLE_STATE_AUTO_DISSOLVING)
	{
		response.set_result(EN_MESSAGE_ERROR_OK);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}
	if (request.choice() == false)
	{
		ptable->table_log << "玩家[" << uid << "] 拒绝解散" << "\n";
		//玩家取消
		response.set_result(EN_MESSAGE_ERROR_OK);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		if (ptable->state() == EN_TABLE_STATE_WAIT_DISSOLVE)
		{
			PBCSMsg notify;
			CSNotifyDissolveTableOperation & cs_notify_dissolve_table_opera = *notify.mutable_cs_notify_dissolve_table_operation();
			cs_notify_dissolve_table_opera.set_uid(uid);
			cs_notify_dissolve_table_opera.set_vote_down(true);
			TableLogic::BroadcastTableMsg(*ptable, notify);
			//恢复之前的状态
			const PBDissolveInfo & dissolve_info = ptable->dissolve_info();
			ptable->set_state(dissolve_info.pre_state());
			ptable->clear_dissolve_info();
			ptable->StopTimer();
		}
		return EN_Handler_Done;
	}
	else
	{
		ptable->table_log << "玩家[" << uid << "] 同意解散" << "\n";
		// 如果选择为true，但是发起者为0，则为过期请求
		if (ptable->dissolve_info().start_uid() == 0 && !request.is_start())
		{
			response.set_result(EN_MESSAGE_ERROR_OK);
			Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
			return EN_Handler_Done;
		}
	}

	//如果牌局尚未开始
	if ((ptable->state() == EN_TABLE_STATE_WAIT || ptable->state() != EN_TABLE_STATE_WAIT_ROBOT) && ptable->round() == 0)
	{
		//尚未开始的牌桌
		if (uid != ptable->creator_uid())
		{
			response.set_result(EN_MESSAGE_INVALID_SEAT_INDEX);
			Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
			return EN_Handler_Done;
		}
		else
		{
			if (is_tea_bar_table && ptable->config().tb_pay_type() == EN_TeaBar_Pay_Type_Master)
			{
				int cost = ptable->config().cost();
				if (cost > 0)
				{
					TeaBarChipsFlow flow;
					flow.set_tbid(ptable->config().tbid());
					flow.set_chips(cost);
					flow.set_reason(EN_Reason_Dissolve_Tea_Bar_Table);
					PBUpdateData updatetb;
					updatetb.set_key(PBUserDataField::kUserTeaBarData);
					PBDBAtomicField& field = *updatetb.add_field_list();
					field.set_field(EN_DB_Field_TeaBar_Chips);
					field.mutable_tea_bar_chips_flow()->CopyFrom(flow);
					psession->NewAddUpdateData(ptable->config().master_uid(), updatetb);
				}
			}
			// 重置所有人的位置信息
			for (int i = 0; i<ptable->seats_size(); i++)
			{
				const PBSDRTableSeat & seat = ptable->seats(i);
				if (seat.state() == EN_SEAT_STATE_NO_PLAYER)
				{
					continue;
				}
				const PBTableUser & user = seat.user();
				PBUpdateData update;
				update.set_key(PBUserDataField::kUserInfo);
				{
					PBDBAtomicField& field = *update.add_field_list();
					field.set_field(EN_DB_Field_POS);
					PBBPlayerPositionInfo& pos = *field.mutable_pos();
					pos.set_pos_type(EN_Position_Hall);
					pos.set_table_id(0);
					pos.set_gamesvrd_id(0);
				}
				if (ptable->creator_uid() == user.uid())
				{
					{
						PBDBAtomicField& field = *update.add_field_list();
						field.set_field(EN_DB_Field_Create_Table);
						field.set_intval(0);
					}
					//牌局尚未开始则需要退还 房卡给用户
					{   // 现在是在游戏开始后才扣房卡，所以这里不需要返还房卡
						//                        int cost = 2;
						//                        PBDBAtomicField & field = *update.add_field_list();
						//                        field.set_field(EN_DB_Field_Chips);
						//                        field.set_intval(cost);
						//                        field.set_reason(EN_Reason_Dissolve_Fpf_Table);
					}
				}
				psession->NewAddUpdateData(user.uid(), update);
			}
			return EN_Handler_Save;
		}
	}
	PBDissolveInfo & dissolve_info = *ptable->mutable_dissolve_info();
	if (ptable->state() != EN_TABLE_STATE_WAIT_DISSOLVE)
	{
		dissolve_info.set_pre_state(ptable->state());
		ptable->set_state(EN_TABLE_STATE_WAIT_DISSOLVE);
		dissolve_info.set_start_uid(uid);
		ptable->StartTimer(180);
	}
	bool already_opera = false;
	for (int i = 0; i<dissolve_info.agree_uid_list_size(); i++)
	{
		if (uid == dissolve_info.agree_uid_list(i))
		{
			already_opera = true;
			break;
		}
	}
	if (already_opera)
	{
		response.set_result(EN_MESSAGE_ERROR_OK);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}

	dissolve_info.add_agree_uid_list(uid);
	//
	PBCSMsg notify;
	CSNotifyDissolveTableOperation & cs_notify_dissolve_table_opera = *notify.mutable_cs_notify_dissolve_table_operation();
	cs_notify_dissolve_table_opera.set_uid(uid);
	cs_notify_dissolve_table_opera.mutable_dissolve_info()->CopyFrom(dissolve_info);
	TableLogic::BroadcastTableMsg(*ptable, notify);

	int dissole_num = ptable->seats_size() == 4 ? 3 : ptable->seats_size();
	if (dissolve_info.agree_uid_list_size() >= dissole_num)
	{
		if (!TableLogic::NeedPayForTable(ptable->round(), dissolve_info.pre_state()))
		{
			if (is_tea_bar_table && ptable->config().tb_pay_type() == EN_TeaBar_Pay_Type_Master)
			{
				int tea_bar_cost = ptable->config().cost();
				TeaBarChipsFlow flow;
				flow.set_tbid(ptable->config().tbid());
				flow.set_chips(tea_bar_cost);
				flow.set_reason(EN_Reason_Dissolve_Tea_Bar_Table);
				PBUpdateData updatetb;
				updatetb.set_key(PBUserDataField::kUserTeaBarData);
				PBDBAtomicField& field = *updatetb.add_field_list();
				field.set_field(EN_DB_Field_TeaBar_Chips);
				field.mutable_tea_bar_chips_flow()->CopyFrom(flow);
				psession->NewAddUpdateData(ptable->config().master_uid(), updatetb);
			}
		}
		// 提前解散房间 直接到GameFinish
		{
			ptable->set_is_dissolve_finish(true);
			ptable->table_log << "table_id[" << ptable->tid() << "] 所有玩家同意解散" << "\n";
		}
		// 重置所有人的位置信息
		for (int i = 0; i<ptable->seats_size(); i++)
		{
			const PBSDRTableSeat & seat = ptable->seats(i);
			if (seat.state() == EN_SEAT_STATE_NO_PLAYER)
			{
				continue;
			}
			const PBTableUser & user = seat.user();
			PBUpdateData update;
			update.set_key(PBUserDataField::kUserInfo);
			{
				PBDBAtomicField& field = *update.add_field_list();
				field.set_field(EN_DB_Field_POS);
				PBBPlayerPositionInfo& pos = *field.mutable_pos();
				pos.set_pos_type(EN_Position_Hall);
				pos.set_table_id(0);
				pos.set_gamesvrd_id(TGlobal::_svid);
			}
			if (ptable->creator_uid() == user.uid())
			{
				PBDBAtomicField& field = *update.add_field_list();
				field.set_field(EN_DB_Field_Create_Table);
				field.set_intval(0);
				VLogMsg(CLIB_LOG_LEV_DEBUG, "清除房主[%lld]的creat_table_id ...", uid);
			}
			psession->NewAddUpdateData(user.uid(), update);
		}
		return EN_Handler_Save;
	}
	return EN_Handler_Done;
}

ENHandlerResult CRequestDissolveTable::ProcessUpdateSucc(CHandlerTokenBasic* ptoken, CSession* psession)
{
	PBCSMsg msg;
	CSResponseDissolveTable & response = *msg.mutable_cs_response_dissolve_table();
	long long uid = psession->_request_route.uid();
	//根据ID获取玩家的房间ID
	int tid = TableManager::Instance()->GetPlayerTableID(uid);
	CPBGameTable * ptable = TableManager::Instance()->FindTable(tid);
	if (ptable == NULL)
	{
		response.set_result(EN_MESSAGE_TABLE_NOT_EXIST);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}
	response.set_result(EN_MESSAGE_ERROR_OK);
	Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
	if (ptable->is_dissolve_finish())
	{
		ptable->set_is_game_over(true);
		TableLogic::GameFinish(*ptable);
	}
	else
	{
		//广播通知解散房间
		PBCSMsg notify;
		notify.mutable_cs_notify_dissolve_table();
		TableLogic::BroadcastTableMsg(*ptable, notify);
		TableManager::Instance()->DissolveTable(tid);
	}
	return EN_Handler_Done;
}

ENHandlerResult CRequestDissolveTable::ProcessUpdateFailed(CHandlerTokenBasic* ptoken, CSession* psession)
{
	PBCSMsg msg;
	CSResponseDissolveTable & response = *msg.mutable_cs_response_dissolve_table();
	long long uid = psession->_request_route.uid();
	//根据ID获取玩家的房间ID
	int tid = TableManager::Instance()->GetPlayerTableID(uid);
	VLogMsg(CLIB_LOG_LEV_ERROR, "tid[%d] 解散房间失败 ProcessUpdateFailed ...", tid);
	CPBGameTable * ptable = TableManager::Instance()->FindTable(tid);
	if (ptable == NULL)
	{
		response.set_result(EN_MESSAGE_TABLE_NOT_EXIST);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}
	response.set_result(EN_MESSAGE_ERROR_OK);
	Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
	if (ptable->is_dissolve_finish())
	{
		ptable->set_is_game_over(true);
		TableLogic::GameFinish(*ptable);
	}
	else
	{
		//广播通知解散房间
		PBCSMsg notify;
		notify.mutable_cs_notify_dissolve_table();
		TableLogic::BroadcastTableMsg(*ptable, notify);
		TableManager::Instance()->DissolveTable(tid);
	}
	return EN_Handler_Done;
}

ENHandlerResult CRequetSendInteractiveProp::ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession)
{
	//    PBCSMsg msg;
	//    long long uid = psession->_request_route.uid();
	//    CSResponseSendInteractiveProp& response = *msg.mutable_cs_response_send_interactive_prop();
	//    const CSRequestSendInteractiveProp& request = psession->_request_msg.cs_request_send_interactive_prop();

	//    //根据ID获取玩家的房间ID
	//    int tid = TableManager::Instance()->GetPlayerTableID(uid);
	//    CPBGameTable * ptable = TableManager::Instance()->FindTable(tid);
	//    if(ptable == NULL)
	//    {
	//        response.set_result(EN_MESSAGE_TABLE_NOT_EXIST);
	//        Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
	//        return EN_Handler_Done;
	//    }
	//    int index  = TableLogic::FindIndexByUid(*ptable,uid);
	//    if(index < 0 || index > ptable->seats_size() - 1)
	//    {
	//        response.set_result(EN_MESSAGE_INVALID_SEAT_INDEX);
	//        Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
	//        return EN_Handler_Done;
	//    }
	//    response.set_result(EN_MESSAGE_ERROR_OK);
	//    Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(),psession,psession->_tmp_msg);
	//    PBCSMsg notify;
	//    CSNotifyInteractionMessage & cs_notify_interaction_message = *msg.mutable_cs_notify_interaction_message();
	//    cs_notify_interaction_message.set_source_index(index);
	//    cs_notify_interaction_message.set_destinaion_index(request.destinaion_index());
	//    cs_notify_interaction_message.set_interaction_id(request.interaction_id());
	//    TableLogic::BroadcastTableMsg(*ptable,notify,uid);
	return EN_Handler_Done;
}

ENHandlerResult CRequestChat::ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession)
{
	PBCSMsg msg;
	long long uid = psession->_request_route.uid();
	CSResponseChat & response = *msg.mutable_cs_response_chat();
	const CSRequestChat & request = psession->_request_msg.cs_request_chat();

	//根据ID获取玩家的房间ID
	int tid = TableManager::Instance()->GetPlayerTableID(uid);
	CPBGameTable * ptable = TableManager::Instance()->FindTable(tid);
	if (ptable == NULL)
	{
		response.set_result(EN_MESSAGE_TABLE_NOT_EXIST);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}
	int index = TableLogic::FindIndexByUid(*ptable, uid);
	if (index < 0 || index > ptable->seats_size() - 1)
	{
		response.set_result(EN_MESSAGE_INVALID_SEAT_INDEX);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}
	response.set_result(EN_MESSAGE_ERROR_OK);
	Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);

	const PBSDRTableSeat & seat = ptable->seats(index);
	if (seat.state() == EN_SEAT_STATE_NO_PLAYER)
	{
		response.set_result(EN_MESSAGE_INVALID_SEAT_INDEX);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}

	const PBTableUser & user = seat.user();
	PBCSMsg notify;
	CSNotifyChat & cs_notify_chat = *notify.mutable_cs_notify_chat();
	cs_notify_chat.set_uid(uid);
	cs_notify_chat.set_ctype(request.ctype());
	cs_notify_chat.set_bigfacechannel(request.bigfacechannel());
	cs_notify_chat.set_bigfaceid(request.bigfaceid());
	cs_notify_chat.set_message(request.message());
	cs_notify_chat.set_nick(user.nick());
	//TableLogic::BroadcastTableMsg(*ptable,notify,uid);
	TableLogic::BroadcastTableMsg(*ptable, notify);
	return EN_Handler_Done;
}

ENHandlerResult CReportGameRecord::ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession)
{
	const SSInnerReportGameRecord & ss_report_game_record = *psession->_request_msg.mutable_ss_inner_report_game_record();
	const PBGameRecord & game_record = ss_report_game_record.game_record();

	CPBGameTable * ptable = TableManager::Instance()->FindTable(game_record.tid());
	if (ptable != NULL)
	{
		for (int i = 0; i < ptable->seats_size(); i++)
		{
			PBUpdateData update;
			update.set_key(PBUserDataField::kUserRecord);
			{
				PBDBAtomicField& field = *update.add_field_list();
				field.set_field(EN_DB_Field_Sdr_Record);
				field.mutable_record()->CopyFrom(game_record);
				field.set_strategy(EN_Update_Strategy_Add);
			}
			psession->NewAddUpdateData(ptable->seats(i).user().uid(), update);
		}
	}
	else
	{
		VLogMsg(CLIB_LOG_LEV_DEBUG, "ptable == NULL ...");
	}

	return EN_Handler_Save;
}

ENHandlerResult CRequestTableRecord::ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession)
{
	PBCSMsg msg;
	long long uid = psession->_request_route.uid();
	CSResponseTableRecord & response = *msg.mutable_cs_response_table_record();
	//根据ID获取玩家的房间ID
	int tid = TableManager::Instance()->GetPlayerTableID(uid);
	CPBGameTable * ptable = TableManager::Instance()->FindTable(tid);
	if (ptable == NULL)
	{
		response.set_result(EN_MESSAGE_TABLE_NOT_EXIST);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}
	response.set_result(EN_MESSAGE_ERROR_OK);
	response.mutable_record()->CopyFrom(ptable->record());
	Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
	return EN_Handler_Done;
}


ENHandlerResult CInterEventAutoDoAction::ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession)
{
	const InterEventAutoAction& inter_event = psession->_request_msg.inter_event_auto_action();
	//    long long uid = inter;

	//根据ID获取玩家的房间ID
	int64 tid = inter_event.sdr_request_action().tid();
	CPBGameTable * ptable = TableManager::Instance()->FindTable(tid);
	if (ptable == NULL)
	{
		VLogMsg(CLIB_LOG_LEV_DEBUG, "InterEventAutoAction return EN_MESSAGE_TABLE_NOT_EXIST");
		return EN_Handler_Done;
	}
	//    if(ptable->state() != EN_TABLE_STATE_PLAYING)
	//    {
	//        VLogMsg(CLIB_LOG_LEV_DEBUG, "InterEventAutoAction return EN_MESSAGE_TABLE_NOT_EXIST");
	//        return EN_Handler_Done;
	//    }
	PBSDRAction request_action;
	request_action.CopyFrom(inter_event.sdr_request_action());
	int32 token = inter_event.token();

	int index = inter_event.sdr_request_action().seat_index();
	if (index < 0 || index > ptable->seats_size() - 1)
	{
		VLogMsg(CLIB_LOG_LEV_DEBUG, "InterEventAutoAction return EN_MESSAGE_INVALID_SEAT_INDEX");
		return EN_Handler_Done;
	}
	PBSDRTableSeat * p_user_seat = ptable->mutable_seats(index);
	if (!TableLogic::IsPlayerAbleDoAction(*p_user_seat, request_action, token))
	{
		VLogMsg(CLIB_LOG_LEV_DEBUG, "InterEventAutoAction return EN_MESSAGE_INVALID_ACTION");
		return EN_Handler_Done;
	}
	// 检验action合法性
	if (TableLogic::DetermineAction(*ptable, *p_user_seat, request_action) == -1)
	{
		VLogMsg(CLIB_LOG_LEV_DEBUG, "InterEventAutoAction return EN_MESSAGE_INVALID_ACTION");
		return EN_Handler_Done;
	}

	// 开始处理action
	VLogMsg(CLIB_LOG_LEV_DEBUG, "InterEventAutoAction: PlayerDoAction ...");
	TableLogic::PlayerDoAction(*ptable, *p_user_seat, request_action);

	return EN_Handler_Done;
}


ENHandlerResult CInterEventOnDoActionOver::ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession)
{
	const InterEventOnDoActionOver& inter_event = psession->_request_msg.inter_event_on_do_action_over();

	int tid = inter_event.sdr_dest_action().tid();
	CPBGameTable * ptable = TableManager::Instance()->FindTable(tid);
	if (ptable == NULL)
	{
		VLogMsg(CLIB_LOG_LEV_DEBUG, "CInterEventOnDoActionOver return EN_MESSAGE_TABLE_NOT_EXIST");
		return EN_Handler_Done;
	}

	// 1.自己出牌 2.下家出牌
	if (inter_event.type() == 1)
	{
		VLogMsg(CLIB_LOG_LEV_DEBUG, "CInterEventOnDoActionOver 当前玩家出牌 ...");
		TableLogic::ProcessChuPaiForCurrentSeat(*ptable);
	}
	else if (inter_event.type() == 2)
	{
		VLogMsg(CLIB_LOG_LEV_DEBUG, "CInterEventOnDoActionOver 下家出牌 ...");
		TableLogic::ProcessChuPaiForNextSeat(*ptable);
	}
	else if (inter_event.type() == 4)
	{
		VLogMsg(CLIB_LOG_LEV_DEBUG, "CInterEventOnDoActionOver 回调 GameStart_2_2 ...");
		TableLogic::GameStart_2(*ptable);
	}
	else if (inter_event.type() == 5)
	{
		VLogMsg(CLIB_LOG_LEV_DEBUG, "CInterEventOnDoActionOver 回调 GameStart_4 ...");
		TableLogic::GameStart_5(*ptable);
	}
	else if (inter_event.type() == 6)
	{
		VLogMsg(CLIB_LOG_LEV_DEBUG, "CInterEventOnDoActionOver 回调 GameStart_4 ...");
		TableLogic::GameStart_2(*ptable);
	}
	else if (inter_event.type() == 7)
	{
		VLogMsg(CLIB_LOG_LEV_DEBUG, "CInterEventOnDoActionOver 回调 GameStart_4 ...");
		TableLogic::GameStart_1_3(*ptable);
	}
	else
	{
		VLogMsg(CLIB_LOG_LEV_DEBUG, "CInterEventOnDoActionOver type invalid ...");
	}

	return EN_Handler_Done;
}


ENHandlerResult CNotifyRoomSvrd::ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession)
{
	TableManager::Instance()->ReportGameInfo();
	return EN_Handler_Done;
}

ENHandlerResult CInnerDissolveTable::ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession)
{
	//打完一局之后解散房间
	psession->BindProcessor(this, ptoken);
	const SSInnerDissolveTable & ss_inner_dissolve_table = psession->_request_msg.ss_inner_dissolve_table();
	int64 iTid = ss_inner_dissolve_table.tid();
	CPBGameTable * pCTable = TableManager::Instance()->FindTable(iTid);
	if (!pCTable)
	{
		return EN_Handler_Done;
	}

	//重置所有人的位置信息
	for (int i = 0; i < pCTable->seats_size(); i++)
	{
		const PBSDRTableSeat & pbSeat = pCTable->seats(i);
		if (pbSeat.state() == EN_SEAT_STATE_NO_PLAYER)
		{
			continue;
		}

		const PBTableUser & pbUser = pbSeat.user();
		PBUpdateData pbUpdate;
		pbUpdate.set_key(PBUserDataField::kUserInfo);
		{
			PBDBAtomicField & pbField = *pbUpdate.add_field_list();
			pbField.set_field(EN_DB_Field_Coin_POS);
			PBBPlayerPositionInfo& pbPos = *pbField.mutable_pos();
			pbPos.set_pos_type(EN_Position_Hall);
			pbPos.set_table_id(0);
			pbPos.set_gamesvrd_id(0);
		}
		psession->NewAddUpdateData(pbUser.uid(), pbUpdate);
	}

	//牌局日志
	pCTable->table_log << "table_id[" << pCTable->tid() << "] 解散" << "\n";
	return EN_Handler_Save;
}

ENHandlerResult CInnerDissolveTable::ProcessUpdateSucc(CHandlerTokenBasic * ptoken, CSession * psession)
{
	const SSInnerDissolveTable & ss_inner_dissolve_table = psession->_request_msg.ss_inner_dissolve_table();
	int64 iTid = ss_inner_dissolve_table.tid();
	CPBGameTable * ptable = TableManager::Instance()->FindTable(iTid);
	if (ptable == NULL)
	{
		return EN_Handler_Done;
	}

	TableManager::Instance()->DissolveTable(iTid);
	return EN_Handler_Done;
}

ENHandlerResult CInnerDissolveTable::ProcessUpdateFailed(CHandlerTokenBasic * ptoken, CSession * psession)
{
	const SSInnerDissolveTable & ss_inner_dissolve_table = psession->_request_msg.ss_inner_dissolve_table();
	int64 iTid = ss_inner_dissolve_table.tid();
	CPBGameTable * ptable = TableManager::Instance()->FindTable(iTid);
	if (ptable == NULL)
	{
		return EN_Handler_Done;
	}

	TableManager::Instance()->DissolveTable(iTid);
	return EN_Handler_Done;
}

void CInnerDissolveTable::ProcessSessionTimeOut(CHandlerTokenBasic * ptoken, CSession * psession)
{
	const SSInnerDissolveTable & ss_inner_dissolve_table = psession->_request_msg.ss_inner_dissolve_table();
	int64 tid = ss_inner_dissolve_table.tid();
	CPBGameTable * ptable = TableManager::Instance()->FindTable(tid);
	if (ptable == NULL)
	{
		return;
	}

	//广播通知解散房间 游戏已经开始 不能发这个通知 会导致看不到总结算，直接弹出去了
	//    PBCSMsg notify;
	//    notify.mutable_cs_notify_dissolve_table();
	//    TableLogic::BroadcastTableMsg(*ptable,notify);
	TableManager::Instance()->DissolveTable(tid);
	return;
}

ENHandlerResult CRequestLogoutTable::ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession)
{
	const CSRequestLogoutTable pbRequest = psession->_request_msg.cs_request_logout_table();

	PBCSMsg msg;
	CSResponseLogoutTable & response = *msg.mutable_cs_response_logout_table();
	long long uid = psession->_request_route.uid();
	int tid = TableManager::Instance()->GetPlayerTableID(uid);
	CPBGameTable * ptable = TableManager::Instance()->FindTable(tid);
	if (ptable == NULL)
	{
		response.set_result(EN_MESSAGE_ERROR_OK);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}
	int index = TableLogic::FindIndexByUid(*ptable, uid);
	if (index < 0 || index > ptable->seats_size() - 1)
	{
		response.set_result(EN_MESSAGE_ERROR_OK);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}

	if (ptable->state() == EN_TABLE_STATE_WAIT || ptable->state() == EN_TABLE_STATE_SINGLE_OVER || ptable->state() != EN_TABLE_STATE_WAIT_ROBOT)
	{
		//重置位置
		{
			PBUpdateData update;
			update.set_key(PBUserDataField::kUserInfo);
			{
				PBDBAtomicField& field = *update.add_field_list();
				field.set_field(EN_DB_Field_Coin_POS);
				PBBPlayerPositionInfo& pos = *field.mutable_pos();
				pos.set_pos_type(EN_Position_Hall);
				pos.set_table_id(0);
				pos.set_gamesvrd_id(0);
			}
			psession->NewAddUpdateData(uid, update);
		}
		return EN_Handler_Save;
	}

	if (pbRequest.reason() == EN_Logout_Reason_Change_Table)
	{
		response.set_result(EN_MESSAGE_PLAYING_CANNOT_CHANGE_TABLE);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}

	TableLogic::LogoutTable(*ptable, index);

	response.set_result(EN_MESSAGE_ERROR_OK);
	Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
	
	return EN_Handler_Done;
}

ENHandlerResult CRequestLogoutTable::ProcessUpdateSucc(CHandlerTokenBasic * ptoken, CSession * psession)
{
	long long iUid = psession->_request_route.uid();
	int tid = TableManager::Instance()->GetPlayerTableID(iUid);

	PBCSMsg msg;
	CSResponseLogoutTable & response = *msg.mutable_cs_response_logout_table();
	const CSRequestLogoutTable pbRequest = psession->_request_msg.cs_request_logout_table();
	if (pbRequest.reason() == EN_Logout_Reason_Change_Table)
	{
		response.set_last_tid(tid);
		response.set_reason(EN_Logout_Reason_Change_Table);
	}
	response.set_result(EN_MESSAGE_ERROR_OK);
	Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);

	CPBGameTable * ptable = TableManager::Instance()->FindTable(tid);
	if (ptable == NULL)
	{
		response.set_result(EN_MESSAGE_ERROR_OK);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}
	int index = TableLogic::FindIndexByUid(*ptable, iUid);
	if (index < 0 || index > ptable->seats_size() - 1)
	{
		response.set_result(EN_MESSAGE_ERROR_OK);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}
	TableLogic::LogoutTable(*ptable, index);

	return EN_Handler_Done;
}

ENHandlerResult CRequestLogoutTable::ProcessUpdateFailed(CHandlerTokenBasic * ptoken, CSession * psession)
{
	long long iUid = psession->_request_route.uid();
	int tid = TableManager::Instance()->GetPlayerTableID(iUid);

	PBCSMsg msg;
	CSResponseLogoutTable & response = *msg.mutable_cs_response_logout_table();
	const CSRequestLogoutTable pbRequest = psession->_request_msg.cs_request_logout_table();
	if (pbRequest.reason() == EN_Logout_Reason_Change_Table)
	{
		response.set_last_tid(tid);
		response.set_reason(EN_Logout_Reason_Change_Table);
	}
	response.set_result(EN_MESSAGE_ERROR_OK);
	Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);

	CPBGameTable * ptable = TableManager::Instance()->FindTable(tid);
	if (ptable == NULL)
	{
		response.set_result(EN_MESSAGE_ERROR_OK);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}
	int index = TableLogic::FindIndexByUid(*ptable, iUid);
	if (index < 0 || index > ptable->seats_size() - 1)
	{
		response.set_result(EN_MESSAGE_ERROR_OK);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}
	TableLogic::LogoutTable(*ptable, index);

	return EN_Handler_Done;
}

ENHandlerResult CNotifyPlayerHandlerClose::ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession)
{
	long long uid = psession->_request_route.uid();
	int tid = TableManager::Instance()->GetPlayerTableID(uid);
	CPBGameTable * ptable = TableManager::Instance()->FindTable(tid);
	if (ptable == NULL)
	{
		return EN_Handler_Done;
	}
	int index = TableLogic::FindIndexByUid(*ptable, uid);
	if (index < 0 || index > ptable->seats_size() - 1)
	{
		return EN_Handler_Done;
	}

	{
		const PBTableUser& user = ptable->seats(index).user();
		if (user.connect_id() != psession->_request_route.source_id())
		{
			return EN_Handler_Done;
		}
	}

	ptable->table_log << "玩家[" << uid << "] 断开链接" << "\n";
	ErrMsg("uid[%lld] disconnect to table[%ld]", uid, ptable->tid());

	PBCSMsg notify;
	CSNOtifyPlayerConnectionState & cs_notify_player_connection_state = *notify.mutable_cs_notify_player_connection_state();
	cs_notify_player_connection_state.set_seat_index(index);
	cs_notify_player_connection_state.set_connection_state(EN_Connection_State_Offline);
	PBSDRTableSeat & seat = *ptable->mutable_seats(index);
	PBTableUser & user = *seat.mutable_user();
	user.set_is_offline(true);
	TableLogic::BroadcastTableMsg(*ptable, notify, uid);
	return EN_Handler_Done;
}

ENHandlerResult CInnerOnGameStart::ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession)
{
	const InterEventOnGameStart& inter_msg = psession->_request_msg.inter_event_on_game_start();
	long long uid = inter_msg.uid();
	psession->NewAddGetData(uid, PBUserDataField::kUserInfo);
	return EN_Handler_Get;
}
ENHandlerResult CInnerOnGameStart::ProcessGetSucc(CHandlerTokenBasic * ptoken, CSession * psession)
{
	const InterEventOnGameStart& inter_msg = psession->_request_msg.inter_event_on_game_start();
	long long uid = inter_msg.uid();
	int tid = inter_msg.table_id();
	const PBUser & user = psession->_kvdb_uid_data_map[uid].user_info();

	CPBGameTable * ptable = TableManager::Instance()->FindTable(tid);
	if (ptable == NULL)
	{
		return EN_Handler_Done;
	}

	bool is_tea_bar_table = false;
	if (ptable->config().has_tbid() && ptable->config().has_master_uid())
	{
		is_tea_bar_table = true;
	}

	int cost = ptable->config().cost();
	if (!is_tea_bar_table)
	{
		// 均摊房卡模式
		if (ptable->config().pay_type() == 1)
		{
			cost /= ptable->seats_size();
			PBUpdateData update;
			update.set_key(PBUserDataField::kUserInfo);
			PBDBAtomicField & field = *update.add_field_list();
			field.set_field(EN_DB_Field_Chips);
			field.set_intval(0 - cost);
			field.set_reason(EN_Reason_Create_Sdr_Table);

			for (int i = 0; i < ptable->seats_size(); i++)
			{
				int mem_id = ptable->seats(i).user().uid();
				psession->NewAddUpdateData(mem_id, update);
			}
			return EN_Handler_Save;
		}
		// 房主开房卡模式
		else if (ptable->config().pay_type() == 0)
		{
			if (user.chips() < cost)
			{
				VLogMsg(CLIB_LOG_LEV_ERROR, "uid[%lld] cant pay for cost of create a table ...", uid);
				return EN_Handler_Done;
			}

			PBUpdateData update;
			update.set_key(PBUserDataField::kUserInfo);
			PBDBAtomicField & field = *update.add_field_list();
			field.set_field(EN_DB_Field_Chips);
			field.set_intval(0 - cost);
			field.set_reason(EN_Reason_Create_Sdr_Table);
			psession->NewAddUpdateData(uid, update);
			return EN_Handler_Save;
		}
	}
	else
	{
		if (ptable->config().tb_pay_type() == EN_TeaBar_Pay_Type_AA)
		{
			cost /= ptable->seats_size();
			PBUpdateData update;
			update.set_key(PBUserDataField::kUserInfo);
			PBDBAtomicField & field = *update.add_field_list();
			field.set_field(EN_DB_Field_Chips);
			field.set_intval(0 - cost);
			field.set_reason(EN_Reason_Create_Sdr_Table);

			for (int i = 0; i < ptable->seats_size(); i++)
			{
				int mem_id = ptable->seats(i).user().uid();
				psession->NewAddUpdateData(mem_id, update);
			}
			return EN_Handler_Save;
		}
	}
	return EN_Handler_Done;
}

ENHandlerResult CInnerOnGameStart::ProcessUpdateSucc(CHandlerTokenBasic * ptoken, CSession * psession)
{
	const InterEventOnGameStart& inter_msg = psession->_request_msg.inter_event_on_game_start();
	int tid = inter_msg.table_id();

	CPBGameTable * ptable = TableManager::Instance()->FindTable(tid);
	if (ptable == NULL)
	{
		return EN_Handler_Done;
	}

	bool is_tea_bar_table = false;
	if (ptable->config().has_tbid() && ptable->config().has_master_uid())
	{
		is_tea_bar_table = true;
	}
	int cost = ptable->config().cost();
	if (!is_tea_bar_table)
	{
		// 均摊房卡模式
		if (ptable->config().pay_type() == 1)
		{
			cost /= ptable->seats_size();
			for (int i = 0; i < ptable->seats_size(); i++)
			{
				PBTableUser& user = *ptable->mutable_seats(i)->mutable_user();
				user.set_chip(user.chip() - cost);
			}
		}
		// 房主开房卡模式
		else if (ptable->config().pay_type() == 0)
		{
			for (int i = 0; i < ptable->seats_size(); i++)
			{
				PBTableUser& user = *ptable->mutable_seats(i)->mutable_user();
				if (user.uid() == ptable->creator_uid())
				{
					user.set_chip(user.chip() - cost);
					break;
				}
			}
		}
	}
	else
	{
		if (ptable->config().tb_pay_type() == EN_TeaBar_Pay_Type_AA)
		{
			cost /= ptable->seats_size();
			for (int i = 0; i < ptable->seats_size(); i++)
			{
				PBTableUser& user = *ptable->mutable_seats(i)->mutable_user();
				user.set_chip(user.chip() - cost);
			}
		}
	}

	return EN_Handler_Done;
}


//ENHandlerResult CNotifyAutoReleaseTable::ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession)
//{
//    psession->BindProcessor(this, ptoken);

//    const SSInnerNotifyAutoReleaseTable & ss_inner_notify_auto_release_table = psession->_request_msg.ss_inner_notify_auto_release_table();
//    int64 tid = ss_inner_notify_auto_release_table.tid();
//    CPBGameTable * ptable = TableManager::Instance()->FindTable(tid);
//    if(ptable==NULL)
//    {
//        VLogMsg(CLIB_LOG_LEV_ERROR,"failed to auto release table[%lld] . table not found",tid);
//        return EN_Handler_Done;
//    }

//	bool is_tea_bar_table = false;
//	if (ptable->config().has_tbid() && ptable->config().has_master_uid())
//	{
//		is_tea_bar_table = true;
//	}

//    //超时解散房间
//    VLogMsg(CLIB_LOG_LEV_ERROR,"try to auto release table[%lld] . owner[%ld]",tid,ptable->creator_uid());
//    if(ptable->state() == EN_TABLE_STATE_WAIT && ptable->round() == 0)
//    {
//        bool need_update = false;
//        {
//			if (is_tea_bar_table && ptable->config().tb_pay_type() == EN_TeaBar_Pay_Type_Master)
//			{
//				int cost = ptable->config().cost();
//				if (cost > 0)
//				{
//					TeaBarChipsFlow flow;
//					flow.set_tbid(ptable->config().tbid());
//					flow.set_chips(cost);
//					flow.set_reason(EN_Reason_Dissolve_Tea_Bar_Table);
//					PBUpdateData updatetb;
//					updatetb.set_key(PBUserDataField::kUserTeaBarData);
//					PBDBAtomicField& field = *updatetb.add_field_list();
//					field.set_field(EN_DB_Field_TeaBar_Chips);
//					field.mutable_tea_bar_chips_flow()->CopyFrom(flow);
//					psession->NewAddUpdateData(ptable->config().master_uid(), updatetb);
//					need_update = true;
//				}
//			}
//            for(int i=0;i<ptable->seats_size();i++)
//            {
//                const PBSDRTableSeat & seat = ptable->seats(i);
//                if(seat.state() == EN_SEAT_STATE_NO_PLAYER)
//                {
//                    continue;
//                }
//                const PBTableUser & user = seat.user();
//                PBUpdateData update;
//                update.set_key(PBUserDataField::kUserInfo);
//                {
//                    PBDBAtomicField& field = *update.add_field_list();
//                    field.set_field(EN_DB_Field_POS);
//                    PBBPlayerPositionInfo& pos = *field.mutable_pos();
//                    pos.set_pos_type(EN_Position_Hall);
//                    pos.set_table_id(0);
//                    pos.set_gamesvrd_id(0);
//                }
//                if(ptable->creator_uid() == user.uid())
//                {
//                    {
//                        PBDBAtomicField& field = *update.add_field_list();
//                        field.set_field(EN_DB_Field_Create_Table);
//                        field.set_intval(0);
//                    }
////                    if (cost>0)
////                    {
////                        PBDBAtomicField & field = *update.add_field_list();
////                        field.set_field(EN_DB_Field_Chips);
////                        field.set_intval(cost);
////                        field.set_reason(EN_Reason_Dissolve_Table);
////                    }
//                }
//                psession->NewAddUpdateData(user.uid(), update);
//                need_update = true;
//            }
//            if(need_update)
//            {
//                VLogMsg(CLIB_LOG_LEV_ERROR,"try to auto release table[%lld] not start . update player info on table.",tid);
//                return EN_Handler_Save;
//            }
//            VLogMsg(CLIB_LOG_LEV_ERROR,"try to auto release table[%lld] not start . but no player in table.",tid);
//            return EN_Handler_Done;
//        }
//    }
//    else
//    {
//        bool need_update = false;
//		if (!TableLogic::NeedPayForTable(ptable->round(), ptable->state()))
//		{
//			//返还房卡
//			if (is_tea_bar_table && ptable->config().tb_pay_type() == EN_TeaBar_Pay_Type_Master)
//			{
//				int tea_bar_cost = ptable->config().cost();
//				TeaBarChipsFlow flow;
//				flow.set_tbid(ptable->config().tbid());
//				flow.set_chips(tea_bar_cost);
//				flow.set_reason(EN_Reason_Dissolve_Tea_Bar_Table);
//				PBUpdateData update;
//				update.set_key(PBUserDataField::kUserTeaBarData);
//				PBDBAtomicField& field = *update.add_field_list();
//				field.set_field(EN_DB_Field_TeaBar_Chips);
//				field.mutable_tea_bar_chips_flow()->CopyFrom(flow);
//				psession->NewAddUpdateData(ptable->config().master_uid(), update);
//				need_update = true;
//			}
//		}

//        for(int i=0;i<ptable->seats_size();i++)
//        {
//            const PBSDRTableSeat & seat = ptable->seats(i);
//            if(seat.state() == EN_SEAT_STATE_NO_PLAYER)
//            {
//                continue;
//            }
//            const PBTableUser & user = seat.user();
//            PBUpdateData update;
//            update.set_key(PBUserDataField::kUserInfo);
//            {
//                PBDBAtomicField& field = *update.add_field_list();
//                field.set_field(EN_DB_Field_POS);
//                PBBPlayerPositionInfo& pos = *field.mutable_pos();
//                pos.set_pos_type(EN_Position_Hall);
//                pos.set_table_id(0);
//                pos.set_gamesvrd_id(TGlobal::_svid);
//            }
//            if(ptable->creator_uid() == user.uid())
//            {
//                {
//                    PBDBAtomicField& field = *update.add_field_list();
//                    field.set_field(EN_DB_Field_Create_Table);
//                    field.set_intval(0);
//                }
//				/* if(true)
//				 {
//					 if (cost>0)
//					 {
//						 PBDBAtomicField & field = *update.add_field_list();
//						 field.set_field(EN_DB_Field_Chips);
//						 field.set_intval(cost);
//						 field.set_reason(EN_Reason_Dissolve_Table);
//					 }
//				 }*/
//            }
//            psession->NewAddUpdateData(user.uid(), update);
//            need_update = true;
//        }
//        if(need_update)
//        {
//            VLogMsg(CLIB_LOG_LEV_ERROR,"try to auto release table[%lld,%d] start . update player info on table.",tid,ptable->state());
//            return EN_Handler_Save;
//        }
//        return EN_Handler_Done;
//    }
//    return EN_Handler_Done;
//}

//ENHandlerResult CNotifyAutoReleaseTable::ProcessUpdateSucc(CHandlerTokenBasic* ptoken, CSession* psession)
//{
//    //根据ID获取玩家的房间ID
//    const SSInnerNotifyAutoReleaseTable & ss_inner_notify_auto_release_table = psession->_request_msg.ss_inner_notify_auto_release_table();
//    int64 tid = ss_inner_notify_auto_release_table.tid();
//    VLogMsg(CLIB_LOG_LEV_ERROR,"auto release table[%lld]. succ to update player info on table.",tid);
//    CPBGameTable * ptable = TableManager::Instance()->FindTable(tid);
//    if(ptable == NULL)
//    {
//        return EN_Handler_Done;
//    }
//    //广播通知解散房间
//    PBCSMsg notify;
//    notify.mutable_cs_notify_dissolve_table();
//    TableLogic::BroadcastTableMsg(*ptable,notify);
//    TableManager::Instance()->DissolveTable(tid);
//    return EN_Handler_Done;
//}

//ENHandlerResult CNotifyAutoReleaseTable::ProcessUpdateFailed(CHandlerTokenBasic* ptoken, CSession* psession)
//{
//    //根据ID获取玩家的房间ID
//    const SSInnerNotifyAutoReleaseTable & ss_inner_notify_auto_release_table = psession->_request_msg.ss_inner_notify_auto_release_table();
//    int64 tid = ss_inner_notify_auto_release_table.tid();
//    VLogMsg(CLIB_LOG_LEV_ERROR,"auto release table[%lld]. fail to update player info on table.",tid);
//    CPBGameTable * ptable = TableManager::Instance()->FindTable(tid);
//    if(ptable == NULL)
//    {
//        return EN_Handler_Done;
//    }
//    //广播通知解散房间
//    PBCSMsg notify;
//    notify.mutable_cs_notify_dissolve_table();
//    TableLogic::BroadcastTableMsg(*ptable,notify);
//    TableManager::Instance()->DissolveTable(tid);
//    return EN_Handler_Done;
//}

//void CNotifyAutoReleaseTable::ProcessSessionTimeOut(CHandlerTokenBasic* ptoken, CSession* psession)
//{
//    //根据ID获取玩家的房间ID
//    const SSInnerNotifyAutoReleaseTable & ss_inner_notify_auto_release_table = psession->_request_msg.ss_inner_notify_auto_release_table();
//    int64 tid = ss_inner_notify_auto_release_table.tid();
//    VLogMsg(CLIB_LOG_LEV_ERROR,"auto release table[%lld]. session timeout.",tid);
//    CPBGameTable * ptable = TableManager::Instance()->FindTable(tid);
//    if(ptable == NULL)
//    {
//        return;
//    }
//    //广播通知解散房间
//    PBCSMsg notify;
//    notify.mutable_cs_notify_dissolve_table();
//    TableLogic::BroadcastTableMsg(*ptable,notify);
//    TableManager::Instance()->DissolveTable(tid);
//    return;
//}

ENHandlerResult CRequestOffLine::ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession)
{
	long long uid = psession->_request_route.uid();
	int tid = TableManager::Instance()->GetPlayerTableID(uid);
	CPBGameTable * ptable = TableManager::Instance()->FindTable(tid);
	if (ptable == NULL)
	{
		return EN_Handler_Done;
	}
	int index = TableLogic::FindIndexByUid(*ptable, uid);
	if (index < 0 || index > ptable->seats_size() - 1)
	{
		return EN_Handler_Done;
	}

	const CSRequestOffLine & request = psession->_request_msg.cs_request_off_line();

	PBSDRTableSeat & seat = *ptable->mutable_seats(index);
	PBTableUser & user = *seat.mutable_user();
	//user.set_is_offline(true);
	user.set_is_backend(request.is_offline());

	ptable->table_log << "玩家[" << uid << "] 主动离线状态： " << user.is_backend() << "\n";

	if (user.is_backend())
	{
		PBCSMsg notify;
		CSNOtifyPlayerConnectionState & cs_notify_player_connection_state = *notify.mutable_cs_notify_player_connection_state();
		cs_notify_player_connection_state.set_seat_index(index);
		cs_notify_player_connection_state.set_connection_state(EN_Connection_State_Offline);
		TableLogic::BroadcastTableMsg(*ptable, notify, uid);
	}
	else
	{
		PBCSMsg notify;
		CSNOtifyPlayerConnectionState & cs_notify_player_connection_state = *notify.mutable_cs_notify_player_connection_state();
		cs_notify_player_connection_state.set_seat_index(index);
		cs_notify_player_connection_state.set_connection_state(EN_Connection_State_Online);
		TableLogic::BroadcastTableMsg(*ptable, notify, uid);
	}

	PBCSMsg msg;
	CSResponseOffLine& response = *msg.mutable_cs_response_off_line();
	response.set_result(EN_MESSAGE_ERROR_OK);
	Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);

	return EN_Handler_Done;
}

ENHandlerResult CNotifyCreateTable::ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession)
{
	const SSNotifyCreateTable& ss_notify_create_table = psession->_request_msg.ss_notify_create_table();
	bool is_tea_bar_table = false;
	if (ss_notify_create_table.conf().has_tbid() && ss_notify_create_table.conf().has_master_uid())
	{
		is_tea_bar_table = true;
	}

	CPBGameTable * ptable = TableManager::Instance()->FindTable(ss_notify_create_table.tid());
	if (ptable != NULL)
	{
		ErrMsg("table exsit, tid:[%ld],tbid[%ld],master_uid[%ld]", ss_notify_create_table.tid(), ss_notify_create_table.conf().tbid(),
			ss_notify_create_table.conf().master_uid());
		//如果是茶馆主人支付，返还茶馆房卡，这里开了一个已经存在的桌子，可能系统存在一些问题
		if (is_tea_bar_table)
		{
			bool need_update = false;
			if (ss_notify_create_table.conf().tb_pay_type() == EN_TeaBar_Pay_Type_Master)
			{
				int cost = ss_notify_create_table.conf().cost();
				if (cost > 0)
				{
					TeaBarChipsFlow flow;
					flow.set_tbid(ss_notify_create_table.conf().tbid());
					flow.set_chips(cost);
					flow.set_reason(EN_Reason_Dissolve_Tea_Bar_Table);
					PBUpdateData update;
					update.set_key(PBUserDataField::kUserTeaBarData);
					PBDBAtomicField& field = *update.add_field_list();
					field.set_field(EN_DB_Field_TeaBar_Chips);
					field.mutable_tea_bar_chips_flow()->CopyFrom(flow);
					psession->NewAddUpdateData(ss_notify_create_table.conf().master_uid(), update);
					need_update = true;
				}
			}

			if (ss_notify_create_table.conf().is_master_delegate() == false)
			{
				PBUpdateData update;
				update.set_key(PBUserDataField::kUserInfo);
				PBDBAtomicField& field = *update.add_field_list();
				field.set_field(EN_DB_Field_Create_Table);
				field.set_intval(0);
				psession->NewAddUpdateData(ss_notify_create_table.uid(), update);
				need_update = true;
			}

			if (need_update)
			{
				return EN_Handler_Save;
			}
		}
		return EN_Handler_Done;
	}

	ptable = TableManager::Instance()->CreateTable(ss_notify_create_table.tid(), ss_notify_create_table.conf());
	ptable->set_creator_uid(ss_notify_create_table.uid());

	if (is_tea_bar_table)
	{
		//上报给茶馆
		Message::PushInnerMsg(RouteManager::Instance()->GetRouteByRandom(), ss_notify_create_table.uid(), psession->_request_msg,
			EN_Node_TeaBar, 1);
	}
	return EN_Handler_Done;
}

ENHandlerResult CNotifyCreateTable::ProcessUpdateSucc(CHandlerTokenBasic * ptoken, CSession * psession)
{
	const SSNotifyCreateTable& ss_notify_create_table = psession->_request_msg.ss_notify_create_table();
	if (ss_notify_create_table.conf().tb_pay_type() == EN_TeaBar_Pay_Type_Master)
	{
		ErrMsg("return chips to teabar, tid[%ld],tbid[%ld],chips[%d] succ", ss_notify_create_table.tid(),
			ss_notify_create_table.conf().tbid(), ss_notify_create_table.conf().cost());
	}
	return EN_Handler_Done;
}

ENHandlerResult CNotifyCreateTable::ProcessUpdateFailed(CHandlerTokenBasic * ptoken, CSession * psession)
{
	const SSNotifyCreateTable& ss_notify_create_table = psession->_request_msg.ss_notify_create_table();
	if (ss_notify_create_table.conf().tb_pay_type() == EN_TeaBar_Pay_Type_Master)
	{
		ErrMsg("return chips to teabar, tid[%ld],tbid[%ld],chips[%d] failed", ss_notify_create_table.tid(),
			ss_notify_create_table.conf().tbid(), ss_notify_create_table.conf().cost());
	}
	return EN_Handler_Done;
}

ENHandlerResult CRequestTableDetail::ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession* psession)
{
	const CSRequestTableDetail& cs_request_table_detail = psession->_request_msg.cs_request_table_detail();
	PBCSMsg msg;
	CSResponseTableDetail& response = *msg.mutable_cs_response_table_detail();
	CPBGameTable * ptable = TableManager::Instance()->FindTable(cs_request_table_detail.tid());
	if (ptable == NULL)
	{
		//如果是茶馆桌子，上报茶馆桌子已经不存在
		if (cs_request_table_detail.has_tbid())
		{
			PBCSMsg notify;
			SSNotifyTeaBarTableNotExist& ss_notify_tea_bar_table_not_exist = *notify.mutable_ss_notify_tea_bar_table_not_exist();
			ss_notify_tea_bar_table_not_exist.set_tbid(cs_request_table_detail.tbid());
			ss_notify_tea_bar_table_not_exist.set_tid(cs_request_table_detail.tid());
			Message::PushInnerMsg(RouteManager::Instance()->GetRouteByRandom(), 0, notify, EN_Node_TeaBar, 1);
		}

		ErrMsg("ptable == NULL,tid:[%ld]", cs_request_table_detail.tid());
		response.set_result(EN_MESSAGE_TABLE_NOT_EXIST);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}
	else
	{
		if (cs_request_table_detail.tbid() != ptable->config().tbid())
		{
			PBCSMsg notify;
			SSNotifyTeaBarTableNotExist& ss_notify_tea_bar_table_not_exist = *notify.mutable_ss_notify_tea_bar_table_not_exist();
			ss_notify_tea_bar_table_not_exist.set_tbid(cs_request_table_detail.tbid());
			ss_notify_tea_bar_table_not_exist.set_tid(cs_request_table_detail.tid());
			Message::PushInnerMsg(RouteManager::Instance()->GetRouteByRandom(), 0, notify, EN_Node_TeaBar, 1);
			ErrMsg("table not match,tid:[%ld],tbid[%ld],req_tbid[%ld]", cs_request_table_detail.tid(), ptable->config().tbid(), cs_request_table_detail.tbid());
			response.set_result(EN_MESSAGE_TABLE_NOT_EXIST);
			Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
			return EN_Handler_Done;
		}
	}

	response.set_result(EN_MESSAGE_ERROR_OK);
	for (int i = 0; i < ptable->seats_size(); i++)
	{
		const PBSDRTableSeat& seat = ptable->seats(i);
		if (seat.state() == EN_SEAT_STATE_NO_PLAYER)
		{
			continue;
		}
		CSTableUserDetail& detail = *response.add_users();
		detail.set_uid(seat.user().uid());
		detail.set_name(seat.user().nick());
		detail.set_url(seat.user().role_picture_url());
	}
	Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
	return EN_Handler_Done;
}


//ENHandlerResult CRequestDissolveTeaBarTable::ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession)
//{
//	long long uid = psession->_request_route.uid();
//	const CSRequestDissolveTeaBarTable & request = psession->_request_msg.cs_request_dissolve_tea_bar_table();
//	PBCSMsg msg;
//	CSResponseDissolveTeaBarTable & response = *msg.mutable_cs_response_dissolve_tea_bar_table();
//	//根据ID获取玩家的房间ID
//	int64 tid = request.tid();
//	CPBGameTable * ptable = TableManager::Instance()->FindTable(tid);
//	if (ptable == NULL)
//	{
//		response.set_result(EN_MESSAGE_TABLE_NOT_EXIST);
//		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
//		return EN_Handler_Done;
//	}

//	//牌局已经开始，不能解散
//	if (ptable->round() > 0)
//	{
//		response.set_result(EN_MESSAGE_TEA_BAR_TABLE_GAME_START_CANNOT_DISSOLVE);
//		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
//		return EN_Handler_Done;
//	}

//	bool is_tea_bar_table = false;
//	if (ptable->config().has_tbid() && ptable->config().has_master_uid())
//	{
//		is_tea_bar_table = true;
//	}

//	if (!is_tea_bar_table)
//	{
//		response.set_result(EN_MESSAGE_TABLE_NOT_EXIST);
//		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
//		return EN_Handler_Done;
//	}

//	if (ptable->config().master_uid() != uid)
//	{
//		response.set_result(EN_MESSAGE_NOT_HAS_RIGHT);
//		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
//		return EN_Handler_Done;
//	}

//	bool need_update = false;
//	//归还房费
//	if (ptable->config().tb_pay_type() == EN_TeaBar_Pay_Type_Master)
//	{
//		int cost = ptable->config().cost();
//		if (cost > 0)
//		{
//			TeaBarChipsFlow flow;
//			flow.set_tbid(ptable->config().tbid());
//			flow.set_chips(cost);
//			flow.set_reason(EN_Reason_Dissolve_Tea_Bar_Table);
//			PBUpdateData update;
//			update.set_key(PBUserDataField::kUserTeaBarData);
//			PBDBAtomicField& field = *update.add_field_list();
//			field.set_field(EN_DB_Field_TeaBar_Chips);
//			field.mutable_tea_bar_chips_flow()->CopyFrom(flow);
//			psession->NewAddUpdateData(ptable->config().master_uid(), update);
//			need_update = true;
//		}
//	}

//	for (int i = 0; i < ptable->seats_size(); i++)
//	{
//        const PBSDRTableSeat & seat = ptable->seats(i);
//		if (seat.state() == EN_SEAT_STATE_NO_PLAYER)
//		{
//			continue;
//		}

//		need_update = true;
//		PBUpdateData update;
//		update.set_key(PBUserDataField::kUserInfo);
//		const PBTableUser & user = seat.user();
//		if (ptable->creator_uid() == user.uid())
//		{
//			PBDBAtomicField& field = *update.add_field_list();
//			field.set_field(EN_DB_Field_Create_Table);
//			field.set_intval(0);
//		}
//		{
//			PBDBAtomicField& field = *update.add_field_list();
//			field.set_field(EN_DB_Field_POS);
//			PBBPlayerPositionInfo& pos = *field.mutable_pos();
//			pos.set_pos_type(EN_Position_Hall);
//			pos.set_table_id(0);
//			pos.set_gamesvrd_id(0);
//		}

//		VLogMsg(CLIB_LOG_LEV_ERROR, "update user[%ld] pos info . on dissolve teabar table[%lld]", user.uid(), tid);
//		psession->NewAddUpdateData(user.uid(), update);
//	}

//	if (need_update)
//	{
//		return EN_Handler_Save;
//	}

//	response.set_result(EN_MESSAGE_ERROR_OK);
//	response.set_tid(tid);
//	Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);

//	//广播通知解散房间
//	PBCSMsg notify;
//	CSNotifyDissolveTable & cs_notify_dissolve_table = *notify.mutable_cs_notify_dissolve_table();
//	cs_notify_dissolve_table.mutable_statistics();
//	TableLogic::BroadcastTableMsg(*ptable, notify);
//	TableManager::Instance()->DissolveTable(tid);
//	return EN_Handler_Done;
//}

ENProcessResult CGmReuqestTableLog::ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession)
{
	const SSRequestTableLog& request = psession->_request_msg.ss_request_table_log();

	int64 tid = request.tid();
	CPBGameTable* ptable = TableManager::Instance()->FindTable(tid);
	if (ptable == NULL)
	{
		ErrMsg("failed to get table for tid[%lld]", tid);
		PBCSMsg msg;
		SSResponseTableLog & response = *msg.mutable_ss_response_table_log();
		response.set_table_data("Error");
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Process_Result_Succ;
	}

	std::string strProto;
	google::protobuf::TextFormat::PrintToString(*ptable, &strProto);
	VLogMsg(CLIB_LOG_LEV_DEBUG, "\n%s", strProto.c_str());
	{
		PBCSMsg msg;
		SSResponseTableLog & response = *msg.mutable_ss_response_table_log();
		response.set_table_data(strProto);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
	}

	return EN_Process_Result_Succ;
}

ENHandlerResult CGmReuqestDissolveTable::ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession)
{
	const SSRequestDissolveTable& request = psession->_request_msg.ss_request_dissolve_table();

	int64 tid = request.tid();
	CPBGameTable* ptable = TableManager::Instance()->FindTable(tid);
	if (ptable == NULL)
	{
		ErrMsg("failed to get table for tid[%lld]", tid);
		PBCSMsg msg;
		SSResponseDissolveTable & response = *msg.mutable_ss_response_dissolve_table();
		response.set_result("Error");
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}

	bool need_update = false;
	for (int i = 0; i < ptable->seats_size(); i++)
	{
		const PBSDRTableSeat & seat = ptable->seats(i);
		if (seat.state() == EN_SEAT_STATE_NO_PLAYER)
		{
			continue;
		}

		need_update = true;
		PBUpdateData update;
		update.set_key(PBUserDataField::kUserInfo);
		const PBTableUser & user = seat.user();
		if (ptable->creator_uid() == user.uid())
		{
			PBDBAtomicField& field = *update.add_field_list();
			field.set_field(EN_DB_Field_Create_Table);
			field.set_intval(0);
		}
		{
			PBDBAtomicField& field = *update.add_field_list();
			field.set_field(EN_DB_Field_POS);
			PBBPlayerPositionInfo& pos = *field.mutable_pos();
			pos.set_pos_type(EN_Position_Hall);
			pos.set_table_id(0);
			pos.set_gamesvrd_id(0);
		}

		VLogMsg(CLIB_LOG_LEV_ERROR, "update user[%ld] pos info . on dissolve teabar table[%lld]", user.uid(), tid);
		psession->NewAddUpdateData(user.uid(), update);
	}

	if (need_update)
	{
		return EN_Handler_Save;
	}

	return EN_Handler_Done;
}

ENHandlerResult CGmReuqestDissolveTable::ProcessUpdateSucc(CHandlerTokenBasic * ptoken, CSession * psession)
{
	const SSRequestDissolveTable& request = psession->_request_msg.ss_request_dissolve_table();
	int64 tid = request.tid();
	CPBGameTable * ptable = TableManager::Instance()->FindTable(tid);
	if (ptable == NULL)
	{
		PBCSMsg msg;
		SSResponseDissolveTable & response = *msg.mutable_ss_response_dissolve_table();
		response.set_result("Error");
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}

	PBCSMsg msg;
	SSResponseDissolveTable & response = *msg.mutable_ss_response_dissolve_table();
	response.set_result("OK");
	Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);

	//广播通知解散房间
	PBCSMsg notify;
	CSNotifyDissolveTable & cs_notify_dissolve_table = *notify.mutable_cs_notify_dissolve_table();
	cs_notify_dissolve_table.mutable_statistics();
	TableLogic::BroadcastTableMsg(*ptable, notify);
	TableManager::Instance()->DissolveTable(tid);
	return EN_Handler_Done;
}

ENHandlerResult CGmReuqestDissolveTable::ProcessUpdateFailed(CHandlerTokenBasic * ptoken, CSession * psession)
{
	const SSRequestDissolveTable& request = psession->_request_msg.ss_request_dissolve_table();
	int64 tid = request.tid();
	CPBGameTable * ptable = TableManager::Instance()->FindTable(tid);
	if (ptable == NULL)
	{
		PBCSMsg msg;
		SSResponseDissolveTable & response = *msg.mutable_ss_response_dissolve_table();
		response.set_result("Error");
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}

	PBCSMsg msg;
	SSResponseDissolveTable & response = *msg.mutable_ss_response_dissolve_table();
	response.set_result("OK");
	Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);

	//广播通知解散房间
	PBCSMsg notify;
	CSNotifyDissolveTable & cs_notify_dissolve_table = *notify.mutable_cs_notify_dissolve_table();
	cs_notify_dissolve_table.mutable_statistics();
	TableLogic::BroadcastTableMsg(*ptable, notify);
	TableManager::Instance()->DissolveTable(tid);
	return EN_Handler_Done;
}


//ENHandlerResult CRequestDissolveTeaBarTable::ProcessUpdateSucc(CHandlerTokenBasic * ptoken, CSession * psession)
//{
//	const CSRequestDissolveTeaBarTable & request = psession->_request_msg.cs_request_dissolve_tea_bar_table();
//	PBCSMsg msg;
//	CSResponseDissolveTeaBarTable & response = *msg.mutable_cs_response_dissolve_tea_bar_table();
//	int64 tid = request.tid();
//	CPBGameTable * ptable = TableManager::Instance()->FindTable(tid);
//	if (ptable == NULL)
//	{
//		response.set_result(EN_MESSAGE_TABLE_NOT_EXIST);
//		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
//		return EN_Handler_Done;
//	}
//	response.set_result(EN_MESSAGE_ERROR_OK);
//	response.set_tid(tid);
//	Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);

//	//广播通知解散房间
//	PBCSMsg notify;
//	CSNotifyDissolveTable & cs_notify_dissolve_table = *notify.mutable_cs_notify_dissolve_table();
//	cs_notify_dissolve_table.mutable_statistics();
//	TableLogic::BroadcastTableMsg(*ptable, notify);
//	TableManager::Instance()->DissolveTable(tid);
//	return EN_Handler_Done;
//}

//ENHandlerResult CRequestSkipMatchGame::ProcessRequestMsg(CHandlerTokenBasic *ptoken, CSession *psession)
//{
//	int64 iUid = psession->_request_route.uid();
//	psession->NewAddGetData(iUid, PBUserDataField::kUserInfo);
//	psession->NewAddGetData(iUid, PBUserDataField::kUserGameData);
//	return EN_Handler_Get;
//}
//
//ENHandlerResult CRequestSkipMatchGame::ProcessGetSucc(CHandlerTokenBasic *ptoken, CSession *psession)
//{
//	CSRequestSkipMatchGame & cs_request_skip_match_game = *psession->_request_msg.mutable_cs_request_skip_match_game();
//	PBSDRTableConfig * pbConf = cs_request_skip_match_game.mutable_conf();
//	int iRequestLevel = cs_request_skip_match_game.level();
//	int64 iUid = psession->_request_route.uid();
//	const PBUser pbUser = psession->_kvdb_uid_data_map[iUid].user_info();
//	int iConnectId = cs_request_skip_match_game.connect_id();
//	const PBUserGameData & pbUserGameData = psession->_kvdb_uid_data_map[iUid].user_game_data();
//	const PBUserSkipMatchInfo & pbUserSkipMatchInfo = pbUserGameData.user_skip_match_info();
//	int iGameType = (int)EN_Table_CG_DDZ;
//
//	const PBUserSkipMatchInfoItem * pbInfoItem = UserSkipMatchManager::GetItemFromSkipMatchInfo(iGameType, pbUserSkipMatchInfo);
//	int iLevel = pbInfoItem == NULL ? 1 : pbInfoItem->skipmatch_level();
//	int iState = pbInfoItem == NULL ? 0 : pbInfoItem->skipmatch_state();
//
//	//用户身上的等级和开房的等级不相等
//	if (pbUser.acc_type() != EN_Account_Robot)
//	{
//		if (iRequestLevel != iLevel)
//		{
//			VLogMsg(CLIB_LOG_LEV_DEBUG, "iRequestLevel != iLevel,iRequestLevel : %d ,iLevel = %d", iRequestLevel, iLevel);
//			PBCSMsg msg;
//			CSResponseSkipMatchGame & response = *msg.mutable_cs_response_skip_match_game();
//			response.set_result(EN_MESSAGE_LEVEL_NOT_MATCH);
//			Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
//			return EN_Handler_Done;
//		}
//	}
//
//	//如果是使用钻石开房，则要将玩家的level_state置为初始状态
//	if (cs_request_skip_match_game.has_use_diamond() && cs_request_skip_match_game.use_diamond())
//	{
//		if (iState != EN_Skip_Match_State_Failed)
//		{
//			VLogMsg(CLIB_LOG_LEV_DEBUG, "iRequestState != iState,iRequestState : %d ,iState = %d,maybe gm change", EN_Skip_Match_State_Failed, iState);
//			PBCSMsg msg;
//			CSResponseSkipMatchGame & response = *msg.mutable_cs_response_skip_match_game();
//			response.set_result(EN_MESSAGE_INVALID_STATE);
//			Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
//			return EN_Handler_Done;
//		}
//
//		//校验钻石数,暂定位1
//		int iDiamondCostByLevel = FailedCostRedisManager::Instance()->GetDiamondCostByLevel(iLevel, iConnectId);
//		/*
//		* 测试用，以后删除
//		*/
//		//iDiamondCostByLevel = 0;
//		if (iDiamondCostByLevel == -1)
//		{
//			VLogMsg(CLIB_LOG_LEV_DEBUG, "Can not find Need iDiamondCost");
//			PBCSMsg msg;
//			CSResponseSkipMatchGame & response = *msg.mutable_cs_response_skip_match_game();
//			response.set_result(EN_MESSAGE_INVALID_STATE);
//			Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
//			return EN_Handler_Done;
//		}
//		//如果用户身上的钻石小于所需要的钻石
//		if (pbUser.diamond() < iDiamondCostByLevel)
//		{
//			VLogMsg(CLIB_LOG_LEV_DEBUG, "pbUser.diamond() < iDiamondCostByLevel,pbUser.diamond() : %ld ,iDiamondCostByLevel = %d"
//				, pbUser.diamond(), iDiamondCostByLevel);
//			PBCSMsg msg;
//			CSResponseSkipMatchGame & response = *msg.mutable_cs_response_skip_match_game();
//			response.set_result(EN_MESSAGE_NO_ENOUGH_DIAMOND);
//			Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
//			return EN_Handler_Done;
//		}
//
//		//如果没有最近的记录
//		if (!pbInfoItem)
//		{
//			VLogMsg(CLIB_LOG_LEV_DEBUG, "Can not find lately pbInfoItem");
//			PBCSMsg msg;
//			CSResponseSkipMatchGame & response = *msg.mutable_cs_response_skip_match_game();
//			response.set_result(EN_MESSAGE_INVALID_STATE);
//			Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
//			return EN_Handler_Done;
//		}
//		//需要检验状态，因为有可能gm那边会改变
//		else if (pbInfoItem->skipmatch_state() != EN_Skip_Match_State_Failed)
//		{
//			VLogMsg(CLIB_LOG_LEV_DEBUG, "Lately pbInfoItem state != Failed,lately pbInfoItem state : %d,maybe gm change", pbInfoItem->skipmatch_state());
//			PBCSMsg msg;
//			CSResponseSkipMatchGame & response = *msg.mutable_cs_response_skip_match_game();
//			response.set_result(EN_MESSAGE_INVALID_STATE);
//			Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
//			return EN_Handler_Done;
//		}
//
//		{
//			PBUpdateData pbUpdate;
//			pbUpdate.set_key(PBUserDataField::kUserGameData);
//			PBDBAtomicField & pbField = *pbUpdate.add_field_list();
//			pbField.set_activity_type(EN_User_Game_Info_Skip_Match);
//			pbField.set_game_type(iGameType);
//			pbField.set_field(EN_DB_Field_Skip_Match_Info_State);
//			pbField.set_skipmatch_session_id(pbInfoItem->lately_session_id());
//			pbField.set_intval(EN_Skip_Match_State_Initial);
//			pbField.set_reason(EN_Reason_LevelAndBonus_Use_Diamond);
//			psession->NewAddUpdateData(psession->_request_route.uid(), pbUpdate);
//		}
//
//		{
//			PBUpdateData pbUpdate;
//			pbUpdate.set_key(PBUserDataField::kUserInfo);
//			PBDBAtomicField & pbField = *pbUpdate.add_field_list();
//			pbField.set_field(EN_DB_Field_Diamond);
//			pbField.set_skipmatch_session_id(pbInfoItem->lately_session_id());
//			pbField.set_intval(-iDiamondCostByLevel);
//			pbField.set_reason(EN_Reason_LevelAndBonus_Use_Diamond);
//			psession->NewAddUpdateData(psession->_request_route.uid(), pbUpdate);
//		}
//
//
//		//寻找一个桌子
//		int64 iTid = TableManager::Instance()->GetTableForUser(iLevel, pbUser.acc_type(), pbUser.nick(), pbConf);
//		if (iTid == -1)
//		{
//			VLogMsg(CLIB_LOG_LEV_DEBUG, "Can not find new tid");
//			PBCSMsg msg;
//			CSResponseSkipMatchGame & response = *msg.mutable_cs_response_skip_match_game();
//			response.set_result(EN_MESSAGE_ALLOC_TABLE_FAILED);
//			Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
//			return EN_Handler_Done;
//		}
//		psession->m_iTidTmp = iTid;
//
//		return EN_Handler_Save;
//	}
//	else if (cs_request_skip_match_game.has_give_up_level() && cs_request_skip_match_game.give_up_level())
//	{
//		if (iState != EN_Skip_Match_State_Failed)
//		{
//			VLogMsg(CLIB_LOG_LEV_DEBUG, "iRequestState != iState,iRequestState : %d ,iState = %d,maybe gm change", EN_Skip_Match_State_Failed, iState);
//			PBCSMsg msg;
//			CSResponseSkipMatchGame & response = *msg.mutable_cs_response_skip_match_game();
//			response.set_result(EN_MESSAGE_INVALID_STATE);
//			Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
//			return EN_Handler_Done;
//		}
//		//如果没有最近的记录
//		if (!pbInfoItem)
//		{
//			VLogMsg(CLIB_LOG_LEV_DEBUG, "Can not find lately pbInfoItem");
//			PBCSMsg msg;
//			CSResponseSkipMatchGame & response = *msg.mutable_cs_response_skip_match_game();
//			response.set_result(EN_MESSAGE_INVALID_STATE);
//			Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
//			return EN_Handler_Done;
//		}
//		//需要检验状态，因为有可能gm那边会改变
//		else if (pbInfoItem->skipmatch_state() != EN_Skip_Match_State_Failed)
//		{
//			VLogMsg(CLIB_LOG_LEV_DEBUG, "Lately pbInfoItem state != Failed,lately pbInfoItem state : %d,maybe gm change", pbInfoItem->skipmatch_state());
//			PBCSMsg msg;
//			CSResponseSkipMatchGame & response = *msg.mutable_cs_response_skip_match_game();
//			response.set_result(EN_MESSAGE_INVALID_STATE);
//			Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
//			return EN_Handler_Done;
//		}
//
//
//		PBUpdateData pbUpdate;
//		pbUpdate.set_key(PBUserDataField::kUserGameData);
//		{
//			PBDBAtomicField & pbField = *pbUpdate.add_field_list();
//			pbField.set_activity_type(EN_User_Game_Info_Skip_Match);
//			pbField.set_game_type(iGameType);
//			pbField.set_field(EN_DB_Field_Skip_Match_Info_Level);
//			pbField.set_skipmatch_session_id(pbInfoItem->lately_session_id());
//			pbField.set_intval(1);
//			pbField.set_reason(EN_Reason_LevelAndBonus_Choice_Reset);
//		}
//
//		{
//			PBDBAtomicField & pbField = *pbUpdate.add_field_list();
//			pbField.set_activity_type(EN_User_Game_Info_Skip_Match);
//			pbField.set_game_type(iGameType);
//			pbField.set_field(EN_DB_Field_Skip_Match_Info_State);
//			pbField.set_skipmatch_session_id(pbInfoItem->lately_session_id());
//			pbField.set_intval(EN_Skip_Match_State_Initial);
//			pbField.set_reason(EN_Reason_LevelAndBonus_Choice_Reset);
//		}
//		psession->NewAddUpdateData(psession->_request_route.uid(), pbUpdate);
//
//		//寻找一个桌子
//		int64 iTid = TableManager::Instance()->GetTableForUser(iLevel, pbUser.acc_type(), pbUser.nick(), pbConf);
//		if (iTid == -1)
//		{
//			VLogMsg(CLIB_LOG_LEV_DEBUG, "Can not find new tid");
//			PBCSMsg msg;
//			CSResponseSkipMatchGame & response = *msg.mutable_cs_response_skip_match_game();
//			response.set_result(EN_MESSAGE_ALLOC_TABLE_FAILED);
//			Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
//			return EN_Handler_Done;
//		}
//		psession->m_iTidTmp = iTid;
//
//		return EN_Handler_Save;
//	}
//	//不使用钻石开房（默认正常状态）
//	else
//	{
//		if (pbUser.acc_type() != EN_Account_Robot)
//		{
//			if (iState != EN_Skip_Match_State_Initial)
//			{
//				VLogMsg(CLIB_LOG_LEV_DEBUG, "Lately pbInfoItem state != Initial,lately pbInfoItem state : %d", pbInfoItem->skipmatch_state());
//				PBCSMsg msg;
//				CSResponseSkipMatchGame & response = *msg.mutable_cs_response_skip_match_game();
//				response.set_result(EN_MESSAGE_LEVEL_STATE_NOT_MATCH);
//				Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
//				return EN_Handler_Done;
//			}
//		}
//	}
//
//	//寻找一个桌子
//	int64 iTid = TableManager::Instance()->GetTableForUser(iLevel, pbUser.acc_type(), pbUser.nick(), pbConf);
//	if (iTid == -1)
//	{
//		VLogMsg(CLIB_LOG_LEV_DEBUG, "Can not find new tid");
//		PBCSMsg msg;
//		CSResponseSkipMatchGame & response = *msg.mutable_cs_response_skip_match_game();
//		response.set_result(EN_MESSAGE_ALLOC_TABLE_FAILED);
//		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
//		return EN_Handler_Done;
//	}
//	psession->m_iTidTmp = iTid;
//
//	PBCSMsg msg;
//	CSResponseSkipMatchGame & response = *msg.mutable_cs_response_skip_match_game();
//	response.set_result(EN_MESSAGE_ERROR_OK);
//	response.set_tid(iTid);
//	response.set_skipmatch_game_svid(TGlobal::_svid);
//	response.set_pos_type(EN_Position_CG_DDZ);
//	response.set_level_after_skip(iLevel);
//	Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
//	return EN_Handler_Done;
//}
//
//ENHandlerResult CRequestSkipMatchGame::ProcessUpdateSucc(CHandlerTokenBasic *ptoken, CSession *psession)
//{
//	CSRequestSkipMatchGame & cs_request_skip_match_game = *psession->_request_msg.mutable_cs_request_skip_match_game();
//	int64 iUid = psession->_request_route.uid();
//	//匹配信息
//	const PBUserGameData & pbUserGameData = psession->_kvdb_uid_data_map[iUid].user_game_data();
//	const PBUserSkipMatchInfo & pbUserSkipMatchInfo = pbUserGameData.user_skip_match_info();
//	int iGameType = (int)EN_Table_CG_DDZ;
//	const PBUserSkipMatchInfoItem * pbInfoItem = UserSkipMatchManager::GetItemFromSkipMatchInfo(iGameType, pbUserSkipMatchInfo);
//
//	int iLevel = pbInfoItem == NULL ? 1 : pbInfoItem->skipmatch_level();
//
//	PBCSMsg msg;
//	CSResponseSkipMatchGame & response = *msg.mutable_cs_response_skip_match_game();
//	response.set_result(EN_MESSAGE_ERROR_OK);
//	response.set_tid(psession->m_iTidTmp);
//	response.set_skipmatch_game_svid(TGlobal::_svid);
//	response.set_pos_type(EN_Position_CG_DDZ);
//	if (cs_request_skip_match_game.has_use_diamond() && cs_request_skip_match_game.use_diamond())
//	{
//		response.set_level_after_skip(iLevel);
//	}
//	else if (cs_request_skip_match_game.has_give_up_level() && cs_request_skip_match_game.give_up_level())
//	{
//		response.set_level_after_skip(1);
//	}
//	Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
//	return EN_Handler_Done;
//}

/*
进入房间
3.金币场进入房间
*/
ENHandlerResult CRequestEnterTable::ProcessRequestMsg(CHandlerTokenBasic *ptoken, CSession *psession)
{
	const CSRequestSdrEnterTable & cs_request_enter_table = psession->_request_msg.cs_request_sdr_enter_table();
	int iConnectId = cs_request_enter_table.connect_id();

	if (!SessionManager::Instance()->LockProcess(psession))
	{
		VLogMsg(CLIB_LOG_LEV_DEBUG, "User[%ld] enter table,lock error", psession->_request_route.uid());
		PBCSMsg msg;
		CSResponseSdrEnterTable & response = *msg.mutable_cs_response_sdr_enter_table();
		response.set_result(EN_MESSAGE_ERROR_REQUEST_PROCESSING);
		Message::PushInnerMsg(RouteManager::Instance()->GetRouteByRandom(), psession->_request_route.uid(), msg, EN_Node_Connect, iConnectId);

		return EN_Handler_Done;
	}
	long long uid = psession->_request_route.uid();
	psession->NewAddGetData(uid, PBUserDataField::kUserInfo);
	return EN_Handler_Get;
}

/*
进入房间
3.金币场进入房间
*/
ENHandlerResult CRequestEnterTable::ProcessGetSucc(CHandlerTokenBasic *ptoken, CSession *psession)
{
	const CSRequestSdrEnterTable & pbRequst = psession->_request_msg.cs_request_sdr_enter_table();
	int64 iUid = psession->_request_route.uid();
	const PBUser pbUser = psession->_kvdb_uid_data_map[iUid].user_info();
	int iTid = pbRequst.tid();
	int iConnectId = pbRequst.connect_id();
	int iLevel = pbRequst.coin_match_level();

	//查找桌子,没有桌子则创建桌子
	CPBGameTable * pCTable = TableManager::Instance()->FindTable(iTid);
	if (!pCTable)
	{
		if (pbUser.has_sdr_conf())	
		{
			PBSDRTableConfig pbConfTmp(pbUser.sdr_conf());
			pCTable = TableManager::Instance()->CreateTableByP(iTid, iLevel, &pbConfTmp);	
		}
		else
		{
			//默认配置
			pCTable = TableManager::Instance()->CreateTableByP(iTid, iLevel, NULL);
		}
	}

	//玩家重连
	if (TableLogic::FindUserInTable(*pCTable, pbUser.uid()) != NULL)
	{
		PBCSMsg innermsg;
		SSNotifyPlayerPosChange &  pbNotify = *innermsg.mutable_ss_notify_player_pos_change();
		pbNotify.mutable_pos()->set_pos_type(EN_Position_3Ren_DDZ_COIN);
		pbNotify.mutable_pos()->set_gamesvrd_id(TGlobal::_svid);
		Message::PushInnerMsg(RouteManager::Instance()->GetRouteByRandom(), psession->_request_route.uid(), innermsg, EN_Node_Connect, iConnectId);

		PBCSMsg msg;
		CSResponseSdrEnterTable & response = *msg.mutable_cs_response_sdr_enter_table();
		VLogMsg(CLIB_LOG_LEV_ERROR, "user[%lld] reconnect to table[%d].", iUid, iTid);
		TableLogic::OnUserReconnect(*pCTable, iUid, iConnectId);
		response.set_result(EN_MESSAGE_ERROR_OK);
		response.set_gamesvrd_id(TGlobal::_svid);
		response.set_pos_type(EN_Position_3Ren_DDZ_COIN);
		CSNotifyTableInfo & info = *response.mutable_table_info();
		TableLogic::CopyTableToNotify(*pCTable, info, iUid);
		TableManager::Instance()->OnPlayerEnterTable(iUid, iTid, true);
		Message::PushInnerMsg(RouteManager::Instance()->GetRouteByRandom(), psession->_request_route.uid(), msg, EN_Node_Connect, iConnectId);
		return EN_Handler_Done;
	}

	//玩家进入桌子操作（更新玩家数据）
	PBSDRTableSeat * ppbSeat = TableLogic::FindEmptySeatInTable(*pCTable);
	if (!ppbSeat)
	{
		VLogMsg(CLIB_LOG_LEV_DEBUG, "Can not find new tid");
		PBCSMsg msg;
		CSResponseSdrEnterTable & response = *msg.mutable_cs_response_sdr_enter_table();
		response.set_result(EN_MESSAGE_NO_EMPTY_SEAT);
		Message::PushInnerMsg(RouteManager::Instance()->GetRouteByRandom(), psession->_request_route.uid(), msg, EN_Node_Connect, iConnectId);

		//这里需要清空玩家的信息
		psession->_fsm_state = EN_EnterTable_State_ULOCK_AF_SAVE;
		PBUpdateData update;
		update.set_key(PBUserDataField::kUserInfo);
		{
			PBDBAtomicField& field = *update.add_field_list();
			field.set_field(EN_DB_Field_Coin_POS);
			PBBPlayerPositionInfo& pos = *field.mutable_pos();
			pos.set_pos_type(EN_Position_Hall);
			pos.set_table_id(0);
			pos.set_gamesvrd_id(0);
		}
		psession->NewAddUpdateData(psession->_request_route.uid(), update);
		return EN_Handler_Done;
	}

	PBUpdateData update;
	update.set_key(PBUserDataField::kUserInfo);
	{
		PBDBAtomicField& field = *update.add_field_list();
		field.set_field(EN_DB_Field_Coin_POS);
		PBBPlayerPositionInfo& pos = *field.mutable_pos();
		pos.set_pos_type(EN_Position_3Ren_DDZ_COIN);
		pos.set_table_id(iTid);
		pos.set_gamesvrd_id(TGlobal::_svid);
	}
	VLogMsg(CLIB_LOG_LEV_DEBUG, "user[%lld] enter table[%d] svrd[%d].", iUid, iTid, TGlobal::_svid);
	psession->NewAddUpdateData(psession->_request_route.uid(), update);
	return EN_Handler_Save;
}

/*
进入房间
3.金币场进入房间
*/
ENHandlerResult CRequestEnterTable::ProcessUpdateSucc(CHandlerTokenBasic * ptoken, CSession * psession)
{
	int64 iUid = psession->_request_route.uid();
	const PBUser & pbUser = psession->_kvdb_uid_data_map[iUid].user_info();
	const CSRequestSdrEnterTable& cs_request_enter_table = psession->_request_msg.cs_request_sdr_enter_table();
	int iTid = cs_request_enter_table.tid();
	int iConnectId = cs_request_enter_table.connect_id();

	if (psession->_fsm_state == EN_EnterTable_State_ULOCK)
	{
		VLogMsg(CLIB_LOG_LEV_DEBUG, "User[%ld] enter table,lock error", psession->_request_route.uid());
		PBCSMsg msg;
		CSResponseSdrEnterTable & response = *msg.mutable_cs_response_sdr_enter_table();
		response.set_result(EN_MESSAGE_TABLE_NOT_EXIST);
		Message::PushInnerMsg(RouteManager::Instance()->GetRouteByRandom(), psession->_request_route.uid(), msg, EN_Node_Connect, iConnectId);
		return EN_Handler_Done;
	}
	else if (psession->_fsm_state == EN_EnterTable_State_ULOCK_AF_SAVE)
	{
		VLogMsg(CLIB_LOG_LEV_DEBUG, "User[%ld] enter table,lock error", psession->_request_route.uid());
		PBCSMsg msg;
		CSResponseSdrEnterTable & response = *msg.mutable_cs_response_sdr_enter_table();
		response.set_result(EN_MESSAGE_NO_EMPTY_SEAT);
		Message::PushInnerMsg(RouteManager::Instance()->GetRouteByRandom(), psession->_request_route.uid(), msg, EN_Node_Connect, iConnectId);
		return EN_Handler_Done;
	}

	PBCSMsg msg;
	CSResponseSdrEnterTable & response = *msg.mutable_cs_response_sdr_enter_table();
	CPBGameTable * pCtable = TableManager::Instance()->FindTable(iTid);
	if (!pCtable)
	{
		VLogMsg(CLIB_LOG_LEV_DEBUG, "Can not find table[%d]", iTid);
		PBCSMsg msg;
		CSResponseSdrEnterTable & response = *msg.mutable_cs_response_sdr_enter_table();
		response.set_result(EN_MESSAGE_INVALID_TABLE_STATE);
		Message::PushInnerMsg(RouteManager::Instance()->GetRouteByRandom(), psession->_request_route.uid(), msg, EN_Node_Connect, iConnectId);

		return EN_Handler_Done;
	}

	//找到一个空位置
	PBSDRTableSeat * ppbSeat = TableLogic::FindEmptySeatInTable(*pCtable);
	if (!ppbSeat)
	{
		//这里需要清空玩家的信息
		psession->_fsm_state = EN_EnterTable_State_ULOCK_AF_SAVE;
		PBUpdateData update;
		update.set_key(PBUserDataField::kUserInfo);
		{
			PBDBAtomicField& field = *update.add_field_list();
			field.set_field(EN_DB_Field_Coin_POS);
			PBBPlayerPositionInfo& pos = *field.mutable_pos();
			pos.set_pos_type(EN_Position_Hall);
			pos.set_table_id(0);
			pos.set_gamesvrd_id(0);
		}
		psession->NewAddUpdateData(psession->_request_route.uid(), update);

		//通知客户端
		{
			VLogMsg(CLIB_LOG_LEV_DEBUG, "Can not find Seat[%ld] int Table[%d]", psession->_request_route.uid(), iTid);
			PBCSMsg msg;
			CSResponseSdrEnterTable & response = *msg.mutable_cs_response_sdr_enter_table();
			response.set_result(EN_MESSAGE_NO_EMPTY_SEAT);
			Message::PushInnerMsg(RouteManager::Instance()->GetRouteByRandom(), psession->_request_route.uid(), msg, EN_Node_Connect, iConnectId);
		}
		return EN_Handler_Done;
	}

	TableLogic::SitDownOnTable(*pCtable, *ppbSeat, pbUser, iConnectId);

	CSNotifyTableInfo & info = *response.mutable_table_info();
	TableLogic::CopyTableToNotify(*pCtable, info, iUid);

	//如果这个房间的人已经满了,开启准备定时器
	int iPlayerNum = TableLogic::GetPlayerNum(*pCtable);
	if (iPlayerNum >= pCtable->config().seat_num())
	{
		pCtable->StartReadyTimer(10, true);
	}
	
	//每进一个人需要重新置桌子的状态
	pCtable->set_state(EN_TABLE_STATE_WAIT);
	pCtable->StartWaitingOhterTimer(5, true);

	PBCSMsg innermsg;
	SSNotifyPlayerPosChange &  pbNotify = *innermsg.mutable_ss_notify_player_pos_change();
	pbNotify.mutable_pos()->set_pos_type(EN_Position_3Ren_DDZ_COIN);
	pbNotify.mutable_pos()->set_gamesvrd_id(TGlobal::_svid);
	Message::PushInnerMsg(RouteManager::Instance()->GetRouteByRandom(), psession->_request_route.uid(), innermsg, EN_Node_Connect, iConnectId);

	response.set_result(EN_MESSAGE_ERROR_OK);
	response.set_gamesvrd_id(TGlobal::_svid);
	response.set_pos_type(EN_Position_3Ren_DDZ_COIN);
	Message::PushInnerMsg(RouteManager::Instance()->GetRouteByRandom(), psession->_request_route.uid(), msg, EN_Node_Connect, iConnectId);

	if (pbUser.acc_type() == EN_Account_Robot)
	{
		//TableLogic::OnReadTimerTimeOut(*pCtable);
	}

	return EN_Handler_Done;
}

//ENHandlerResult CInnerUpdateSkipMatchResult::GetState_NoInfo::ProcessGet(CHandlerTokenBasic * ptoken, CSession * psession)
//{
//	psession->m_pVoidTmp = &this->m_pCInnerUpdateSkipMatchResult->m_CHadInfoGetState; //状态置为已获得信息状态
//
//	const SSInnerUpdateSkipMatchResult & pbRequest = psession->_request_msg.ss_inner_update_skip_match_result();
//	int iTid = pbRequest.tid();
//
//	//找到牌桌
//	CPBGameTable * CTable = TableManager::Instance()->FindTable(iTid);
//	if (!CTable)
//	{
//		return EN_Handler_Done;
//	}
//
//	bool bIsNeedUpdate = false;
//	int64 iNowTime = time(NULL);
//	int64 iSessionIdLeft = time(NULL) << 4;
//
//	int iTrusteeshipNum = 0;
//	for (int i = 0; i < CTable->seats_size(); i++)
//	{
//		const PBSDRTableSeat & pbSeat = CTable->seats(i);
//
//		//处于托管状态
//		if (pbSeat.is_trusteeship())
//		{
//			iTrusteeshipNum++;
//		}
//		//没有处于托管状态，但是在结束前的30秒处于托管状态
//		else if (pbSeat.has_end_trusteeship_time())
//		{
//			if (iNowTime > pbSeat.end_trusteeship_time() && (iNowTime - 30) < pbSeat.end_trusteeship_time())
//			{
//				iTrusteeshipNum++;
//			}
//		}
//	}
//
//	//如果3个人都托管了
//	if (iTrusteeshipNum >= 3)
//	{
//		for (int i = 0; i < CTable->seats_size(); i++)
//		{
//			const PBSDRTableSeat & pbSeat = CTable->seats(i);
//			if (pbSeat.state() == EN_SEAT_STATE_NO_PLAYER)
//			{
//				continue;
//			}
//
//			const PBTableUser & pbUser = pbSeat.user();
//			psession->m_iTableUserTmp.push_back(pbUser);
//			{
//				int iSkipmatchState = EN_Skip_Match_State_Failed;
//				int iNeedShareNum = 0;
//				int iNeedDiamondCost = 0;
//				UserSkipMatchManager::GetCostValByLevelByConnectType(pbUser.skipmatch_level(), pbUser.connect_id(), iNeedShareNum,
//					iNeedDiamondCost);
//				{
//					PBUpdateData pbUpdate;
//					pbUpdate.set_key(PBUserDataField::kUserGameData);
//					PBDBAtomicField & pbField = *pbUpdate.add_field_list();
//					pbField.set_activity_type(EN_User_Game_Info_Skip_Match);
//					pbField.set_game_type(EN_Position_3Ren_DDZ_COIN);
//					pbField.set_field(EN_DB_Field_Skip_Match_Info_State);
//					pbField.set_skipmatch_session_id(iSessionIdLeft + pbUser.uid());
//					pbField.set_intval(iSkipmatchState);
//					pbField.set_reason(EN_Reason_LevelAndBonus_Game_Over_Settle);
//					pbField.set_skipmatch_need_share_num(iNeedShareNum);
//					pbField.set_skipmatch_need_diamond_num(iNeedDiamondCost);
//
//					psession->NewAddUpdateData(pbUser.uid(), pbUpdate);
//				}
//				bIsNeedUpdate = true;
//			}
//		}
//	}
//	//只有1~2个人是托管的状态(且距离30秒)
//	else if (iTrusteeshipNum >= 1 && iTrusteeshipNum < 3)
//	{
//		for (int i = 0; i < CTable->seats_size(); i++)
//		{
//			const PBSDRTableSeat & pbSeat = CTable->seats(i);
//			if (pbSeat.state() == EN_SEAT_STATE_NO_PLAYER)
//			{
//				continue;
//			}
//
//			const PBTableUser & pbUser = pbSeat.user();
//			psession->m_iTableUserTmp.push_back(pbUser);
//			//如果是在托管状态,需要置为失败状态
//			if (pbSeat.is_trusteeship())
//			{
//				int iSkipmatchState = EN_Skip_Match_State_Failed;
//				int iNeedShareNum = 0;
//				int iNeedDiamondCost = 0;
//				UserSkipMatchManager::GetCostValByLevelByConnectType(pbUser.skipmatch_level(), pbUser.connect_id(), iNeedShareNum,
//					iNeedDiamondCost);
//				{
//					PBUpdateData pbUpdate;
//					pbUpdate.set_key(PBUserDataField::kUserGameData);
//					PBDBAtomicField & pbField = *pbUpdate.add_field_list();
//					pbField.set_activity_type(EN_User_Game_Info_Skip_Match);
//					pbField.set_game_type(EN_Position_3Ren_DDZ_COIN);
//					pbField.set_field(EN_DB_Field_Skip_Match_Info_State);
//					pbField.set_skipmatch_session_id(iSessionIdLeft + pbUser.uid());
//					pbField.set_intval(iSkipmatchState);
//					pbField.set_reason(EN_Reason_LevelAndBonus_Game_Over_Settle);
//					pbField.set_skipmatch_need_share_num(iNeedShareNum);
//					pbField.set_skipmatch_need_diamond_num(iNeedDiamondCost);
//					psession->NewAddUpdateData(pbUser.uid(), pbUpdate);
//				}
//				bIsNeedUpdate = true;
//			}
//			else
//			{
//				//如果没有托管，并且获胜了,需要给它往上晋升一级
//				if (pbSeat.final_score() > 0)
//				{
//					int iSkipmatchLevelChangeVal = 0;
//					int iSkipmatchState = EN_Skip_Match_State_Initial;
//					//如果等级是7
//					if (pbUser.skipmatch_level() + 1 >= 8)
//					{
//						iSkipmatchLevelChangeVal = -pbUser.skipmatch_level() + 1;
//						iSkipmatchState = EN_Skip_Match_State_Initial;
//
//						PBUpdateData pbUpdate;
//						pbUpdate.set_key(PBUserDataField::kUserGameData);
//						{
//							PBDBAtomicField & pbField = *pbUpdate.add_field_list();
//							pbField.set_activity_type(EN_User_Game_Info_Skip_Match);
//							pbField.set_game_type(EN_Position_3Ren_DDZ_COIN);
//							pbField.set_field(EN_DB_Field_Skip_Match_Info_Level);
//							pbField.set_strategy(EN_Update_Strategy_Replace);
//							pbField.set_skipmatch_session_id(iSessionIdLeft + pbUser.uid());
//							pbField.set_intval(1);
//							pbField.set_reason(EN_Reason_LevelAndBonus_Game_Over_Settle);
//						}
//						{
//							PBDBAtomicField & pbField = *pbUpdate.add_field_list();
//							pbField.set_activity_type(EN_User_Game_Info_Skip_Match);
//							pbField.set_game_type(EN_Position_3Ren_DDZ_COIN);
//							pbField.set_field(EN_DB_Field_Skip_Match_Info_State);
//							pbField.set_skipmatch_session_id(iSessionIdLeft + pbUser.uid());
//							pbField.set_intval(iSkipmatchState);
//							pbField.set_reason(EN_Reason_LevelAndBonus_Game_Over_Settle);
//							pbField.set_skipmatch_need_share_num(0);
//							pbField.set_skipmatch_need_diamond_num(0);
//						}
//						{
//							PBDBAtomicField & pbField = *pbUpdate.add_field_list();
//							pbField.set_activity_type(EN_User_Game_Info_Skip_Match);
//							pbField.set_game_type(EN_Position_3Ren_DDZ_COIN);
//							pbField.set_field(EN_DB_Field_Skip_Match_Info_Total_Win_Num);
//							pbField.set_skipmatch_session_id(iSessionIdLeft + pbUser.uid());
//							pbField.set_intval(1);
//							pbField.set_reason(EN_Reason_LevelAndBonus_Game_Over_Settle);
//						}
//						psession->NewAddUpdateData(pbUser.uid(), pbUpdate);
//						bIsNeedUpdate = true;
//					}
//					//如果等级是7以下
//					else if (pbUser.skipmatch_level() > 0)
//					{
//						iSkipmatchLevelChangeVal = 1;
//						iSkipmatchState = EN_Skip_Match_State_Initial;
//
//						PBUpdateData pbUpdate;
//						pbUpdate.set_key(PBUserDataField::kUserGameData);
//						{
//							PBDBAtomicField & pbField = *pbUpdate.add_field_list();
//							pbField.set_activity_type(EN_User_Game_Info_Skip_Match);
//							pbField.set_game_type(EN_Position_3Ren_DDZ_COIN);
//							pbField.set_field(EN_DB_Field_Skip_Match_Info_Level);
//							pbField.set_skipmatch_session_id(iSessionIdLeft + pbUser.uid());
//							pbField.set_strategy(EN_Update_Strategy_Inc);
//							pbField.set_intval(iSkipmatchLevelChangeVal);
//							pbField.set_reason(EN_Reason_LevelAndBonus_Game_Over_Settle);
//						}
//						{
//							PBDBAtomicField & pbField = *pbUpdate.add_field_list();
//							pbField.set_activity_type(EN_User_Game_Info_Skip_Match);
//							pbField.set_game_type(EN_Position_3Ren_DDZ_COIN);
//							pbField.set_field(EN_DB_Field_Skip_Match_Info_State);
//							pbField.set_skipmatch_session_id(iSessionIdLeft + pbUser.uid());
//							pbField.set_intval(iSkipmatchState);
//							pbField.set_reason(EN_Reason_LevelAndBonus_Game_Over_Settle);
//							pbField.set_skipmatch_need_share_num(0);
//							pbField.set_skipmatch_need_diamond_num(0);
//						}
//						psession->NewAddUpdateData(pbUser.uid(), pbUpdate);
//						bIsNeedUpdate = true;
//					}
//				}
//				else
//				{
//					int iSkipmatchState = EN_Skip_Match_State_Initial;
//					PBUpdateData pbUpdate;
//					pbUpdate.set_key(PBUserDataField::kUserGameData);
//					PBDBAtomicField & pbField = *pbUpdate.add_field_list();
//					pbField.set_activity_type(EN_User_Game_Info_Skip_Match);
//					pbField.set_game_type(EN_Position_3Ren_DDZ_COIN);
//					pbField.set_field(EN_DB_Field_Skip_Match_Info_State);
//					pbField.set_skipmatch_session_id(iSessionIdLeft + pbUser.uid());
//					pbField.set_intval(iSkipmatchState);
//					pbField.set_reason(EN_Reason_LevelAndBonus_Game_Over_Settle);
//					pbField.set_lately_session_has_win_or_lose(false);
//					psession->NewAddUpdateData(pbUser.uid(), pbUpdate);
//				}
//			}
//		}
//	}
//	//正常结束游戏
//	else
//	{
//		for (int i = 0; i < CTable->seats_size(); i++)
//		{
//			const PBSDRTableSeat & pbSeat = CTable->seats(i);
//			if (pbSeat.state() == EN_SEAT_STATE_NO_PLAYER)
//			{
//				continue;
//			}
//
//			const PBTableUser & pbUser = pbSeat.user();
//			psession->m_iTableUserTmp.push_back(pbUser);
//			int iSkipmatchLevelChangeVal = 0;
//			int iSkipmatchState = 0;
//			if (pbSeat.final_score() > 0)
//			{
//				//如果等级是7
//				if (pbUser.skipmatch_level() + 1 >= 8)
//				{
//					iSkipmatchLevelChangeVal = -pbUser.skipmatch_level() + 1;
//					iSkipmatchState = EN_Skip_Match_State_Initial;
//
//					PBUpdateData pbUpdate;
//					pbUpdate.set_key(PBUserDataField::kUserGameData);
//					{
//						PBDBAtomicField & pbField = *pbUpdate.add_field_list();
//						pbField.set_activity_type(EN_User_Game_Info_Skip_Match);
//						pbField.set_game_type(EN_Position_3Ren_DDZ_COIN);
//						pbField.set_field(EN_DB_Field_Skip_Match_Info_Level);
//						pbField.set_strategy(EN_Update_Strategy_Replace);
//						pbField.set_skipmatch_session_id(iSessionIdLeft + pbUser.uid());
//						pbField.set_intval(1);
//						pbField.set_reason(EN_Reason_LevelAndBonus_Game_Over_Settle);
//					}
//					{
//						PBDBAtomicField & pbField = *pbUpdate.add_field_list();
//						pbField.set_activity_type(EN_User_Game_Info_Skip_Match);
//						pbField.set_game_type(EN_Position_3Ren_DDZ_COIN);
//						pbField.set_field(EN_DB_Field_Skip_Match_Info_State);
//						pbField.set_skipmatch_session_id(iSessionIdLeft + pbUser.uid());
//						pbField.set_intval(iSkipmatchState);
//						pbField.set_reason(EN_Reason_LevelAndBonus_Game_Over_Settle);
//						pbField.set_skipmatch_need_share_num(0);
//						pbField.set_skipmatch_need_diamond_num(0);
//					}
//					{
//						PBDBAtomicField & pbField = *pbUpdate.add_field_list();
//						pbField.set_activity_type(EN_User_Game_Info_Skip_Match);
//						pbField.set_game_type(EN_Position_3Ren_DDZ_COIN);
//						pbField.set_field(EN_DB_Field_Skip_Match_Info_Total_Win_Num);
//						pbField.set_skipmatch_session_id(iSessionIdLeft + pbUser.uid());
//						pbField.set_intval(1);
//						pbField.set_reason(EN_Reason_LevelAndBonus_Game_Over_Settle);
//					}
//					psession->NewAddUpdateData(pbUser.uid(), pbUpdate);
//					bIsNeedUpdate = true;
//				}
//				//如果等级是7以下
//				else if (pbUser.skipmatch_level() > 0)
//				{
//					iSkipmatchLevelChangeVal = 1;
//					iSkipmatchState = EN_Skip_Match_State_Initial;
//
//					PBUpdateData pbUpdate;
//					pbUpdate.set_key(PBUserDataField::kUserGameData);
//					{
//						PBDBAtomicField & pbField = *pbUpdate.add_field_list();
//						pbField.set_activity_type(EN_User_Game_Info_Skip_Match);
//						pbField.set_game_type(EN_Position_3Ren_DDZ_COIN);
//						pbField.set_field(EN_DB_Field_Skip_Match_Info_Level);
//						pbField.set_skipmatch_session_id(iSessionIdLeft + pbUser.uid());
//						pbField.set_strategy(EN_Update_Strategy_Inc);
//						pbField.set_intval(iSkipmatchLevelChangeVal);
//						pbField.set_reason(EN_Reason_LevelAndBonus_Game_Over_Settle);
//					}
//					{
//						PBDBAtomicField & pbField = *pbUpdate.add_field_list();
//						pbField.set_activity_type(EN_User_Game_Info_Skip_Match);
//						pbField.set_game_type(EN_Position_3Ren_DDZ_COIN);
//						pbField.set_field(EN_DB_Field_Skip_Match_Info_State);
//						pbField.set_skipmatch_session_id(iSessionIdLeft + pbUser.uid());
//						pbField.set_intval(iSkipmatchState);
//						pbField.set_reason(EN_Reason_LevelAndBonus_Game_Over_Settle);
//						pbField.set_skipmatch_need_share_num(0);
//						pbField.set_skipmatch_need_diamond_num(0);
//					}
//					psession->NewAddUpdateData(pbUser.uid(), pbUpdate);
//					bIsNeedUpdate = true;
//				}
//			}
//			//输了
//			else if (pbSeat.final_score() < 0)
//			{
//				iSkipmatchState = EN_Skip_Match_State_Failed;
//				int iNeedShareNum = 0;
//				int iNeedDiamondCost = 0;
//				UserSkipMatchManager::GetCostValByLevelByConnectType(pbUser.skipmatch_level(), pbUser.connect_id(), iNeedShareNum,
//					iNeedDiamondCost);
//				PBUpdateData pbUpdate;
//				pbUpdate.set_key(PBUserDataField::kUserGameData);
//				{
//					PBDBAtomicField & pbField = *pbUpdate.add_field_list();
//					pbField.set_activity_type(EN_User_Game_Info_Skip_Match);
//					pbField.set_game_type(EN_Position_3Ren_DDZ_COIN);
//					pbField.set_field(EN_DB_Field_Skip_Match_Info_State);
//					pbField.set_skipmatch_session_id(iSessionIdLeft + pbUser.uid());
//					pbField.set_intval(iSkipmatchState);
//					pbField.set_reason(EN_Reason_LevelAndBonus_Game_Over_Settle);
//					pbField.set_skipmatch_need_share_num(iNeedShareNum);
//					pbField.set_skipmatch_need_diamond_num(iNeedDiamondCost);
//					psession->NewAddUpdateData(pbUser.uid(), pbUpdate);
//				}
//				bIsNeedUpdate = true;
//			}
//			else
//			{
//				PBUpdateData pbUpdate;
//				pbUpdate.set_key(PBUserDataField::kUserGameData);
//				{
//					PBDBAtomicField & pbField = *pbUpdate.add_field_list();
//					pbField.set_activity_type(EN_User_Game_Info_Skip_Match);
//					pbField.set_game_type(EN_Position_3Ren_DDZ_COIN);
//					pbField.set_field(EN_DB_Field_Skip_Match_Info_State);
//					pbField.set_skipmatch_session_id(iSessionIdLeft + pbUser.uid());
//					pbField.set_intval(0);
//					pbField.set_reason(EN_Reason_LevelAndBonus_Game_Over_Settle);
//					pbField.set_skipmatch_need_share_num(0);
//					pbField.set_skipmatch_need_diamond_num(0);
//					psession->NewAddUpdateData(pbUser.uid(), pbUpdate);
//				}
//			}
//		}
//	}
//
//	return EN_Handler_Save;
//}
//
//ENHandlerResult CInnerUpdateSkipMatchResult::GetState_HadInfo::ProcessGet(CHandlerTokenBasic * ptoken, CSession * psession)
//{
//	for (unsigned int i = 0; i < psession->m_iTableUserTmp.size(); i++)
//	{
//		const PBTableUser & pbUser = psession->m_iTableUserTmp.at(i);
//
//		const PBUserGameData & pbUserGameData = psession->_kvdb_uid_data_map[pbUser.uid()].user_game_data();
//		const PBUserSkipMatchInfo & pbUserSkipMatchInfo = pbUserGameData.user_skip_match_info();
//		int iGameType = (int)EN_Position_3Ren_DDZ_COIN;
//		const PBUserSkipMatchInfoItem * pbInfoItem = UserSkipMatchManager::GetItemFromSkipMatchInfo(iGameType, pbUserSkipMatchInfo);
//		if (!pbInfoItem)
//		{
//			continue;
//		}
//
//		//通知客户端的结果暂存
//		{
//			PBCSMsg pbNotify;
//			CSNotifySkipMatchResult & pbResult = *pbNotify.mutable_cs_notify_skip_match_result();
//			pbResult.mutable_user_game_data()->mutable_user_skip_match_info()->add_skip_match_info_item()->CopyFrom(*pbInfoItem);
//			Message::PushMsg(RouteManager::Instance()->GetRouteByRandom(), pbUser.uid(), pbNotify, EN_Node_Connect, pbUser.connect_id());
//		}
//	}
//
//	return EN_Handler_Done;
//}
//
////通知匹配结果
//ENHandlerResult CInnerUpdateSkipMatchResult::ProcessRequestMsg(CHandlerTokenBasic *ptoken, CSession *psession)
//{
//	psession->m_pVoidTmp = &m_CNoInfoGetState; //设置这个sessionid的默认get状态
//
//	const SSInnerUpdateSkipMatchResult & pbRequest = psession->_request_msg.ss_inner_update_skip_match_result();
//	int iTid = pbRequest.tid();
//
//	//找到牌桌
//	CPBGameTable * CTable = TableManager::Instance()->FindTable(iTid);
//	if (!CTable)
//	{
//		return EN_Handler_Done;
//	}
//
//	for (int i = 0; i < CTable->seats_size(); i++)
//	{
//		const PBSDRTableSeat & pbSeat = CTable->seats(i);
//		if (pbSeat.state() == EN_SEAT_STATE_NO_PLAYER)
//		{
//			continue;
//		}
//		const PBTableUser & pbUser = pbSeat.user();
//		psession->NewAddGetData(pbUser.uid(), PBUserDataField::kUserInfo);
//		psession->NewAddGetData(pbUser.uid(), PBUserDataField::kUserGameData);
//	}
//
//	return EN_Handler_Get;
//}
//
//ENHandlerResult CInnerUpdateSkipMatchResult::ProcessUpdateSucc(CHandlerTokenBasic *ptoken, CSession *psession)
//{
//	for (unsigned int i = 0; i < psession->m_iTableUserTmp.size(); i++)
//	{
//		const PBTableUser & pbUser = psession->m_iTableUserTmp.at(i);
//		psession->NewAddGetData(pbUser.uid(), PBUserDataField::kUserInfo);
//		psession->NewAddGetData(pbUser.uid(), PBUserDataField::kUserGameData);
//	}
//
//	return EN_Handler_Get;
//}
//
//ENHandlerResult CInnerUpdateSkipMatchResult::ProcessUpdateFailed(CHandlerTokenBasic *ptoken, CSession *psession)
//{
//	for (unsigned int i = 0; i < psession->m_iTableUserTmp.size(); i++)
//	{
//		const PBTableUser & pbUser = psession->m_iTableUserTmp.at(i);
//
//		const PBUserGameData & pbUserGameData = psession->_kvdb_uid_data_map[pbUser.uid()].user_game_data();
//		const PBUserSkipMatchInfo & pbUserSkipMatchInfo = pbUserGameData.user_skip_match_info();
//		int iGameType = (int)EN_Position_3Ren_DDZ_COIN;
//		const PBUserSkipMatchInfoItem * pbInfoItem = UserSkipMatchManager::GetItemFromSkipMatchInfo(iGameType, pbUserSkipMatchInfo);
//		if (!pbInfoItem)
//		{
//			continue;
//		}
//
//		//通知客户端的结果暂存
//		{
//			PBCSMsg pbNotify;
//			CSNotifySkipMatchResult & pbResult = *pbNotify.mutable_cs_notify_skip_match_result();
//			pbResult.mutable_user_game_data()->mutable_user_skip_match_info()->add_skip_match_info_item()->CopyFrom(*pbInfoItem);
//			Message::PushMsg(RouteManager::Instance()->GetRouteByRandom(), pbUser.uid(), pbNotify, EN_Node_Connect, pbUser.connect_id());
//		}
//	}
//
//	return EN_Handler_Done;
//}
//
//ENHandlerResult CInnerUpdateSkipMatchResult::ProcessGetSucc(CHandlerTokenBasic *ptoken, CSession *psession)
//{
//	return ((GetState*)(psession->m_pVoidTmp))->ProcessGet(ptoken, psession);
//}

ENHandlerResult CRequestTrusteeship::ProcessRequestMsg(CHandlerTokenBasic *ptoken, CSession *psession)
{
	const CSRequestTrusteeship & cs_request_Truesteeship = psession->_request_msg.cs_request_trusteeship();
	PBCSMsg pbMsg;
	CSResponseTrusteeship & cs_response_truesteeship = *pbMsg.mutable_cs_response_truesteeship();
	long long iUid = psession->_request_route.uid();
	int iTid = TableManager::Instance()->GetPlayerTableID(iUid);
	CPBGameTable * pTable = TableManager::Instance()->FindTable(iTid);
	//桌子不存在
	if (!pTable)
	{
		cs_response_truesteeship.set_result(EN_MESSAGE_TABLE_NOT_EXIST);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, pbMsg);
		return EN_Handler_Done;
	}
	//准备期间不允许托管
	if (pTable->state() == EN_TABLE_STATE_WAIT || pTable->state() == EN_TABLE_STATE_SINGLE_OVER)
	{
		cs_response_truesteeship.set_result(EN_MESSAGE_INVALID_TABLE_STATE);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, pbMsg);
		return EN_Handler_Done;
	}
	//座位号需要合理
	int iIndex = TableLogic::FindIndexByUid(*pTable, iUid);
	if (iIndex < 0 || iIndex > pTable->seats_size() - 1)
	{
		cs_response_truesteeship.set_result(EN_MESSAGE_INVALID_SEAT_INDEX);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, pbMsg);
		return EN_Handler_Done;
	}

	PBSDRTableSeat & pbSeat = *pTable->mutable_seats(iIndex);
	bool bIsTrusteeship = pbSeat.is_trusteeship();
	//状态是未托管状态
	if (bIsTrusteeship == false)
	{
		//是托管操作
		if (cs_request_Truesteeship.is_trusteeship())
		{
			cs_response_truesteeship.set_result(EN_MESSAGE_ERROR_OK);
			cs_response_truesteeship.set_is_trusteeship(true);
			Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, pbMsg);

			PBCSMsg pbNotify;
			CSNotifyTrusteeship & cs_notify_trusteeship = *pbNotify.mutable_cs_notify_trusteeship();
			cs_notify_trusteeship.set_seat_index(iIndex);
			cs_notify_trusteeship.set_is_trusteeship(true);
			TableLogic::BroadcastTableMsg(*pTable, pbNotify);

			pbSeat.set_is_trusteeship(true);
			pbSeat.set_trusteeship_time(time(NULL));
		}
		else
		{
			cs_response_truesteeship.set_result(EN_MESSAGE_ERROR_OK);
			cs_response_truesteeship.set_is_trusteeship(false);
		}
	}
	//状态是托管状态
	else
	{
		//是取消托管状态
		if (!cs_request_Truesteeship.is_trusteeship())
		{
			cs_response_truesteeship.set_result(EN_MESSAGE_ERROR_OK);
			cs_response_truesteeship.set_is_trusteeship(false);
			Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, pbMsg);

			PBCSMsg pbNotify;
			CSNotifyTrusteeship & cs_notify_trusteeship = *pbNotify.mutable_cs_notify_trusteeship();
			cs_notify_trusteeship.set_seat_index(iIndex);
			cs_notify_trusteeship.set_is_trusteeship(false);
			TableLogic::BroadcastTableMsg(*pTable, pbNotify);

			pbSeat.set_is_trusteeship(false);
			pbSeat.set_end_trusteeship_time(time(NULL));
			//pbSeat.set_trusteeship_time(-1);
		}
		else
		{
			cs_response_truesteeship.set_result(EN_MESSAGE_ERROR_OK);
			cs_response_truesteeship.set_is_trusteeship(true);
		}
	}

	return EN_Handler_Done;
}

ENHandlerResult CRequestSetNextRoundHandCards::ProcessRequestMsg(CHandlerTokenBasic *ptoken, CSession *psession)
{
	const CSRequestSetNextRoundHandCards & request = psession->_request_msg.cs_request_set_next_round_hand_cards();
	PBCSMsg msg;
	CSResponseSetNextRoundHandCards & response = *msg.mutable_cs_response_set_next_round_hand_cards();
	long long uid = psession->_request_route.uid();
	int64 tid = TableManager::Instance()->GetPlayerTableID(uid);
	CPBGameTable * ptable = TableManager::Instance()->FindTable(tid);

	if (ptable == NULL)
	{
		response.set_i_ret(EN_MESSAGE_TABLE_NOT_EXIST);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}
	if (ptable->state() == EN_TABLE_STATE_WAIT_DISSOLVE)
	{
		return EN_Handler_Done;
	}

	response.set_i_ret(EN_MESSAGE_ERROR_OK);
	Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);

	//牌桌定庄
	if (request.has_i_table_dealer_index() && request.i_table_dealer_index() >= 0 && request.i_table_dealer_index() <= ptable->seats_size() - 1)
	{
		ptable->set_i_test_table_dealer_index(request.i_table_dealer_index());
	}

	//设置赖子牌
	if (request.has_i_joker_card() && request.i_joker_card() != -1)
	{
		ptable->set_i_test_joker_card(request.i_joker_card());
	}
	//设置将牌
	if (request.has_i_jiang_card() && request.i_jiang_card() != -1)
	{
		ptable->set_i_test_jiang_card(request.i_jiang_card());
	}

	//设置牌堆
	if (request.i_cards_pile_cards_size() > 0)
	{
		ptable->mutable_i_test_cards_pile_cards()->CopyFrom(request.i_cards_pile_cards());
	}

	//各个座位上的手牌设置
	{
		if (ptable->seats_size() >= 1 && request.i_hand_cards_for_seat_0_size() > 0)
		{
			PBSDRTableSeat & pbSeat = *ptable->mutable_seats(0);
			pbSeat.mutable_i_test_hand_cards()->CopyFrom(request.i_hand_cards_for_seat_0());
		}

		if (ptable->seats_size() >= 2 && request.i_hand_cards_for_seat_1_size() > 0)
		{
			PBSDRTableSeat & pbSeat = *ptable->mutable_seats(1);
			pbSeat.mutable_i_test_hand_cards()->CopyFrom(request.i_hand_cards_for_seat_1());
		}

		if (ptable->seats_size() >= 3 && request.i_hand_cards_for_seat_2_size() > 0)
		{
			PBSDRTableSeat & pbSeat = *ptable->mutable_seats(2);
			pbSeat.mutable_i_test_hand_cards()->CopyFrom(request.i_hand_cards_for_seat_2());
		}

		if (ptable->seats_size() >= 4 && request.i_hand_cards_for_seat_3_size() > 0)
		{
			PBSDRTableSeat & pbSeat = *ptable->mutable_seats(3);
			pbSeat.mutable_i_test_hand_cards()->CopyFrom(request.i_hand_cards_for_seat_3());
		}
	}

	return EN_Handler_Done;
}

ENHandlerResult CRequestSetNextCard::ProcessRequestMsg(CHandlerTokenBasic *ptoken, CSession *psession)
{
	const CSRequestSetNextCard & request = psession->_request_msg.cs_request_set_next_card();
	PBCSMsg msg;
	CSResponseSetNextCard & response = *msg.mutable_cs_response_set_next_card();
	long long uid = psession->_request_route.uid();
	int64 tid = TableManager::Instance()->GetPlayerTableID(uid);
	CPBGameTable * ptable = TableManager::Instance()->FindTable(tid);
	if (ptable == NULL)
	{
		response.set_i_ret(EN_MESSAGE_TABLE_NOT_EXIST);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}
	if (ptable->state() == EN_TABLE_STATE_WAIT_DISSOLVE)
	{
		return EN_Handler_Done;
	}
	int index = TableLogic::FindIndexByUid(*ptable, uid);
	if (index < 0 || index > ptable->seats_size() - 1)
	{
		response.set_i_ret(EN_MESSAGE_INVALID_SEAT_INDEX);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}
	if (ptable->state() != EN_TABLE_STATE_PLAYING)
	{
		response.set_i_ret(EN_MESSAGE_INVALID_ACTION);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}

	response.set_i_ret(EN_MESSAGE_ERROR_OK);
	Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);

	for (int i = 0; i < request.i_next_card_size(); i++)
	{
		if (i + 1 > ptable->cards_size())
		{
			break;
		}

		int iNextCard = request.i_next_card(i);
		int iTableNextCard = ptable->cards(i);

		//寻找这张牌在牌堆的位置，方便替换
		int iRequestNextCardIndex = TableLogic::FindCardFromVect(*ptable->mutable_cards(), iNextCard);
		if (iRequestNextCardIndex != -1)
		{
			ptable->set_cards(i, iNextCard);
			ptable->set_cards(iRequestNextCardIndex, iTableNextCard);
		}
	}

	return EN_Handler_Done;
}

/*
金币场匹配
*/
ENHandlerResult CRequestCoinMatchGame::ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession)
{
	//查询玩家数据user
	int64 iUid = psession->_request_route.uid();
	psession->NewAddGetData(iUid, PBUserDataField::kUserInfo);
	return EN_Handler_Get;
}

/*
金币场匹配
*/
ENHandlerResult CRequestCoinMatchGame::ProcessGetSucc(CHandlerTokenBasic * ptoken, CSession * psession)
{
	int64 iUid = psession->_request_route.uid();
	const CSRequestCoinMatchGame & pbRequest = psession->_request_msg.cs_request_coin_match_game();
	const PBUser pbUser = psession->_kvdb_uid_data_map[iUid].user_info();
	//int64 iCoins = pbUser.coins();
	//int32 iGameType = pbRequest.ttype();
	//获得玩家数据
	//判定位置(弃用，改在room判定)
	int iLevel = -1;
	//如果是快速比赛
	if (pbRequest.has_is_quick_start() && pbRequest.is_quick_start())
	{
		int iLastIndex = -1;
		int iMinDist = -1;
		for (int i = 0; i < PokerAutoMatchRoomConfig::Instance()->items_size(); i++)
		{
			const PBAutoMatchRoomItem & pbItem = PokerAutoMatchRoomConfig::Instance()->items(i);
			if (pbItem.ttype() != pbRequest.ttype())
			{
				continue;
			}

			if (pbItem.chip_floor_limit() <= pbUser.coins())
			{
				if (iLastIndex == -1 || iMinDist >= (pbUser.coins() - pbItem.chip_floor_limit()))
				{
					iLastIndex = i;
					iMinDist = pbUser.coins() - pbItem.chip_floor_limit();
				}
			}
		}

		if (iLastIndex == -1)
		{
			//低于最小携带数
			if (PokerAutoMatchRoomConfig::Instance()->items_size() != 0)
			{
				PBCSMsg pbMsg;
				CSResponseCoinMatchGame & pbResponse = *pbMsg.mutable_cs_response_coin_match_game();
				pbResponse.set_result(EN_MESSAGE_HAS_NOT_ENOUGU_COIN);
				Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, pbMsg);
				return EN_Handler_Done;
			}

			PBCSMsg pbMsg;
			CSResponseCoinMatchGame & pbResponse = *pbMsg.mutable_cs_response_coin_match_game();
			pbResponse.set_result(EN_MESSAGE_INVALID_LEVEL);
			Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, pbMsg);
			return EN_Handler_Done;
		}

		const PBAutoMatchRoomItem & pbFinalItem = PokerAutoMatchRoomConfig::Instance()->items(iLastIndex);
		iLevel = pbFinalItem.level();
	}
	//如果是指定等级的比赛
	else if (pbRequest.has_level())
	{
		int iExpectLevel = pbRequest.has_level();
		PBAutoMatchRoomItem* pitem = PokerAutoMatchRoomConfig::Instance()->GetConf(pbRequest.ttype(), iExpectLevel);
		if (pitem)
		{
			/*> - <*/ //暂时
			//if (pbUser.coins() > pitem->chip_upper_limit())
			//{
			//	PBCSMsg pbMsg;
			//	CSResponseCoinMatchGame & pbResponse = *pbMsg.mutable_cs_response_coin_match_game();
			//	pbResponse.set_result(EN_MESSAGE_HAS_TOO_MATCH_COIN);
			//	Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, pbMsg);
			//	return EN_Handler_Done;
			//}

			//if (pbUser.coins() < pitem->chip_floor_limit())
			//{
			//	PBCSMsg pbMsg;
			//	CSResponseCoinMatchGame & pbResponse = *pbMsg.mutable_cs_response_coin_match_game();
			//	pbResponse.set_result(EN_MESSAGE_HAS_NOT_ENOUGU_COIN);
			//	Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, pbMsg);
			//	return EN_Handler_Done;
			//}
		}

		iLevel = pbRequest.level();
	}

	if (iLevel == -1)
	{
		PBCSMsg pbMsg;
		CSResponseCoinMatchGame & pbResponse = *pbMsg.mutable_cs_response_coin_match_game();
		pbResponse.set_result(EN_MESSAGE_INVALID_LEVEL);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, pbMsg);
		return EN_Handler_Done;
	}

	//寻找相应的房间号
	int iTid = -1;
	iTid = TableManager::Instance()->GetTableForUser(iLevel, pbUser.acc_type(), pbUser.nick(),pbRequest.exclude_tid(),NULL);
	if (iTid == -1)
	{
		PBCSMsg pbMsg;
		CSResponseCoinMatchGame & pbResponse = *pbMsg.mutable_cs_response_coin_match_game();
		pbResponse.set_result(EN_MESSAGE_ALLOC_TABLE_FAILED);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, pbMsg);
		return EN_Handler_Done;
	}

	//返回成功结果
	PBCSMsg pbMsg;
	CSResponseCoinMatchGame & pbResponse = *pbMsg.mutable_cs_response_coin_match_game();
	pbResponse.set_result(EN_MESSAGE_ERROR_OK);
	pbResponse.set_tid(iTid);
	pbResponse.set_coin_game_svid(TGlobal::_svid);
	pbResponse.set_pos_type(EN_Position_3Ren_DDZ_COIN);
	Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, pbMsg);

	return EN_Handler_Done;
}

/*
金币场匹配（暂存，暂时无用）
*/
ENHandlerResult CRequestCoinMatchGame::ProcessUpdateSucc(CHandlerTokenBasic *ptoken, CSession *psession)
{
	//返回结果
	return EN_Handler_Done;
}

/*
结果在数据库中更新
*/
ENHandlerResult CInnerUpdateCoinMatchResult::ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession)
{
	//状态改变 **(状态置为无数据)**
	psession->m_pVoidTmp = &m_CNoInfoGetState;

	//这里需要更新金币
	//找到桌子
	const SSInnerUpdateMatchResult & pbRequest = psession->_request_msg.ss_inner_update_match_result();
	int iTid = pbRequest.tid();
	CPBGameTable * CTable = TableManager::Instance()->FindTable(iTid);
	if (!CTable)
	{
		return EN_Handler_Done;
	}

	for (int i = 0; i < CTable->seats_size(); i++)
	{
		const PBSDRTableSeat & pbSeat = CTable->seats(i);
		if (pbSeat.state() == EN_SEAT_STATE_NO_PLAYER)
		{
			continue;
		}
		const PBTableUser & pbUser = pbSeat.user();
		psession->NewAddGetData(pbUser.uid(), PBUserDataField::kUserInfo);
	}

	return EN_Handler_Get;
}

/*
结果在数据库中更新
*/
ENHandlerResult CInnerUpdateCoinMatchResult::ProcessGetSucc(CHandlerTokenBasic * ptoken, CSession * psession)
{
	//执行合适的get
	return ((GetState*)(psession->m_pVoidTmp))->ProcessGet(ptoken, psession);
}

/*
结果在数据库中更新
*/
ENHandlerResult CInnerUpdateCoinMatchResult::ProcessUpdateSucc(CHandlerTokenBasic * ptoken, CSession * psession)
{
	//成功之后重新来取用户的金币数
	for (int i = 0 ; i < psession->m_iTableUserTmp.size(); i++)
	{
		const PBTableUser & pbUser = psession->m_iTableUserTmp.at(i);
		psession->NewAddGetData(pbUser.uid(), PBUserDataField::kUserInfo);
	}

	return EN_Handler_Get;
}

/*
结果在数据库中更新
*/
ENHandlerResult CInnerUpdateCoinMatchResult::ProcessUpdateFailed(CHandlerTokenBasic * ptoken, CSession * psession)
{
	//失败之后按照之前的数据返回给用户
	//发送结果给客户端
	for (unsigned int i = 0; i < psession->m_iTableUserTmp.size(); i++)
	{
		const PBTableUser & pbTableUser = psession->m_iTableUserTmp.at(i);
		Message::PushMsg(RouteManager::Instance()->GetRouteByRandom(), pbTableUser.uid(), psession->_notify_msg, EN_Node_Connect, pbTableUser.connect_id());
	}

	return EN_Handler_Done;
}

/*
结果在数据库中更新
*/
ENHandlerResult CInnerUpdateCoinMatchResult::GetState_NoInfo::ProcessGet(CHandlerTokenBasic * ptoken, CSession * psession)
{
	//状态改变 **(状态置为有数据)**
	psession->m_pVoidTmp = &this->m_pCInnerUpdateCoinMatchResult->m_CHadInfoGetState;

	const SSInnerUpdateMatchResult & pbRequest = psession->_request_msg.ss_inner_update_match_result();
	int iTid = pbRequest.tid();

	//找到牌桌
	CPBGameTable * CTable = TableManager::Instance()->FindTable(iTid);
	if (!CTable)
	{
		return EN_Handler_Done;
	}

	//这里需要先计算有多少的金币可以流通（因为有可能会有人没有这么多的金币）
	int iMaxCoinsChange = 0;
	int iWinNum = 0;
	for (int i = 0; i < CTable->seats_size(); i++)
	{
		PBSDRTableSeat & pbSeat = *CTable->mutable_seats(i);
		int iCoinChangeValue = pbSeat.win_info().coins_change_value(); //这里需要在游戏里设置
		if (iCoinChangeValue >= 0)
		{
			iWinNum++;
			continue;
		}

		//取非零值
		const PBTableUser & pbTableUser = pbSeat.user();
		int iUid = pbTableUser.uid();
		const PBUser & pbUser = psession->_kvdb_uid_data_map[iUid].user_info();
		if (pbUser.coins() + iCoinChangeValue < 0)
		{
			iCoinChangeValue = (-pbUser.coins());
			pbSeat.mutable_win_info()->set_coins_change_value(-pbUser.coins());
		}

		iMaxCoinsChange += iCoinChangeValue;
	}

	if (iWinNum == 0)
	{
		return EN_Handler_Done;
	}

	//获得每个单位的加分
	int iUnitAddCoins = (-iMaxCoinsChange / iWinNum);

	//正式加减金币，并且通知
	CSNotifyMatchResult & pbNotifyResult = *psession->_notify_msg.mutable_cs_notify_match_result();
	for (int i = 0 ; i < CTable->seats_size() ; i ++)
	{
		PBSDRTableSeat & pbSeat = *CTable->mutable_seats(i);
		const PBTableUser & pbTableUser = pbSeat.user();
		psession->m_iTableUserTmp.push_back(pbTableUser);
		int iCoinChangeValue = pbSeat.win_info().coins_change_value();

		//如果是加金币，需要重新设置
		if(iCoinChangeValue  > 0)
		{
			iCoinChangeValue = iUnitAddCoins;
			pbSeat.mutable_win_info()->set_coins_change_value(iCoinChangeValue);
		}

		//结果发送更新
		PBSeatMatchResult & pbResult = *pbNotifyResult.add_seat_result();
		pbResult.set_uid(pbTableUser.uid());
		pbResult.set_coins_change_value(iCoinChangeValue);
		pbResult.set_coins_after_update(iCoinChangeValue);	//先用旧值暂存
		pbResult.mutable_win_info()->CopyFrom(pbSeat.win_info());
		pbResult.set_pre_level(CTable->config().level());

		//牌局金币更新
		PBUpdateData pbUpdate;
		pbUpdate.set_key(PBUserDataField::kUserInfo);
		{
			PBDBAtomicField & pbField = *pbUpdate.add_field_list();
			pbField.set_field(EN_DB_Field_Coin);
			pbField.set_intval(iCoinChangeValue);
			pbField.set_reason(EN_Reason_Coin_Game_Over_Settle);
		}

		//牌局抽水更新
		{
			PBDBAtomicField & pbField = *pbUpdate.add_field_list();
			pbField.set_field(EN_DB_Field_Coin);
			pbField.set_intval(-CTable->fee());
			pbField.set_reason(EN_Reason_Coin_Game_Fee);
		}

		psession->NewAddUpdateData(pbTableUser.uid(), pbUpdate);
	}

	return EN_Handler_Save;
}

/*
结果在数据库中更新
*/
ENHandlerResult CInnerUpdateCoinMatchResult::GetState_HadInfo::ProcessGet(CHandlerTokenBasic * ptoken, CSession * psession)
{
	if (psession->_notify_msg.cs_notify_match_result().seat_result_size() <= 0)
	{
		return EN_Handler_Done;
	}

	int level = psession->_notify_msg.cs_notify_match_result().seat_result(0).pre_level();

	//金币的最低下限
	int iMinCoinsItemLimit = -1;
	//当前场次的配置
	const PBAutoMatchRoomItem* pitem = NULL;
	//更新完成后，需要检查玩家的金币是否能够支持该场匹配
	for (int i = 0; i < PokerAutoMatchRoomConfig::Instance()->items_size(); i++)
	{
		const PBAutoMatchRoomItem & pbItem = PokerAutoMatchRoomConfig::Instance()->items(i);
		if (pbItem.ttype() != EN_Table_3Ren_DDZ_COIN)
		{
			continue;
		}

		if (iMinCoinsItemLimit == -1 || iMinCoinsItemLimit >= pbItem.chip_floor_limit())
		{
			iMinCoinsItemLimit = pbItem.chip_floor_limit();
		}

		if (pbItem.level() != level)
		{
			continue;
		}

		pitem = &pbItem;
	}

	for (int i = 0; i < psession->_notify_msg.cs_notify_match_result().seat_result_size(); i++)
	{
		PBSeatMatchResult & pbResult = *psession->_notify_msg.mutable_cs_notify_match_result()->mutable_seat_result(i);
		int iUid = pbResult.uid();
		const PBUser pbUser = psession->_kvdb_uid_data_map[iUid].user_info();
		pbResult.set_coins_after_update(pbUser.coins());

		//破产
		if (iMinCoinsItemLimit != -1 && pbUser.coins() < iMinCoinsItemLimit)
		{
			pbResult.set_coins_state(EN_Coins_State_Broke);
		}
		//与当前等级相比太多
		if (pitem && pbUser.coins() > pitem->chip_upper_limit())
		{
			pbResult.set_coins_state(EN_Coins_State_Too_Much_For_level);
		}
		//与当前等级相比太少
		if (pitem && pbUser.coins() < pitem->chip_floor_limit())
		{
			pbResult.set_coins_state(EN_Coins_State_Too_Less_For_level);
		}
	}

	//发送结果给客户端
	for (unsigned int i = 0; i < psession->m_iTableUserTmp.size(); i++)
	{
		const PBTableUser & pbTableUser = psession->m_iTableUserTmp.at(i);
		Message::PushMsg(RouteManager::Instance()->GetRouteByRandom(), pbTableUser.uid(), psession->_notify_msg, EN_Node_Connect, pbTableUser.connect_id());
	}

	return EN_Handler_Done;
}

/*
踢出用户
*/
ENHandlerResult CInnerNotifyKickoutUser::ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession * psession)
{
	//查找桌子，重置状态
	const SSInnerNotifyKickoutUser & pbRequest = psession->_request_msg.ss_inner_notify_kickout_user();
	int iTid = pbRequest.tid();

	CPBGameTable * pTable = TableManager::Instance()->FindTable(iTid);
	if (pTable == NULL)
	{
		return EN_Handler_Done;
	}

	for (int i = 0 ; i < pbRequest.indexs_size() ; i ++)
	{
		const PBSDRTableSeat & pbSeat = pTable->seats(i);
		if (pbSeat.state() == EN_SEAT_STATE_NO_PLAYER)
		{
			continue;
		}

		PBUpdateData update;
		update.set_key(PBUserDataField::kUserInfo);
		{
			PBDBAtomicField& field = *update.add_field_list();
			field.set_field(EN_DB_Field_Coin_POS);
			PBBPlayerPositionInfo& pos = *field.mutable_pos();
			pos.set_pos_type(EN_Position_Hall);
			pos.set_table_id(0);
			pos.set_gamesvrd_id(0);
		}
		psession->NewAddUpdateData(pbSeat.user().uid(), update);
	}
	return EN_Handler_Save;
}

/*
踢出用户
*/
ENHandlerResult CInnerNotifyKickoutUser::ProcessUpdateSucc(CHandlerTokenBasic * ptoken, CSession * psession)
{
	const SSInnerNotifyKickoutUser & pbRequest = psession->_request_msg.ss_inner_notify_kickout_user();
	int iTid = pbRequest.tid();

	CPBGameTable * pTable = TableManager::Instance()->FindTable(iTid);
	if (pTable == NULL)
	{
		return EN_Handler_Done;
	}


	for (int i = 0; i < pbRequest.indexs_size(); i++)
	{
		const PBSDRTableSeat & pbSeat = pTable->seats(i);
		if (pbSeat.state() == EN_SEAT_STATE_NO_PLAYER)
		{
			continue;
		}

		if (!pbSeat.user().is_offline())
		{
			PBCSMsg msg;
			CSResponseLogoutTable & response = *msg.mutable_cs_response_logout_table();
			response.set_result(EN_MESSAGE_ERROR_OK);
			response.set_reason(EN_Logout_Reason_Normal);
			Message::PushMsg(RouteManager::Instance()->GetRouteByRandom(), pbSeat.user().uid(), msg, EN_Node_Connect, pbSeat.user().connect_id());
		}

		TableLogic::LogoutTable(*pTable, pbRequest.indexs(i));
	}

	return EN_Handler_Done;
}

/*
踢出用户
*/
ENHandlerResult CInnerNotifyKickoutUser::ProcessUpdateFailed(CHandlerTokenBasic * ptoken, CSession * psession)
{
	const SSInnerNotifyKickoutUser & pbRequest = psession->_request_msg.ss_inner_notify_kickout_user();
	int iTid = pbRequest.tid();

	CPBGameTable * pTable = TableManager::Instance()->FindTable(iTid);
	if (pTable == NULL)
	{
		return EN_Handler_Done;
	}


	for (int i = 0; i < pbRequest.indexs_size(); i++)
	{
		const PBSDRTableSeat & pbSeat = pTable->seats(i);
		if (pbSeat.state() == EN_SEAT_STATE_NO_PLAYER)
		{
			continue;
		}

		if (!pbSeat.user().is_offline())
		{
			PBCSMsg msg;
			CSResponseLogoutTable & response = *msg.mutable_cs_response_logout_table();
			response.set_result(EN_MESSAGE_ERROR_OK);
			response.set_reason(EN_Logout_Reason_Normal);
			Message::PushMsg(RouteManager::Instance()->GetRouteByRandom(), pbSeat.user().uid(), msg, EN_Node_Connect, pbSeat.user().connect_id());
		}

		TableLogic::LogoutTable(*pTable, pbRequest.indexs(i));
	}

	return EN_Handler_Done;
}