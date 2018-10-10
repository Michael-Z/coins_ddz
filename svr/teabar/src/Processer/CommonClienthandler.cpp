#include "CommonClienthandler.h"
#include "RouteManager.h"
#include "TeaBarManager.h"
#include "TeaBarHandlerToken.h"
#include "PBConfigBasic.h"

ENProcessResult CRegistInnerServer::ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession)
{
	//
	const SSNotifyInnerServer ss_notify_inner_server = psession->_request_msg.ss_notify_inner_server();
	int routeid = ss_notify_inner_server.routeid();
	RouteManager::Instance()->_route_map[routeid] = ptoken->_phandler;
	VLogMsg(CLIB_LOG_LEV_ERROR,"receive noitfy from route[%d]",routeid);
	return EN_Process_Result_Completed;
}

ENHandlerResult CRequestCreateTeaBar::ProcessRequestMsg(CHandlerTokenBasic * ptoken, CSession* psession)
{
	psession->NewAddGetData(psession->_request_route.uid(), PBUserDataField::kUserInfo);
	psession->NewAddGetData(psession->_request_route.uid(), PBUserDataField::kUserTeaBarData);
	return EN_Handler_Get;
}

ENHandlerResult CRequestCreateTeaBar::ProcessGetSucc(CHandlerTokenBasic* ptoken, CSession* psession)
{
	long long uid = psession->_request_route.uid();
	const PBUser & user = psession->_kvdb_uid_data_map[uid].user_info();
	PBCSMsg msg;
	CSResponseCreateTeaBar& reponse = *msg.mutable_cs_response_create_tea_bar();
	if (user.roletype() != EN_Role_Type_Saler)
	{
		reponse.set_result(EN_MESSAGE_NOT_SALER_CAN_NOT_CREATE_TEA_BAR);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}

	const CSRequestCreateTeaBar& cs_request_create_tea_bar = psession->_request_msg.cs_request_create_tea_bar();
	if (cs_request_create_tea_bar.name() == "")
	{
		reponse.set_result(EN_MESSAGE_TEA_BAR_NAME_EMPTY);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}

	if (cs_request_create_tea_bar.desc() == "")
	{
		reponse.set_result(EN_MESSAGE_TEA_BAR_DESC_EMPTY);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}

	const PBUserTeaBarData & tbdata = psession->_kvdb_uid_data_map[uid].user_tea_bar_data();
	if (tbdata.brief_data_size() >= MAX_USER_TEA_BAR_MUM)
	{
		reponse.set_result(EN_MESSAGE_TEA_BAR_COUNT_LIMIT);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}
	
	
	PBTeaBarData* pdata = TeaBarManager::Instance()->CreateTeaBar(user, cs_request_create_tea_bar.name(), cs_request_create_tea_bar.desc());
	if (pdata == NULL)
	{
		reponse.set_result(EN_MESSAGE_SYSTEM_FAILED);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}

	if (cs_request_create_tea_bar.has_pay_type())
	{
		pdata->set_pay_type(cs_request_create_tea_bar.pay_type());
	}
	else
	{
		pdata->set_pay_type(EN_TeaBar_Pay_Type_Master);
	}

	psession->_tbid = pdata->tbid();
	//更新用户信息
	{
		TeaBarBriefData brief;
		brief.set_tbid(pdata->tbid());
		brief.set_tbname(pdata->tbname());
		brief.set_master_uid(pdata->master_uid());
		brief.set_chips(0);
		brief.set_wait_table_num(0);
		brief.set_pay_type(pdata->pay_type());
		
		PBUpdateData update;
		update.set_key(PBUserDataField::kUserTeaBarData);
		PBDBAtomicField & field = *update.add_field_list();
		field.set_field(EN_DB_Field_TeaBar_Brief);
		field.set_strategy(EN_Update_Strategy_Add);
		field.mutable_tea_bar_brief()->CopyFrom(brief);
		psession->NewAddUpdateData(uid, update);
	}
	return EN_Handler_Save;
}

ENHandlerResult CRequestCreateTeaBar::ProcessUpdateSucc(CHandlerTokenBasic* ptoken, CSession* psession)
{
	PBCSMsg msg;
	CSResponseCreateTeaBar & cs_response_create_tea_bar = *msg.mutable_cs_response_create_tea_bar();
	cs_response_create_tea_bar.set_result(EN_MESSAGE_ERROR_OK);
	cs_response_create_tea_bar.set_create_tbid(psession->_tbid);
	Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
	return EN_Handler_Done;
}

ENHandlerResult CRequestEnterTeaBar::ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession)
{
	psession->NewAddGetData(psession->_request_route.uid(), PBUserDataField::kUserTeaBarData);
	return EN_Handler_Get;
}


ENHandlerResult CRequestEnterTeaBar::ProcessGetSucc(CHandlerTokenBasic* ptoken, CSession* psession)
{
	long long uid = psession->_request_route.uid();
	const PBUserTeaBarData & tbdata = psession->_kvdb_uid_data_map[uid].user_tea_bar_data();
	PBCSMsg msg;
	CSResponseEnterTeaBar & cs_response_enter_tea_bar = *msg.mutable_cs_response_enter_tea_bar();
	const CSRequestEnterTeaBar& cs_request_enter_tea_bar = psession->_request_msg.cs_request_enter_tea_bar();
	const TeaBarBriefData* pbrief = NULL;
	for (int i = 0; i < tbdata.brief_data_size(); i++)
	{
		const TeaBarBriefData& brief = tbdata.brief_data(i);
		if (brief.tbid() == cs_request_enter_tea_bar.tbid())
		{
			pbrief = &brief;
			break;
		}
	}

	if (!pbrief)
	{
		cs_response_enter_tea_bar.set_result(EN_MESSAGE_NOT_IN_TEA_BAR);
		cs_response_enter_tea_bar.set_is_refresh(cs_request_enter_tea_bar.is_refresh());
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}

	PBTeaBarData* pdata = TeaBarManager::Instance()->GetTeaBar(cs_request_enter_tea_bar.tbid());
	if (pdata == NULL)
	{
		ErrMsg("Tea bar not find,%ld", cs_request_enter_tea_bar.tbid());
		cs_response_enter_tea_bar.set_result(EN_MESSAGE_NOT_IN_TEA_BAR);
		cs_response_enter_tea_bar.set_is_refresh(cs_request_enter_tea_bar.is_refresh());
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}

	cs_response_enter_tea_bar.set_result(EN_MESSAGE_ERROR_OK);
	cs_response_enter_tea_bar.set_tbid(pdata->tbid());
	cs_response_enter_tea_bar.set_tbname(pdata->tbname());
	cs_response_enter_tea_bar.set_master_uid(pdata->master_uid());
	cs_response_enter_tea_bar.set_master_name(pdata->master_name());
	cs_response_enter_tea_bar.set_master_url(pdata->master_url());
	cs_response_enter_tea_bar.set_chips(pbrief->chips());
	cs_response_enter_tea_bar.set_user_num(pdata->users_size());
	cs_response_enter_tea_bar.set_max_user_num(MAX_TEA_BAR_USER_NUM);
	cs_response_enter_tea_bar.set_desc(pdata->desc());
	cs_response_enter_tea_bar.set_pay_type(pdata->pay_type());

    for (int i = 0; i < pdata->teabar_room_create_info_size(); i++)
    {
        TeabarRoomCreateInfo& create_info = *cs_response_enter_tea_bar.add_teabar_create_info();
        create_info.CopyFrom(pdata->teabar_room_create_info(i));
    }

	//cs_response_enter_tea_bar.mutable_tables()->CopyFrom(pdata->tables());
	vector<PBTeaBarTable> vec;
	TeaBarManager::Instance()->GetSortTables(*pdata, vec);
	for (int i = 0; i < (int)vec.size(); i++)
	{
		PBTeaBarTable& tbtable = *cs_response_enter_tea_bar.add_tables();
		tbtable.CopyFrom(vec[i]);
	}
	//测试代码
	/*const PBTeaBarTable* ptbtale = NULL;
	if (pdata->tables_size() > 0)
	{
	ptbtale = &(pdata->tables(0));
	}

	if (ptbtale != NULL)
	{
	for (int i = 0; i < 100; i++)
	{
	PBTeaBarTable& tbtable = *cs_response_enter_tea_bar.add_tables();
	tbtable.CopyFrom(*ptbtale);
	}
	}*/
	
	//测试代码--end
	cs_response_enter_tea_bar.set_is_refresh(cs_request_enter_tea_bar.is_refresh());
	Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);

	//如果是群主，推送有新消息
	if (pdata->master_uid() == uid)
	{
		if (pdata->apply_join_msg_list_size() > 0 || pdata->apply_drop_msg_list_size() > 0)
		{
			PBCSMsg notify;
			CSNotifyTeaBarHasNewMessage& cs_notify_tea_bar_has_new_message = *notify.mutable_cs_notify_tea_bar_has_new_message();
			cs_notify_tea_bar_has_new_message.set_tbid(pdata->tbid());
			Message::PushInnerMsg(RouteManager::Instance()->GetRouteByRandom(), uid, notify, EN_Node_Connect, psession->_request_route.source_id());
		}
	}
	return EN_Handler_Done;
}

ENHandlerResult CRequestApplyJoinTeaBar::ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession)
{
	const CSRequestApplyJoinTeaBar& cs_request_apply_join_tea_bar = psession->_request_msg.cs_request_apply_join_tea_bar();
	PBTeaBarData* pdata = TeaBarManager::Instance()->GetTeaBar(cs_request_apply_join_tea_bar.tbid());
	if (pdata == NULL)
	{
		ErrMsg("Tea bar not find,%ld", cs_request_apply_join_tea_bar.tbid());
		PBCSMsg msg;
		CSResponseApplyJoinTeaBar& reponse = *msg.mutable_cs_response_apply_join_tea_bar();
		reponse.set_result(EN_MESSAGE_TEA_BAR_NOT_EXIST);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}
	psession->NewAddGetData(psession->_request_route.uid(), PBUserDataField::kUserInfo);
	psession->NewAddGetData(psession->_request_route.uid(), PBUserDataField::kUserTeaBarData);
	psession->NewAddGetData(pdata->master_uid(), PBUserDataField::kUserInfo);
	return EN_Handler_Get;
}

ENHandlerResult CRequestApplyJoinTeaBar::ProcessGetSucc(CHandlerTokenBasic* ptoken, CSession* psession)
{
	long long uid = psession->_request_route.uid();
	const PBUser & user = psession->_kvdb_uid_data_map[uid].user_info();
	const PBUserTeaBarData & tbdata = psession->_kvdb_uid_data_map[uid].user_tea_bar_data();
	PBCSMsg msg;
	CSResponseApplyJoinTeaBar& reponse = *msg.mutable_cs_response_apply_join_tea_bar();

	if (tbdata.brief_data_size() >= MAX_USER_TEA_BAR_MUM)
	{
		reponse.set_result(EN_MESSAGE_TEA_BAR_COUNT_LIMIT);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}

	const CSRequestApplyJoinTeaBar& cs_request_apply_join_tea_bar = psession->_request_msg.cs_request_apply_join_tea_bar();

	PBTeaBarData* pdata = TeaBarManager::Instance()->GetTeaBar(cs_request_apply_join_tea_bar.tbid());
	if (pdata == NULL)
	{
		ErrMsg("Tea bar not find,%ld", cs_request_apply_join_tea_bar.tbid());
		reponse.set_result(EN_MESSAGE_TEA_BAR_NOT_EXIST);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}

	if (pdata->users_size() >= MAX_TEA_BAR_USER_NUM)
	{
		reponse.set_result(EN_MESSAGE_TEA_BAR_USER_COUNT_LIMIT);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}

	if (pdata->apply_join_msg_list_size() >= MAX_TEA_BAR_APPLY_USER_NUM)
	{
		reponse.set_result(EN_MESSAGE_TEA_BAR_CAN_NOT_APPLY);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}

	bool is_apply = false;
	for (int i = 0; i < pdata->apply_join_msg_list_size(); i++)
	{
		const PBTeaBarUser& tbuser = pdata->apply_join_msg_list(i).user();
		if (tbuser.uid() == uid)
		{
			is_apply = true;
			break;
		}
	}
	if (is_apply)
	{
		reponse.set_result(EN_MESSAGE_TEA_BAR_IS_APPLY);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}

	bool is_in_tea_bar = false;
	for (int i = 0; i < pdata->users_size(); i++)
	{
		const PBTeaBarUser& tbuser = pdata->users(i);
		if (tbuser.uid() == uid)
		{
			is_in_tea_bar = true;
			break;
		}
	}

	if (is_in_tea_bar)
	{
		reponse.set_result(EN_MESSAGE_IS_IN_TEA_BAR);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}

	//将申请信息加入到茶馆申请加入列表中
	PBTeaBarApplyMsg& apply = *pdata->add_apply_join_msg_list();
	PBTeaBarUser& applyuser = *apply.mutable_user();
	applyuser.set_uid(uid);
	applyuser.set_name(user.nick());
	applyuser.set_url(user.pic_url());
	applyuser.set_apply_time((int)time(NULL));

	reponse.set_result(EN_MESSAGE_ERROR_OK);
	reponse.set_tbid(cs_request_apply_join_tea_bar.tbid());
	Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);

	//给群主推一个消息
	const PBUser & master_user = psession->_kvdb_uid_data_map[pdata->master_uid()].user_info();
	PBCSMsg notify;
	CSNotifyTeaBarHasNewMessage& cs_notify_tea_bar_has_new_message = *notify.mutable_cs_notify_tea_bar_has_new_message();
	cs_notify_tea_bar_has_new_message.set_tbid(pdata->tbid());
	Message::PushInnerMsg(RouteManager::Instance()->GetRouteByRandom(), pdata->master_uid(), notify, EN_Node_Connect, master_user.hallsvid());
	return EN_Handler_Done;
}

ENHandlerResult CRequestAgreeUserJoinTeaBar::ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession)
{
	long long uid = psession->_request_route.uid();
	PBCSMsg msg;
	CSResponseAgreeUserJoinTeaBar& reponse = *msg.mutable_cs_response_agree_user_join_tea_bar();
	const CSRequestAgreeUserJoinTeaBar& cs_request_agree_user_join_tea_bar = psession->_request_msg.cs_request_agree_user_join_tea_bar();
	PBTeaBarData* pdata = TeaBarManager::Instance()->GetTeaBar(cs_request_agree_user_join_tea_bar.tbid());
	if (pdata == NULL)
	{
		reponse.set_result(EN_MESSAGE_TEA_BAR_NOT_EXIST);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}

	if (pdata->master_uid() != uid)
	{
		reponse.set_result(EN_MESSAGE_NOT_HAS_RIGHT);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}

	if (pdata->users_size() >= MAX_TEA_BAR_USER_NUM)
	{
		reponse.set_result(EN_MESSAGE_TEA_BAR_USER_COUNT_LIMIT);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}

	bool is_in_apply_list = false;
	for (int i = 0; i < pdata->apply_join_msg_list_size(); i++)
	{
		const PBTeaBarUser& tbuser = pdata->apply_join_msg_list(i).user();
		if (tbuser.uid() == cs_request_agree_user_join_tea_bar.uid())
		{
			is_in_apply_list = true;
			break;
		}
	}

	if (!is_in_apply_list)
	{
		reponse.set_result(EN_MESSAGE_TEA_BAR_APPLY_NOT_EXSIT);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}

	psession->NewAddGetData(cs_request_agree_user_join_tea_bar.uid(), PBUserDataField::kUserInfo);
	psession->NewAddGetData(cs_request_agree_user_join_tea_bar.uid(), PBUserDataField::kUserTeaBarData);
	return EN_Handler_Get;
}

ENHandlerResult CRequestAgreeUserJoinTeaBar::ProcessGetSucc(CHandlerTokenBasic* ptoken, CSession* psession)
{
	const CSRequestAgreeUserJoinTeaBar& cs_request_agree_user_join_tea_bar = psession->_request_msg.cs_request_agree_user_join_tea_bar();
	const PBUser & user = psession->_kvdb_uid_data_map[cs_request_agree_user_join_tea_bar.uid()].user_info();
	const PBUserTeaBarData& tbdata = psession->_kvdb_uid_data_map[cs_request_agree_user_join_tea_bar.uid()].user_tea_bar_data();

	bool is_in_tea_bar = false;
	for (int i = 0; i < tbdata.brief_data_size(); i++)
	{
		const TeaBarBriefData& brief = tbdata.brief_data(i);
		if (brief.tbid() == cs_request_agree_user_join_tea_bar.tbid())
		{
			is_in_tea_bar = true;
			break;
		}
	}

	PBCSMsg msg;
	CSResponseAgreeUserJoinTeaBar& reponse = *msg.mutable_cs_response_agree_user_join_tea_bar();
	
	PBTeaBarData* pdata = TeaBarManager::Instance()->GetTeaBar(cs_request_agree_user_join_tea_bar.tbid());
	if (pdata == NULL)
	{
		reponse.set_result(EN_MESSAGE_TEA_BAR_NOT_EXIST);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}

	if (tbdata.brief_data_size() >= MAX_USER_TEA_BAR_MUM)
	{
		//删除申请信息
		int apply_list_index = -1;
		for (int i = 0; i < pdata->apply_join_msg_list_size(); i++)
		{
			const PBTeaBarUser& tbuser = pdata->apply_join_msg_list(i).user();
			if (tbuser.uid() == cs_request_agree_user_join_tea_bar.uid())
			{
				apply_list_index = i;
				break;
			}
		}

		if (apply_list_index != -1)
		{
			//从申请列表中删除
			pdata->mutable_apply_join_msg_list()->DeleteSubrange(apply_list_index, 1);
		}

		reponse.set_result(EN_MESSAGE_TEA_BAR_COUNT_LIMIT);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}
	
	int apply_list_index = -1;
	for (int i = 0; i < pdata->apply_join_msg_list_size(); i++)
	{
		const PBTeaBarUser& tbuser = pdata->apply_join_msg_list(i).user();
		if (tbuser.uid() == cs_request_agree_user_join_tea_bar.uid())
		{
			apply_list_index = i;
			break;
		}
	}

	if (apply_list_index == -1)
	{
		reponse.set_result(EN_MESSAGE_TEA_BAR_APPLY_NOT_EXSIT);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}


	if (!cs_request_agree_user_join_tea_bar.if_agree())
	{
		reponse.set_result(EN_MESSAGE_ERROR_OK);
		reponse.set_tbid(cs_request_agree_user_join_tea_bar.tbid());
		reponse.set_uid(cs_request_agree_user_join_tea_bar.uid());
		reponse.set_if_agree(cs_request_agree_user_join_tea_bar.if_agree());
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);

		//从申请列表中删除
		pdata->mutable_apply_join_msg_list()->DeleteSubrange(apply_list_index, 1);

		//通知申请的玩家结果
		PBCSMsg notify;
		CSNotifyJoinTeaBarResult& cs_notify_join_tea_bar_result = *notify.mutable_cs_notify_join_tea_bar_result();
		cs_notify_join_tea_bar_result.set_if_agree(false);
		cs_notify_join_tea_bar_result.set_tbid(pdata->tbid());
		cs_notify_join_tea_bar_result.set_tbname(pdata->tbname());
		cs_notify_join_tea_bar_result.set_master_uid(pdata->master_uid());
		cs_notify_join_tea_bar_result.set_master_name(pdata->master_name());
		Message::PushInnerMsg(RouteManager::Instance()->GetRouteByRandom(), user.uid(), notify, EN_Node_Connect, user.hallsvid());
		return EN_Handler_Done;
	}

	if (pdata->users_size() >= MAX_TEA_BAR_USER_NUM)
	{
		reponse.set_result(EN_MESSAGE_TEA_BAR_USER_COUNT_LIMIT);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}

	//已经在茶馆了
	if (is_in_tea_bar)
	{
		//从申请列表中删除
		pdata->mutable_apply_join_msg_list()->DeleteSubrange(apply_list_index, 1);

		reponse.set_result(EN_MESSAGE_IS_IN_TEA_BAR);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}

	//添加用户
	if (!TeaBarManager::Instance()->AddUser(user, *pdata))
	{
		reponse.set_result(EN_MESSAGE_SYSTEM_FAILED);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}

	//从申请列表中删除
	pdata->mutable_apply_join_msg_list()->DeleteSubrange(apply_list_index, 1);

	//通知申请的玩家结果
	PBCSMsg notify;
	CSNotifyJoinTeaBarResult& cs_notify_join_tea_bar_result = *notify.mutable_cs_notify_join_tea_bar_result();
	cs_notify_join_tea_bar_result.set_if_agree(true);
	cs_notify_join_tea_bar_result.set_tbid(pdata->tbid());
	cs_notify_join_tea_bar_result.set_tbname(pdata->tbname());
	cs_notify_join_tea_bar_result.set_master_uid(pdata->master_uid());
	cs_notify_join_tea_bar_result.set_master_name(pdata->master_name());
	Message::PushInnerMsg(RouteManager::Instance()->GetRouteByRandom(), user.uid(), notify, EN_Node_Connect, user.hallsvid());
	
	//更新用户信息
	TeaBarBriefData brief;
	brief.set_tbid(pdata->tbid());
	brief.set_tbname(pdata->tbname());
	brief.set_master_uid(pdata->master_uid());

	PBUpdateData update;
	update.set_key(PBUserDataField::kUserTeaBarData);
	PBDBAtomicField & field = *update.add_field_list();
	field.set_field(EN_DB_Field_TeaBar_Brief);
	field.set_strategy(EN_Update_Strategy_Add);
	field.mutable_tea_bar_brief()->CopyFrom(brief);
	psession->NewAddUpdateData(cs_request_agree_user_join_tea_bar.uid(), update);
	return EN_Handler_Save;
}

ENHandlerResult CRequestAgreeUserJoinTeaBar::ProcessUpdateSucc(CHandlerTokenBasic* ptoken, CSession* psession)
{
	const CSRequestAgreeUserJoinTeaBar& cs_request_agree_user_join_tea_bar = psession->_request_msg.cs_request_agree_user_join_tea_bar();
	PBCSMsg msg;
	CSResponseAgreeUserJoinTeaBar& reponse = *msg.mutable_cs_response_agree_user_join_tea_bar();
	reponse.set_result(EN_MESSAGE_ERROR_OK);
	reponse.set_tbid(cs_request_agree_user_join_tea_bar.tbid());
	reponse.set_uid(cs_request_agree_user_join_tea_bar.uid());
	reponse.set_if_agree(cs_request_agree_user_join_tea_bar.if_agree());
	Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
	return EN_Handler_Done;
}

ENHandlerResult CRequestApplyDropTeaBar::ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession)
{
	/*const CSRequestApplyDropTeaBar& cs_request_apply_drop_tea_bar = psession->_request_msg.cs_request_apply_drop_tea_bar();
	PBTeaBarData* pdata = TeaBarManager::Instance()->GetTeaBar(cs_request_apply_drop_tea_bar.tbid());
	if (pdata == NULL)
	{
		PBCSMsg msg;
		CSResponseApplyDropTeaBar& reponse = *msg.mutable_cs_response_apply_drop_tea_bar();
		reponse.set_result(EN_MESSAGE_TEA_BAR_NOT_EXIST);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}
	psession->NewAddGetData(psession->_request_route.uid(), PBUserDataField::kUserInfo);*/


    //需要请求玩家的数据，获得玩家是否有所在的游戏房间
    psession->NewAddGetData(psession->_request_route.uid(), PBUserDataField::kUserInfo);
	psession->NewAddGetData(psession->_request_route.uid(), PBUserDataField::kUserTeaBarData);
	return EN_Handler_Get;
}

ENHandlerResult CRequestApplyDropTeaBar::ProcessGetSucc(CHandlerTokenBasic* ptoken, CSession* psession)
{	
	long long uid = psession->_request_route.uid();

    const PBUser & user = psession->_kvdb_uid_data_map[uid].user_info();
    const PBBPlayerPositionInfo & pos = user.pos();

    /*
     * 正常情况下：玩家创建进入\进入房间，有房间信息，需要向游戏服申请table信息
     * 非正常情况：玩家创建未进入房间，游戏服没有该table的信息，也未有影响(反正也没人报卡房间)
    */
    if(pos.pos_type() != EN_Position_Hall)  //玩家在游戏服
    {
        int iPosType = pos.pos_type();
        int iGameSvrdId = pos.gamesvrd_id();
        int iTableId = pos.table_id();

        //向room服请求，因为只有room知道node的对应
        PBCSMsg pbRequestMsg;
        SSRequestTableInfo & ss_request_table_info = *pbRequestMsg.mutable_ss_request_table_info();
        ss_request_table_info.set_i64_tid(iTableId);
        ss_request_table_info.set_i_pos_type(iPosType);
        ss_request_table_info.set_i_game_svid_id(iGameSvrdId);

        Message::SendRequestMsg(RouteManager::Instance()->GetRouteByRandom(),psession,pbRequestMsg,EN_Node_Room,1);
        return EN_Handler_Succ;
    }

    //如果玩家这时候没有在游戏服，则按照正常的逻辑
	const PBUserTeaBarData & tbdata = psession->_kvdb_uid_data_map[uid].user_tea_bar_data();
	const CSRequestApplyDropTeaBar& cs_request_apply_drop_tea_bar = psession->_request_msg.cs_request_apply_drop_tea_bar();
	bool is_in_tea_bar = false;
	for (int i = 0; i < tbdata.brief_data_size(); i++)
	{
		const TeaBarBriefData& brief = tbdata.brief_data(i);
		if (brief.tbid() == cs_request_apply_drop_tea_bar.tbid())
		{
			is_in_tea_bar = true;
			break;
		}
	}
	
	PBCSMsg msg;
	CSResponseApplyDropTeaBar& reponse = *msg.mutable_cs_response_apply_drop_tea_bar();
	if (!is_in_tea_bar)
	{
		reponse.set_result(EN_MESSAGE_NOT_IN_TEA_BAR);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}
	PBTeaBarData* pdata = TeaBarManager::Instance()->GetTeaBar(cs_request_apply_drop_tea_bar.tbid());
	if (pdata == NULL)
	{
		reponse.set_result(EN_MESSAGE_TEA_BAR_NOT_EXIST);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}

	if (pdata->master_uid() == uid)
	{
		reponse.set_result(EN_MESSAGE_OWNER_CAN_NOT_DROP_TEA_BAR);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}

	bool is_apply = false;
	for (int i = 0; i < pdata->apply_drop_msg_list_size(); i++)
	{
		const PBTeaBarUser& applyuser = pdata->apply_drop_msg_list(i).user();
		if (applyuser.uid() == uid)
		{
			is_apply = true;
			break;
		}
	}

	//已经申请过离开
	//if (is_apply)
	//{
	//	reponse.set_result(EN_MESSAGE_TEA_BAR_IS_APPLY);
	//	Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
	//	return EN_Handler_Done;
	//}

	////将申请信息加入到茶馆申请退出列表中
	//PBTeaBarApplyMsg& apply = *pdata->add_apply_drop_msg_list();
	//PBTeaBarUser& applyuser = *apply.mutable_user();
	//applyuser.set_uid(uid);
	//applyuser.set_name(user.nick());
	//applyuser.set_url(user.pic_url());
	//applyuser.set_apply_time((int)time(NULL));

	//reponse.set_result(EN_MESSAGE_ERROR_OK);
	//reponse.set_tbid(cs_request_apply_drop_tea_bar.tbid());
	//Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
	//return EN_Handler_Done;
	//更新用户信息
	TeaBarBriefData brief;
	brief.set_tbid(pdata->tbid());

	PBUpdateData update;
	update.set_key(PBUserDataField::kUserTeaBarData);
	PBDBAtomicField & field = *update.add_field_list();
	field.set_field(EN_DB_Field_TeaBar_Brief);
	field.set_strategy(EN_Update_Strategy_Del);
	field.mutable_tea_bar_brief()->CopyFrom(brief);
	psession->NewAddUpdateData(uid, update);
	return EN_Handler_Save;
}

ENHandlerResult CRequestApplyDropTeaBar::ProcessResponseMsg(CHandlerTokenBasic *ptoken, CSession *psession)
{
    long long uid = psession->_request_route.uid();
    const PBUserTeaBarData & tbdata = psession->_kvdb_uid_data_map[uid].user_tea_bar_data();
    const CSRequestApplyDropTeaBar& cs_request_apply_drop_tea_bar = psession->_request_msg.cs_request_apply_drop_tea_bar();
    const SSResponseTableInfo & ss_reponse_table_info = psession->_response_msg.ss_response_table_info();

    PBCSMsg msg;
    CSResponseApplyDropTeaBar& reponse = *msg.mutable_cs_response_apply_drop_tea_bar();

    if(ss_reponse_table_info.i_ret() == EN_MESSAGE_ERROR_OK)
    {
        //如果他在还在这个茶馆的房间中
        PBSDRTableConfig pbTableConf = ss_reponse_table_info.pb_config();
        if(pbTableConf.has_tbid() && pbTableConf.tbid() == cs_request_apply_drop_tea_bar.tbid())
        {
            reponse.set_result(EN_MESSAGE_TEA_BAR_IN_TEABAR_TABLE_IN_DROPING);
            Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
            return EN_Handler_Done;
        }
    }

    bool is_in_tea_bar = false;
    for (int i = 0; i < tbdata.brief_data_size(); i++)
    {
        const TeaBarBriefData& brief = tbdata.brief_data(i);
        if (brief.tbid() == cs_request_apply_drop_tea_bar.tbid())
        {
            is_in_tea_bar = true;
            break;
        }
    }

    if (!is_in_tea_bar)
    {
        reponse.set_result(EN_MESSAGE_NOT_IN_TEA_BAR);
        Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
        return EN_Handler_Done;
    }
    PBTeaBarData* pdata = TeaBarManager::Instance()->GetTeaBar(cs_request_apply_drop_tea_bar.tbid());
    if (pdata == NULL)
    {
        reponse.set_result(EN_MESSAGE_TEA_BAR_NOT_EXIST);
        Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
        return EN_Handler_Done;
    }

    if (pdata->master_uid() == uid)
    {
        reponse.set_result(EN_MESSAGE_OWNER_CAN_NOT_DROP_TEA_BAR);
        Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
        return EN_Handler_Done;
    }

    bool is_apply = false;
    for (int i = 0; i < pdata->apply_drop_msg_list_size(); i++)
    {
        const PBTeaBarUser& applyuser = pdata->apply_drop_msg_list(i).user();
        if (applyuser.uid() == uid)
        {
            is_apply = true;
            break;
        }
    }

    //已经申请过离开
    //if (is_apply)
    //{
    //	reponse.set_result(EN_MESSAGE_TEA_BAR_IS_APPLY);
    //	Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
    //	return EN_Handler_Done;
    //}

    ////将申请信息加入到茶馆申请退出列表中
    //PBTeaBarApplyMsg& apply = *pdata->add_apply_drop_msg_list();
    //PBTeaBarUser& applyuser = *apply.mutable_user();
    //applyuser.set_uid(uid);
    //applyuser.set_name(user.nick());
    //applyuser.set_url(user.pic_url());
    //applyuser.set_apply_time((int)time(NULL));

    //reponse.set_result(EN_MESSAGE_ERROR_OK);
    //reponse.set_tbid(cs_request_apply_drop_tea_bar.tbid());
    //Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
    //return EN_Handler_Done;
    //更新用户信息
    TeaBarBriefData brief;
    brief.set_tbid(pdata->tbid());

    PBUpdateData update;
    update.set_key(PBUserDataField::kUserTeaBarData);
    PBDBAtomicField & field = *update.add_field_list();
    field.set_field(EN_DB_Field_TeaBar_Brief);
    field.set_strategy(EN_Update_Strategy_Del);
    field.mutable_tea_bar_brief()->CopyFrom(brief);
    psession->NewAddUpdateData(uid, update);
    return EN_Handler_Save;
}

ENHandlerResult CRequestApplyDropTeaBar::ProcessUpdateSucc(CHandlerTokenBasic* ptoken, CSession* psession)
{
	long long uid = psession->_request_route.uid();
	const CSRequestApplyDropTeaBar& cs_request_apply_drop_tea_bar = psession->_request_msg.cs_request_apply_drop_tea_bar();
	PBCSMsg msg;
	CSResponseApplyDropTeaBar& reponse = *msg.mutable_cs_response_apply_drop_tea_bar();
	PBTeaBarData* pdata = TeaBarManager::Instance()->GetTeaBar(cs_request_apply_drop_tea_bar.tbid());
	if (pdata == NULL)
	{
		reponse.set_result(EN_MESSAGE_TEA_BAR_NOT_EXIST);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}

	//PBTeaBarUser* pUser = TeaBarManager::Instance()->GetUser(uid, *pdata);
	//if (pUser != NULL)
	//{
	//	//将申请信息加入到茶馆申请退出列表中
	//	PBTeaBarApplyMsg& apply = *pdata->add_apply_drop_msg_list();
	//	PBTeaBarUser& applyuser = *apply.mutable_user();
	//	applyuser.set_uid(uid);
	//	applyuser.set_name(pUser->name());
	//	applyuser.set_url(pUser->url());
	//	applyuser.set_apply_time((int)time(NULL));
	//}

	if (!TeaBarManager::Instance()->DeleteUser(uid, *pdata))
	{
		ErrMsg("delete user error,tbid[%ld],uid:[%lld]", pdata->tbid(), uid);
	}

	reponse.set_result(EN_MESSAGE_ERROR_OK);
	reponse.set_tbid(cs_request_apply_drop_tea_bar.tbid());
	Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
    VLogMsg(CLIB_LOG_LEV_ERROR,"user[%lld] leave teabar[%ld]",uid,pdata->tbid());

	return EN_Handler_Done;
}


ENHandlerResult CRequestAgreeUserDropTeaBar::ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession)
{
	long long uid = psession->_request_route.uid();
	PBCSMsg msg;
	CSResponseAgreeUserDropTeaBar& reponse = *msg.mutable_cs_response_agree_user_drop_tea_bar();
	const CSRequestAgreeUserDropTeaBar& cs_request_agree_user_drop_tea_bar = psession->_request_msg.cs_request_agree_user_drop_tea_bar();
    PBTeaBarData* pdata = TeaBarManager::Instance()->GetTeaBar(cs_request_agree_user_drop_tea_bar.tbid());
    if (pdata == NULL)
	{
		reponse.set_result(EN_MESSAGE_TEA_BAR_NOT_EXIST);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}

	if (pdata->master_uid() != uid)
	{
		reponse.set_result(EN_MESSAGE_NOT_HAS_RIGHT);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}

	bool is_in_apply_list = false;
	for (int i = 0; i < pdata->apply_drop_msg_list_size(); i++)
	{
		const PBTeaBarUser& tbuser = pdata->apply_drop_msg_list(i).user();
		if (tbuser.uid() == uid)
		{
			is_in_apply_list = true;
			break;
		}
	}

	if (!is_in_apply_list)
	{
		reponse.set_result(EN_MESSAGE_TEA_BAR_APPLY_NOT_EXSIT);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}

	psession->NewAddGetData(cs_request_agree_user_drop_tea_bar.uid(), PBUserDataField::kUserTeaBarData);
	psession->NewAddGetData(cs_request_agree_user_drop_tea_bar.uid(), PBUserDataField::kUserInfo);
	return EN_Handler_Get;
}

ENHandlerResult CRequestAgreeUserDropTeaBar::ProcessGetSucc(CHandlerTokenBasic* ptoken, CSession* psession)
{
	const CSRequestAgreeUserDropTeaBar& cs_request_agree_user_drop_tea_bar = psession->_request_msg.cs_request_agree_user_drop_tea_bar();
	const PBUserTeaBarData & tbdata = psession->_kvdb_uid_data_map[cs_request_agree_user_drop_tea_bar.uid()].user_tea_bar_data();
	const PBUser& user = psession->_kvdb_uid_data_map[cs_request_agree_user_drop_tea_bar.uid()].user_info();
	PBCSMsg msg;
	CSResponseAgreeUserDropTeaBar& reponse = *msg.mutable_cs_response_agree_user_drop_tea_bar();
	
	bool is_in_tea_bar = false;
	for (int i = 0; i < tbdata.brief_data_size(); i++)
	{
		const TeaBarBriefData& brief = tbdata.brief_data(i);
		if (brief.tbid() == cs_request_agree_user_drop_tea_bar.tbid())
		{
			is_in_tea_bar = true;
			break;
		}
	}

	PBTeaBarData* pdata = TeaBarManager::Instance()->GetTeaBar(cs_request_agree_user_drop_tea_bar.tbid());
	if (pdata == NULL)
	{
		reponse.set_result(EN_MESSAGE_TEA_BAR_NOT_EXIST);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}

	int apply_list_index = -1;
	for (int i = 0; i < pdata->apply_drop_msg_list_size(); i++)
	{
		const PBTeaBarUser& tbuser = pdata->apply_drop_msg_list(i).user();
		if (tbuser.uid() == cs_request_agree_user_drop_tea_bar.uid())
		{
			apply_list_index = i;
			break;
		}
	}

	if (apply_list_index == -1)
	{
		reponse.set_result(EN_MESSAGE_TEA_BAR_APPLY_NOT_EXSIT);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}

	//拒绝
	if (!cs_request_agree_user_drop_tea_bar.if_agree())
	{
		reponse.set_result(EN_MESSAGE_ERROR_OK);
		reponse.set_tbid(cs_request_agree_user_drop_tea_bar.tbid());
		reponse.set_uid(cs_request_agree_user_drop_tea_bar.uid());
		reponse.set_if_agree(cs_request_agree_user_drop_tea_bar.if_agree());
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);

		//从申请列表中删除
		pdata->mutable_apply_drop_msg_list()->DeleteSubrange(apply_list_index, 1);

		//通知申请的玩家结果
		PBCSMsg notify;
		CSNotifyDropTeaBarResult& cs_notify_drop_tea_bar_result = *notify.mutable_cs_notify_drop_tea_bar_result();
		cs_notify_drop_tea_bar_result.set_if_agree(false);
		cs_notify_drop_tea_bar_result.set_tbid(pdata->tbid());
		cs_notify_drop_tea_bar_result.set_tbname(pdata->tbname());
		cs_notify_drop_tea_bar_result.set_master_uid(pdata->master_uid());
		cs_notify_drop_tea_bar_result.set_master_name(pdata->master_name());
		Message::PushInnerMsg(RouteManager::Instance()->GetRouteByRandom(), user.uid(), notify, EN_Node_Connect, user.hallsvid());
		return EN_Handler_Done;
	}

	//删除用户
	if (!TeaBarManager::Instance()->DeleteUser(cs_request_agree_user_drop_tea_bar.uid(), *pdata))
	{
		reponse.set_result(EN_MESSAGE_SYSTEM_FAILED);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}

	//从申请列表中删除
	pdata->mutable_apply_join_msg_list()->DeleteSubrange(apply_list_index, 1);

	if (is_in_tea_bar)
	{
		//更新用户信息
		TeaBarBriefData brief;
		brief.set_tbid(pdata->tbid());

		PBUpdateData update;
		update.set_key(PBUserDataField::kUserTeaBarData);
		PBDBAtomicField & field = *update.add_field_list();
		field.set_field(EN_DB_Field_TeaBar_Brief);
		field.set_strategy(EN_Update_Strategy_Del);
		field.mutable_tea_bar_brief()->CopyFrom(brief);
		psession->NewAddUpdateData(cs_request_agree_user_drop_tea_bar.uid(), update);
		return EN_Handler_Save;
	}
	return EN_Handler_Done;
}

ENHandlerResult CRequestAgreeUserDropTeaBar::ProcessUpdateSucc(CHandlerTokenBasic* ptoken, CSession* psession)
{
	const CSRequestAgreeUserDropTeaBar& cs_request_agree_user_drop_tea_bar = psession->_request_msg.cs_request_agree_user_drop_tea_bar();
	PBCSMsg msg;
	CSResponseAgreeUserDropTeaBar& reponse = *msg.mutable_cs_response_agree_user_drop_tea_bar();
	reponse.set_result(EN_MESSAGE_ERROR_OK);
	reponse.set_tbid(cs_request_agree_user_drop_tea_bar.tbid());
	reponse.set_uid(cs_request_agree_user_drop_tea_bar.uid());
	reponse.set_if_agree(cs_request_agree_user_drop_tea_bar.if_agree());
	Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
	return EN_Handler_Done;
}

ENHandlerResult CRequestTeaBarList::ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession)
{
	psession->NewAddGetData(psession->_request_route.uid(), PBUserDataField::kUserTeaBarData);
	psession->NewAddGetData(psession->_request_route.uid(), PBUserDataField::kUserInfo);
	return EN_Handler_Get;
}


ENHandlerResult CRequestTeaBarList::ProcessGetSucc(CHandlerTokenBasic* ptoken, CSession* psession)
{
	long long uid = psession->_request_route.uid();
	const PBUserTeaBarData & tbdata = psession->_kvdb_uid_data_map[uid].user_tea_bar_data();
	const PBUser& user = psession->_kvdb_uid_data_map[uid].user_info();
	PBCSMsg msg;
	CSResponseTeaBarList& reponse = *msg.mutable_cs_response_tea_bar_list();
	reponse.set_result(EN_MESSAGE_ERROR_OK);
	vector<PBTeaBarData*> vec;
	for (int i = 0; i < tbdata.brief_data_size(); i++)
	{
		PBTeaBarData* pdata = TeaBarManager::Instance()->GetTeaBar(tbdata.brief_data(i).tbid());
		if (pdata == NULL)
		{
			ErrMsg("pdata == NULL, tbid[%ld]", tbdata.brief_data(i).tbid());
			continue;
		}

		//修复bug
		PBTeaBarUser* puser = TeaBarManager::Instance()->GetUser(uid, *pdata);
		if (puser == NULL)
		{
			if (!TeaBarManager::Instance()->AddUser(user, *pdata))
			{
				ErrMsg("add user failed,uid:[%lld]", uid);
			}
			else
			{
				ErrMsg("add user succ,uid:[%lld]", uid);
			}
		}

		CSTeaBarBriefData& brief = *reponse.add_brief_data();
		brief.set_tbid(pdata->tbid());
		brief.set_tbname(pdata->tbname());
		brief.set_master_uid(pdata->master_uid());
		for (int k = 0; k < pdata->users_size() && k < 9; k++)
		{
			brief.add_url(pdata->users(k).url());
		}
		brief.set_desc(pdata->desc());
		vec.push_back(pdata);
	}

	//推荐的茶馆填充
	for (int i = 0; i < PokerPBTeaBarConfig::Instance()->recommend_teabars_size(); i++)
	{
		PBTeaBarData* pdata = TeaBarManager::Instance()->GetTeaBar(PokerPBTeaBarConfig::Instance()->recommend_teabars(i).tbid());
		if (pdata == NULL)
		{
			ErrMsg("pdata == NULL, tbid[%ld]", PokerPBTeaBarConfig::Instance()->recommend_teabars(i).tbid());
			continue;
		}
		CSTeaBarBriefData& brief = *reponse.add_recommend_brief_data();
		brief.set_tbid(pdata->tbid());
		brief.set_tbname(pdata->tbname());
		brief.set_master_uid(pdata->master_uid());
		for (int k = 0; k < pdata->users_size() && k < 9; k++)
		{
			brief.add_url(pdata->users(k).url());
		}
		brief.set_desc(pdata->desc());
	}
	Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);

	for (int i = 0; i<(int)vec.size(); i++)
	{
		PBTeaBarData* pdata = vec[i];
		if (pdata == NULL)
		{
			continue;
		}
		//如果是群主，推送有新消息
		if (pdata->master_uid() == uid)
		{
			if (pdata->apply_join_msg_list_size() > 0 || pdata->apply_drop_msg_list_size() > 0)
			{
				PBCSMsg notify;
				CSNotifyTeaBarHasNewMessage& cs_notify_tea_bar_has_new_message = *notify.mutable_cs_notify_tea_bar_has_new_message();
				cs_notify_tea_bar_has_new_message.set_tbid(pdata->tbid());
				Message::PushInnerMsg(RouteManager::Instance()->GetRouteByRandom(), uid, notify, EN_Node_Connect, psession->_request_route.source_id());
			}
		}
	}
	return EN_Handler_Done;
}

ENHandlerResult CNotifyCreateTable::ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession)
{
	const SSNotifyCreateTable& ss_notify_create_table = psession->_request_msg.ss_notify_create_table();
	PBTeaBarData* pdata = TeaBarManager::Instance()->GetTeaBar(ss_notify_create_table.conf().tbid());
	if (pdata == NULL)
	{
		ErrMsg("tea bar not exist,tbid[%ld]", ss_notify_create_table.conf().tbid());
		return EN_Handler_Done;
	}

	PBTeaBarUser* puser = TeaBarManager::Instance()->GetUser(ss_notify_create_table.uid(), *pdata);
	if (puser == NULL)
	{
		ErrMsg("tea bar not exist user,tbid[%ld],uid[%ld]", ss_notify_create_table.conf().tbid(), ss_notify_create_table.uid());
		return EN_Handler_Done;
	}
	PBTeaBarTable& tbtable = *pdata->add_tables();
	tbtable.mutable_conf()->CopyFrom(ss_notify_create_table.conf());
	tbtable.set_tid(ss_notify_create_table.tid());
	tbtable.set_player_num(0);
	tbtable.set_if_start(false);
	tbtable.set_owner_uid(puser->uid());
	tbtable.set_owner_name(puser->name());
	tbtable.set_create_time((int)time(NULL));

	//更新redis数据
	if (!TeaBarManager::Instance()->UpdateDataToRedis(*pdata))
	{
		ErrMsg("update redis failed,%ld", pdata->tbid());
	}

	int wait_table_num = 0;
	for (int i = 0; i < pdata->tables_size(); i++)
	{
		const PBTeaBarTable& table = pdata->tables(i);
		if (table.player_num() < table.conf().seat_num())
		{
			wait_table_num++;
		}
	}

	TeaBarWaitTableData table_data;
	table_data.set_tbid(ss_notify_create_table.conf().tbid());
	table_data.set_wait_table_num(wait_table_num);
	PBUpdateData update;
	update.set_key(PBUserDataField::kUserTeaBarData);
	PBDBAtomicField & field = *update.add_field_list();
	field.set_field(EN_DB_Field_TeaBar_Wait_Table_Num);
	field.mutable_tea_bar_wait_table_data()->CopyFrom(table_data);
	psession->NewAddUpdateData(ss_notify_create_table.conf().master_uid(), update);
	return EN_Handler_Save;
}

ENHandlerResult CNotifyCreateTable::ProcessUpdateSucc(CHandlerTokenBasic* ptoken, CSession* psession)
{
	return EN_Handler_Done;
}

ENHandlerResult CNotifyTeabarTableGameStart::ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession)
{
	const SSNotifyTeaBarTableGameStart& ss_notify_teabar_table_game_start = psession->_request_msg.ss_notify_teabar_table_game_start();
	PBTeaBarData* pdata = TeaBarManager::Instance()->GetTeaBar(ss_notify_teabar_table_game_start.conf().tbid());
	if (pdata == NULL)
	{
		ErrMsg("tea bar not exist,tbid[%ld]", ss_notify_teabar_table_game_start.conf().tbid());
		return EN_Handler_Done;
	}

	for (int i = 0; i < pdata->tables_size(); i++)
	{
		PBTeaBarTable& table = *pdata->mutable_tables(i);
		if (table.tid() == ss_notify_teabar_table_game_start.tid())
		{
			table.set_if_start(true);
			//最新的配置覆盖
			table.mutable_conf()->CopyFrom(ss_notify_teabar_table_game_start.conf());
			break;
		}
	}

	//更新redis数据
	if (!TeaBarManager::Instance()->UpdateDataToRedis(*pdata))
	{
		ErrMsg("update redis failed,%ld", pdata->tbid());
	}

	return EN_Handler_Done;
}

ENHandlerResult CNotifyTeabarTableGameOver::ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession)
{
	const SSNotifyTeaBarTableGameOver& ss_notify_teabar_table_game_over = psession->_request_msg.ss_notify_teabar_table_game_over();
	const PBSDRTableConfig& conf = ss_notify_teabar_table_game_over.conf();
	PBTeaBarData* pdata = TeaBarManager::Instance()->GetTeaBar(conf.tbid());
	if (pdata == NULL)
	{
		ErrMsg("tea bar not exist,tbid[%ld]", conf.tbid());
		return EN_Handler_Done;
	}

	//数据统计
	PBDateTeaBarUserGameRecordList* pdatelist = TeaBarManager::Instance()->GetGameRecordList(*pdata, EN_TeaBar_Date_Type_Today);
	if (pdatelist == NULL)
	{
		ErrMsg("pdatelist NULL,tbid[%ld]", conf.tbid());
		return EN_Handler_Done;
	}
	const PBTableStatistics& statistics = ss_notify_teabar_table_game_over.statistics();
	string best_name = "";
	int64 best_uid = 0;
	//正常结算
	if (statistics.seat_statistics_list_size() > 0)
	{
		for (int i = 0; i < statistics.seat_statistics_list_size(); i++)
		{
			const PBSeatStatistics& seat_statistics = statistics.seat_statistics_list(i);
			PBTeaBarUser *puser = TeaBarManager::Instance()->GetUser(seat_statistics.uid(), *pdata);
			if (puser == NULL)
			{
				ErrMsg("puser == NULL,tbid[%ld],uid[%ld]", pdata->tbid(), seat_statistics.uid());
				continue;
			}

			//更新下图像
			if (puser->url() != seat_statistics.pic_url())
			{
				puser->set_url(seat_statistics.pic_url());
			}

			PBTeaBarUserGameRecord *puserrecord = TeaBarManager::Instance()->GetUserGameRecord(seat_statistics.uid(), *pdatelist);
			if (puserrecord == NULL)
			{
				ErrMsg("puserrecord == NULL,tbid[%ld],uid[%ld]", pdata->tbid(), seat_statistics.uid());
				continue;
			}

			puserrecord->set_play_num(puserrecord->play_num() + 1);
			if (ss_notify_teabar_table_game_over.owner_uid() == seat_statistics.uid())
			{
				puserrecord->set_create_table_num(puserrecord->create_table_num() + 1);
			}
			if (statistics.best_winner() == seat_statistics.index())
			{
				puserrecord->set_best_score_num(puserrecord->best_score_num() + 1);
				best_name = seat_statistics.nick();
				best_uid = seat_statistics.uid();
			}
		}

		//pdatelist->set_create_table_num(pdatelist->create_table_num() + 1);
		//int cost = PokerPBCostConfig::Instance()->GetCostByRound(conf.round());
		//int cost = conf.cost();
		//pdatelist->set_cost_chips(pdatelist->cost_chips() + cost);

		if (statistics.has_best_winner())
		{
			//加入今日桌子记录
			int now = (int)time(NULL);
			PBDateTeaBarTableRecordList* precordlist = TeaBarManager::Instance()->GetTodayTableRecordList(*pdata);
			if (precordlist != NULL)
			{
				PBTeaBarTable& tbtable = *precordlist->add_tables();
				tbtable.set_tid(ss_notify_teabar_table_game_over.tid());
				tbtable.mutable_conf()->CopyFrom(ss_notify_teabar_table_game_over.conf());
				tbtable.set_statistics_time(now);
				tbtable.set_best_uid(best_uid);
				tbtable.set_best_name(best_name);
				//生成一个统计id
				int64 statistics_id = (tbtable.tid() << 32) + now;
				tbtable.set_statistics_id(statistics_id);
				//写入具体牌局记录到redis
				PBTeaBarTableDetailStatistics detail_statistics;
				detail_statistics.set_tid(tbtable.tid());
				detail_statistics.mutable_conf()->CopyFrom(tbtable.conf());
				detail_statistics.mutable_statistics()->CopyFrom(statistics);
				TeaBarManager::Instance()->AddTableStatistics(statistics_id, detail_statistics);

				//测试代码
				//for (int i = 0; i < 1000; i++)
				//{
				//	PBTeaBarTable& tbtable = *precordlist->add_tables();
				//	tbtable.set_tid(ss_notify_teabar_table_game_over.tid());
				//	tbtable.mutable_conf()->CopyFrom(ss_notify_teabar_table_game_over.conf());
				//	tbtable.set_statistics_time(now);
				//	//生成一个统计id
				//	statistics_id++;
				//	tbtable.set_statistics_id(statistics_id);
				//	//写入具体牌局记录到redis
				//	PBTeaBarTableDetailStatistics detail_statistics;
				//	detail_statistics.set_tid(tbtable.tid());
				//	detail_statistics.mutable_conf()->CopyFrom(tbtable.conf());
				//	detail_statistics.mutable_statistics()->CopyFrom(statistics);
				//	TeaBarManager::Instance()->AddTableStatistics(statistics_id, detail_statistics);
				//}
				//测试代码--end
			}
		}
	}
	//删除桌子
	int find_index = -1;
	for (int i = 0; i < pdata->tables_size(); i++)
	{
		const PBTeaBarTable& table = *pdata->mutable_tables(i);
		if (table.tid() == ss_notify_teabar_table_game_over.tid())
		{
			find_index = i;
			break;
		}
	}

	if (find_index != -1)
	{
		pdata->mutable_tables()->DeleteSubrange(find_index, 1);
	}

	//更新redis数据
	if (!TeaBarManager::Instance()->UpdateDataToRedis(*pdata))
	{
		ErrMsg("update redis failed,%ld", pdata->tbid());
	}

	return EN_Handler_Done;
}

ENHandlerResult CNotifyTeabarTablePlayerNum::ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession)
{
	const SSNotifyTeaBarTablePlayerNum& ss_notify_teabar_table_player_num = psession->_request_msg.ss_notify_teabar_table_player_num();
	const PBSDRTableConfig& conf = ss_notify_teabar_table_player_num.conf();
	PBTeaBarData* pdata = TeaBarManager::Instance()->GetTeaBar(conf.tbid());
	if (pdata == NULL)
	{
		ErrMsg("tea bar not exist,tbid[%ld]", conf.tbid());
		return EN_Handler_Done;
	}
	
	int wait_table_num = 0;
	for (int i = 0; i < pdata->tables_size(); i++)
	{
		PBTeaBarTable& table = *pdata->mutable_tables(i);
		if (table.tid() == ss_notify_teabar_table_player_num.tid())
		{
			table.set_player_num(ss_notify_teabar_table_player_num.player_num());
		}
		if (table.player_num() < conf.seat_num())
		{
			wait_table_num++;
		}
	}
	TeaBarWaitTableData table_data;
	table_data.set_tbid(conf.tbid());
	table_data.set_wait_table_num(wait_table_num);
	PBUpdateData update;
	update.set_key(PBUserDataField::kUserTeaBarData);
	PBDBAtomicField & field = *update.add_field_list();
	field.set_field(EN_DB_Field_TeaBar_Wait_Table_Num);
	field.mutable_tea_bar_wait_table_data()->CopyFrom(table_data);
	psession->NewAddUpdateData(conf.master_uid(), update);

	return EN_Handler_Save;
}

ENHandlerResult CNotifyTeabarTablePlayerNum::ProcessUpdateSucc(CHandlerTokenBasic* ptoken, CSession* psession)
{
	return EN_Handler_Done;
}

ENHandlerResult CRequestGetTeaBarUserList::ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession)
{
	long long uid = psession->_request_route.uid();
	int connect_id = psession->_request_route.source_id();
	const CSRequestGetTeaBarUserList& cs_request_get_tea_bar_user_list = psession->_request_msg.cs_request_get_tea_bar_user_list();
	PBCSMsg msg;
	CSResponseGetTeaBarUserList& response = *msg.mutable_cs_response_get_tea_bar_user_list();
	PBTeaBarData* pdata = TeaBarManager::Instance()->GetTeaBar(cs_request_get_tea_bar_user_list.tbid());
	if (pdata == NULL)
	{
		ErrMsg("tea bar not exist,tbid[%ld]", cs_request_get_tea_bar_user_list.tbid());
		
		response.set_result(EN_MESSAGE_TEA_BAR_NOT_EXIST);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}

	//分段发送，群的人太多可能导致一个包发不下
	response.set_result(EN_MESSAGE_ERROR_OK);
	response.set_tbid(cs_request_get_tea_bar_user_list.tbid());
	response.set_date_type(cs_request_get_tea_bar_user_list.date_type());
	bool is_finish = false;
	map<int64, PBTeaBarUserGameRecord> m;
	m.clear();
	int create_table_num = 0;
	int64 cost_chips = 0;
	TeaBarManager::Instance()->GetGameRecordList(m, create_table_num, cost_chips, *pdata, cs_request_get_tea_bar_user_list.date_type());
	PBTeaBarUser* pmaster = TeaBarManager::Instance()->GetUser(pdata->master_uid(), *pdata);
	if (pmaster == NULL)
	{
		ErrMsg("tea bar master cannot find,tbid[%ld],uid[%ld]", pdata->tbid(), pdata->master_uid());
		response.set_result(EN_MESSAGE_TEA_BAR_NOT_EXIST);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}
	int idx = 0;
	//群主默认放第一个
	{
		CSTeaBarUser& user = *response.add_users();
		user.set_index(idx);
		user.set_uid(pmaster->uid());
		user.set_name(pmaster->name());
		user.set_url(pmaster->url());
		map<int64, PBTeaBarUserGameRecord>::iterator iter = m.find(pmaster->uid());
		if (iter != m.end())
		{
			user.set_create_table_num(iter->second.create_table_num());
			user.set_play_num(iter->second.play_num());
			user.set_best_score_num(iter->second.best_score_num());
			user.set_settle_num(iter->second.settle_num());
		}
		else
		{
			user.set_create_table_num(0);
			user.set_play_num(0);
			user.set_best_score_num(0);
			user.set_settle_num(0);
		}
		idx++;
	}

#ifdef _DEBUG
    for(int i = 1 ; i < 22 ; i ++)
    {
        PBTeaBarUser * pbAddUser =  pdata->add_users();
        pbAddUser->set_uid(10000 + i);
        pbAddUser->set_name("");
        pbAddUser->set_url("");
    }

    create_table_num = 200;
    cost_chips = 300;
#endif

	for (int i = 0; i < pdata->users_size(); i++)
	{
        const PBTeaBarUser& tbuser = pdata->users(i);
		if (tbuser.uid() == pdata->master_uid())
		{
			continue;
		}
		CSTeaBarUser& user = *response.add_users();
		user.set_index(idx);
		user.set_uid(tbuser.uid());
		user.set_name(tbuser.name());
		user.set_url(tbuser.url());
		map<int64, PBTeaBarUserGameRecord>::iterator iter = m.find(tbuser.uid());
		if (iter != m.end())
		{
			user.set_create_table_num(iter->second.create_table_num());
			user.set_play_num(iter->second.play_num());
			user.set_best_score_num(iter->second.best_score_num());
			user.set_settle_num(iter->second.settle_num());
		}
		else
		{
			user.set_create_table_num(0);
			user.set_play_num(0);
			user.set_best_score_num(0);
			user.set_settle_num(0);
		}
		
		idx++;
		if (idx % 20 == 0)
        {
			if (idx == pdata->users_size())
			{
				response.set_is_end(true);
				response.set_create_table_num(create_table_num);
                response.set_cost_chips(cost_chips);
				Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
				is_finish = true;
			}
			else
			{
//                response.set_create_table_num(create_table_num);
//                response.set_cost_chips(cost_chips);
				Message::PushInnerMsg(RouteManager::Instance()->GetRouteByRandom(), uid, msg, EN_Node_Connect, connect_id);
				//清除信息继续发送
				response.clear_users();
			}
		}
	}

	if (!is_finish)
	{
		response.set_is_end(true);
		response.set_create_table_num(create_table_num);
		response.set_cost_chips(cost_chips);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
	}
	return EN_Handler_Done;
}

ENHandlerResult CRequestTeaBarInfo::ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession)
{
	const CSRequestTeaBarInfo& cs_request_tea_bar_info = psession->_request_msg.cs_request_tea_bar_info();
	PBCSMsg msg;
	CSResponseTeaBarInfo& response = *msg.mutable_cs_response_tea_bar_info();
	PBTeaBarData* pdata = TeaBarManager::Instance()->GetTeaBar(cs_request_tea_bar_info.tbid());
	if (pdata == NULL)
	{
		ErrMsg("tea bar not exist,tbid[%ld]", cs_request_tea_bar_info.tbid());
		response.set_result(EN_MESSAGE_TEA_BAR_NOT_EXIST);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}
	response.set_result(EN_MESSAGE_ERROR_OK);
	CSTeaBarBriefData& brief = *response.mutable_brief_data();
	brief.set_tbid(pdata->tbid());
	brief.set_tbname(pdata->tbname());
	brief.set_master_uid(pdata->master_uid());
	for (int k = 0; k < pdata->users_size() && k < 9; k++)
	{
		brief.add_url(pdata->users(k).url());
	}
	brief.set_desc(pdata->desc());
	Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
	return EN_Handler_Done;
}

ENHandlerResult CRequestTeaBarMessage::ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession)
{
	long long uid = psession->_request_route.uid();
	const CSRequestTeaBarMessage& cs_request_tea_bar_message = psession->_request_msg.cs_request_tea_bar_message();
	PBCSMsg msg;
	CSResponseTeaBarMessage& response = *msg.mutable_cs_response_tea_bar_message();
	PBTeaBarData* pdata = TeaBarManager::Instance()->GetTeaBar(cs_request_tea_bar_message.tbid());
	if (pdata == NULL)
	{
		ErrMsg("tea bar not exist,tbid[%ld]", cs_request_tea_bar_message.tbid());
		response.set_result(EN_MESSAGE_TEA_BAR_NOT_EXIST);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}
	response.set_result(EN_MESSAGE_ERROR_OK);
	response.set_tbid(pdata->tbid());
	if (pdata->master_uid() == uid)
	{
		for (int i = 0; i < pdata->apply_join_msg_list_size(); i++)
		{
			PBTeaBarCommMessage& message = *response.add_msg_list();
			message.set_msg_type(EN_Tea_Bar_Message_Type_Apply_Jion);
			message.mutable_apply_join_msg()->CopyFrom(pdata->apply_join_msg_list(i));
		}

		for (int i = 0; i < pdata->apply_drop_msg_list_size(); i++)
		{
			PBTeaBarCommMessage& message = *response.add_msg_list();
			message.set_msg_type(EN_Tea_Bar_Message_Type_Apply_Drop);
			message.mutable_apply_drop_msg()->CopyFrom(pdata->apply_drop_msg_list(i));
		}
	}
	Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
	return EN_Handler_Done;
}

ENHandlerResult CRequestPutChipsToTeaBar::ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession)
{
	long long uid = psession->_request_route.uid();
	const CSRequestPutChipsToTeaBar& cs_request_put_chips_to_tea_bar = psession->_request_msg.cs_request_put_chips_to_tea_bar();
	PBCSMsg msg; 
	CSResponsePutChipsToTeaBar& response = *msg.mutable_cs_response_put_chips_to_tea_bar();
	PBTeaBarData* pdata = TeaBarManager::Instance()->GetTeaBar(cs_request_put_chips_to_tea_bar.tbid());
	if (pdata == NULL)
	{
		ErrMsg("tea bar not exist,tbid[%ld]", cs_request_put_chips_to_tea_bar.tbid());
		response.set_result(EN_MESSAGE_TEA_BAR_NOT_EXIST);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}

	if (uid != pdata->master_uid())
	{
		response.set_result(EN_MESSAGE_NOT_HAS_RIGHT);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}

	if (cs_request_put_chips_to_tea_bar.addchips() == 0)
	{
		response.set_result(EN_MESSAGE_ADD_CHIPS_ZERO);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}

	psession->NewAddGetData(psession->_request_route.uid(), PBUserDataField::kUserInfo);
	psession->NewAddGetData(psession->_request_route.uid(), PBUserDataField::kUserTeaBarData);
	return EN_Handler_Get;
}

ENHandlerResult CRequestPutChipsToTeaBar::ProcessGetSucc(CHandlerTokenBasic* ptoken, CSession* psession)
{
	long long uid = psession->_request_route.uid();
	const CSRequestPutChipsToTeaBar& cs_request_put_chips_to_tea_bar = psession->_request_msg.cs_request_put_chips_to_tea_bar();
	const PBUser & user = psession->_kvdb_uid_data_map[uid].user_info();
	const PBUserTeaBarData& usertbdata = psession->_kvdb_uid_data_map[uid].user_tea_bar_data();
	PBCSMsg msg;
	CSResponsePutChipsToTeaBar& response = *msg.mutable_cs_response_put_chips_to_tea_bar();
	const TeaBarBriefData* pbrief = NULL;
	for (int i = 0; i<usertbdata.brief_data_size(); i++)
	{
		const TeaBarBriefData& brief = usertbdata.brief_data(i);
		if (brief.tbid() == cs_request_put_chips_to_tea_bar.tbid())
		{
			pbrief = &brief;
			break;
		}
	}

	if (pbrief == NULL)
	{
		response.set_result(EN_MESSAGE_TEA_BAR_NOT_EXIST);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}
	
	
	if (cs_request_put_chips_to_tea_bar.addchips() > 0)
	{
		if (user.chips() < cs_request_put_chips_to_tea_bar.addchips())
		{
			response.set_result(EN_MESSAGE_PUT_CHIPS_TO_TEA_BAR_NO_ENOUGH);
			Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
			return EN_Handler_Done;
		}
		//增加茶馆房卡，减少群主房卡
		{
			TeaBarChipsFlow flow;
			flow.set_tbid(cs_request_put_chips_to_tea_bar.tbid());
			flow.set_chips(cs_request_put_chips_to_tea_bar.addchips());
			flow.set_reason(EN_Reason_Master_Put_Chips_To_Tea_Bar);
			PBUpdateData update;
			update.set_key(PBUserDataField::kUserTeaBarData);
			PBDBAtomicField& field = *update.add_field_list();
			field.set_field(EN_DB_Field_TeaBar_Chips);
			field.mutable_tea_bar_chips_flow()->CopyFrom(flow);
			psession->NewAddUpdateData(uid, update);
		}

		{
			PBUpdateData update;
			update.set_key(PBUserDataField::kUserInfo);
			PBDBAtomicField & field = *update.add_field_list();
			field.set_field(EN_DB_Field_Chips);
			field.set_intval(0 - cs_request_put_chips_to_tea_bar.addchips());
			field.set_reason(EN_Reason_Master_Put_Chips_To_Tea_Bar);
			psession->NewAddUpdateData(uid, update);
		}
	}
	else
	{
		if (cs_request_put_chips_to_tea_bar.addchips()+ pbrief->chips() < 0)
		{
			response.set_result(EN_MESSAGE_GET_CHIPS_FROM_TEA_BAR_NO_ENOUGH);
			Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
			return EN_Handler_Done;
		}

		//减少茶馆房卡，增加群主房卡
		{
			TeaBarChipsFlow flow;
			flow.set_tbid(cs_request_put_chips_to_tea_bar.tbid());
			flow.set_chips(cs_request_put_chips_to_tea_bar.addchips());
			flow.set_reason(EN_Reason_Master_Get_Chips_From_Tea_Bar);
			PBUpdateData update;
			update.set_key(PBUserDataField::kUserTeaBarData);
			PBDBAtomicField& field = *update.add_field_list();
			field.set_field(EN_DB_Field_TeaBar_Chips);
			field.mutable_tea_bar_chips_flow()->CopyFrom(flow);
			psession->NewAddUpdateData(uid, update);
		}

		{
			PBUpdateData update;
			update.set_key(PBUserDataField::kUserInfo);
			PBDBAtomicField & field = *update.add_field_list();
			field.set_field(EN_DB_Field_Chips);
			field.set_intval(0 - cs_request_put_chips_to_tea_bar.addchips());
			field.set_reason(EN_Reason_Master_Get_Chips_From_Tea_Bar);
			psession->NewAddUpdateData(uid, update);
		}
	}
	return EN_Handler_Save;
}

ENHandlerResult CRequestPutChipsToTeaBar::ProcessUpdateSucc(CHandlerTokenBasic* ptoken, CSession* psession)
{
	long long uid = psession->_request_route.uid();
	const CSRequestPutChipsToTeaBar& cs_request_put_chips_to_tea_bar = psession->_request_msg.cs_request_put_chips_to_tea_bar();
	const PBUserTeaBarData& usertbdata = psession->_kvdb_uid_data_map[uid].user_tea_bar_data();
	PBCSMsg msg;
	CSResponsePutChipsToTeaBar& response = *msg.mutable_cs_response_put_chips_to_tea_bar();
	const TeaBarBriefData* pbrief = NULL;
	for (int i = 0; i < usertbdata.brief_data_size(); i++)
	{
		const TeaBarBriefData& brief = usertbdata.brief_data(i);
		if (brief.tbid() == cs_request_put_chips_to_tea_bar.tbid())
		{
			pbrief = &brief;
			break;
		}
	}

	if (pbrief == NULL)
	{
		response.set_result(EN_MESSAGE_TEA_BAR_NOT_EXIST);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}

	response.set_result(EN_MESSAGE_ERROR_OK);
	response.set_tbid(cs_request_put_chips_to_tea_bar.tbid());
	response.set_chips(pbrief->chips());
	response.set_addchips(cs_request_put_chips_to_tea_bar.addchips());
	Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
	return EN_Handler_Done;
}

ENHandlerResult CRequestModifySettleNum::ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession)
{
	long long uid = psession->_request_route.uid();
	const CSRequestModifySettleNum& cs_request_modify_settle_num = psession->_request_msg.cs_request_modify_settle_num();
	PBCSMsg msg;
	CSResponseModifySettleNum& response = *msg.mutable_cs_response_modify_settle_num();
	PBTeaBarData* pdata = TeaBarManager::Instance()->GetTeaBar(cs_request_modify_settle_num.tbid());
	if (pdata == NULL)
	{
		ErrMsg("tea bar not exist,tbid[%ld]", cs_request_modify_settle_num.tbid());
		response.set_result(EN_MESSAGE_TEA_BAR_NOT_EXIST);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}

	if (pdata->master_uid() != uid)
	{
		response.set_result(EN_MESSAGE_NOT_HAS_RIGHT);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}
	
	PBDateTeaBarUserGameRecordList* pdatelist = TeaBarManager::Instance()->GetGameRecordList(*pdata, cs_request_modify_settle_num.date_type());
	if (pdatelist == NULL)
	{
		ErrMsg("pdatelist == NULL,tbid[%ld],date_type:[%d]", cs_request_modify_settle_num.tbid(), cs_request_modify_settle_num.date_type());
		response.set_result(EN_MESSAGE_NOT_HAS_RIGHT);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}

	PBTeaBarUserGameRecord* puserrecord = TeaBarManager::Instance()->GetUserGameRecord(cs_request_modify_settle_num.uid(), *pdatelist);
	if (puserrecord == NULL)
	{
		ErrMsg("puserrecord == NULL,tbid[%ld],date_type:[%d],uid:[%ld]", cs_request_modify_settle_num.tbid(), cs_request_modify_settle_num.date_type(),
			cs_request_modify_settle_num.uid());
		response.set_result(EN_MESSAGE_NOT_HAS_RIGHT);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}
	puserrecord->set_settle_num(cs_request_modify_settle_num.settle_num());

	if (!TeaBarManager::Instance()->UpdateDataToRedis(*pdata))
	{
		ErrMsg("update redis failed,%ld", pdata->tbid());
	}

	response.set_result(EN_MESSAGE_ERROR_OK);
	response.set_tbid(cs_request_modify_settle_num.tbid());
	response.set_uid(cs_request_modify_settle_num.uid());
	response.set_settle_num(cs_request_modify_settle_num.settle_num());
	response.set_date_type(cs_request_modify_settle_num.date_type());
	Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
	return EN_Handler_Done;
}


ENHandlerResult CRequestRemoveUser::ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession)
{
	long long uid = psession->_request_route.uid();
	const CSRequestRemoveUser& cs_request_remove_user = psession->_request_msg.cs_request_remove_user();
	PBCSMsg msg;
	CSResponseRemoveUser& response = *msg.mutable_cs_response_remove_user();
	PBTeaBarData* pdata = TeaBarManager::Instance()->GetTeaBar(cs_request_remove_user.tbid());
	if (pdata == NULL)
	{
		ErrMsg("tea bar not exist,tbid[%ld]", cs_request_remove_user.tbid());
		response.set_result(EN_MESSAGE_TEA_BAR_NOT_EXIST);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}

	if (pdata->master_uid() != uid)
	{
		response.set_result(EN_MESSAGE_NOT_HAS_RIGHT);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}

	if (cs_request_remove_user.uid() == pdata->master_uid())
	{
		response.set_result(EN_MESSAGE_OWNER_CAN_NOT_DROP_TEA_BAR);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}

	psession->NewAddGetData(cs_request_remove_user.uid(), PBUserDataField::kUserTeaBarData);
	return EN_Handler_Get;
}

ENHandlerResult CRequestRemoveUser::ProcessGetSucc(CHandlerTokenBasic* ptoken, CSession* psession)
{
	const CSRequestRemoveUser& cs_request_remove_user = psession->_request_msg.cs_request_remove_user();
	const PBUserTeaBarData& usertbdata = psession->_kvdb_uid_data_map[cs_request_remove_user.uid()].user_tea_bar_data();
	PBCSMsg msg;
	CSResponseRemoveUser& response = *msg.mutable_cs_response_remove_user();
	const TeaBarBriefData* pbrief = NULL;
	for (int i = 0; i < usertbdata.brief_data_size(); i++)
	{
		const TeaBarBriefData& brief = usertbdata.brief_data(i);
		if (brief.tbid() == cs_request_remove_user.tbid())
		{
			pbrief = &brief;
			break;
		}
	}

	PBTeaBarData* pdata = TeaBarManager::Instance()->GetTeaBar(cs_request_remove_user.tbid());
	if (pdata == NULL)
	{
		ErrMsg("tea bar not exist,tbid[%ld]", cs_request_remove_user.tbid());
		response.set_result(EN_MESSAGE_TEA_BAR_NOT_EXIST);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}

	if (TeaBarManager::Instance()->DeleteUser(cs_request_remove_user.uid(), *pdata))
	{
		ErrMsg("delete user error,tbid[%ld],uid:[%ld]", pdata->tbid(), cs_request_remove_user.uid());
	}

	if (pbrief != NULL)
	{
		TeaBarBriefData brief;
		brief.set_tbid(pdata->tbid());

		PBUpdateData update;
		update.set_key(PBUserDataField::kUserTeaBarData);
		PBDBAtomicField & field = *update.add_field_list();
		field.set_field(EN_DB_Field_TeaBar_Brief);
		field.set_strategy(EN_Update_Strategy_Del);
		field.mutable_tea_bar_brief()->CopyFrom(brief);
		psession->NewAddUpdateData(cs_request_remove_user.uid(), update);
		return EN_Handler_Save;
	}

	response.set_result(EN_MESSAGE_ERROR_OK);
	response.set_tbid(cs_request_remove_user.tbid());
	response.set_uid(cs_request_remove_user.uid());
	Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
	return EN_Handler_Done;
}

ENHandlerResult CRequestRemoveUser::ProcessUpdateSucc(CHandlerTokenBasic* ptoken, CSession* psession)
{
	const CSRequestRemoveUser& cs_request_remove_user = psession->_request_msg.cs_request_remove_user();
	PBCSMsg msg;
	CSResponseRemoveUser& response = *msg.mutable_cs_response_remove_user();
	response.set_result(EN_MESSAGE_ERROR_OK);
	response.set_tbid(cs_request_remove_user.tbid());
	response.set_uid(cs_request_remove_user.uid());
	Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
	return EN_Handler_Done;
}

ENHandlerResult CNotifyTeaBarTableNotExist::ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession)
{
	const SSNotifyTeaBarTableNotExist& ss_notify_tea_bar_table_not_exist = psession->_request_msg.ss_notify_tea_bar_table_not_exist();
	PBTeaBarData* pdata = TeaBarManager::Instance()->GetTeaBar(ss_notify_tea_bar_table_not_exist.tbid());
	if (pdata == NULL)
	{
		ErrMsg("tea bar not exist,tbid[%ld]", ss_notify_tea_bar_table_not_exist.tbid());
		return EN_Handler_Done;
	}

	//删除桌子
	int find_index = -1;
	for (int i = 0; i < pdata->tables_size(); i++)
	{
		const PBTeaBarTable& table = pdata->tables(i);
		if (table.tid() == ss_notify_tea_bar_table_not_exist.tid())
		{
			find_index = i;
			break;
		}
	}

	if (find_index != -1)
	{
		pdata->mutable_tables()->DeleteSubrange(find_index, 1);
	}

	if (!TeaBarManager::Instance()->UpdateDataToRedis(*pdata))
	{
		ErrMsg("update redis failed,%ld", pdata->tbid());
	}

	return EN_Handler_Done;
}

ENHandlerResult CRequestModifyTeaBarDesc::ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession)
{
	long long uid = psession->_request_route.uid();
	const CSRequestModifyTeaBarDesc& cs_request_modify_tea_bar_desc = psession->_request_msg.cs_request_modify_tea_bar_desc();
	PBCSMsg msg;
	CSResponseModifyTeaBarDesc& response = *msg.mutable_cs_response_modify_tea_bar_desc();
	PBTeaBarData* pdata = TeaBarManager::Instance()->GetTeaBar(cs_request_modify_tea_bar_desc.tbid());
	if (pdata == NULL)
	{
		ErrMsg("tea bar not exist,tbid[%ld]", cs_request_modify_tea_bar_desc.tbid());
		response.set_result(EN_MESSAGE_TEA_BAR_NOT_EXIST);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}
	if (uid != pdata->master_uid())
	{
		response.set_result(EN_MESSAGE_NOT_HAS_RIGHT);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}

	if (cs_request_modify_tea_bar_desc.has_desc())
	{
		pdata->set_desc(cs_request_modify_tea_bar_desc.desc());
	}
	
	if (cs_request_modify_tea_bar_desc.has_pay_type())
	{
		//更新茶馆支付方式
		pdata->set_pay_type(cs_request_modify_tea_bar_desc.pay_type());

		if (!TeaBarManager::Instance()->UpdateDataToRedis(*pdata))
		{
			ErrMsg("update redis failed,%ld", pdata->tbid());
		}
		//更新用户茶馆概要信息
		TeaBarPayTypeData paytypedata;
		paytypedata.set_tbid(pdata->tbid());
		paytypedata.set_pay_type(cs_request_modify_tea_bar_desc.pay_type());

		PBUpdateData update;
		update.set_key(PBUserDataField::kUserTeaBarData);
		PBDBAtomicField & field = *update.add_field_list();
		field.set_field(EN_DB_Field_TeaBar_Pay_Type);
		field.mutable_tea_bar_pay_type_data()->CopyFrom(paytypedata);
		psession->NewAddUpdateData(uid, update);
		return EN_Handler_Save;
	}

	if (!TeaBarManager::Instance()->UpdateDataToRedis(*pdata))
	{
		ErrMsg("update redis failed,%ld", pdata->tbid());
	}

	response.set_result(EN_MESSAGE_ERROR_OK);
	response.set_tbid(cs_request_modify_tea_bar_desc.tbid());
	response.set_desc(pdata->desc());
	Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
	return EN_Handler_Done;
}


ENHandlerResult CRequestModifyTeaBarDesc::ProcessUpdateSucc(CHandlerTokenBasic* ptoken, CSession* psession)
{
	const CSRequestModifyTeaBarDesc& cs_request_modify_tea_bar_desc = psession->_request_msg.cs_request_modify_tea_bar_desc();
	PBCSMsg msg;
	CSResponseModifyTeaBarDesc& response = *msg.mutable_cs_response_modify_tea_bar_desc();
	response.set_result(EN_MESSAGE_ERROR_OK);
	response.set_tbid(cs_request_modify_tea_bar_desc.tbid());
	if (cs_request_modify_tea_bar_desc.has_desc())
	{
		response.set_desc(cs_request_modify_tea_bar_desc.desc());
	}
	response.set_pay_type(cs_request_modify_tea_bar_desc.pay_type());
	Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
	return EN_Handler_Done;
}

ENHandlerResult CRequestFreeTeaBar::ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession)
{
	long long uid = psession->_request_route.uid();
	psession->NewAddGetData(uid, PBUserDataField::kUserTeaBarData);
	return EN_Handler_Get;
}

ENHandlerResult CRequestFreeTeaBar::ProcessGetSucc(CHandlerTokenBasic* ptoken, CSession* psession)
{
	long long uid = psession->_request_route.uid();
	const PBUserTeaBarData & tbdata = psession->_kvdb_uid_data_map[uid].user_tea_bar_data();
	const CSRequestFreeTeaBar& cs_request_free_tea_bar = psession->_request_msg.cs_request_free_tea_bar();
	PBCSMsg msg;
	CSResponseFreeTeaBar& response = *msg.mutable_cs_response_free_tea_bar();
	const TeaBarBriefData* pbrief = NULL;
	for (int i = 0; i < tbdata.brief_data_size(); i++)
	{
		const TeaBarBriefData& brief = tbdata.brief_data(i);
		if (brief.tbid() == cs_request_free_tea_bar.tbid())
		{
			pbrief = &brief;
			break;
		}
	}

	if (pbrief == NULL)
	{
		ErrMsg("not in tea bar,tbid[%ld]", cs_request_free_tea_bar.tbid());
		response.set_result(EN_MESSAGE_NOT_IN_TEA_BAR);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}

	if (pbrief->master_uid() != uid)
	{
		response.set_result(EN_MESSAGE_NOT_HAS_RIGHT);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}

	PBTeaBarData* pdata = TeaBarManager::Instance()->GetTeaBar(cs_request_free_tea_bar.tbid());
	if (pdata == NULL)
	{
		ErrMsg("tea bar not exist,tbid[%ld]", cs_request_free_tea_bar.tbid());
		response.set_result(EN_MESSAGE_TEA_BAR_NOT_EXIST);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}

	if (pdata->master_uid() != uid)
	{
		response.set_result(EN_MESSAGE_NOT_HAS_RIGHT);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}

	//返还房卡给茶馆主人
	if (pbrief->chips() > 0)
	{
		PBUpdateData update;
		update.set_key(PBUserDataField::kUserInfo);
		PBDBAtomicField & field = *update.add_field_list();
		field.set_field(EN_DB_Field_Chips);
		field.set_intval(pbrief->chips());
		field.set_reason(EN_Reason_Master_Get_Chips_From_Tea_Bar);
		psession->NewAddUpdateData(uid, update);
	}

	for (int i = 0; i < pdata->users_size(); i++)
	{
		const PBTeaBarUser& tbuser = pdata->users(i);
		//更新用户信息
		TeaBarBriefData brief;
		brief.set_tbid(pdata->tbid());

		PBUpdateData update;
		update.set_key(PBUserDataField::kUserTeaBarData);
		PBDBAtomicField & field = *update.add_field_list();
		field.set_field(EN_DB_Field_TeaBar_Brief);
		field.set_strategy(EN_Update_Strategy_Del);
		field.mutable_tea_bar_brief()->CopyFrom(brief);
		psession->NewAddUpdateData(tbuser.uid(), update);
	}

	if (!TeaBarManager::Instance()->DeleteTeaBar(cs_request_free_tea_bar.tbid()))
	{
		ErrMsg("delete tea bar err,tbid[%ld]", cs_request_free_tea_bar.tbid());
	}

	response.set_result(EN_MESSAGE_ERROR_OK);
	response.set_tbid(cs_request_free_tea_bar.tbid());
	Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);

	return EN_Handler_Save;
}

ENHandlerResult CRequestFreeTeaBar::ProcessUpdateSucc(CHandlerTokenBasic* ptoken, CSession* psession)
{
	const CSRequestFreeTeaBar& cs_request_free_tea_bar = psession->_request_msg.cs_request_free_tea_bar();
	ErrMsg("free tea bar succ,tbid[%ld]", cs_request_free_tea_bar.tbid());
	return EN_Handler_Done;
}

ENHandlerResult CRequestFreeTeaBar::ProcessUpdateFailed(CHandlerTokenBasic* ptoken, CSession* psession)
{
	const CSRequestFreeTeaBar& cs_request_free_tea_bar = psession->_request_msg.cs_request_free_tea_bar();
	ErrMsg("free tea bar fail,tbid[%ld]", cs_request_free_tea_bar.tbid());
	return EN_Handler_Done;
}

ENHandlerResult CRequestStatisticsTableRecordList::ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession)
{
	long long uid = psession->_request_route.uid();
	int connect_id = psession->_request_route.source_id();
	const CSRequestStatisticsTableRecordList& cs_request_statistics_table_record_list = psession->_request_msg.cs_request_statistics_table_record_list();
	PBCSMsg msg;
	CSResponseStatisticsTableRecordList& response = *msg.mutable_cs_response_statistics_table_record_list();
	PBTeaBarData* pdata = TeaBarManager::Instance()->GetTeaBar(cs_request_statistics_table_record_list.tbid());
	if (pdata == NULL)
	{
		ErrMsg("tea bar not exist,tbid[%ld]", cs_request_statistics_table_record_list.tbid());
		response.set_result(EN_MESSAGE_TEA_BAR_NOT_EXIST);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}
	response.set_result(EN_MESSAGE_ERROR_OK);
	response.set_tbid(pdata->tbid());
	vector<PBTeaBarTable> vec;
	vec.clear();
	TeaBarManager::Instance()->GetAllTableStatistics(vec, *pdata);
	int idx = 0;
	bool is_finish = false;
	int tsize = (int)vec.size();
	for (int i = 0; i < tsize; i++)
	{
		CSTeaBarTableStatistics& stat = *response.add_stat_tables();
		stat.set_index(idx);
		stat.mutable_table()->CopyFrom(vec[i]);
		idx++;
		//测试代码
		/*if (idx == 21)
		{
		break;
		}*/
		//测试代码--end
		if (idx % 20 == 0)
		{
			if (idx == tsize)
			{
				response.set_is_end(true);
				Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
				is_finish = true;
			}
			else
			{
				Message::PushInnerMsg(RouteManager::Instance()->GetRouteByRandom(), uid, msg, EN_Node_Connect, connect_id);
				//清除信息继续发送
				response.clear_stat_tables();
			}
		}
	}

	if (!is_finish)
	{
		response.set_is_end(true);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
	}
	return EN_Handler_Done;
}

ENHandlerResult CRequestTeaBarStatistics::ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession)
{
	const CSRequestTeaBarTableStatistics& cs_request_tea_bar_statistics = psession->_request_msg.cs_request_tea_bar_statistics();
	PBCSMsg msg;
	CSResponseTeaBarTableStatistics& response = *msg.mutable_cs_response_tea_bar_statistics();
	PBTeaBarTableDetailStatistics* pstat = TeaBarManager::Instance()->GetTableStatistics(cs_request_tea_bar_statistics.statistics_id());
	if (pstat == NULL)
	{
		ErrMsg("GetTableStatistics error, %ld", cs_request_tea_bar_statistics.statistics_id());
		response.set_result(EN_MESSAGE_TEA_BAR_TABLE_STAT_NOT_EXIST);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}
	response.set_result(EN_MESSAGE_ERROR_OK);
	response.mutable_detail()->CopyFrom(*pstat);
	Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
	return EN_Handler_Done;
}

ENHandlerResult CRequestTeaBarTableSettle::ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession)
{
	long long uid = psession->_request_route.uid();
	const CSRequestTeaBarTableSettle& cs_request_tea_bar_table_settle = psession->_request_msg.cs_request_tea_bar_table_settle();
	PBCSMsg msg;
	CSResponseTeaBarTableSettle& response = *msg.mutable_cs_response_tea_bar_table_settle();
	PBTeaBarData* pdata = TeaBarManager::Instance()->GetTeaBar(cs_request_tea_bar_table_settle.tbid());
	if (pdata == NULL)
	{
		ErrMsg("tea bar not exist,tbid[%ld]", cs_request_tea_bar_table_settle.tbid());
		response.set_result(EN_MESSAGE_TEA_BAR_NOT_EXIST);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}

	if (uid != pdata->master_uid())
	{
		response.set_result(EN_MESSAGE_NOT_HAS_RIGHT);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}

	PBTeaBarTable* ptbtable = NULL;
	for (int i = 0; i < pdata->table_record_list_size(); i++)
	{
		PBDateTeaBarTableRecordList& date_list = *pdata->mutable_table_record_list(i);
		for (int j = 0; j < date_list.tables_size(); j++)
		{
			PBTeaBarTable& tbtable = *date_list.mutable_tables(j);
			if (tbtable.statistics_id() == cs_request_tea_bar_table_settle.statistics_id())
			{
				ptbtable = &tbtable;
				break;
			}
		}
	}

	if (ptbtable == NULL)
	{
		response.set_result(EN_MESSAGE_TEA_BAR_TABLE_STAT_NOT_EXIST);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}

	if (ptbtable->if_settled())
	{
		response.set_result(EN_MESSAGE_TEA_BAR_TABLE_STAT_SETTLED);
		Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
		return EN_Handler_Done;
	}

	ptbtable->set_if_settled(true);

	if (!TeaBarManager::Instance()->UpdateDataToRedis(*pdata))
	{
		ErrMsg("update redis failed,%ld", pdata->tbid());
	}
	response.set_result(EN_MESSAGE_ERROR_OK);
	response.set_tbid(cs_request_tea_bar_table_settle.tbid());
	response.set_statistics_id(cs_request_tea_bar_table_settle.statistics_id());
	Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
	return EN_Handler_Done;
}

ENHandlerResult CLogTeaBarChipsFlow::ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession)
{
	const LogTeaBarChipsFlow& flow = psession->_request_msg.log_tea_bar_chips_flow();
	PBTeaBarData* pdata = TeaBarManager::Instance()->GetTeaBar(flow.tbid());
	if (pdata == NULL)
	{
		ErrMsg("tea bar not exist,tbid[%ld]", flow.tbid());
		return EN_Handler_Done;
	}

	//数据统计
	PBDateTeaBarUserGameRecordList* pdatelist = TeaBarManager::Instance()->GetGameRecordList(*pdata, EN_TeaBar_Date_Type_Today);
	if (pdatelist == NULL)
	{
		ErrMsg("pdatelist NULL,tbid[%ld]", flow.tbid());
		return EN_Handler_Done;
	}
	if (flow.reason() == EN_Reason_Create_Tea_Bar_Table)
	{
		pdatelist->set_create_table_num(pdatelist->create_table_num()+1);
	}
	else if (flow.reason() == EN_Reason_Dissolve_Tea_Bar_Table)
	{
		pdatelist->set_create_table_num(pdatelist->create_table_num()-1);
	}
	pdatelist->set_cost_chips(pdatelist->cost_chips() - flow.act_num());
	return EN_Handler_Done;
}

ENHandlerResult CRequestTransferTeaBar::ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession)
{
	const SSRequestTransferTeaBar& ss_request_transfer_tea_bar = psession->_request_msg.ss_request_transfer_tea_bar();
	psession->NewAddGetData(ss_request_transfer_tea_bar.old_master_uid(), PBUserDataField::kUserTeaBarData);
	psession->NewAddGetData(ss_request_transfer_tea_bar.new_master_uid(), PBUserDataField::kUserTeaBarData);
	psession->NewAddGetData(ss_request_transfer_tea_bar.new_master_uid(), PBUserDataField::kUserInfo);
	return EN_Handler_Get;
}

ENHandlerResult CRequestTransferTeaBar::ProcessGetSucc(CHandlerTokenBasic* ptoken, CSession* psession)
{
	const SSRequestTransferTeaBar& ss_request_transfer_tea_bar = psession->_request_msg.ss_request_transfer_tea_bar();
	//1、取出用户的茶馆概要信息
	const PBUserTeaBarData & tboldbriefdata = psession->_kvdb_uid_data_map[ss_request_transfer_tea_bar.old_master_uid()].user_tea_bar_data();
	const PBUserTeaBarData & tbnewbriefdata = psession->_kvdb_uid_data_map[ss_request_transfer_tea_bar.new_master_uid()].user_tea_bar_data();
	const PBUser & user = psession->_kvdb_uid_data_map[ss_request_transfer_tea_bar.new_master_uid()].user_info();
	bool need_update = false;
	//添加老主人的所有创建的茶馆给新群主
	for (int i = 0; i < tboldbriefdata.brief_data_size(); i++)
	{
		const  TeaBarBriefData& old_brief = tboldbriefdata.brief_data(i);
		//不是老主人的茶馆不用处理
		if (old_brief.master_uid() != ss_request_transfer_tea_bar.old_master_uid())
		{
			continue;
		}
		PBTeaBarData* pdata = TeaBarManager::Instance()->GetTeaBar(old_brief.tbid());
		if (pdata == NULL)
		{
			continue;
		}
		need_update = true;
		bool is_in_tea_bar = false;
		for (int j = 0; j < tbnewbriefdata.brief_data_size(); j++)
		{
			if (old_brief.tbid() == tbnewbriefdata.brief_data(j).tbid())
			{
				//新群主在老的茶馆
				is_in_tea_bar = true;
				break;
			}
		}

		if (is_in_tea_bar)
		{
			//删除老的
			{
				TeaBarBriefData brief;
				brief.set_tbid(old_brief.tbid());

				PBUpdateData update;
				update.set_key(PBUserDataField::kUserTeaBarData);
				PBDBAtomicField & field = *update.add_field_list();
				field.set_field(EN_DB_Field_TeaBar_Brief);
				field.set_strategy(EN_Update_Strategy_Del);
				field.mutable_tea_bar_brief()->CopyFrom(brief);
				psession->NewAddUpdateData(ss_request_transfer_tea_bar.new_master_uid(), update);
			}
		}
		//增加新的
		{
			TeaBarBriefData brief;
			brief.CopyFrom(old_brief);
			brief.set_master_uid(ss_request_transfer_tea_bar.new_master_uid());

			PBUpdateData update;
			update.set_key(PBUserDataField::kUserTeaBarData);
			PBDBAtomicField & field = *update.add_field_list();
			field.set_field(EN_DB_Field_TeaBar_Brief);
			field.set_strategy(EN_Update_Strategy_Add);
			field.mutable_tea_bar_brief()->CopyFrom(brief);
			psession->NewAddUpdateData(ss_request_transfer_tea_bar.new_master_uid(), update);

			//修改茶馆主人信息
			pdata->set_master_uid(user.uid());
			pdata->set_master_name(user.nick());
			pdata->set_master_url(user.pic_url());
			//茶馆添加用户
			PBTeaBarUser* ptbuser = TeaBarManager::Instance()->GetUser(ss_request_transfer_tea_bar.new_master_uid(), *pdata);
			if (ptbuser == NULL)
			{
				if (!TeaBarManager::Instance()->AddUser(user, *pdata))
				{
					ErrMsg("Add user error, tbid[%ld], uid[%ld]", old_brief.tbid(), user.uid());
				}
				else
				{
					ErrMsg("Add user sucess, tbid[%ld], uid[%ld]", old_brief.tbid(), user.uid());
				}
			}
		}

		//删除老主人的茶馆
		{
			TeaBarBriefData brief;
			brief.set_tbid(old_brief.tbid());

			PBUpdateData update;
			update.set_key(PBUserDataField::kUserTeaBarData);
			PBDBAtomicField & field = *update.add_field_list();
			field.set_field(EN_DB_Field_TeaBar_Brief);
			field.set_strategy(EN_Update_Strategy_Del);
			field.mutable_tea_bar_brief()->CopyFrom(brief);
			psession->NewAddUpdateData(ss_request_transfer_tea_bar.old_master_uid(), update);
			////茶馆删除用户
			//if (!TeaBarManager::Instance()->DeleteUser(ss_request_transfer_tea_bar.old_master_uid(), *pdata))
			//{
			//	ErrMsg("delete user error,tbid[%ld],uid:[%ld]", pdata->tbid(), ss_request_transfer_tea_bar.old_master_uid());
			//}
			//else
			//{
			//	ErrMsg("delete user succ,tbid[%ld],uid:[%ld]", pdata->tbid(), ss_request_transfer_tea_bar.old_master_uid());
			//}
		}
		//老号成为茶馆成员
		{
			TeaBarBriefData brief;
			brief.set_tbid(pdata->tbid());
			brief.set_tbname(pdata->tbname());
			brief.set_master_uid(pdata->master_uid());

			PBUpdateData update;
			update.set_key(PBUserDataField::kUserTeaBarData);
			PBDBAtomicField & field = *update.add_field_list();
			field.set_field(EN_DB_Field_TeaBar_Brief);
			field.set_strategy(EN_Update_Strategy_Add);
			field.mutable_tea_bar_brief()->CopyFrom(brief);
			psession->NewAddUpdateData(ss_request_transfer_tea_bar.old_master_uid(), update);
		}

		if (!TeaBarManager::Instance()->UpdateDataToRedis(*pdata))
		{
			ErrMsg("update redis failed,%ld", pdata->tbid());
		}
		else
		{
			ErrMsg("update redis succ,%ld", pdata->tbid());
		}
	}
	if (need_update)
	{
		return EN_Handler_Save;
	}
	return EN_Handler_Done;
}

ENHandlerResult CRequestTransferTeaBar::ProcessUpdateSucc(CHandlerTokenBasic* ptoken, CSession* psession)
{
	const SSRequestTransferTeaBar& ss_request_transfer_tea_bar = psession->_request_msg.ss_request_transfer_tea_bar();
	const PBUserTeaBarData & tboldbriefdata = psession->_kvdb_uid_data_map[ss_request_transfer_tea_bar.old_master_uid()].user_tea_bar_data();
	const PBUserTeaBarData & tbnewbriefdata = psession->_kvdb_uid_data_map[ss_request_transfer_tea_bar.new_master_uid()].user_tea_bar_data();
	for (int i = 0; i < tboldbriefdata.brief_data_size(); i++)
	{
		ErrMsg("transfer teaBar old, tbid[%ld], uid[%ld]", tboldbriefdata.brief_data(i).tbid(), tboldbriefdata.brief_data(i).master_uid());
	}

	for (int i = 0; i < tbnewbriefdata.brief_data_size(); i++)
	{
		ErrMsg("transfer teaBar new, tbid[%ld], uid[%ld]", tbnewbriefdata.brief_data(i).tbid(), tbnewbriefdata.brief_data(i).master_uid());
	}
	return EN_Handler_Done;
}

ENHandlerResult CRequestTransferTeaBar::ProcessUpdateFailed(CHandlerTokenBasic* ptoken, CSession* psession)
{
	const SSRequestTransferTeaBar& ss_request_transfer_tea_bar = psession->_request_msg.ss_request_transfer_tea_bar();
	const PBUserTeaBarData & tboldbriefdata = psession->_kvdb_uid_data_map[ss_request_transfer_tea_bar.old_master_uid()].user_tea_bar_data();
	const PBUserTeaBarData & tbnewbriefdata = psession->_kvdb_uid_data_map[ss_request_transfer_tea_bar.new_master_uid()].user_tea_bar_data();
	for (int i = 0; i < tboldbriefdata.brief_data_size(); i++)
	{
		ErrMsg("transfer teaBar old, tbid[%ld], uid[%ld]", tboldbriefdata.brief_data(i).tbid(), tboldbriefdata.brief_data(i).master_uid());
	}

	for (int i = 0; i < tbnewbriefdata.brief_data_size(); i++)
	{
		ErrMsg("transfer teaBar new, tbid[%ld], uid[%ld]", tbnewbriefdata.brief_data(i).tbid(), tbnewbriefdata.brief_data(i).master_uid());
	}
	return EN_Handler_Done;
}

ENHandlerResult CGMRequestQueryTeaBarUserList::ProcessRequestMsg(CHandlerTokenBasic *ptoken, CSession *psession)
{
    const GMRequestQueryTeaBarUserList & gm_request_query_teabar_user_list = psession->_request_msg.gm_request_query_teabar_user_list();
    int64 lTbid = gm_request_query_teabar_user_list.tbid();
    PBTeaBarData* pdata = TeaBarManager::Instance()->GetTeaBar(lTbid);
    PBCSMsg pbMsg;
    GMResponseQueryTeaBarUserList & gm_response_query_teabar_user_list = *pbMsg.mutable_gm_response_query_teabar_user_list();

    if(!pdata)
    {
        ErrMsg("tea bar not exist,tbid[%lld]",lTbid);
        gm_response_query_teabar_user_list.set_result(EN_MESSAGE_TEA_BAR_NOT_EXIST);
        Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, pbMsg);
        return EN_Handler_Done;
    }

    for(int i = 0 ; i < pdata->users_size() ; i ++)
    {
        const PBTeaBarUser & pbTbUser = pdata->users(i);
        gm_response_query_teabar_user_list.add_uids(pbTbUser.uid());
    }
    gm_response_query_teabar_user_list.set_result(EN_MESSAGE_ERROR_OK);
    Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, pbMsg);

    return EN_Handler_Done;
}

ENHandlerResult CRequestChangeTeabarCreateInfo::ProcessRequestMsg(CHandlerTokenBasic* ptoken, CSession* psession)
{
    psession->NewAddGetData(psession->_request_route.uid(), PBUserDataField::kUserTeaBarData);
    return EN_Handler_Get;
}

ENHandlerResult CRequestChangeTeabarCreateInfo::ProcessGetSucc(CHandlerTokenBasic* ptoken, CSession* psession)
{
    long long uid = psession->_request_route.uid();
    const CSRequestChangeTeabarCreateInfo& cs_request_change_teabar_create_info = psession->_request_msg.cs_request_change_teabar_create_info();
//    const PBUser & user = psession->_kvdb_uid_data_map[uid].user_info();
//    const PBUserTeaBarData& usertbdata = psession->_kvdb_uid_data_map[uid].user_tea_bar_data();
    PBCSMsg msg;
    CSResponseChangeTeabarCreateInfo& response = *msg.mutable_cs_response_change_teabar_create_info();
//    TeaBarBriefData pbrief;

    response.set_tbid(cs_request_change_teabar_create_info.tbid());
    response.set_teabar_create_info(cs_request_change_teabar_create_info.teabar_create_info());
    response.set_game_type(cs_request_change_teabar_create_info.game_type());
    PBTeaBarData* pdata = TeaBarManager::Instance()->GetTeaBar(cs_request_change_teabar_create_info.tbid());
    if (pdata == NULL)
    {
        response.set_result(EN_MESSAGE_TEA_BAR_NOT_EXIST);
        Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
        return EN_Handler_Done;
    }

    if (pdata->master_uid() != uid)
    {
        response.set_result(EN_MESSAGE_NOT_HAS_RIGHT);
        Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);
        return EN_Handler_Done;
    }

    TeabarRoomCreateInfo create_info_ch;
    create_info_ch.set_game_type(cs_request_change_teabar_create_info.game_type());
    create_info_ch.set_create_info(cs_request_change_teabar_create_info.teabar_create_info());

    bool has_info = false;
    for (int i = 0; i < pdata->teabar_room_create_info_size(); i++)
    {
        if (pdata->teabar_room_create_info(i).game_type() ==
               cs_request_change_teabar_create_info.game_type())
        {
            TeabarRoomCreateInfo& create_info = *pdata->mutable_teabar_room_create_info(i);
            create_info.CopyFrom(create_info_ch);
            has_info = true;
            response.set_result(EN_MESSAGE_ERROR_OK);
            break;
        }
    }

    if (!has_info)
    {
        TeabarRoomCreateInfo& create_info = *pdata->add_teabar_room_create_info();
        response.set_result(EN_MESSAGE_ERROR_OK);
        create_info.CopyFrom(create_info_ch);
    }

    Message::SendResponseMsg(RouteManager::Instance()->GetRouteByRandom(), psession, msg);

    return EN_Handler_Done;
}

