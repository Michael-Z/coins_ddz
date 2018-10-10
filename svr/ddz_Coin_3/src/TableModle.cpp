#include "TableModle.h"
#include "TableManager.h"
#include "GameHandlerProxy.h"
#include "PBConfigBasic.h"
#include "GlobalRedisClient.h"
#include "LogWriter.h"
#include "RecordRedisClient.h"
#include "PokerLogic.h"
#include "ActionBehaviorManager.h"

PBSDRTableSeat * TableLogic::FindEmptySeatInTable(CPBGameTable & table)
{
    PBSDRTableSeat * pSeat = NULL;
    for (int i=0;i<table.seats_size();i++)
    {
        PBSDRTableSeat & seat = *table.mutable_seats(i);
        if (seat.state() == EN_SEAT_STATE_NO_PLAYER)
        {
            pSeat = & seat;
            break;
        }
    }
    return pSeat;
}

PBTableUser * TableLogic::FindUserInTable(CPBGameTable & table ,long long uid)
{
    PBTableUser * pUser = NULL;
    for (int i=0;i<table.seats_size();i++)
    {
        PBSDRTableSeat & seat = *table.mutable_seats(i);
        if(seat.state() == EN_SEAT_STATE_NO_PLAYER)
        {
            continue;
        }
        PBTableUser & user = *seat.mutable_user();
        if (user.uid() == uid)
        {
            pUser = &user;
            break;
        }
    }
    return pUser;
}

int TableLogic::FindIndexByUid(const CPBGameTable & table,long long uid)
{
    for (int i=0;i<table.seats_size();i++)
    {
        if(table.seats(i).state() != EN_SEAT_STATE_NO_PLAYER && table.seats(i).user().uid()==uid) return i;
    }
    return -1;
}


void TableLogic::OnUserReconnect(CPBGameTable & table,long long uid, int connect_id /*= 1*/)
{
    int index = FindIndexByUid(table,uid);
    if(index != -1)
    {
        table.table_log << "玩家[" << uid << "] 重新链接" << "\n";
        ErrMsg("uid[%lld] reconnect to table[%ld]", uid, table.tid());

        PBSDRTableSeat & seat = *table.mutable_seats(index);
        //需要将托管状态置为非托管状态
        //DisableTrusteeship(table,seat);
        PBTableUser & user = *seat.mutable_user();
        user.set_is_offline(false);
        user.set_is_backend(false);
        user.set_connect_id(connect_id);
        PBCSMsg notify;
        CSNOtifyPlayerConnectionState & cs_notify_player_connection_state = *notify.mutable_cs_notify_player_connection_state();
        cs_notify_player_connection_state.set_seat_index(index);
        cs_notify_player_connection_state.set_connection_state(EN_Connection_State_Online);
        TableLogic::BroadcastTableMsg(table,notify,uid);
    }
}

void TableLogic::SitDownOnTable(CPBGameTable & table,PBSDRTableSeat & seat,const PBUser & user, int connect_id /*= 1*/)
{
    //如果这个桌子上没有人，则需要将start_stamp置为现在时间
    if(GetPlayerNum(table) == 0)
    {
        table.start_stamp = time(NULL);
    }

    //玩家坐下
    PBTableUser & tableuser = *seat.mutable_user();
    tableuser.set_uid(user.uid());
    tableuser.set_nick(user.nick());
    tableuser.set_role_picture_url(user.pic_url());
    tableuser.set_acc_type(user.acc_type());
    tableuser.set_channel(user.channel());
    tableuser.set_last_login_ip(user.last_login_ip());
    tableuser.set_gender(user.gender());
    tableuser.set_chip(user.chips());
    tableuser.set_connect_id(connect_id);
    //tableuser.set_items_info(user.items_info());
    tableuser.set_diamond(user.diamond());
    tableuser.set_bonus(user.bonus());
    tableuser.set_total_bonus(user.total_bonus());
	tableuser.set_is_robot(user.acc_type() == EN_Account_Robot);
	tableuser.set_coins(user.coins());

	////如果是机器人
	//if (user.acc_type() == EN_Account_Robot)
	//{
	//	float fWinRaite = float((rand() % 20 + 30)) / 100;	//30% ~ 50%
	//	int iGameNum = rand() % 150 + 50;			//50 ~ 200
	//	int iSuccessSkipNum = iGameNum * fWinRaite;	
	//	int iWinNum = iSuccessSkipNum * 5 / 100;
	//	float iTotalBonus = float(iWinNum * 55) / 100;

	//	tableuser.set_skipmatch_game_num(iGameNum);
	//	tableuser.set_skipmatch_success_skip_level_num(iSuccessSkipNum);
 //       tableuser.set_skipmatch_total_win_num_on_type(iWinNum);
	//	tableuser.set_total_bonus(iTotalBonus);
	//	tableuser.set_bonus(iTotalBonus);
	//}

    seat.set_state(EN_SEAT_STATE_WAIT_FOR_NEXT_ONE_GAME);

//    //玩家进入桌子后自动准备,置为准备状态
//    seat.set_state(EN_SEAT_STATE_READY_FOR_NEXT_ONE_GAME);

    seat.set_final_score(0);
    seat.set_total_score(0);

    //广播玩家坐下
    PBCSMsg msg;
    CSNotifySitDown & notify_sit_down = *msg.mutable_cs_notify_sit_down();
    notify_sit_down.mutable_sdr_seat()->CopyFrom(seat);
    BroadcastTableMsg(table, msg, user.uid());

    TableManager::Instance()->OnPlayerEnterTable(user.uid(),table.tid());

    //如果是茶馆房间，刷新下茶馆人数
    if (table.config().has_tbid() && table.config().has_master_uid())
    {
        PBCSMsg notify;
        SSNotifyTeaBarTablePlayerNum& ss_notify_teabar_table_player_num = *notify.mutable_ss_notify_teabar_table_player_num();
        ss_notify_teabar_table_player_num.mutable_conf()->CopyFrom(table.config());
        ss_notify_teabar_table_player_num.set_tid(table.tid());
        ss_notify_teabar_table_player_num.set_player_num(table.seats_size() - GetPlayerNumByState(table, EN_SEAT_STATE_NO_PLAYER));
        Message::PushInnerMsg(RouteManager::Instance()->GetRouteByRandom(), 0, notify, EN_Node_TeaBar, 1);
    }
    return;
}

bool TableLogic::IsPlayerAbleDoAction(const PBSDRTableSeat & seat,PBSDRAction & action,int token)
{
    if(seat.index() != action.seat_index())
    {
        VLogMsg(CLIB_LOG_LEV_ERROR, "uid[%ld] seat_index err ==========================", seat.user().uid());
        return false;
    }
    const PBSDRActionChoice & seat_action_choice = seat.action_choice();
    if(seat_action_choice.is_determine() == true)
    {
        VLogMsg(CLIB_LOG_LEV_ERROR, "uid[%ld] is_determine err ==========================", seat.user().uid());
        return false;
    }
    if(seat.action_token() != token)
    {
        VLogMsg(CLIB_LOG_LEV_ERROR, "uid[%ld] action_token err ==========================", seat.user().uid());
        return false;
    }

    for(int i=0;i<seat_action_choice.choices_size();i++)
    {
        const PBSDRAction & action_choice = seat_action_choice.choices(i);
        if(action_choice.tid() == action.tid() && action_choice.seat_index() == action.seat_index())
        {
            VLogMsg(CLIB_LOG_LEV_DEBUG, " choices[%d]==========================", i);
            if(action_choice.act_type() != action.act_type())
            {
                continue;
            }
            VLogMsg(CLIB_LOG_LEV_DEBUG, " act_type[%d]==========================", action.act_type());
            bool match = true;
            // 出牌操作
            if(action_choice.act_type() == EN_SDR_ACTION_CHUPAI)
            {
                RepeatedField<int> hand_cards = seat.hand_cards();
                for (int j = 0; j < action.col_info().cards_size(); j++)
                {
                    int card = action.col_info().cards(j);
                    if (std::find(hand_cards.begin(), hand_cards.end(), card) == hand_cards.end())
                    {
                        match = false;
                        VLogMsg(CLIB_LOG_LEV_DEBUG,"not find dest card[%d] ...", card);
                        break;
                    }
                }
                return match;
            }
            else    // 非出牌操作
            {
                return true;
            }
        }
    }

    VLogMsg(CLIB_LOG_LEV_DEBUG,"------------- seat_action_choice.choices_size[%d] -------------------", seat_action_choice.choices_size());
    return false;
}

int TableLogic::GetLastOperateSeatIndex(const CPBGameTable & table)
{
    if(table.total_action_flows_size() == 0)
    {
        return -1;
    }
    const PBSDRActionFlow & current_flow = table.total_action_flows(table.total_action_flows_size()-1);
    const PBSDRAction & current_action = current_flow.action();
    int index = current_action.seat_index();
    return index;
}

int TableLogic::DetermineAction(CPBGameTable & table,PBSDRTableSeat & seat,const PBSDRAction & request_action)
{
    PBSDRActionChoice & seat_action_choice = *seat.mutable_action_choice();
    bool find_choice = false;
    for (int i = 0; i < seat_action_choice.choices_size(); i++)
    {
        if (seat_action_choice.choices(i).act_type() == request_action.act_type())
        {
            find_choice = true;
            break;
        }
    }

    if (!find_choice)
    {
        // 操作不匹配
        VLogMsg(CLIB_LOG_LEV_ERROR,"操作不匹配");
        return -1;
    }

    // TODO 分析出牌类型，检查出牌合法性
//    ENPokerType poker_type = EN_POKER_TYPE_UNKONWN;
//    int poker_val = 0;
    if (request_action.act_type() == EN_SDR_ACTION_CHUPAI)
    {
        RepeatedField<int> cards;
        cards.CopyFrom(request_action.col_info().cards());
        if (!PokerLogic::AnalyseFromCards(table, seat.index(), cards,
                                        (ENPokerType)request_action.cardtype(), request_action.real(), request_action.num()))
        {
            VLogMsg(CLIB_LOG_LEV_DEBUG,"牌型不合法[type:%d, val:%d]", request_action.cardtype(), request_action.real());
            return -1;
        }
        if (request_action.cardtype() == EN_POKER_TYPE_BOMB || request_action.cardtype() == EN_POKER_TYPE_BOMB_OF_JOKER)
        {
            table.set_bomb_num(table.bomb_num()+1);
            seat.set_bomb_num(seat.bomb_num()+1);
        }
        if (request_action.cardtype() == EN_POKER_TYPE_SOFT_BOMB || request_action.cardtype() == EN_POKER_TYPE_SOFT_BOMB_OF_JOKER)
        {
            table.set_soft_bomb_num(table.soft_bomb_num()+1);
            seat.set_soft_bomb_num(seat.soft_bomb_num()+1);
        }
//        else if (request_action.cardtype() == EN_POKER_TYPE_TRIPLE)
//        {
//            // 三不带，只能最后出
//            if (seat.hand_cards_size() != cards.size())
//            {
//                VLogMsg(CLIB_LOG_LEV_DEBUG,"牌型不合法[type:%d, val:%d]", request_action.cardtype(), request_action.real());
//                return -1;
//            }
//        }
//        else if (request_action.cardtype() == EN_POKER_TYPE_STRAIGHT_3 && cards.size()/request_action.num() == 3)
//        {
//            // 三顺不带，只能最后出
//            if (seat.hand_cards_size() != cards.size())
//            {
//                VLogMsg(CLIB_LOG_LEV_DEBUG,"牌型不合法[type:%d, val:%d]", request_action.cardtype(), request_action.real());
//                return -1;
//            }
//        }
    }

    // 设置seat determined action
    seat_action_choice.set_determined_level(CalcChoiceLevel(request_action));
    seat_action_choice.mutable_determined_action()->CopyFrom(request_action);
    seat_action_choice.set_is_determine(true);
    VLogMsg(CLIB_LOG_LEV_DEBUG,"request_action: determined_level[%d] ...", seat_action_choice.determined_level());

    // 设置 action
    if (request_action.act_type() == EN_SDR_ACTION_CHUPAI && table.total_action_flows_size() > 0)
    {
        PBSDRActionFlow & current_flow = *table.mutable_total_action_flows(table.total_action_flows_size()-1);
        PBSDRAction & current_action = *current_flow.mutable_action();
        if (current_action.act_type() != EN_SDR_ACTION_CHUPAI)
        {
            // 操作不匹配
            VLogMsg(CLIB_LOG_LEV_ERROR,"操作不匹配");
            return -1;
        }
        current_action.mutable_col_info()->mutable_cards()->CopyFrom(request_action.col_info().cards());
        current_action.set_poker_type((ENPokerType)request_action.cardtype());
        current_action.set_poker_val(request_action.real());
        // 透传数据
        current_action.set_cardtype(request_action.cardtype());
        current_action.set_real(request_action.real());
        current_action.set_num(request_action.num());
        current_action.mutable_cards()->CopyFrom(request_action.cards());
        {
            for (int i = table.replay_action_flows_size()-1; i >= 0; i--)
            {
                PBSDRActionFlow & current_flow = *table.mutable_replay_action_flows(i);
                PBSDRAction & current_action = *current_flow.mutable_action();
                if (current_action.act_type() == EN_SDR_ACTION_CHUPAI)
                {
                    current_action.mutable_col_info()->mutable_cards()->CopyFrom(request_action.col_info().cards());
                    current_action.set_poker_type((ENPokerType)request_action.cardtype());
                    current_action.set_poker_val(request_action.real());
                    // 透传数据
                    current_action.set_cardtype(request_action.cardtype());
                    current_action.set_real(request_action.real());
                    current_action.set_num(request_action.num());
                    current_action.mutable_cards()->CopyFrom(request_action.cards());
                    break;
                }
            }
        }
    }


    return 0;
}

int TableLogic::ProcessChuPaiForCurrentSeat(CPBGameTable & table)
{
    VLogMsg(CLIB_LOG_LEV_DEBUG,"---------------出牌--------------------");
    const PBSDRActionFlow & current_flow = table.total_action_flows(table.total_action_flows_size()-1);
    const PBSDRAction & current_action = current_flow.action();
    int index = current_action.seat_index();
    if(index<0 || index>table.seats_size()-1)
    {
        VLogMsg(CLIB_LOG_LEV_DEBUG,"index invalid ...");
        return -1;
    }
    VLogMsg(CLIB_LOG_LEV_DEBUG, "SendInterEventOnDoActionOver 本人[%d]出牌操作 ...", index);

    // 广播等待操作
    BroadcastNextOperation(table, index);
    table.set_operation_index(index);

    // 清掉选择列表
    ResetPlayerChoice(table);

    // 添加出牌到选择列表
    PBSDRTableSeat & seat = *table.mutable_seats(index);
    PBSDRAction action;
    action.set_tid(table.tid());
    action.set_seat_index(index);
    action.set_act_type(EN_SDR_ACTION_CHUPAI);
    AddChoise(seat, action);

    // 添加到操作流
    AppendActionFlow(table, seat, action);
    const PBSDRAction* p_action = TableLogic::GetLastDeterminedActionChuPai(table);
    if (p_action == NULL || p_action->seat_index() != index)
    {
        action.set_act_type(EN_SDR_ACTION_PASS);
        AddChoise(seat, action);
    }

	OnOperateAfterAddChoice(table, seat);
    return 0;
}

int TableLogic::ProcessChuPaiForNextSeat(CPBGameTable & table)
{
    VLogMsg(CLIB_LOG_LEV_DEBUG,"---------------出牌--------------------");
    if (table.total_action_flows_size() < 1)
    {
        VLogMsg(CLIB_LOG_LEV_DEBUG,"total_action_flows_size == 0 ...");
        return -1;
    }
    const PBSDRActionFlow & current_flow = table.total_action_flows(table.total_action_flows_size()-1);
    const PBSDRAction & current_action = current_flow.action();
    int index = (current_action.seat_index()+1)%table.seats_size();
    if(index<0 || index>table.seats_size()-1)
    {
        VLogMsg(CLIB_LOG_LEV_DEBUG,"index invalid ...");
        return -1;
    }
    VLogMsg(CLIB_LOG_LEV_DEBUG, "SendInterEventOnDoActionOver 下家[%d]出牌操作 ...", index);

    // 广播等待操作
    BroadcastNextOperation(table, index);
    table.set_operation_index(index);

    // 清掉选择列表
    ResetPlayerChoice(table);

    // 添加出牌到选择列表
    PBSDRTableSeat & seat = *table.mutable_seats(index);
    PBSDRAction action;
    action.set_tid(table.tid());
    action.set_seat_index(index);
    action.set_act_type(EN_SDR_ACTION_CHUPAI);
    AddChoise(seat, action);
    // 添加到操作流
    AppendActionFlow(table, seat, action);

    const PBSDRAction* p_action = TableLogic::GetLastDeterminedActionChuPai(table);
    if (p_action != NULL && p_action->seat_index() != index)
    {
        action.set_act_type(EN_SDR_ACTION_PASS);
        AddChoise(seat, action);
    }

	OnOperateAfterAddChoice(table, seat);

    return 0;
}

bool TableLogic::PlayerDoAction(CPBGameTable & table,PBSDRTableSeat & seat, PBSDRAction & request_action)
{
    VLogMsg(CLIB_LOG_LEV_DEBUG,"player[%ld] process action[%d] on table[%ld]", seat.user().uid(), request_action.act_type(), table.tid());
    if (table.is_game_over())
    {
        return true;
    }

    bool ret = false;
    switch (request_action.act_type())
    {
        case EN_SDR_ACTION_CHUPAI:  // 出牌
            ret = PlayerDoActionChupai(table, seat, request_action);
            break;
        case EN_SDR_ACTION_PASS:        // 过
        case EN_SDR_ACTION_BU_QIANG:    // 不抢地主
        case EN_SDR_ACTION_BU_JIA_BEI:  // 不加倍
        case EN_SDR_ACTION_BU_MING:  // 不明牌
        case EN_SDR_ACTION_BU_JIAO:  // 不叫
            ret = PlayerDoActionPass(table, seat, request_action);
            break;
        case EN_SDR_ACTION_QIANG_DI_ZHU:     // 抢地主
        case EN_SDR_ACTION_JIAO_DI_ZHU:     // 叫地主
            ret = PlayerDoActionQiangDiZhu(table, seat, request_action);
            break;
        default:
            VLogMsg(CLIB_LOG_LEV_DEBUG," invalid act_type[%d]", request_action.act_type());
            break;
    }
    // 到这儿表示操作成功了
    VLogMsg(CLIB_LOG_LEV_DEBUG," action:act_type[%d] 处理完成 ...", request_action.act_type());

    // ret 为 false 的话，就不调用 ResetPlayerChoice
    if (ret)
    {
        // 清掉选择列表
        ResetPlayerChoice(table);
    }

    // 清理max_pre_choice
    table.clear_max_pre_choice();

    return ret;
}

bool TableLogic::PlayerDoActionMingPai(CPBGameTable & table,PBSDRTableSeat & seat, PBSDRAction & action)
{
    // 抓牌相关逻辑
    VLogMsg(CLIB_LOG_LEV_DEBUG, "PlayerDoActionMingPai ...");

    int choice_mingpai_num = 0;

    if (table.first_mingpai_seat() == -1)
    {
        table.set_first_mingpai_seat(seat.index());
    }

    AppendActionFlow(table, seat, action);
    BroadcastActionFlow(table);

    if (table.state() == EN_TABLE_STATE_WAIT_MING_PAI_2)
    {
//        seat.set_is_mingpai(1);
        seat.set_is_mingpai_after_deal_cards(1);

        PBCSMsg notify;
        CSNotifyMingPai & cs_notify_mingpai = *notify.mutable_cs_notify_ming_pai();
        cs_notify_mingpai.set_seat_index(seat.index());
        cs_notify_mingpai.mutable_card()->CopyFrom(seat.hand_cards());
        BroadcastTableMsg(table, notify);

        for (int i = 0 ; i < table.seats_size(); i++)
        {
            PBSDRTableSeat & o_seat = *table.mutable_seats(i);
            if (o_seat.is_mingpai_after_deal_cards() >= 0)
            {
                choice_mingpai_num++;
            }
        }
        if (choice_mingpai_num >= table.seats_size())
        {
            table.set_operation_index(table.dealer_index());
            SendInterEventOnDoActionOver(table,6, action);
        }
    }
//    GameStart_3(table);
    return false;
}

bool TableLogic::PlayerDoActionFanDiPai(CPBGameTable & table,PBSDRTableSeat & seat, PBSDRAction & action)
{
    // 抓牌相关逻辑
    VLogMsg(CLIB_LOG_LEV_DEBUG, "PlayerDoActionFanDiPai ...");

    PBSDRAction action_fan;
    action_fan.set_tid(table.tid());
    action_fan.set_seat_index(table.dealer_index());
    action_fan.set_act_type(EN_SDR_ACTION_FAN_DI_PAI);
    action_fan.mutable_col_info()->mutable_cards()->CopyFrom(table.cards());
    AppendActionFlow(table, seat, action_fan);

    // 记录抢庄
    table.set_dealer_index_2(table.dealer_index());
    table.set_dealer_index(seat.index());
    seat.set_is_baozi_zhuang(true);

    BroadcastActionFlow(table);
    SendInterEventOnDoActionOver(table,6, action);
//    GameStart_3(table);
    return true;
}

bool TableLogic::PlayerDoActionQiangDiZhu(CPBGameTable & table,PBSDRTableSeat & seat, PBSDRAction & action)
{
    VLogMsg(CLIB_LOG_LEV_DEBUG,"-----------------------------------");

    if (action.act_type() == EN_SDR_ACTION_QIANG_DI_ZHU)
    {
        seat.set_qiang_di_zhu_times(seat.qiang_di_zhu_times()+10);
    }
    else
    {
        seat.set_qiang_di_zhu_times(seat.qiang_di_zhu_times()+1);
    }

    AppendActionFlow(table, seat, action);
    BroadcastActionFlow(table);

    if (action.act_type() == EN_SDR_ACTION_JIAO_DI_ZHU)
    {
        table.set_jiao_dizhu_index(seat.index());
        seat.set_is_jiao_dizhu(true);
    }
    bool is_bi_zhua = true;
    for (int i = 0; i < seat.action_choice().choices_size(); i++)
    {
        if (seat.action_choice().choices(i).act_type() == EN_SDR_ACTION_BU_QIANG)
        {
            is_bi_zhua = false;
        }
    }
    if (is_bi_zhua)
    {
        seat.set_is_baozi_zhuang(true);
    }
    table.set_dealer_index_2(seat.index());

    //动作操作后处理托管状态
    if(DealWithTrusteeshipAfterAct(table,seat,action))
    {
        return true;
    }

    VLogMsg(CLIB_LOG_LEV_DEBUG, "玩家[%d] 抢庄 ...", seat.index());
    if (table.state() == EN_TABLE_STATE_WAIT_QIANG_DI_ZHU)
    {
        // 回调 GameStart_2
        SendInterEventOnDoActionOver(table,6, action);
        VLogMsg(CLIB_LOG_LEV_DEBUG, "SendInterEventOnDoActionOver 玩家[%d] 抢庄 回调 GameStart_2 ...", seat.index());
    }

    return true;
}

bool TableLogic::PlayerDoActionMianZhan(CPBGameTable & table,PBSDRTableSeat & seat, PBSDRAction & action)
{
    VLogMsg(CLIB_LOG_LEV_DEBUG,"-----------------------------------");

    seat.mutable_win_info()->add_styles(EN_SDR_STYLE_TYPE_Mian_Zhan);
    GameOver(table);

    return true;
}

bool TableLogic::PlayerDoActionChupai(CPBGameTable & table,PBSDRTableSeat & seat, PBSDRAction & action)
{
    VLogMsg(CLIB_LOG_LEV_DEBUG,"-----------------------------------");

    for (int i = 0; i < action.col_info().cards_size(); i++)
    {
        int card = action.col_info().cards(i);
        // 1. 移除要出的牌
        RemoveCardForSeat(seat, card);
    }
    // 2. 广播最后一次操作流
    BroadcastActionFlow(table, EN_SDR_ACTION_CHUPAI);

    // 报警
    if (seat.hand_cards_size() <= 2 && seat.hand_cards_size() > 0)
    {
        seat.set_bao_type(seat.hand_cards_size());

        PBSDRAction tmp_action;
        tmp_action.set_tid(table.tid());
        tmp_action.set_seat_index(seat.index());
        tmp_action.set_act_type(EN_SDR_ACTION_BAO);
        tmp_action.mutable_cards()->Resize(seat.hand_cards_size(), 0);

        // 凭空构造的action_flow
        PBCSMsg notify;
        CSNotifyActionFlow & cs_notify_action_flow = *notify.mutable_cs_notify_action_flow();
        PBSDRActionFlow & notify_new_action_flow = *cs_notify_action_flow.mutable_new_sdr_action_flow();
        notify_new_action_flow.mutable_action()->CopyFrom(tmp_action);
        BroadcastTableMsg(table, notify);
    }

    if (seat.hand_cards_size() <= 0)
    {
        table.set_last_winner(seat.index());
        GameOver(table);
        return true;
    }

    {
        table.table_log<<"玩家["<<action.seat_index()<<"] ";
        table.table_log<<ActionToString(action)<<"\n";
    }

    //动作操作后处理托管状态
    if(DealWithTrusteeshipAfterAct(table,seat,action))
    {
        return true;
    }

	//如果是机器人，出完牌后重新计算一下手数
	if (seat.user().acc_type() == EN_Account_Robot)
	{
		AnalyzeModle::AnalyzeSeatPaiZhang(table, seat);
	}

    // 下家出牌
    SendInterEventOnDoActionOver(table,2, action);

    return true;
}

bool TableLogic::PlayerDoActionPass(CPBGameTable & table,PBSDRTableSeat & seat, PBSDRAction & action)
{
    VLogMsg(CLIB_LOG_LEV_DEBUG,"-------------  过牌 ----------------------");

//    {
//        table.table_log<<"玩家["<<action.seat_index()<<"] ";
//        table.table_log<<ActionToString(action)<<"\n";
//    }
    // 如果处于抢地主阶段 这里处理继续下一个人抢地主
    if (table.state() == EN_TABLE_STATE_WAIT_QIANG_DI_ZHU)
    {
        seat.set_qiang_di_zhu_times(seat.qiang_di_zhu_times()+1);
        AppendActionFlow(table, seat, action);
        BroadcastActionFlow(table);

        //动作操作后处理托管状态
        if(DealWithTrusteeshipAfterAct(table,seat,action))
        {
            return true;
        }

        SendInterEventOnDoActionOver(table,6, action);
        return true;
    }

    // 如果处于加倍阶段 这里处理继续下一个人加倍
    if (table.state() == EN_TABLE_STATE_WAIT_DOUBLE)
    {
        int choice_jiabei_num = 0;
        AppendActionFlow(table, seat, action);
        BroadcastActionFlow(table);

        seat.set_double_times(0);

        for (int i = 0 ; i < table.seats_size(); i++)
        {
            PBSDRTableSeat & o_seat = *table.mutable_seats(i);
            if (o_seat.double_times() >= 0)
            {
                choice_jiabei_num++;
            }
        }

        //动作操作后处理托管状态
        if(DealWithTrusteeshipAfterAct(table,seat,action))
        {
            return true;
        }

        if (choice_jiabei_num >= table.seats_size())
        {
            SendInterEventOnDoActionOver(table,5, action);
        }
        return false;
    }

    if (table.state() == EN_TABLE_STATE_WAIT_MING_PAI_2)
    {
        int choice_mingpai_num = 0;
        AppendActionFlow(table, seat, action);
        BroadcastActionFlow(table);
//        seat.set_is_mingpai(0);
        seat.set_is_mingpai_after_deal_cards(0);

        for (int i = 0 ; i < table.seats_size(); i++)
        {
            PBSDRTableSeat & o_seat = *table.mutable_seats(i);
            if (o_seat.is_mingpai_after_deal_cards() >= 0)
            {
                choice_mingpai_num++;
            }
        }

        //动作操作后处理托管状态
        if(DealWithTrusteeshipAfterAct(table,seat,action))
        {
            return true;
        }

        if (choice_mingpai_num >= table.seats_size())
        {
            table.set_operation_index(table.dealer_index());
            SendInterEventOnDoActionOver(table,6, action);
        }
        return false;
    }

    if (CheckSingleOver(table, seat))
    {
        // 单回结束
        action.set_is_single_over(true);
    }

    // 删除没确定的出牌
    if (seat.action_choice().choices(0).act_type() == EN_SDR_ACTION_CHUPAI)
    {
        RemoveActionFlow(table);
    }

    // 加入
    AppendActionFlow(table, seat, action);
    // 广播最后一次操作流
    BroadcastActionFlow(table);

    //动作操作后处理托管状态
    if(DealWithTrusteeshipAfterAct(table,seat,action))
    {
        return true;
    }

    // 下家出牌
    SendInterEventOnDoActionOver(table,2, action);
    return true;
}

bool TableLogic::PlayerDoActionJiaBei(CPBGameTable & table,PBSDRTableSeat & seat, PBSDRAction & action)
{
    VLogMsg(CLIB_LOG_LEV_DEBUG,"-----------------------------------");

    AppendActionFlow(table, seat, action);
    BroadcastActionFlow(table);

    {
        table.table_log<<"玩家["<<action.seat_index()<<"] ";
        table.table_log<<ActionToString(action)<<"\n";
    }
    int choice_jiabei_num = 0;

    seat.set_double_times(1);
    for (int i = 0 ; i < table.seats_size(); i++)
    {
        PBSDRTableSeat & o_seat = *table.mutable_seats(i);
        if (o_seat.double_times() >= 0)
        {
            choice_jiabei_num++;
        }
    }
    if (choice_jiabei_num >= table.seats_size())
    {
        // 回调 GameStart_4
        SendInterEventOnDoActionOver(table,5, action);
    }

    return false;
}

void TableLogic::ProcessTable(CPBGameTable & table)
{
    while(true)
    {
        switch(table.state())
        {
            case EN_TABLE_STATE_READY_TO_START : GameStart_1(table); return;
            case EN_TABLE_STATE_FINISH :  GameFinish(table); return;
            default:
                //ErrMsg("invalid table[%ld] state, %d",table.tid(),table.state());
                return;
        }
    }
}

void TableLogic::GameStart_1(CPBGameTable & table)
{
    table.StopReadyTimer();

    //如果是茶馆桌子，上报开局
    if (table.config().has_tbid() && table.config().has_master_uid() && table.round() == 0)
    {
        PBCSMsg msg;
        SSNotifyTeaBarTableGameStart & ss_notify_teabar_table_game_start = *msg.mutable_ss_notify_teabar_table_game_start();
        ss_notify_teabar_table_game_start.mutable_conf()->CopyFrom(table.config());
        ss_notify_teabar_table_game_start.set_tid(table.tid());
        Message::PushInnerMsg(RouteManager::Instance()->GetRouteByRandom(), 0, msg, EN_Node_TeaBar, 1);
    }

    VLogMsg(CLIB_LOG_LEV_DEBUG, "GameStart: GameStart_1 ... on table[%ld]",table.tid());
    table.set_round(table.round()+1);
    table.clear_total_action_flows();
    table.clear_replay_action_flows();
    table.set_state(EN_TABLE_STATE_PLAYING);
    table.clear_cards();
    table.set_operation_index(0);
    table.set_is_game_over(false);
    table.clear_max_pre_choice();
    table.table_log.reset();
    table.flow_record_item.Clear();
    table.flow_record_item.set_tid(table.tid());
    table.flow_record_item.mutable_sdr_conf()->CopyFrom(table.config());
    table.set_dealer_index_2(-1);
    table.clear_out_cards();
    table.set_bomb_num(0);
    table.set_soft_bomb_num(0);
    table.clear_dipai_cards();
    table.clear_laizi_card();
    table.set_jiao_dizhu_index(-1);


    table.set_first_mingpai_seat(-1);
    int start_index = random() % table.seats_size();
    for (int i = start_index; i < table.seats_size()+start_index; i++)
    {
        PBSDRTableSeat & seat = *table.mutable_seats(i % table.seats_size());
        if (seat.is_mingpai() == 1)
        {
            table.set_first_mingpai_seat(i % table.seats_size());
            break;
        }
    }

    // 重置座位
    for(int i=0;i<table.seats_size();i++)
    {
        PBSDRTableSeat & seat = *table.mutable_seats(i);
        seat.clear_action_choice();
        seat.clear_hand_cards();
        seat.set_state(EN_SEAT_STATE_PLAYING);
        seat.set_action_token(0);
        seat.set_final_score(0);
        seat.set_multiple(0);
        seat.set_need_check_double(false);
        seat.set_double_times(-1);
        seat.set_bomb_num(0);
        seat.set_bao_type(0);
        seat.clear_win_info();
        seat.set_is_buy(false);
        seat.set_soft_bomb_num(0);
        seat.set_is_lai_zhuang(false);
        seat.set_is_baozi_zhuang(false);
        seat.set_qiang_di_zhu_times(0);
        seat.set_is_mingpai_after_deal_cards(-1);
        if (seat.is_mingpai() == 1)
        {
            seat.set_is_mingpai_after_deal_cards(2);
        }
        seat.set_is_jiao_dizhu(false);
        seat.clear_is_trusteeship();
        seat.clear_trusteeship_time();
        seat.clear_end_trusteeship_time();
    }
    // 定庄
    int dealer_index = -1;
    // 第一局随机庄，之后赢家轮庄
    if (table.round() == 1)
    {
        dealer_index = 0;
        dealer_index = random() % table.seats_size();
    }
    else
    {
        dealer_index = table.last_winner();
    }

#ifdef _DEBUG
    {
        dealer_index = 1;
    }
#endif
    table.set_dealer_index(dealer_index);
    table.set_jiao_dizhu_index(dealer_index);

    table.flow_record_item.set_dealer(dealer_index);
    table.flow_record_item.set_round(table.round());
    {
        table.table_log << "牌局开始[牌桌ID:" << table.tid() << ",第" << table.round() << "回" << "]" <<"\n";
    }

    // 默认值-1 表示本局无人胡牌 即黄庄
    table.set_last_winner(-1);

    table.set_operation_index(-1);

    GameStart_1_3(table);
}

void TableLogic::GameStart_1_2(CPBGameTable & table)
{
    VLogMsg(CLIB_LOG_LEV_DEBUG, "GameStart: GameStart_1_2 ...");
//    table.set_state(EN_TABLE_STATE_WAIT_MING_PAI);
//    for (int i = 0; i < table.seats_size(); i++)
//    {
//        PBSDRTableSeat& seat = *table.mutable_seats(i);
//        seat.set_action_token(seat.action_token()+1);
//        // 添加明牌操作
//        {
//            PBSDRAction action;
//            action.set_tid(table.tid());
//            action.set_seat_index(seat.index());
//            action.set_act_type(EN_SDR_ACTION_MING_PAI);
//            AddChoise(seat,action);
//            VLogMsg(CLIB_LOG_LEV_DEBUG,"Add Choice[Type:%d] For User[%ld] On Seat[%d] Of Table[%ld]",
//                    action.act_type(), seat.user().uid(), seat.index(), table.tid());

//            PBSDRAction action_pass;
//            action_pass.set_tid(table.tid());
//            action_pass.set_seat_index(seat.index());
//            action_pass.set_act_type(EN_SDR_ACTION_BU_MING);
//            AddChoise(seat,action_pass);
//            VLogMsg(CLIB_LOG_LEV_DEBUG,"Add Choice[Type:%d] For User[%ld] On Seat[%d] Of Table[%ld]",
//                    action.act_type(), seat.user().uid(), seat.index(), table.tid());

//            NotifyOperationChoiceForSeat(table, seat);
//        }
//    }
    return;
}

void TableLogic::GameStart_1_3(CPBGameTable & table)
{
    //注册所有的动作的等待时间
    RegisterAllActionWaitingTime(table);

    // 洗牌
    ShuffleCards(table);

    // 发手牌
    DealHandCards(table);

    // 手牌排序
    SortHandCards(table);
    table.flow_record_item.mutable_sdr_seats()->CopyFrom(table.seats());


    // 通知游戏开始

    SendGameStartNotify(table);

//    {
//        GameOver(table);
//        return ;
//    }

    GameStart_2(table);

    //暂时没有倒计时
    return;
}

void TableLogic::GameStart_1_4(CPBGameTable & table)
{
    VLogMsg(CLIB_LOG_LEV_DEBUG, "GameStart: GameStart_1_4 ...");
//    ResetPlayerChoice(table);
//    bool has_mingpai_choice = false;
//    table.set_state(EN_TABLE_STATE_WAIT_MING_PAI_2);
//    for (int i = 0; i < table.seats_size(); i++)
//    {
//        PBSDRTableSeat& seat = *table.mutable_seats(i);
//        if (seat.is_mingpai() == 1 || seat.is_mingpai_after_deal_cards() == 1)
//        {
//            continue;
//        }
//        has_mingpai_choice = true;
//        seat.set_action_token(seat.action_token()+1);
//        // 添加明牌操作
//        {
//            PBSDRAction action;
//            action.set_tid(table.tid());
//            action.set_seat_index(seat.index());
//            action.set_act_type(EN_SDR_ACTION_MING_PAI);
//            AddChoise(seat,action);
//            VLogMsg(CLIB_LOG_LEV_DEBUG,"Add Choice[Type:%d] For User[%ld] On Seat[%d] Of Table[%ld]",
//                    action.act_type(), seat.user().uid(), seat.index(), table.tid());

//            PBSDRAction action_pass;
//            action_pass.set_tid(table.tid());
//            action_pass.set_seat_index(seat.index());
//            action_pass.set_act_type(EN_SDR_ACTION_BU_MING);
//            AddChoise(seat,action_pass);
//            VLogMsg(CLIB_LOG_LEV_DEBUG,"Add Choice[Type:%d] For User[%ld] On Seat[%d] Of Table[%ld]",
//                    action.act_type(), seat.user().uid(), seat.index(), table.tid());

//            NotifyOperationChoiceForSeat(table, seat);
//        }
//    }
//    if (!has_mingpai_choice)
//    {
//        GameStart_2(table);
//    }
    return;
}

void TableLogic::GameStart_2(CPBGameTable & table)
{
    VLogMsg(CLIB_LOG_LEV_DEBUG, "GameStart: GameStart_2 ...");

    if (table.first_mingpai_seat() != -1)
    {
        table.set_dealer_index(table.first_mingpai_seat());
        table.set_jiao_dizhu_index(table.first_mingpai_seat());
        table.set_first_mingpai_seat(-1);
    }

    if (table.dealer_index_2() == -1 || true)
    {
        ResetPlayerChoice(table);
        table.set_state(EN_TABLE_STATE_WAIT_QIANG_DI_ZHU);
        bool is_last_qiangdi_zhu = false;
        int qiangdizhu_times = 0;

        for (int i = 0; i < table.seats_size(); i++)
        {

            int seat_index = (table.dealer_index()+i)%table.seats_size();
            PBSDRTableSeat& seat = *table.mutable_seats(seat_index);
            if (seat.qiang_di_zhu_times() > 0)
            {
                qiangdizhu_times++;
                if (qiangdizhu_times >= table.seats_size())
                {
                    is_last_qiangdi_zhu = true;
                }
                continue;
            }

            if (qiangdizhu_times >= table.seats_size())
            {
                is_last_qiangdi_zhu = true;
                break;
            }
            VLogMsg(CLIB_LOG_LEV_DEBUG,"process For User[%ld] On Seat[%d] Of Table[%ld]", table.seats(seat_index).user().uid(), seat_index, table.tid());

            table.set_operation_index(seat_index);
            int jiaodizhu_index = table.jiao_dizhu_index();
            {
                seat.set_action_token(seat.action_token()+1);
                // 添加抢地主操作
                {
                    PBSDRAction action;
                    action.set_tid(table.tid());
                    action.set_seat_index(seat.index());
                    if (jiaodizhu_index == seat_index)
                    {
                        action.set_act_type(EN_SDR_ACTION_JIAO_DI_ZHU);
                    }
                    else
                        action.set_act_type(EN_SDR_ACTION_QIANG_DI_ZHU);
                    AddChoise(seat,action);
                    VLogMsg(CLIB_LOG_LEV_DEBUG,"Add Choice[Type:%d] For User[%ld] On Seat[%d] Of Table[%ld]",
                            action.act_type(), seat.user().uid(), seat.index(), table.tid());
                }
                // 添加过操作
                if (!IsBiZhua(table, seat) || true)
                {
                    PBSDRAction action;
                    action.set_tid(table.tid());
                    action.set_seat_index(seat.index());
                    if (jiaodizhu_index == seat_index)
                    {
                        action.set_act_type(EN_SDR_ACTION_BU_JIAO);
                        table.set_jiao_dizhu_index((jiaodizhu_index + 1) % table.seats_size());
                    }
                    else
                        action.set_act_type(EN_SDR_ACTION_BU_QIANG);
                    AddChoise(seat,action);
                    VLogMsg(CLIB_LOG_LEV_DEBUG,"Add Choice[Type:%d] For User[%ld] On Seat[%d] Of Table[%ld]",
                            action.act_type(), seat.user().uid(), seat.index(), table.tid());
                }

                BroadcastNextOperation(table, seat_index);

				OnOperateAfterAddChoice(table, seat);
                VLogMsg(CLIB_LOG_LEV_DEBUG, "GameStart_2: 等待买牌 ...");
                return;
            }
        }
        if (is_last_qiangdi_zhu)
        {
            PBSDRTableSeat& seat = *table.mutable_seats(table.jiao_dizhu_index());
            int seat_index = seat.index();
            bool has_qiang_dizhu = false;

            for (int i = 0 ; i < table.total_action_flows_size() ; i++)
            {
                if (table.total_action_flows(i).action().act_type() == EN_SDR_ACTION_QIANG_DI_ZHU)
                {
                    has_qiang_dizhu = true;
                    break;
                }
            }

            if (seat.is_jiao_dizhu() && has_qiang_dizhu)
            {
                seat.set_is_jiao_dizhu(false);
                table.set_operation_index(seat.index());
                {
                    seat.set_action_token(seat.action_token()+1);
                    // 添加抢地主操作
                    {
                        PBSDRAction action;
                        action.set_tid(table.tid());
                        action.set_seat_index(seat.index());
                            action.set_act_type(EN_SDR_ACTION_QIANG_DI_ZHU);
                        AddChoise(seat,action);
                        VLogMsg(CLIB_LOG_LEV_DEBUG,"Add Choice[Type:%d] For User[%ld] On Seat[%d] Of Table[%ld]",
                                action.act_type(), seat.user().uid(), seat.index(), table.tid());
                    }
                    // 添加过操作
                    if (!IsBiZhua(table, seat) || true)
                    {
                        PBSDRAction action;
                        action.set_tid(table.tid());
                        action.set_seat_index(seat.index());
                            action.set_act_type(EN_SDR_ACTION_BU_QIANG);
                        AddChoise(seat,action);
                        VLogMsg(CLIB_LOG_LEV_DEBUG,"Add Choice[Type:%d] For User[%ld] On Seat[%d] Of Table[%ld]",
                                action.act_type(), seat.user().uid(), seat.index(), table.tid());
                    }

                    BroadcastNextOperation(table, seat_index);

					OnOperateAfterAddChoice(table, seat);
                    VLogMsg(CLIB_LOG_LEV_DEBUG, "GameStart_2: 等待买牌 ...");
                    return;
                }
            }
        }
        // 如果没人抢地主，重新开始牌局  如果有人明牌，那就第一个明牌玩家当地主
        bool has_mingpai = false;
        if (table.seats(table.dealer_index()).is_mingpai() > 0 || table.seats(table.dealer_index()).is_mingpai_after_deal_cards() > 0)
        {
            has_mingpai = true;
        }
        if (table.dealer_index_2() == -1 && !has_mingpai)
        {
            if(table.is_trusteeship_error())
            {
                return;
            }
            else
            {
                table.set_round(table.round()-1);
                GameStart_1(table);
            }

            return;
        }
        else if (table.dealer_index_2() == -1 && has_mingpai)
        {
            PBSDRTableSeat & seat = *table.mutable_seats(table.dealer_index());
            table.set_dealer_index_2(table.dealer_index());

            PBSDRAction action;
            action.set_tid(table.tid());
            action.set_seat_index(seat.index());
            action.set_act_type(EN_SDR_ACTION_JIAO_DI_ZHU);
            action.set_is_baozi_zhuang(true);

            AppendActionFlow(table, seat, action);
            BroadcastActionFlow(table);
        }
    }


    table.set_operation_index(-1);
    {
        // 设置庄家
        int chupai_index = table.dealer_index();
        table.set_dealer_index(table.dealer_index_2());
        table.set_dealer_index_2(chupai_index);
    }
    // 抢庄完成，到GameStart_3进行后续流程
    GameStart_3(table);

    //暂时没有倒计时
    return;
}

void TableLogic::GameStart_3(CPBGameTable & table)
{
    ResetPlayerChoice(table);
    // 抓牌相关逻辑
    VLogMsg(CLIB_LOG_LEV_DEBUG, "GameStart: GameStart_3 ...");
    // 地主抓牌，要给所有人看
    int dealer_index = table.dealer_index();
    PBSDRTableSeat& dealer = *table.mutable_seats(dealer_index);
    // 抓牌
    dealer.mutable_hand_cards()->MergeFrom(table.cards());

    PBSDRAction action;
    action.set_tid(table.tid());
    action.set_seat_index(dealer.index());
    action.set_act_type(EN_SDR_ACTION_ZHUA_PAI);
    action.mutable_col_info()->mutable_cards()->CopyFrom(table.cards());
    AppendActionFlow(table, dealer, action);
    BroadcastActionFlow(table);

    // 清空牌堆
//    table.clear_cards();

    // 抓牌完成 到GameStart_5继续后面的正常流程
//    dealer.set_need_check_double(false);
    GameStart_5(table);

    //暂时没有倒计时
    return;
}

void TableLogic::GameStart_4(CPBGameTable & table)
{
    VLogMsg(CLIB_LOG_LEV_DEBUG, "GameStart: GameStart_4 ...");
//    ResetPlayerChoice(table);
//    table.set_state(EN_TABLE_STATE_WAIT_DOUBLE);

//    for (int i = 0; i < table.seats_size(); i++)
//    {
//        PBSDRTableSeat& seat = *table.mutable_seats(i);
//        seat.set_action_token(seat.action_token()+1);
//        // 添加加倍操作
//        {
//            PBSDRAction action;
//            action.set_tid(table.tid());
//            action.set_seat_index(seat.index());
//            action.set_act_type(EN_SDR_ACTION_JIA_BEI);
//            AddChoise(seat,action);
//            VLogMsg(CLIB_LOG_LEV_DEBUG,"Add Choice[Type:%d] For User[%ld] On Seat[%d] Of Table[%ld]",
//                    action.act_type(), seat.user().uid(), seat.index(), table.tid());
//        }
//        // 添加过操作
//        {
//            PBSDRAction action;
//            action.set_tid(table.tid());
//            action.set_seat_index(seat.index());
//            action.set_act_type(EN_SDR_ACTION_BU_JIA_BEI);
//            AddChoise(seat,action);
//            VLogMsg(CLIB_LOG_LEV_DEBUG,"Add Choice[Type:%d] For User[%ld] On Seat[%d] Of Table[%ld]",
//                    action.act_type(), seat.user().uid(), seat.index(), table.tid());
//        }
//        NotifyOperationChoiceForSeat(table, seat);
//        VLogMsg(CLIB_LOG_LEV_DEBUG, "GameStart_4: 等待加倍 ...");
//    }
    return;
}

void TableLogic::GameStart_5(CPBGameTable & table)
{
    VLogMsg(CLIB_LOG_LEV_DEBUG, "GameStart: GameStart_5 ...");
    ResetPlayerChoice(table);
    table.set_state(EN_TABLE_STATE_PLAYING);

    int dealer_index = table.dealer_index();
    PBSDRTableSeat& dealer = *table.mutable_seats(dealer_index);

    // 添加操作流 出牌
    PBSDRAction action;
    action.set_tid(table.tid());
    action.set_seat_index(dealer.index());
    action.set_act_type(EN_SDR_ACTION_CHUPAI);
    AppendActionFlow(table, dealer, action);

    // 庄家只能出牌
    AddChupaiChoiceForSeat(table, dealer);

    // 广播操作
    BroadcastNextOperation(table, dealer.index());
    table.set_operation_index(dealer.index());

	OnOperateAfterAddChoice(table, dealer);
}

void TableLogic::GameOver(CPBGameTable & table)
{
    table.table_log<<"牌局结束"<<"\n";
    bool is_finished = false;

    if (table.state() == EN_TABLE_STATE_FINISH
            || table.state() == EN_TABLE_STATE_SINGLE_OVER)
    {
        return;
    }
    // 游戏结束
    if (table.round() >= table.config().round())
    {
        is_finished = true;
    }

    // 手牌排序
    {
        SortHandCards(table);
    }

    if (table.last_winner() == -1)
    {
        int next_deal_index = (table.dealer_index() + 1) % table.seats_size();
        table.set_last_winner(next_deal_index);
        PBCSMsg msg;
        CSNotifyGameOver& notify = *msg.mutable_cs_notify_game_over();
        CSSdrResult& result = *notify.mutable_sdr_result();
        result.set_winner_index(table.last_winner());
        result.set_is_finished(is_finished);
        result.set_bomb_num(0);
        result.set_soft_bomb_num(1);

        int single_score = 0;
        int multiple = 1;
        if (table.total_action_flows_size() > 0)
        {
            PBSDRAction last_actoin =  GetLastActionInFlow(table);
            // 没有翻底牌的情况下 免战是赔1软炸，算分上无论是相加还是相乘，都是两分
            // 在翻牌后，并且底牌中有赖子时，也是赔一软炸
            // 在翻牌后，并且底牌中没有赖子时
            if (last_actoin.act_type() != EN_SDR_ACTION_FAN_DI_PAI && table.seats(table.dealer_index()).is_lai_zhuang())
            {
                single_score = 1;
                multiple = 0;
            }
            else if (last_actoin.act_type() == EN_SDR_ACTION_FAN_DI_PAI)
            {
                for (int i = 0; i < table.cards_size();i++)
                {
                    if (GetCardValue(table.cards(i)) == GetCardValue(table.laizi_card()) )
                    {
                        single_score = 2;
                        multiple = 1;
                        break;
                    }
                    else
                    {
                        single_score = 1;
                        multiple = 0;
                    }
                }
            }
        }
        else
        {
            single_score = 1;
            multiple = 0;
        }

        for (int i = 0; i < table.seats_size(); i++)
        {
            if (i == table.dealer_index())
                continue;
            PBSDRTableSeat& i_seat = *table.mutable_seats(i);
            i_seat.set_final_score(single_score);
        }
        PBSDRTableSeat& dealer = *table.mutable_seats(table.dealer_index());
        dealer.set_final_score(0 - 2 * single_score);

        // 输家
        CSSdrPlayerInfo& player = *result.add_players();
        player.set_uid(table.seats(table.dealer_index()).user().uid());
        player.set_score(0 - single_score * (table.seats_size()-1) );
        player.set_multiple(multiple);
        player.mutable_hand_cards_info()->mutable_hand_cards()->CopyFrom(table.seats(table.dealer_index()).hand_cards());
        player.set_bomb_num(0);
        player.set_soft_bomb_num(0);
        player.mutable_win_info()->mutable_styles()->CopyFrom(table.seats(table.dealer_index()).win_info().styles());

        // 赢家
        for (int i = 0; i < table.seats_size(); i++)
        {
            if (table.dealer_index() == i)
                continue;
            CSSdrPlayerInfo& player = *result.add_players();
            player.set_uid(table.seats(i).user().uid());
            player.set_score(single_score);
            player.set_multiple(multiple);
            player.mutable_hand_cards_info()->mutable_hand_cards()->CopyFrom(table.seats(i).hand_cards());
            player.set_bomb_num(0);
            player.set_soft_bomb_num(0);
            player.mutable_win_info()->mutable_styles()->CopyFrom(table.seats(i).win_info().styles());
        }
        table.flow_record_item.mutable_game_over()->CopyFrom(notify);
        BroadcastTableMsg(table,msg);
    }
    else
    {
        PBSDRTableSeat& seat = *table.mutable_seats(table.last_winner());
        // 算分
        AnalyzeHuStyles(table, seat);

        // 3. 发送结束通知
        PBCSMsg msg;
        CSNotifyGameOver& notify = *msg.mutable_cs_notify_game_over();
        CSSdrResult& result = *notify.mutable_sdr_result();
        result.set_winner_index(table.last_winner());
        result.set_is_finished(is_finished);
        result.set_bomb_num(table.bomb_num());
        result.set_soft_bomb_num(table.soft_bomb_num());
        // 赢家
        CSSdrPlayerInfo& player = *result.add_players();
        player.set_uid(seat.user().uid());
        player.set_score(seat.final_score());
        player.set_multiple(seat.multiple());
        player.mutable_hand_cards_info()->mutable_hand_cards()->CopyFrom(seat.hand_cards());
        player.set_bomb_num(seat.bomb_num());
        player.set_soft_bomb_num(seat.soft_bomb_num());
        player.set_double_times(seat.double_times());
        player.mutable_win_info()->mutable_styles()->CopyFrom(seat.win_info().styles());
        // 输家
        for (int i = 0; i < table.seats_size(); i++)
        {
            if (i == seat.index()) continue;
            CSSdrPlayerInfo& player = *result.add_players();
            player.set_uid(table.seats(i).user().uid());
            player.set_score(table.seats(i).final_score());
            player.set_multiple(table.seats(i).multiple());
            player.mutable_hand_cards_info()->mutable_hand_cards()->CopyFrom(table.seats(i).hand_cards());
            player.set_bomb_num(table.seats(i).bomb_num());
            player.set_soft_bomb_num(table.seats(i).soft_bomb_num());
            player.set_double_times(table.seats(i).double_times());
            player.mutable_win_info()->mutable_styles()->CopyFrom(table.seats(i).win_info().styles());
        }
        table.flow_record_item.mutable_game_over()->CopyFrom(notify);
        BroadcastTableMsg(table,msg);
    }

    //通知更新用户的匹配游戏的结果
    if(is_finished)
    {
        PBCSMsg pbMsg;
		SSInnerUpdateMatchResult & ss_inner_update_match_result = *pbMsg.mutable_ss_inner_update_match_result();
		ss_inner_update_match_result.set_tid(table.tid());
        GameHandlerProxy::Instance()->PushInnerMsg(pbMsg);
    }

    VLogMsg(CLIB_LOG_LEV_DEBUG,"table[%ld] 单局游戏结束", table.tid());
    // 记录table flow
    table.flow_record_item.mutable_sdr_total_action_flows()->CopyFrom(table.replay_action_flows());
    UpdateTableFlowRecord(table);

    {   // 记录每局的游戏记录
        PBGameRecord& record = *table.mutable_record();
        PBRoundResult& round_result = *record.add_round_results();
        round_result.set_round_index(table.round());
        round_result.set_stamp(time(NULL));
        round_result.set_flow_size(table.replay_action_flows_size());

        for (int i = 0; i < table.seats_size(); i++)
        {
            PBUserScoreInfo& score_info = *round_result.add_scores();
            score_info.set_score(table.seats(i).final_score());
            score_info.set_uid(table.seats(i).user().uid());
        }
    }

    // 牌局日志记录结算信息
    {
        table.table_log << "牌局结算[ ";
        for (int i = 0; i < table.seats_size(); i++)
        {
            table.table_log << table.seats(i).final_score() << " ";
        }
        table.table_log << " ]\n";
    }

    {
        PBCSMsg msg;
        LogGameLog & log = *msg.mutable_log_game_log();
        log.set_table_id(table.tid());
        log.set_game_id((table.tid()<<32)+table.start_stamp);
        log.set_table_log(table.table_log.c_str());
        log.set_begin_time(table.start_stamp);
        log.set_end_time(time(NULL));
        log.set_seat_num(table.config().seat_num());
        log.set_game_type(table.config().ttype());
        log.set_is_finished_game(true);   
        for(int i=0;i<table.seats_size();i++)
        {
            const PBSDRTableSeat & seat = table.seats(i);
            log.add_players(seat.user().uid());

            PBLogPlayersInfo & pbPlayersInfo = *log.add_players_info();
            pbPlayersInfo.set_player_uid(seat.user().uid());
            pbPlayersInfo.set_channel(seat.user().channel());
        }
        CLogWriter::Instance()->Send(msg);
    }
    // 游戏结束
    if (is_finished)
    {
        table.set_state(EN_TABLE_STATE_FINISH);
        ProcessTable(table);
        return;
    }

    // 要求点准备开始下一句时需要设置
    {
        for(int i=0;i<table.seats_size();i++)
        {
            PBSDRTableSeat & seat = *table.mutable_seats(i);
            seat.set_final_score(0);
            seat.set_state(EN_SEAT_STATE_WAIT_FOR_NEXT_ONE_GAME);
        }
        table.clear_out_cards();
    }


    table.set_state(EN_TABLE_STATE_SINGLE_OVER);
}

bool TableLogic::UpdateTableFlowRecord(CPBGameTable & table)
{
    time_t t_now = table.start_stamp;
    struct tm *p;
    p = localtime(&t_now);
    char mainkey[125] = {0};
    snprintf(mainkey,sizeof(mainkey),"SDR_TABLE_FLOW_RECORD_%04d%02d%02d",p->tm_year + 1900, p->tm_mon + 1, p->tm_mday);
    char subkey[125] = {0};
    snprintf(subkey,sizeof(subkey),"%lld_%d",(table.tid()<<32)+table.start_stamp,table.round());
    PBHashDataField pb_hash_data_field;
    pb_hash_data_field.mutable_sdr_table_flow_record_item()->CopyFrom(table.flow_record_item);
//    GlobalRedisClient::Instance()->UpdateHashObject(mainkey,subkey,pb_hash_data_field);
    return RecordRedisClient::Instance()->UpdateHashObject(mainkey,subkey,pb_hash_data_field);

    return true;
}

void TableLogic::GameFinish(CPBGameTable & table)
{
    // 游戏结束 总结算通知
    SendGameFinishNotify(table);

    // 更新牌局排行榜
    if (!table.is_dissolve_finish() || table.round()-1 > 0)
    {
        int real_round = table.round();
        real_round = table.is_dissolve_finish() ? real_round - 1 : real_round;
        for (int i = 0; i < table.seats_size(); i++)
        {
            /*
                         * saber lily
                         * 更新排行榜数据
                         * 逻辑后期修改
                        */
            SendUpdateRankMsg(EN_Rank_List_Type_Day_Round, table.seats(i).user().uid(), real_round);
            SendUpdateRankMsg(EN_Rank_List_Type_Week_Round, table.seats(i).user().uid(), real_round);
        }
    }

    // 写入牌桌信息日志
    {
        PBCSMsg msg;
        LogGameInfoLog & log = *msg.mutable_log_game_info_log();
        log.set_game_id((table.tid()<<32)+table.start_stamp);
        log.set_game_type(table.config().ttype());
        log.set_conf_round(table.config().round());
        log.set_real_round(table.round());
        log.set_seat_num(table.config().seat_num());
        CLogWriter::Instance()->Send(msg);
    }

    // 牌局中途结算 牌局日志也要写
    if (table.state() != EN_TABLE_STATE_FINISH)
    {
        table.table_log<<"房间提前解散"<<"\n";

        PBCSMsg msg;
        LogGameLog & log = *msg.mutable_log_game_log();
        log.set_table_id(table.tid());
        log.set_game_id((table.tid()<<32)+table.start_stamp);
        log.set_table_log(table.table_log.c_str());
        log.set_begin_time(table.start_stamp);
        log.set_end_time(time(NULL));
        log.set_seat_num(table.config().seat_num());
        log.set_game_type(table.config().ttype());
        for(int i=0;i<table.seats_size();i++)
        {
            const PBSDRTableSeat & seat = table.seats(i);
            log.add_players(seat.user().uid());
        }
        CLogWriter::Instance()->Send(msg);
    }

    // 记录战绩 玩牌记录
    {
        PBCSMsg innermsg;
        SSInnerReportGameRecord& inner_report_game_record = *innermsg.mutable_ss_inner_report_game_record();
        PBGameRecord& record = *inner_report_game_record.mutable_game_record();
        record.CopyFrom(table.record());
        GameHandlerProxy::Instance()->PushInnerMsg(innermsg);
    }

    {
        // 牌局结束 解散房间
        PBCSMsg innermsg;
        SSInnerDissolveTable & ss_inner_dissolve_table = *innermsg.mutable_ss_inner_dissolve_table();
        ss_inner_dissolve_table.set_tid(table.tid());
        GameHandlerProxy::Instance()->PushInnerMsg(innermsg);
    }
    VLogMsg(CLIB_LOG_LEV_DEBUG,"table[%ld] 游戏结束", table.tid());
}

//bool TableLogic::NeedWaitingOtherOpera(CPBGameTable & table,PBSDRTableSeat & request_seat,const PBSDRAction & request_action)
//{
//    const PBSDRActionChoice & user_action_choice = request_seat.action_choice();
//    const PBTableUser & user = request_seat.user();
//    int64 uid = user.uid();
//    int64 tid = table.tid();
//    int last_opera_index = GetLastOperateSeatIndex(table);
//    if (last_opera_index == -1)
//    {
//        last_opera_index = table.dealer_index();
//    }

//    if (table.state() == EN_TABLE_STATE_WAIT_AN_PAI)
//    {
//        VLogMsg(CLIB_LOG_LEV_DEBUG,"暗牌阶段,不需要等待别人操作...");
//        return false;
//    }

//    if (table.state() == EN_TABLE_STATE_WAIT_QIANG_ZHUANG)
//    {
//        VLogMsg(CLIB_LOG_LEV_DEBUG,"抢庄阶段,不需要等待别人操作...");
//        return false;
//    }

//    for(int i = 0; i < table.seats_size(); i++)
//    {
//        if(i == request_seat.index())
//        {
//            continue;
//        }
//        const PBSDRTableSeat & seat = table.seats(i);
//        const PBSDRActionChoice & action_choice = seat.action_choice();
//        if(action_choice.is_determine() == true /*|| seat.state() == EN_SEAT_STATE_WIN*/)
//        {
//            continue;
//        }
//        if(action_choice.max_level() == 0)
//        {
//            //当前座位没有操作
//            continue;
//        }
//        VLogMsg(CLIB_LOG_LEV_DEBUG,"seat[%d]:max_level[%d] cur_seat:determined_level[%d]",
//                i, action_choice.max_level(), user_action_choice.determined_level());

//        if(action_choice.max_level() > user_action_choice.determined_level())
//        {
//            //还有等级更高的操作
//            VLogMsg(CLIB_LOG_LEV_DEBUG,"player[%lld] do action[%d] on table[%lld],waiting for other seat[%d,%d]",
//                    uid,request_action.act_type(),tid,i,action_choice.max_level());
//            return true;
//        }
//        else if(action_choice.max_level() == user_action_choice.determined_level())
//        {
//            // 优先级相同的情况下 比较操作顺序 按顺序
//            int user_opera_index = (request_seat.index()-last_opera_index)%table.seats_size();
//            user_opera_index = (user_opera_index < 0) ? (user_opera_index + table.seats_size()) : user_opera_index;
//            int seat_opera_index = (i-last_opera_index)%table.seats_size();
//            seat_opera_index = (seat_opera_index < 0) ? (seat_opera_index + table.seats_size()) : seat_opera_index;
//            VLogMsg(CLIB_LOG_LEV_DEBUG, "seat[%d] user_opera_index[%d] seat_opera_index[%d] ...", seat.index(), user_opera_index, seat_opera_index);
//            if(user_opera_index > seat_opera_index)
//            {
//                //还有等级更高的操作
//                VLogMsg(CLIB_LOG_LEV_DEBUG,"player[%lld] do action[%d] on table[%lld],waiting for other seat[%d,%d]",
//                        uid,request_action.act_type(),tid,i,action_choice.max_level());
//                return true;
//            }
//        }
//    }
//    return false;
//}

void TableLogic::RemoveActionFlow(CPBGameTable & table)
{
//    table.mutable_total_action_flows()->ExtractSubrange(table.total_action_flows_size()-1,1,NULL);
//    table.mutable_replay_action_flows()->ExtractSubrange(table.replay_action_flows_size()-1,1,NULL);

    table.mutable_total_action_flows()->RemoveLast();
    table.mutable_replay_action_flows()->RemoveLast();
}

void TableLogic::RemoveLastActionFlow(CPBGameTable & table, int type)
{
    if (table.total_action_flows_size() > 0)
    {
        for (int i = table.total_action_flows_size()-1; i >= 0; i--)
        {
            const PBSDRActionFlow & current_flow = table.total_action_flows(i);
            const PBSDRAction & current_action = current_flow.action();
            if (current_action.act_type() == type)
            {
                table.mutable_total_action_flows()->DeleteSubrange(i, 1);
                break;
            }
        }
    }
    if (table.replay_action_flows_size() > 0)
    {
        for (int i = table.replay_action_flows_size()-1; i >= 0; i--)
        {
            const PBSDRActionFlow & current_flow = table.replay_action_flows(i);
            const PBSDRAction & current_action = current_flow.action();
            if (current_action.act_type() == type)
            {
                table.mutable_replay_action_flows()->DeleteSubrange(i, 1);
                break;
            }
        }
    }
}

void TableLogic::AppendActionFlow(CPBGameTable & table,PBSDRTableSeat & seat,const PBSDRAction & action)
{
    PBSDRActionFlow & flow = *table.add_total_action_flows();
    flow.set_flow_token(table.total_action_flows_size());
    flow.mutable_action()->CopyFrom(action);
    VLogMsg(CLIB_LOG_LEV_DEBUG,"add action flow[%d] act_type[%d] for uid[%ld]", flow.flow_token(), action.act_type(), seat.user().uid());
    if (action.act_type() != EN_SDR_ACTION_CHUPAI)
    {
        table.table_log<<"玩家["<<action.seat_index()<<"] ";
        table.table_log<<ActionToString(action)<<"\n";
    }
    // 记录到回放操作流
    AppendActionFlowForReplay(table, seat, action);
}
// 回放用的action_flow
void TableLogic::AppendActionFlowForReplay(CPBGameTable & table,PBSDRTableSeat & seat,const PBSDRAction & action)
{
    PBSDRActionFlow & flow = *table.add_replay_action_flows();
    flow.set_flow_token(table.replay_action_flows_size());
    flow.mutable_action()->CopyFrom(action);
    VLogMsg(CLIB_LOG_LEV_DEBUG,"add replay action flow[%d] act_type[%d] for uid[%ld]", flow.flow_token(), action.act_type(), seat.user().uid());

}

int TableLogic::GetPlayerNumByState(const CPBGameTable & table,ENSeatState expected_state)
{
    int num = 0;
    for (int i=0;i<table.seats_size();i++)
    {
        const PBSDRTableSeat & seat = table.seats(i);
        if(seat.state() == expected_state)
        {
            num ++ ;
        }
    }
    return num;
}

void TableLogic::SendGameStartNotify(const CPBGameTable & table)
{
    for(int i=0;i<table.seats_size();i++)
    {
        const PBSDRTableSeat & seat = table.seats(i);
        if(seat.state() != EN_SEAT_STATE_NO_PLAYER && seat.user().is_offline() == false)
        {
            PBCSMsg msg;
            CSNotifyGameStart & cs_notify_game_start = *msg.mutable_cs_notify_game_start();
            cs_notify_game_start.set_left_card_num(table.cards_size());
            cs_notify_game_start.set_dealer(-1);
//            cs_notify_game_start.set_xiaojia_index(table.xiaojia_index());
            cs_notify_game_start.set_round(table.round());


            for(int j=0;j<table.seats_size();j++)
            {
                PBSDRTableSeat * pseat = cs_notify_game_start.add_sdr_seats();
                pseat->CopyFrom(table.seats(j));
                if(i != j && pseat->is_mingpai() < 1)
                {
                    // 把自己的手牌发给自己
                    for(int k = 0; k < pseat->hand_cards_size(); k++)
                    {
                        pseat->set_hand_cards(k, 0);
                    }
                }
            }
            Message::PushMsg(RouteManager::Instance()->GetRouteByRandom(),seat.user().uid(),msg,EN_Node_Connect,seat.user().connect_id());
        }
    }
}

void TableLogic::NotifyOperationChoiceForSeat(CPBGameTable & table,const PBSDRTableSeat & seat)
{
    PBCSMsg msg;
    if(seat.state() == EN_SEAT_STATE_NO_PLAYER)
    {
        VLogMsg(CLIB_LOG_LEV_DEBUG, "EN_SEAT_STATE_NO_PLAYER ...");
        return;
    }
    const PBTableUser & user = seat.user();
    CSNotifySeatOperationChoice & cs_notify_seat_choice = *msg.mutable_cs_notify_seat_operation_choice();
    PBSDRActionChoice seat_action_choice = seat.action_choice();
    if(seat_action_choice.choices_size() != 0)
    {
        table.table_log<<"给玩家["<<seat.index()<<"] "<<"添加操作:";
        for(int i=0;i<seat_action_choice.choices_size();i++)
        {
            const PBSDRAction & action = seat_action_choice.choices(i);
            table.table_log<<ActionToString(action)<<" ";
        }
        table.table_log<<"\n";
    }
    if(seat_action_choice.is_determine() == true || seat_action_choice.choices_size() == 0)
    {
        VLogMsg(CLIB_LOG_LEV_DEBUG, "is_determine[%d], choices_size[%d]", seat_action_choice.is_determine(), seat_action_choice.choices_size());
        return;
    }

    //通知下家选项后，需要开启操作定时器
    table.StartCheckUserOffTimer(15);
    VLogMsg(CLIB_LOG_LEV_DEBUG, "Start StartCheckUserOffTimer 15");

    if(user.is_offline())
    {
        VLogMsg(CLIB_LOG_LEV_DEBUG, "is_offline ...");
        return;
    }
    if (seat_action_choice.choices_size() != 0)
    {
        cs_notify_seat_choice.mutable_sdr_choices()->CopyFrom(seat_action_choice.choices());
        cs_notify_seat_choice.set_action_token(seat.action_token());
        Message::PushMsg(RouteManager::Instance()->GetRouteByRandom(),seat.user().uid(),msg,EN_Node_Connect,seat.user().connect_id());
    }
}

void TableLogic::BroadcastActionFlow(const CPBGameTable & table)
{
    if(table.total_action_flows_size()<1)
    {
        return;
    }
    //拷贝新的操作
    const PBSDRActionFlow & new_action_flow = table.total_action_flows(table.total_action_flows_size()-1);

    PBCSMsg notify;
    CSNotifyActionFlow & cs_notify_action_flow = *notify.mutable_cs_notify_action_flow();
    PBSDRActionFlow & notify_new_action_flow = *cs_notify_action_flow.mutable_new_sdr_action_flow();
    notify_new_action_flow.CopyFrom(new_action_flow);

    BroadcastTableMsg(table,notify);
}
void TableLogic::BroadcastActionFlowForReplay(const CPBGameTable & table)
{
    if(table.replay_action_flows_size()<1)
    {
        return;
    }
    //拷贝新的操作
    const PBSDRActionFlow & new_action_flow = table.replay_action_flows(table.replay_action_flows_size()-1);

    PBCSMsg notify;
    CSNotifyActionFlow & cs_notify_action_flow = *notify.mutable_cs_notify_action_flow();
    PBSDRActionFlow & notify_new_action_flow = *cs_notify_action_flow.mutable_new_sdr_action_flow();
    notify_new_action_flow.CopyFrom(new_action_flow);

    BroadcastTableMsg(table,notify);
}

void TableLogic::BroadcastActionFlow(const CPBGameTable & table, ENSDRActionType type)
{
    if(table.total_action_flows_size()<1)
    {
        return;
    }

    for (int i = table.total_action_flows_size()-1; i >= 0 ; i--)
    {
        //拷贝新的操作
        const PBSDRActionFlow & new_action_flow = table.total_action_flows(i);
        if (new_action_flow.action().act_type() == type)
        {
            PBCSMsg notify;
            CSNotifyActionFlow & cs_notify_action_flow = *notify.mutable_cs_notify_action_flow();
            PBSDRActionFlow & notify_new_action_flow = *cs_notify_action_flow.mutable_new_sdr_action_flow();
            notify_new_action_flow.CopyFrom(new_action_flow);
            cs_notify_action_flow.set_display_anpai(table.display_anpai());

            BroadcastTableMsg(table,notify);
            return;
        }
    }
}

void TableLogic::BroadcastTableMsg(const CPBGameTable & table,PBCSMsg & notify,int excepted_uid)
{
    for (int i=0;i<table.seats_size();i++)
    {
        const PBSDRTableSeat & seat = table.seats(i);
        if (seat.state()==EN_SEAT_STATE_NO_PLAYER)
        {
            continue;
        }
        const PBTableUser & user = seat.user();
        if (user.uid() != excepted_uid && user.is_offline() == false)
        {
            VLogMsg(CLIB_LOG_LEV_DEBUG,"broadcast msg[0x%x] to user[%ld].seat index[%d]",notify.msg_union_case(),user.uid(),i);
            {
                Message::PushMsg(RouteManager::Instance()->GetRouteByRandom(),seat.user().uid(),notify,EN_Node_Connect,seat.user().connect_id());
            }
        }
    }
}

void TableLogic::CopyTableToNotify(const CPBGameTable & table,CSNotifyTableInfo & info,int64 uid)
{
    for(int i = 0;i<table.seats_size();i++)
    {
        PBSDRTableSeat & seat = *info.add_sdr_seats();
        seat.CopyFrom(table.seats(i));

        if (seat.is_mingpai_after_deal_cards() == 1)
        {
            seat.set_is_mingpai(1);
        }

        if(seat.state()!=EN_SEAT_STATE_NO_PLAYER && seat.user().uid() != uid)
        {
            if (seat.is_mingpai() != 1 && seat.is_mingpai_after_deal_cards() != 1)
            {
                for(int j=0;j<seat.hand_cards_size();j++)
                {
                    seat.set_hand_cards(j,0);
                }
            }
            seat.clear_action_choice();
        }
    }
    info.mutable_out_cards()->CopyFrom(table.out_cards());
    info.set_display_anpai(table.display_anpai());
    info.set_state(table.state());
    info.set_round(table.round());
    info.set_operate_period(15);
	info.set_ready_wait_period(10);
    for (int i = 0; i < table.dipai_cards_size(); i++)
    {
        info.add_dipai_cards(table.dipai_cards(i));
    }
    info.set_laizi_card(table.laizi_card());

//    if (table.dealer_index_2() != -1)
//    {
//        info.mutable_cards()->CopyFrom(table.cards());
//    }
//    else
//    {
//        int zhua_cards_size = table.config().lai_zi_num() > 0 ? 4 : 3;
//        info.mutable_cards()->Resize(zhua_cards_size, 0);
//    }
    if (table.total_action_flows_size() > 0)
    {
        for (int i = table.total_action_flows_size()-1; i >= 0; i--)
        {
            if (table.total_action_flows(i).action().act_type() == EN_SDR_ACTION_PASS
                    && table.total_action_flows(i).action().is_single_over())
            {
                for (int j = i+1; j < table.total_action_flows_size(); j++)
                {
                    info.add_sdr_total_action_flows()->CopyFrom(table.total_action_flows(j));
                }
                break;
            }
            if (i == 0 && info.sdr_total_action_flows_size() == 0)
            {
                for (int j = 0; j < table.total_action_flows_size(); j++)
                {
                    if (table.total_action_flows(j).action().act_type() == EN_SDR_ACTION_JIA_BEI
                            || table.total_action_flows(j).action().act_type() == EN_SDR_ACTION_BU_JIA_BEI)
                    {
                       if (table.state() == EN_TABLE_STATE_WAIT_QIANG_DI_ZHU)
                       {
                           info.add_sdr_total_action_flows()->CopyFrom(table.total_action_flows(j));
                       }
                    }
                    else
                    {
                        info.add_sdr_total_action_flows()->CopyFrom(table.total_action_flows(j));
                    }
                }
            }
        }
        if (info.sdr_total_action_flows_size() > 0)
        {
            if (!info.sdr_total_action_flows(info.sdr_total_action_flows_size()-1).action().has_col_info())
            {
                info.mutable_sdr_total_action_flows()->RemoveLast();
            }
        }
    }
//    info.set_xiaojia_index(table.xiaojia_index());
    info.set_dealer(-1);
    info.set_tid(table.tid());
    info.mutable_sdr_conf()->CopyFrom(table.config());
    info.set_creator_uid(table.creator_uid());
    info.set_left_card_num(table.cards_size());
    info.set_operation_index(table.operation_index());
    if(table.has_dissolve_info())
    {
        info.mutable_dissolve_info()->CopyFrom(table.dissolve_info());
    }
    {
        const PBSDRAction* paction = GetLastActionInFlow(table);
        if (paction != NULL)
        {
            if (paction->act_type() == EN_SDR_ACTION_NAPAI)
            {
                info.set_is_mopai(true);
            }
            info.set_dest_card(paction->dest_card());
            // 只有出牌和拿牌的时候才需要传 dest_card
            if (paction->act_type() != EN_SDR_ACTION_NAPAI && paction->act_type() != EN_SDR_ACTION_CHUPAI)
            {
                info.set_dest_card(0);
            }
        }
    }
}

//void TableLogic::GetChoiceList(CPBGameTable & table, PBSDRTableSeat & seat, int dest_card, vector<PBSDRAction> & ret)
//{

//}

void TableLogic::ResetPlayerChoice(CPBGameTable & table)
{
    VLogMsg(CLIB_LOG_LEV_DEBUG, "ResetPlayerChoice all ...");
    for(int i=0;i<table.seats_size();i++)
    {
        PBSDRTableSeat & seat = *table.mutable_seats(i);
        PBSDRActionChoice & seat_action_choice = *seat.mutable_action_choice();
        seat_action_choice.clear_choices();
        seat_action_choice.clear_determined_action();
        seat_action_choice.set_max_level(0);
        seat_action_choice.set_is_determine(false);
        seat_action_choice.clear_determined_action();
        seat_action_choice.set_determined_level(0);
    }
}
void TableLogic::ResetPlayerChoice(CPBGameTable & table, int seat_index)
{
    VLogMsg(CLIB_LOG_LEV_DEBUG, "ResetPlayerChoice cur[%d] ...", seat_index);
    for(int i=0;i<table.seats_size();i++)
    {
        if (seat_index != i) continue;
        PBSDRTableSeat & seat = *table.mutable_seats(i);
        PBSDRActionChoice & seat_action_choice = *seat.mutable_action_choice();
        seat_action_choice.clear_choices();
        seat_action_choice.clear_determined_action();
        seat_action_choice.set_max_level(0);
        seat_action_choice.set_is_determine(false);
        seat_action_choice.clear_determined_action();
        seat_action_choice.set_determined_level(0);
    }
}

bool TableLogic::RemoveCardForSeat(PBSDRTableSeat & seat, int card)
{
    bool ret = false;
    for(int i = 0; i < seat.hand_cards_size(); i++)
    {
        if(seat.hand_cards(i) == card)
        {
            int del_card = 0;
            seat.mutable_hand_cards()->ExtractSubrange(i, 1, &del_card);
            VLogMsg(CLIB_LOG_LEV_DEBUG, "remove card[%d] for seat[%d] uid[%ld] size[%d]",
                    del_card, seat.index(), seat.user().uid(), seat.hand_cards_size());
            ret =  true;
            break;
        }
    }
    if (!ret)
    {
        VLogMsg(CLIB_LOG_LEV_DEBUG, "remove card[%d] for seat[%d] uid[%ld] failed ...", card, seat.index(), seat.user().uid());
    }

    return ret;
}

int TableLogic::CalcChoiceLevel(const PBSDRAction & action)
{
    // choice 优先级 0x0 - 0xB
    if (action.act_type() == EN_SDR_ACTION_PASS)
    {
        return 0x1;
    }
    else
    {
        return 0x2;
    }
}

void TableLogic::AddChoise(PBSDRTableSeat & seat,const PBSDRAction & action)
{
    PBSDRActionChoice & action_choice = *seat.mutable_action_choice();
    action_choice.add_choices()->CopyFrom(action);

    int level = CalcChoiceLevel(action);
    if(action_choice.max_level() < level)
    {
        action_choice.set_max_level(level);
    }
}



//int TableLogic::GetChoiceListOnChupai(CPBGameTable & table, PBSDRTableSeat & seat)
//{
//    int flow_index = 0;
//    for (int i = table.total_action_flows_size()-1; i >= 0; i--)
//    {
//        if (table.total_action_flows(i).action().act_type() == EN_SDR_ACTION_CHUPAI
//                || table.total_action_flows(i).action().act_type() == EN_SDR_ACTION_NAPAI)
//        {
//            flow_index = i;
//            break;
//        }
//    }
//    const PBSDRActionFlow & currentActionflow = table.total_action_flows(flow_index);
//    const PBSDRAction & currentAction = currentActionflow.action();
//    int cur_card = currentAction.dest_card();
//    bool inc_choice_token = false;

//    vector<PBSDRAction> ret;
//    GetChoiceList(table, seat, cur_card, ret);
//    for(uint32_t i = 0; i < ret.size(); i++)
//    {
//        PBSDRAction action;
//        action.CopyFrom(ret[i]);
//        AddChoise(seat,action);
//        inc_choice_token = true;
//        VLogMsg(CLIB_LOG_LEV_DEBUG,"Add Choice[Type:%d] For User[%ld] On Seat[%d] Of Table[%ld]",
//                action.act_type(), seat.user().uid(), seat.index(), table.tid());
//    }
//    if(seat.action_choice().choices_size() == 0)
//    {
//        return -1;
//    }

//    if(inc_choice_token)
//    {
//        seat.set_action_token(seat.action_token()+1);
//    }

//    return 0;
//}


void TableLogic::ShuffleCards(CPBGameTable& table)
{
    for (int h = 1; h <= 4; h++)
    {
        for (int l = 2; l <= 14; l++)
        {
            int card = ComposeCard(h, l);
            table.mutable_cards()->Add(card);
        }
    }

    {
        table.mutable_cards()->Add(0x51);   // 小王
        table.mutable_cards()->Add(0x52);   // 大王
    }

    int count = (time(NULL) & 3) + 3;
    for (int i = 0; i <= count; i++)
    {
        std::random_shuffle(table.mutable_cards()->begin(), table.mutable_cards()->end());
    }
}
void TableLogic::DealCards(RepeatedField<int>& src, RepeatedField<int>& des, int num/* = 1*/)
{
    for (int i = 0; i < num; i++)
    {
        des.Add(src.Get(i));
    }
    src.ExtractSubrange(0, num, NULL);
}

void TableLogic::DealHandCards(CPBGameTable & table)
{
    table.table_log << "牌堆：\n";
    for (int i = 0; i < table.cards_size(); i++)
    {
        table.table_log << table.cards(i) << ",";
        if (i%10 == 9)
            table.table_log << "\n";
    }
    table.table_log << "\n";

#ifdef _DEBUG
    {
        int vec[60] = {78,82,57,23,25,71,24,22,66,38,
                       39,41,46,43,68,34,26,51,35,75,
                       44,77,40,55,74,70,54,72,18,69,
                       36,21,81,19,45,50,30,67,42,28,
                       29,62,20,102,52,73,53,56,27,61,
                       58,59,37,76,60,};

         table.clear_cards();
         for (int i = 0; i < 60; i++)
         {
             if (vec[i] == 0)
                 break;
             table.mutable_cards()->Add(vec[i]);
         }
    }
#endif
    //牌堆的写牌
    RepeatedField<int> rfTableCardCopy(table.cards());

    if (table.seats_size() == 3)   // 三人玩法
    {
        // 每人发17张
        for (int i = 0; i < 51; i++)
        {
            int index = (table.dealer_index()+i) % table.seats_size();
            DealCards(*table.mutable_cards(), *table.mutable_seats(index)->mutable_hand_cards(), 1);
        }
    }

    WriteCard(table,rfTableCardCopy);

#ifdef _DEBUG
    {   //   胡牌测试
        //TestHu(table);
        //TestHu_2(table);
//        TestHu_3(table);
    }
#endif


    for (int i = 0; i < table.seats_size(); i++)
    {
        const PBSDRTableSeat& seat = table.seats(i);
        if (i == table.dealer_index())
        {
            table.table_log<<"给庄家["<<seat.index()<<","<<seat.user().uid()<<"] 发手牌 : " << CardsToString(seat.hand_cards()) << "\n";
        }
        else
        {
            table.table_log<<"给闲家["<<seat.index()<<","<<seat.user().uid()<<"] 发手牌 : " << CardsToString(seat.hand_cards()) << "\n";
        }
    }
}

bool FuncHandCards(int card1, int cards2)
{
    // 最基本的排序
    return card1 < cards2;
}

void TableLogic::SortHandCards(CPBGameTable& table)
{
    for(int i=0;i<table.seats_size();i++)
    {
        PBSDRTableSeat& seat = *table.mutable_seats(i);
        std::sort(seat.mutable_hand_cards()->begin(), seat.mutable_hand_cards()->end(), FuncHandCards);
    }
}

int TableLogic::ProcessAutoAction(PBSDRGameTable & table, const PBSDRTableSeat & seat, ENSDRActionType act_type)
{
	// 听牌是自动操作的
	// seat 的 choice列表是按优先级牌的，所以第一个就是优先级最高的
	const PBSDRActionChoice& choice = seat.action_choice();

	for (int i = 0 ; i < choice.choices_size() ; i ++)
	{
		const PBSDRAction & action = choice.choices(i);
		if (action.act_type() == act_type && !choice.is_determine())
		{
			PBCSMsg msg;
			InterEventAutoAction& inter_event = *msg.mutable_inter_event_auto_action();
			inter_event.mutable_sdr_request_action()->CopyFrom(action);
			inter_event.set_token(seat.action_token());
			GameHandlerProxy::Instance()->PushInnerMsg(msg);

			VLogMsg(CLIB_LOG_LEV_DEBUG, "自动操作：uid[%ld] act_type[%d] dest_card[%d]",
				seat.user().uid(), action.act_type(), action.dest_card());

			return 0; // 返回0表示不需要给客户端发送操作列表通知
		}
	}

	return -1;
}

int TableLogic::ProcessAutoActionX(CPBGameTable & table, const PBSDRTableSeat & seat, ENSDRActionType act_type)
{
    // 听牌是自动操作的
    // seat 的 choice列表是按优先级牌的，所以第一个就是优先级最高的
    const PBSDRActionChoice& choice = seat.action_choice();
    const PBSDRAction& request_action = choice.choices(0);
    if (request_action.act_type() == act_type && !choice.is_determine())
    {
        PBCSMsg msg;
        InterEventAutoAction& inter_event = *msg.mutable_inter_event_auto_action();
        inter_event.mutable_sdr_request_action()->CopyFrom(request_action);
        inter_event.set_token(seat.action_token());
        GameHandlerProxy::Instance()->PushInnerMsg(msg);

        VLogMsg(CLIB_LOG_LEV_DEBUG, "自动操作：uid[%ld] act_type[%d] dest_card[%d]",
                seat.user().uid(), request_action.act_type(), request_action.dest_card());

        return 0; // 返回0表示不需要给客户端发送操作列表通知
    }
    return -1;
}

void TableLogic::BroadcastNextOperation(const CPBGameTable & table, int seat_index)
{
    //广播操作
    PBCSMsg msg;
    CSNotifyNextOperation & cs_notify_next_operation = *msg.mutable_cs_notify_next_operation();
    cs_notify_next_operation.set_operation_index(seat_index);
    cs_notify_next_operation.set_left_card_num(table.cards_size());
    //cs_notify_next_operation.set_state(table.state());
    BroadcastTableMsg(table,msg);
}

void TableLogic::AddChupaiChoiceForSeat(CPBGameTable & table, PBSDRTableSeat & seat)
{
//    const PBSDRActionFlow & currentActionflow = table.total_action_flows(table.total_action_flows_size()-1);
//    const PBSDRAction & currentAction = currentActionflow.action();
//    int cur_card = currentAction.dest_card();
    bool inc_choice_token = false;

    // 轮到玩家操作 添加一个出牌操作给玩家
    PBSDRAction action;
    action.set_seat_index(seat.index());
    action.set_tid(table.tid());
    action.set_act_type(EN_SDR_ACTION_CHUPAI);
    AddChoise(seat, action);
//    action.set_act_type(EN_SDR_ACTION_PASS);
//    AddChoise(seat, action);

    inc_choice_token = true;

    if(inc_choice_token)
    {
        seat.set_action_token(seat.action_token()+1);
    }
}

PBSDRAction& TableLogic::GetLastActionInFlow(CPBGameTable & table)
{
    PBSDRActionFlow & current_flow = *table.mutable_total_action_flows(table.total_action_flows_size()-1);
    PBSDRAction & current_action = *current_flow.mutable_action();
    return current_action;
}

const PBSDRAction* TableLogic::GetLastActionInFlow(const CPBGameTable & table)
{
    if (table.total_action_flows_size() > 0)
    {
        const PBSDRActionFlow & current_flow = table.total_action_flows(table.total_action_flows_size()-1);
        const PBSDRAction & current_action = current_flow.action();
        return &current_action;
    }
    return NULL;
}

const PBSDRAction* TableLogic::GetLastActionPtrInFlow(const CPBGameTable & table)
{
    if (table.total_action_flows_size() > 0)
    {
        const PBSDRActionFlow & current_flow = table.total_action_flows(table.total_action_flows_size()-1);
        const PBSDRAction & current_action = current_flow.action();
        return &current_action;
    }
    return NULL;
}

const PBSDRAction* TableLogic::GetLastActionInFlow_ForXPai(const CPBGameTable & table)
{
    if (table.total_action_flows_size() > 0)
    {
        for (int i = table.total_action_flows_size()-1; i >= 0; i--)
        {
            const PBSDRActionFlow & current_flow = table.total_action_flows(i);
            const PBSDRAction & current_action = current_flow.action();
            if (current_action.act_type() == EN_SDR_ACTION_NAPAI
                    || current_action.act_type() == EN_SDR_ACTION_CHUPAI)
            {
                return &current_action;
            }
        }
    }
    return NULL;
}

const PBSDRAction* TableLogic::GetLastActionPtrInFlow(const CPBGameTable & table, int index)
{
    if (table.total_action_flows_size() > index)
    {
        const PBSDRActionFlow & current_flow = table.total_action_flows(index);
        const PBSDRAction & current_action = current_flow.action();
        return &current_action;
    }
    return NULL;
}

const PBSDRAction* TableLogic::GetLastActionPtrInFlow(const CPBGameTable & table, ENSDRActionType type)
{
    if (table.total_action_flows_size() > 0)
    {
        for (int i = table.total_action_flows_size()-1; i >= 0; i--)
        {
            const PBSDRActionFlow & current_flow = table.total_action_flows(i);
            const PBSDRAction & current_action = current_flow.action();
            if (current_action.act_type() == type)
            {
                return &current_action;
            }
        }
    }
    return NULL;
}

void TableLogic::SendInterEventOnDoActionOver(CPBGameTable& table,int type, const PBSDRAction& action)
{
    VLogMsg(CLIB_LOG_LEV_DEBUG, "SendInterEventOnDoActionOver type[%d]", type);

    PBCSMsg msg;
    InterEventOnDoActionOver& inter_event = *msg.mutable_inter_event_on_do_action_over();
    inter_event.mutable_sdr_dest_action()->CopyFrom(action);
    inter_event.set_type(type);

//    //延迟发送
//    int iMilliSecond = GetWaitingTime(table,action.act_type());
//    table.StartActionWaitingTimerByMillis(iMilliSecond,false);
//    table.m_vtInnerMsg.push_back(msg);
    GameHandlerProxy::Instance()->PushInnerMsg(msg);
}


void TableLogic::OnDissolveTableTimeOut(CPBGameTable & table)
{
    table.table_log << "table_id[" << table.tid() << "] 超时解散" << "\n";
    table.set_is_dissolve_finish(true);
    table.set_is_game_over(true);
    table.set_state(EN_TABLE_STATE_AUTO_DISSOLVING);
    GameFinish(table);
}


void TableLogic::LogoutTable(CPBGameTable & table,int index)
{
    if(index<0 || index >= table.seats_size())
    {
        return;
    }
    PBCSMsg notify;
    CSNotifyLogoutTable & cs_notify_logout_table = *notify.mutable_cs_notify_logout_table();
    cs_notify_logout_table.set_seat_index(index);
    BroadcastTableMsg(table,notify);
    PBSDRTableSeat & seat = *table.mutable_seats(index);

	//如果是茶馆房间，刷新下茶馆人数
	if (table.config().has_tbid() && table.config().has_master_uid())
	{
		PBCSMsg notify;
		SSNotifyTeaBarTablePlayerNum& ss_notify_teabar_table_player_num = *notify.mutable_ss_notify_teabar_table_player_num();
		ss_notify_teabar_table_player_num.mutable_conf()->CopyFrom(table.config());
		ss_notify_teabar_table_player_num.set_tid(table.tid());
		ss_notify_teabar_table_player_num.set_player_num(table.seats_size() - GetPlayerNumByState(table, EN_SEAT_STATE_NO_PLAYER));
		Message::PushInnerMsg(RouteManager::Instance()->GetRouteByRandom(), 0, notify, EN_Node_TeaBar, 1);
	}

    TableManager::Instance()->OnPlayerLeaveTable(seat.user().uid(), table.tid());

	seat.set_state(EN_SEAT_STATE_NO_PLAYER);
	seat.clear_action_choice();
	seat.clear_hand_cards();
	seat.set_action_token(0);
	seat.set_final_score(0);
	seat.set_multiple(0);
	seat.set_need_check_double(false);
	seat.set_double_times(-1);
	seat.set_bomb_num(0);
	seat.set_bao_type(0);
	seat.clear_win_info();
	seat.set_is_buy(false);
	seat.set_soft_bomb_num(0);
	seat.set_is_lai_zhuang(false);
	seat.set_is_baozi_zhuang(false);
	seat.set_qiang_di_zhu_times(0);
	seat.set_is_mingpai_after_deal_cards(-1);
	if (seat.is_mingpai() == 1)
	{
		seat.set_is_mingpai_after_deal_cards(2);
	}
	seat.set_is_jiao_dizhu(false);
	seat.clear_is_trusteeship();
	seat.clear_trusteeship_time();
	seat.clear_end_trusteeship_time();
}

void TableLogic::SendGameFinishNotify(CPBGameTable & table)
{
    PBCSMsg msg;
    CSNotifyFPFFinish& notify = *msg.mutable_cs_notify_fpf_finish();


    for (int i = 0; i < table.seats_size(); i++)
    {
        const PBSDRTableSeat& seat = table.seats(i);
        NotifyFPFFinishItem& item = *notify.add_result_list();
        item.set_uid(seat.user().uid());
        item.set_total_score(seat.total_score());
        item.set_chips(seat.user().chip());
    }

    if (table.is_dissolve_finish())
    {
        notify.set_is_dissolve_finish(true);
    }

    // 缓存牌局结果
    PBGameRecord& record = *table.mutable_record();
    record.set_tid(table.tid());
    record.set_stamp(table.start_stamp);
    record.set_recordid((table.tid()<<32) + table.start_stamp);
    record.set_game_type(EN_Table_3Ren_DDZ_COIN);
    for (int i = 0; i < table.seats_size(); i++)
    {
        const PBSDRTableSeat& seat = table.seats(i);
        {
            // 保护代码 后面可考虑删除
            if (record.final_user_scores_size() > table.seats_size())
            {
                VLogMsg(CLIB_LOG_LEV_DEBUG, "二次调用 GameFinish ...");
                break;
            }
        }
        PBUserScoreInfo& score_info = *record.add_final_user_scores();
        score_info.set_nick(seat.user().nick());
        score_info.set_score(seat.total_score());
        score_info.set_uid(seat.user().uid());
        score_info.set_role_pic_url(table.seats(i).user().role_picture_url());
    }

    BroadcastTableMsg(table, msg);
}


string TableLogic::CardToString(int src)
{
    string out = "[";

    int h = GetHigh(src);
    int l = GetLow(src);
    switch(h)
    {
        case 1:     out += "方块"; break;
        case 2:     out += "梅花"; break;
        case 3:     out += "红心"; break;
        case 4:     out += "黑桃"; break;
    }
    switch(l)
    {
        case 2:     out += "2"; break;
        case 3:     out += "3"; break;
        case 4:     out += "4"; break;
        case 5:     out += "5"; break;
        case 6:     out += "6"; break;
        case 7:     out += "7"; break;
        case 8:     out += "8"; break;
        case 9:     out += "9"; break;
        case 10:     out += "10"; break;
        case 11:     out += "J"; break;
        case 12:     out += "Q"; break;
        case 13:     out += "K"; break;
        case 14:     out += "A"; break;
    }
    switch(l)
    {
        case 0x51:     out += "小王"; break;
        case 0x52:     out += "大王"; break;
        case 0x66:     out += "花牌"; break;
    }

    out += "]";
    return out;
}
string TableLogic::CardsToString(const RepeatedField<int> & src)
{
    string out = "[ ";
    for(int i=0;i<src.size();i++)
    {
        int h = GetHigh(src.Get(i));
        int l = GetLow(src.Get(i));
        switch(src.Get(i))
        {
            case 0x51:     out += "小王"; break;
            case 0x52:     out += "大王"; break;
            case 0x66:     out += "花牌"; break;
            default:
            {
                switch(h)
                {
                    case 1:     out += "方块"; break;
                    case 2:     out += "梅花"; break;
                    case 3:     out += "红心"; break;
                    case 4:     out += "黑桃"; break;
                }
                switch(l)
                {
                    case 2:     out += "2"; break;
                    case 3:     out += "3"; break;
                    case 4:     out += "4"; break;
                    case 5:     out += "5"; break;
                    case 6:     out += "6"; break;
                    case 7:     out += "7"; break;
                    case 8:     out += "8"; break;
                    case 9:     out += "9"; break;
                    case 10:     out += "10"; break;
                    case 11:     out += "J"; break;
                    case 12:     out += "Q"; break;
                    case 13:     out += "K"; break;
                    case 14:     out += "A"; break;
                }
                break;
            }
        }
        out += ", ";
    }
    out += "]";
    return out;
}

string TableLogic::ActionToString(const PBSDRAction & action)
{
    string out = "";
    char buff[1024] = {0};
    switch(action.act_type())
    {
        case EN_SDR_ACTION_PASS:
        {
            out += "过";
            break;
        }
        case EN_SDR_ACTION_CHUPAI:
        {
            snprintf(buff,sizeof(buff),"出牌%s",CardsToString(action.cards()).c_str());
            out+=buff;
            break;
        }
        case EN_SDR_ACTION_JIA_BEI :
        {
            out += "加倍";
            break;
        }
        case EN_SDR_ACTION_BU_JIA_BEI :
        {
            out += "不加倍";
            break;
        }
        case EN_SDR_ACTION_BUY :
        {
            out += "买牌";
            break;
        }
        case EN_SDR_ACTION_SELL :
        {
            out += "卖牌";
            break;
        }
        default:
            snprintf(buff, sizeof(buff), " 未知操作[%d]", action.act_type());
            out += buff;
            break;
    }
    return out;
}

void TableLogic::TestHu_3(CPBGameTable & table)
{
    {
        RepeatedField<int>& cards = *table.mutable_seats(0)->mutable_hand_cards();
        cards.Clear();

        cards.Add(0x19);
        cards.Add(0x29);
        cards.Add(0x39);
        cards.Add(0x1a);
        cards.Add(0x2a);
        cards.Add(0x3a);

        cards.Add(0x14);
        cards.Add(0x25);
        cards.Add(0x36);
        cards.Add(0x66);
    }

//    {
//        RepeatedField<int>& cards = *table.mutable_seats(1)->mutable_hand_cards();
//        cards.Clear();
//    }

//    {
//        RepeatedField<int>& cards = *table.mutable_seats(2)->mutable_hand_cards();
//        cards.Clear();
//    }

}


void TableLogic::GetLastXPai(const CPBGameTable& table, const PBSDRTableSeat& seat, bool& is_zimo, bool& is_tamo, bool& is_tachu)
{
    const PBSDRAction* p_last_action = GetLastActionInFlow_ForXPai(table);
    if (p_last_action != NULL)
    {
        if (p_last_action->act_type() == EN_SDR_ACTION_NAPAI)
        {
            if (p_last_action->seat_index() == seat.index())
            {
                is_zimo = true;
            }
            else
            {
                is_tamo = true;
            }
        }
        if (p_last_action->act_type() == EN_SDR_ACTION_CHUPAI)
        {
            is_tachu = true;
        }
    }
}


void TableLogic::SendUpdateRankMsg(int rank_id, long long rank_key, long long rank_score)
{
    PBCSMsg msg;
    SSRequestUpdateRankList& request = *msg.mutable_ss_request_update_rank_list();
    request.set_rank_id(rank_id);
    request.set_rank_key(rank_key);
    request.set_rank_score(rank_score);

    Message::PushInnerMsg(RouteManager::Instance()->GetRouteByRandom(), rank_id, msg, EN_Node_DBProxy,rank_id , EN_Route_hash);
}

bool TableLogic::NeedPayForTable(const CPBGameTable& table)
{
    // 第一局，并且庄家已经出牌了 保证这一局不是重开的
    if (table.round() == 1)
    {
        for (int i = 0; i < table.total_action_flows_size(); i++)
        {
            if (table.total_action_flows(i).action().act_type() == EN_SDR_ACTION_CHUPAI)
            {
                return true;
            }
        }
    }
    return false;
}

bool TableLogic::NeedPayForTable(int round, int state)
{
    if (round < 1)
    {
        return false;
    }
    if (round == 1 && state != EN_TABLE_STATE_SINGLE_OVER)
    {
        //第一轮还没有结束
        return false;
    }
    return true;
}

// ====================================================================================================================
bool TableLogic::IsLaiZi(const PBSDRGameTable & table, int card)
{
    // 赖子
    if (GetCardValue(card) == GetCardValue(table.laizi_card()) )
    {
        return true;
    }
    // 3点也可能是赖子
//    if (table.config().lai_zi_num() >= 5 && GetCardValue(card) == 3)
//    {
//        return true;
//    }
    return false;
}

bool TableLogic::IsBiZhua(const CPBGameTable & table, const PBSDRTableSeat & seat)
{
//    std::map<int, int> card_map;
    int joker_num = 0;
    int boom_num = 0;
    int two_num = 0;
    map<int , int> card_map;
    for (int i = 0; i < seat.hand_cards_size(); i++)
    {
        int card = seat.hand_cards(i);
        card_map[GetCardValue(seat.hand_cards(i))]++;
//        card_map[card]++;
        if (GetCardType(card) == 5)
        {
            joker_num ++;
        }
        else if (GetCardValue(card) == 2)
        {
            two_num++;
        }
    }

    map<int,int>::iterator it = card_map.begin();
    for ( ;it != card_map.end();++it)
    {
        if (it->second == 4)
        {
            boom_num++;
        }
    }

//    1、大小王必须拿牌叫地主(吃包子)
//    2、一王二2必须拿牌叫地主(吃包子)
//    3、二个硬炸必须拿牌叫地主(吃包子)
//    4、1王+1二+1硬炸，必须拿牌叫地主(吃包子)

    if (joker_num == 2
            || (joker_num >= 1 && two_num >= 2)
            || (boom_num >= 2)
            || (joker_num >= 1 && two_num >= 1 && boom_num >= 1))
    {
        return true;
    }

    return false;
}

const PBSDRAction* TableLogic::GetLastDeterminedActionChuPai(const CPBGameTable & table)
{
    if (table.total_action_flows_size() > 0)
    {
        for (int i = table.total_action_flows_size()-1; i >= 0; i--)
        {
            const PBSDRActionFlow & current_flow = table.total_action_flows(i);
            const PBSDRAction & current_action = current_flow.action();
            if (current_action.act_type() == EN_SDR_ACTION_CHUPAI
                    && current_action.col_info().cards_size() > 0)
            {
                return &current_action;
            }
        }
    }
    return NULL;
}

bool TableLogic::CheckSingleOver(const CPBGameTable& table, const PBSDRTableSeat& seat)
{
    int next_index = (seat.index()+1)%table.seats_size();
    const PBSDRAction* p_action = TableLogic::GetLastDeterminedActionChuPai(table);
    if (p_action != NULL && p_action->seat_index() == next_index)
    {
       return true;
    }
    return false;
}

void TableLogic::AnalyzeHuStyles(CPBGameTable& table, PBSDRTableSeat & seat)
{
    bool is_chun_tian = false;
//    bool is_fan_chun_tian = false;
//    bool is_wei_jia = false;
    // 统计番数
    int multiple = 0;
    int is_mingpai_start = 1;
    {
        std::map<int, int> record;
        if (table.total_action_flows_size() > 0)
        {
            for (int i = 0; i < table.total_action_flows_size(); i++)
            {
                const PBSDRActionFlow & current_flow = table.total_action_flows(i);
                const PBSDRAction & current_action = current_flow.action();
                if (current_action.act_type() == EN_SDR_ACTION_CHUPAI)
                {
                    record[current_action.seat_index()]++;
                }
            }
        }

        if (record.size() == 1 && record.begin()->first == table.dealer_index())
        {
            is_chun_tian = true;
            // 春天
            multiple += 1;
            VLogMsg(CLIB_LOG_LEV_DEBUG, "春天");
        }
        // 炸弹
        {
            int bomb_num = table.bomb_num();
            int soft_bomb_num = table.soft_bomb_num();
            multiple += bomb_num;
            multiple += soft_bomb_num;
            VLogMsg(CLIB_LOG_LEV_DEBUG, "硬炸弹[%d] ， 软炸弹[%d]", bomb_num,soft_bomb_num);
        }
//        // 尾家
//        if (table.dealer_index() == (table.seats_size()+table.dealer_index_2()-1)%table.seats_size())
//        {
//            is_wei_jia = true;
//            multiple++;
//            VLogMsg(CLIB_LOG_LEV_DEBUG, "尾家");
//        }
    }

    // 封顶
    if (table.config().max_multiple() > 0)
    {
        multiple = multiple > table.config().max_multiple() ? table.config().max_multiple() : multiple;
    }

    // 设置分数
    {
        int inc = 1 * powl(2,multiple) * is_mingpai_start;
        bool dealer_win = seat.index() == table.dealer_index();

        PBSDRTableSeat& dealer = *table.mutable_seats(table.dealer_index());

        {
            if (dealer.qiang_di_zhu_times() >= 10)
            {
                inc = inc * 2;
                dealer.mutable_win_info()->add_styles(EN_SDR_STYLE_TYPE_Qiang_Di_Zhu);
            }
            if (dealer.qiang_di_zhu_times() >= 20)
            {
                inc = inc * 2;
                dealer.mutable_win_info()->add_styles(EN_SDR_STYLE_TYPE_Qiang_Di_Zhu);
            }
            if (dealer.double_times() > 0)
            {
                inc = inc * 2;
                dealer.mutable_win_info()->add_styles(EN_SDR_STYLE_TYPE_Jia_Bei);
            }
            if (dealer.is_mingpai_after_deal_cards() == 1)
            {
                inc = inc * 2;
                dealer.mutable_win_info()->add_styles(EN_SDR_STYLE_TYPE_Ming_Pai);
            }
            if (dealer.is_mingpai() == 1)
            {
                inc = inc * 5;
                dealer.mutable_win_info()->add_styles(EN_SDR_STYLE_TYPE_Ming_Pai_Start);
            }
        }

        for (int i = 0; i < table.seats_size(); i++)
        {
            if (i == table.dealer_index())
            {
                continue;
            }
            PBSDRTableSeat& i_seat = *table.mutable_seats(i);
            int single_score = inc;
            if (i_seat.qiang_di_zhu_times() >= 10)
            {
                single_score = single_score * 2;
                i_seat.mutable_win_info()->add_styles(EN_SDR_STYLE_TYPE_Qiang_Di_Zhu);
            }
            if (i_seat.qiang_di_zhu_times() >= 20)
            {
                single_score = single_score * 2;
                i_seat.mutable_win_info()->add_styles(EN_SDR_STYLE_TYPE_Qiang_Di_Zhu);
            }
            if (i_seat.double_times() > 0)
            {
                single_score = single_score * 2;
                i_seat.mutable_win_info()->add_styles(EN_SDR_STYLE_TYPE_Jia_Bei);
            }
            if (i_seat.is_mingpai_after_deal_cards() == 1)
            {
                single_score = single_score * 2;
                i_seat.mutable_win_info()->add_styles(EN_SDR_STYLE_TYPE_Ming_Pai);
            }
            if (i_seat.is_mingpai() == 1)
            {
                single_score = single_score * 5;
                i_seat.mutable_win_info()->add_styles(EN_SDR_STYLE_TYPE_Ming_Pai_Start);
            }

            if (i != table.dealer_index())
            {
                // 封顶
                if (table.config().max_line() > 0)
                {
                    single_score = single_score > table.config().max_line() ? table.config().max_line() : single_score;
                }
                if (dealer_win)
                {
                    i_seat.set_final_score(0-single_score);
                    dealer.set_final_score(dealer.final_score()+single_score);
                }
                else
                {
                    i_seat.set_final_score(single_score);
                    dealer.set_final_score(dealer.final_score()-single_score);
                }
            }

			//金币场中设置金币加减
			i_seat.mutable_win_info()->set_coins_change_value(i_seat.final_score());
        }

        for (int i = 0; i < table.seats_size(); i++)
        {
            PBSDRTableSeat& i_seat = *table.mutable_seats(i);
            i_seat.set_total_score(i_seat.total_score()+i_seat.final_score());
            VLogMsg(CLIB_LOG_LEV_DEBUG, "seat[%d] multiple[%d] final_score[%d] total_score[%d]", i, multiple, i_seat.final_score(), i_seat.total_score());
        }
    }

    // 设置名堂
    {
        for (int i = 0; i < table.seats_size(); i++)
        {
            PBSDRTableSeat& i_seat = *table.mutable_seats(i);
            if (i == table.dealer_index())
            {
                if (is_chun_tian)
                {
                    i_seat.mutable_win_info()->add_styles(EN_SDR_STYLE_TYPE_Chun_Tian);
                }
            }
//            else
//            {
//                if (is_fan_chun_tian)
//                {
//                    i_seat.mutable_win_info()->add_styles(EN_SDR_STYLE_TYPE_Chun_Tian);
//                }
//            }
        }
    }
}

bool TableLogic::NeedWaitingOtherOpera(CPBGameTable & table,PBSDRTableSeat & request_seat,const PBSDRAction & request_action)
{
    const PBSDRActionChoice & user_action_choice = request_seat.action_choice();
    const PBTableUser & user = request_seat.user();
    int64 uid = user.uid();
    int64 tid = table.tid();
    int last_opera_index = GetLastOperateSeatIndex(table);
    if (last_opera_index == -1)
    {
        last_opera_index = table.dealer_index();
    }

    int determine_max_level = table.max_pre_choice().determined_level();
    for(int i = 0; i < table.seats_size(); i++)
    {
        if(i == request_seat.index())
        {
            continue;
        }
        const PBSDRTableSeat & seat = table.seats(i);
        const PBSDRActionChoice & action_choice = seat.action_choice();
        if(action_choice.is_determine() == true /*|| seat.state() == EN_SEAT_STATE_WIN*/)
        {
            continue;
        }
        if(action_choice.max_level() == 0 || action_choice.max_level() < determine_max_level)
        {
            //当前座位没有操作, 或者操作小于已经决定了的最大操作
            continue;
        }
        VLogMsg(CLIB_LOG_LEV_DEBUG,"seat[%d]:max_level[%d] cur_seat:determined_level[%d]",
                i, action_choice.max_level(), user_action_choice.determined_level());

        if(action_choice.max_level() > user_action_choice.determined_level())
        {
            //还有等级更高的操作
            VLogMsg(CLIB_LOG_LEV_DEBUG,"player[%lld] do action[%d] on table[%lld],waiting for other seat[%d,%d]",
                    uid,request_action.act_type(),tid,i,action_choice.max_level());
            return true;
        }
        else if(action_choice.max_level() == user_action_choice.determined_level())
        {
//            // 优先级相同的情况下 比较操作顺序 按顺序
//            int user_opera_index = (request_seat.index()-last_opera_index)%table.seats_size();
//            user_opera_index = (user_opera_index < 0) ? (user_opera_index + table.seats_size()) : user_opera_index;
//            int seat_opera_index = (i-last_opera_index)%table.seats_size();
//            seat_opera_index = (seat_opera_index < 0) ? (seat_opera_index + table.seats_size()) : seat_opera_index;
//            VLogMsg(CLIB_LOG_LEV_DEBUG, "seat[%d] user_opera_index[%d] seat_opera_index[%d] ...", seat.index(), user_opera_index, seat_opera_index);
//            if(user_opera_index > seat_opera_index)
//            {
//                //还有等级更高的操作
//                VLogMsg(CLIB_LOG_LEV_DEBUG,"player[%lld] do action[%d] on table[%lld],waiting for other seat[%d,%d]",
//                        uid,request_action.act_type(),tid,i,action_choice.max_level());
//                return true;
//            }
            return true;
        }
    }
    return false;
}

/*------------------------------------------------------------闯关-------------------------------------------------------------------------------------------*/
/*
 * 获得桌子上玩家的数量
*/
int TableLogic::GetPlayerNum(const CPBGameTable & a_CTable)
{
    int iNum = 0;
    for(int i = 0 ; i < a_CTable.seats_size() ; i ++)
    {
        const PBSDRTableSeat & pbSeat = a_CTable.seats(i);
        if(pbSeat.state() != EN_SEAT_STATE_NO_PLAYER)
        {
            iNum ++;
        }
    }

    return iNum;
}
/*
 * 操作定时器到达后操作
*/
void TableLogic::OnOperationTimeOut(CPBGameTable & a_CTable)
{
    for(int i = 0 ; i < a_CTable.seats_size() ; i ++)
    {
        PBSDRTableSeat & pbSeat = *a_CTable.mutable_seats(i);
        if(pbSeat.state() == EN_SEAT_STATE_NO_PLAYER)
        {
            continue;
        }

        if(pbSeat.action_choice().choices_size() == 0 ||
                pbSeat.action_choice().is_determine() == true)
        {
            continue;
        }

        //如果这个玩家现在是托管状态,需要广播他现在开始托管
        if(!pbSeat.is_trusteeship())
        {
            PBCSMsg pbNotify;
            CSNotifyTrusteeship & cs_notify_trusteeship = *pbNotify.mutable_cs_notify_trusteeship();
            cs_notify_trusteeship.set_seat_index(i);
            cs_notify_trusteeship.set_is_trusteeship(true);
            BroadcastTableMsg(a_CTable, pbNotify);
        }

        VLogMsg(CLIB_LOG_LEV_DEBUG, "Seat[%d] Trusteeship",pbSeat.index());
        DoDefaultAction(a_CTable,pbSeat);
    }
}

/*
 * 执行默认的动作
*/
void TableLogic::DoDefaultAction(CPBGameTable & a_CTable,PBSDRTableSeat & a_pbSeat)
{
    EnableTrusteeship(a_CTable,a_pbSeat);

    //做自动操作
    PBSDRActionChoice & pbActionChoice = *a_pbSeat.mutable_action_choice();
    if(pbActionChoice.choices_size() == 0)
    {
        return ;
    }
	
    //找到里面的过操作
    int iPassIndex = -1;
    int iChuPaiIndex = -1;
    for(int i = 0 ; i < pbActionChoice.choices_size() ; i ++)
    {
       const PBSDRAction & pbAction = pbActionChoice.choices(i);
       if(CheckActionIsPassAction(pbAction.act_type()))
       {
            iPassIndex = i;
            break;
       }
       else if(pbAction.act_type() == EN_SDR_ACTION_CHUPAI)
       {
            iChuPaiIndex = i;
       }
    }

    PBCSMsg pbMsg;
    InterEventAutoAction & inter_event = *pbMsg.mutable_inter_event_auto_action();
    inter_event.set_token(a_pbSeat.action_token());

    //有不出选项的时候,做不出选项
    if(iPassIndex != -1)
    {
        int iActType = (int)pbActionChoice.choices(iPassIndex).act_type();
        VLogMsg(CLIB_LOG_LEV_DEBUG,"table[%ld] seats[%d] auto do action[%d]",a_CTable.tid(),a_pbSeat.index(),iActType);
        inter_event.mutable_sdr_request_action()->CopyFrom(pbActionChoice.choices(iPassIndex));
        inter_event.mutable_sdr_request_action()->set_is_trustee_ship_auto_action(true);
        GameHandlerProxy::Instance()->PushInnerMsg(pbMsg);
    }
    //该当前玩家出牌,默认出第一张
    else if(iChuPaiIndex != -1 && a_pbSeat.hand_cards_size() >= 1)
    {
        int iFirstCard = a_pbSeat.hand_cards(0);
        //构建一个模拟的出牌动作
        PBSDRAction & pbChuPaiAction = *pbActionChoice.mutable_choices(iChuPaiIndex);
        pbChuPaiAction.set_cardtype(EN_POKER_TYPE_SINGLE_CARD);
        pbChuPaiAction.set_real(GetRealValue(iFirstCard));
        pbChuPaiAction.set_num(1);
        pbChuPaiAction.add_cards(iFirstCard);
        pbChuPaiAction.mutable_col_info()->add_cards(iFirstCard);
        pbChuPaiAction.set_is_trustee_ship_auto_action(true);

        VLogMsg(CLIB_LOG_LEV_DEBUG,"table[%ld] seats[%d] auto do action[%d]",a_CTable.tid(),a_pbSeat.index(),pbChuPaiAction.act_type());
        inter_event.mutable_sdr_request_action()->CopyFrom(pbChuPaiAction);
        GameHandlerProxy::Instance()->PushInnerMsg(pbMsg);
    }
    //不出个出牌选项都没有，则默认第一选项
    else
    {
        if(pbActionChoice.choices_size() >= 1)
        {
            VLogMsg(CLIB_LOG_LEV_DEBUG,"table[%ld] seats[%d] auto do action[%d]",a_CTable.tid(),a_pbSeat.index(),pbActionChoice.choices(0).act_type());
            inter_event.mutable_sdr_request_action()->CopyFrom(pbActionChoice.choices(0));
            inter_event.mutable_sdr_request_action()->set_is_trustee_ship_auto_action(true);
            GameHandlerProxy::Instance()->PushInnerMsg(pbMsg);
        }
    }
}

/*
 * 检查是否所有人托管结束
*/
bool TableLogic::CheckIsTrusteeshipOver(CPBGameTable & a_CTable)
{
    bool bIsTrusteeshipError = true;
    for(int i = 0 ; i < a_CTable.seats_size() ; i ++)
    {
        const PBSDRTableSeat & pbSeat = a_CTable.seats(i);
        if(!pbSeat.is_trusteeship())
        {
            bIsTrusteeshipError = false;
        }
    }

    return bIsTrusteeshipError;
}

/*
 * 注册所有动作的延迟时间
*/
void TableLogic::RegisterAllActionWaitingTime(CPBGameTable & a_CTable)
{
    RegisterActionWaitingTime(a_CTable,EN_SDR_ACTION_CHUPAI,500);
    RegisterActionWaitingTime(a_CTable,EN_SDR_ACTION_PASS,300);
    RegisterActionWaitingTime(a_CTable,EN_SDR_ACTION_BU_QIANG,300);
    RegisterActionWaitingTime(a_CTable,EN_SDR_ACTION_BU_JIA_BEI,300);
    RegisterActionWaitingTime(a_CTable,EN_SDR_ACTION_BU_MING,300);
    RegisterActionWaitingTime(a_CTable,EN_SDR_ACTION_BU_JIAO,300);
    RegisterActionWaitingTime(a_CTable,EN_SDR_ACTION_QIANG_DI_ZHU,300);
    RegisterActionWaitingTime(a_CTable,EN_SDR_ACTION_JIAO_DI_ZHU,300);
}

/*
 * 注册特定动作的延迟时间
 * a_iType : 动作类型
 * a_iMilliSecond ： 毫秒级时间
*/
void TableLogic::RegisterActionWaitingTime(CPBGameTable & a_CTable,int a_iType,int a_iMilliSecond)
{
    a_CTable.RegisterActionWaitingTime(a_iType,a_iMilliSecond);
}

/*
 * 获得某个动作完成后等待时间
 * a_iType ： 动作类型
*/
int TableLogic::GetWaitingTime(CPBGameTable & a_CTable,int a_iType)
{
    return a_CTable.GetWaitingTime(a_iType);
}

/*
 * 将table上暂存的消息发送出去
*/
void TableLogic::OnActionWaitingTimerOut(CPBGameTable & a_CTable)
{
    for(unsigned int i = 0 ; i < a_CTable.m_vtInnerMsg.size() ; i ++)
    {
        PBCSMsg pbMsg = a_CTable.m_vtInnerMsg.at(i);
        GameHandlerProxy::Instance()->PushInnerMsg(pbMsg);
    }

    a_CTable.m_vtInnerMsg.clear();
}

/*
 * 检测用户是否离线定时器到达后
 * 自动做动作，并且置托管状态
*/
void TableLogic::OnCheckUserOffTimerOut(CPBGameTable & a_CTable)
{
    OnOperationTimeOut(a_CTable);
}

/*
 * 动作操作后处理托管状态
 * 返回true为结束
 * 返回false为继续当前行为
*/
bool TableLogic::DealWithTrusteeshipAfterAct(CPBGameTable & a_CTable,PBSDRTableSeat & a_pbSeat,const PBSDRAction & action)
{
    //每做一个动作需要停止离线检测
    a_CTable.StopCheckUserOffTimer();

    //如果这个动作是玩家主动出牌，不是托管状态下的出牌，需要停止定时器
    if(!action.is_trustee_ship_auto_action())
    {
        VLogMsg(CLIB_LOG_LEV_DEBUG,"table[%ld] seat[%d] do action by himself, leave trusteeship",a_CTable.tid(),a_pbSeat.index());
        //DisableTrusteeship(a_CTable,a_pbSeat);
    }

    //检查玩家是否3人都托管
    if(CheckIsTrusteeshipOver(a_CTable))
    {
        //桌子的状态不处于已结束或者单局结束的状态
        if(a_CTable.state() != EN_TABLE_STATE_FINISH && a_CTable.state() != EN_TABLE_STATE_SINGLE_OVER)
        {
			//测试，需要打开
			a_CTable.set_is_trusteeship_error(true);
			VLogMsg(CLIB_LOG_LEV_DEBUG, "table[%ld] 3 seats trusteeship, game over", a_CTable.tid());
			GameOver(a_CTable);
			return true;
        }
    }

    return false;
}

/*
 * 检查这个操作是否是过操作
*/
bool TableLogic::CheckActionIsPassAction(int a_iActType)
{
    if(a_iActType == EN_SDR_ACTION_PASS || a_iActType == EN_SDR_ACTION_BU_QIANG || a_iActType == EN_SDR_ACTION_BU_JIA_BEI
            || a_iActType == EN_SDR_ACTION_BU_MING || a_iActType == EN_SDR_ACTION_BU_JIAO)
    {
        return true;
    }

    return false;
}

/*
 * 进入托管状态
*/
void TableLogic::EnableTrusteeship(CPBGameTable & a_CTable,PBSDRTableSeat & a_pbSeat)
{
    //置托管状态
    a_pbSeat.set_is_trusteeship(true);
    a_pbSeat.set_trusteeship_time(time(NULL));
    VLogMsg(CLIB_LOG_LEV_DEBUG,"table[%ld] seat[%d] enters trusteeship",a_CTable.tid(),a_pbSeat.index());
}

/*
 * 离开托管状态
*/
void TableLogic::DisableTrusteeship(CPBGameTable & a_CTable,PBSDRTableSeat & a_pbSeat)
{
    a_pbSeat.set_is_trusteeship(false);
    a_pbSeat.set_end_trusteeship_time(time(NULL));
    VLogMsg(CLIB_LOG_LEV_DEBUG,"table[%ld] seat[%d] leave trusteeship",a_CTable.tid(),a_pbSeat.index());

    //这里仍然需要开启定时器，防止再次不出牌
    a_CTable.StartCheckUserOffTimer(15);
    VLogMsg(CLIB_LOG_LEV_DEBUG, "Start StartCheckUserOffTimer 15");
}

/*
 * 清理人员
*/
void TableLogic::OnTableRecycleTimerOut(const CPBGameTable & a_CTable)
{
    if(time(NULL) - a_CTable.start_stamp >= 60 * 60 * 24)
    {
        PBCSMsg innermsg;
        SSInnerDissolveTable & ss_inner_dissolve_table = *innermsg.mutable_ss_inner_dissolve_table();
        ss_inner_dissolve_table.set_tid(a_CTable.tid());
        GameHandlerProxy::Instance()->PushInnerMsg(innermsg);
        VLogMsg(CLIB_LOG_LEV_DEBUG,"table[%ld] recycle, seats clean",a_CTable.tid());
    }
}

/*
 * 判定写牌类型
 * bImComSetPileCards ： 是否是不完全写牌（同时写玩家和牌堆，或者只写玩家）
 * bComSetPileCards  ： 是否是完全写牌（只写牌堆）
 */
void TableLogic::JugWriteCardsType(CPBGameTable & table,bool & bImComSetPileCards,bool & bComSetPileCards)
{
    if(table.i_test_cards_pile_cards_size() > 0)
    {
        bComSetPileCards = true;
    }

    for(int i = 0 ; i < table.seats_size() ; i ++)
    {
        const PBSDRTableSeat & pbSeat = table.seats(i);
        if(pbSeat.i_test_hand_cards_size() > 0)
        {
            bImComSetPileCards = true;
            bComSetPileCards = false;
        }
    }
}

/*
 * 根据写牌类型，修改牌型，设置庄家，将牌
 */
void TableLogic::InterRoundData(CPBGameTable & table,RepeatedField<int> & rfTableCardCopy,int iHandNumMax,
                                bool bImComSetPileCards,bool bComSetPileCards)
{
    if(bComSetPileCards)
    {
        ComSetPileCards(table,rfTableCardCopy,iHandNumMax);
    }
    else if(bImComSetPileCards)
    {
        ImComSetPileCards(table,rfTableCardCopy,iHandNumMax);
    }

    if(table.has_i_test_table_dealer_index() && table.i_test_table_dealer_index() != -1)
    {
        table.set_dealer_index(table.i_test_table_dealer_index());
    }

    if(table.has_i_test_jiang_card() && table.i_test_jiang_card() != -1)
    {
        table.set_jiang_card(table.i_test_jiang_card());
    }

    if(table.has_i_test_joker_card() && table.i_test_joker_card() != -1)
    {
        table.set_joker_card(table.i_test_joker_card());
    }

    //清空缓冲
    for(int i = 0 ; i < table.seats_size() ; i ++)
    {
        PBSDRTableSeat & pbSeat = *table.mutable_seats(i);
        pbSeat.clear_i_test_hand_cards();
    }
    table.clear_i_test_joker_card();
    table.clear_i_test_jiang_card();
    table.clear_i_test_table_dealer_index();
}


/*
 * 不完全写牌（同时写玩家和牌堆，或者只写玩家）
 */
void TableLogic::ImComSetPileCards(CPBGameTable & table,RepeatedField<int> & rfTableCardCopy,int iHandNumMax)
{
    //先写有写牌标志玩家的牌
    for(int i = 0 ; i < table.seats_size() ; i++)
    {
        PBSDRTableSeat & pbSeat = *table.mutable_seats(i);
        int iHandCardNum = pbSeat.hand_cards_size();

        //有写牌则写牌
        if(pbSeat.i_test_hand_cards_size() > 0)
        {
            pbSeat.clear_hand_cards();
            for(int j = 0 ; j < pbSeat.i_test_hand_cards_size() ; j ++)
            {
                if(j + 1 > iHandCardNum)
                {
                    break;
                }

                int iHandCard = pbSeat.i_test_hand_cards(j);
                DealCardByValue(rfTableCardCopy,*pbSeat.mutable_hand_cards(),iHandCard);
            }
        }
    }

    //后写没有写牌标志玩家的牌
    for(int i = 0 ; i < table.seats_size() ; i ++)
    {
        PBSDRTableSeat & pbSeat = *table.mutable_seats(i);
        int iHandCardNum = pbSeat.hand_cards_size();

        if(pbSeat.i_test_hand_cards_size() <= 0)
        {
            pbSeat.clear_hand_cards();
            for(int j = 0 ; j < iHandCardNum ; j ++)
            {
                DealCards(rfTableCardCopy,*pbSeat.mutable_hand_cards(),1);
            }
        }
    }

    //牌堆写牌
    if(table.i_test_cards_pile_cards_size() > 0)
    {
        table.mutable_cards()->CopyFrom(table.i_test_cards_pile_cards());
    }
    else
    {
        table.mutable_cards()->CopyFrom(rfTableCardCopy);
    }
}

/*
 * 完全写牌（只写牌堆）
 */
void TableLogic::ComSetPileCards(CPBGameTable & table,RepeatedField<int> & rfTableCardCopy,int iHandNumMax)
{
    rfTableCardCopy.CopyFrom(table.i_test_cards_pile_cards());

    for(int i = 0 ; i < table.seats_size() ; i ++)
    {
        PBSDRTableSeat & pbSeat = *table.mutable_seats(i);
        int iCardNum = pbSeat.hand_cards_size();
        pbSeat.clear_hand_cards();
        for(int j = 0 ; j < iCardNum ; j ++)
        {
            DealCards(rfTableCardCopy,*pbSeat.mutable_hand_cards(),1);
        }
    }
}

/*
 * 写牌总入口
 */
void TableLogic::WriteCard(CPBGameTable & table,RepeatedField<int> & rfTableCardCopy)
{
    bool bImComSetPileCards = false; //是否是不完全牌堆写牌
    bool bComSetPileCards = false; //是否是完全牌堆写牌
    JugWriteCardsType(table,bImComSetPileCards,bComSetPileCards);   //写牌类型的判定
    if(bImComSetPileCards || bComSetPileCards)
    {
        int iHandNumMax = 23;
        InterRoundData(table,rfTableCardCopy,iHandNumMax,bImComSetPileCards,bComSetPileCards);  //改写牌桌数据
    }
}

int TableLogic::DealCardByValue(RepeatedField < int > & rfSrc,RepeatedField < int > & rfDes,int iValue)
{
    for(int i = 0 ; i < rfSrc.size() ; i ++)
    {
        if(rfSrc.Get(i) == iValue)
        {
            rfDes.Add(rfSrc.Get(i));
            rfSrc.ExtractSubrange(i,1,NULL);
            return 0;
        }
    }

    return -1;
}

int TableLogic::FindCardFromVect(RepeatedField < int > & rfSrc,int iCard)
{
    for(int i=0;i<rfSrc.size();i++)
    {
        if(rfSrc.Get(i) == iCard)
        {
            return i;
        }
    }
    return -1;
}

/*
 * 未准备玩家将会被踢出
*/
void TableLogic::OnReadTimerTimeOut(CPBGameTable & a_pbTable)
{
    for(int i = 0 ; i < a_pbTable.seats_size() ; i ++)
    {
        PBSDRTableSeat & pbSeat = *a_pbTable.mutable_seats(i);
        if(pbSeat.state() == EN_SEAT_STATE_WAIT_FOR_NEXT_ONE_GAME)
        {
			PBCSMsg pbNotify;
			SSInnerNotifyKickoutUser & ss_inner_notify_kickout_user = *pbNotify.mutable_ss_inner_notify_kickout_user();
			ss_inner_notify_kickout_user.set_tid(a_pbTable.tid());
			ss_inner_notify_kickout_user.add_indexs(i);

			GameHandlerProxy::Instance()->PushInnerMsg(pbNotify);
        }
    }
}

/*
* 获得在添加出牌action_flow前的一个action_flow
*/
const PBSDRAction* TableLogic::GetLastChuActionInFlowExceptEmptyChu(const PBSDRGameTable &a_pbTable)
{
	for (int i = 0 ; i < a_pbTable.total_action_flows_size() ; i ++)
	{
		const PBSDRActionFlow & current_flow = a_pbTable.total_action_flows(a_pbTable.total_action_flows_size() - 1 - i);
		if (current_flow.action().act_type() == EN_SDR_ACTION_CHUPAI && current_flow.action().cards_size() == 0)
		{
			continue;
		}
		else if (current_flow.action().act_type() == EN_SDR_ACTION_CHUPAI)
		{
			const PBSDRAction & current_action = current_flow.action();
			return &current_action;
		}
	}

	return NULL;
}

/*
 * 机器人定时器
*/
void TableLogic::OnRobotTimerTimeOut(CPBGameTable & a_pbTable)
{
	//执行缓冲区的中的动作
	for (auto iter = a_pbTable.mutable_seats()->begin(); iter != a_pbTable.mutable_seats()->end(); iter++)
	{
		PBSDRTableSeat & pbSeat = *iter;
		if (pbSeat.has_robot_action_buff() && !pbSeat.action_choice().is_determine())
		{
			ProcessAutoAction(a_pbTable, pbSeat, pbSeat.robot_action_buff().act_type());
		}
		pbSeat.clear_robot_action_buff();
	}

	/*
	for (auto iter = a_pbTable.mutable_seats()->begin() ; iter != a_pbTable.mutable_seats()->end() ; iter ++)
	{
		PBSDRTableSeat & pbSeat = *iter;
		if (pbSeat.action_choice().choices_size() > 0 && !pbSeat.action_choice().is_determine())
		{
			BehaviorManager::Instance()->DoAction(a_pbTable, pbSeat);
		}
	}
	*/
}

/*
* 在添加操作后的动作
* 如果是机器人则开启机器人定时器
* 如果是玩家则根据状态来执行后面的行为
*/
void TableLogic::OnOperateAfterAddChoice(CPBGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat)
{
	//如果是机器人，需要另外开启一个机器人定时器
	if (a_pbSeat.user().is_robot())
	{
		//随机时间
		int iWaitringTime = rand() % 5 + 1;

		//会将操作存与缓冲区中
		BehaviorManager::Instance()->DoAction(a_pbTable, a_pbSeat);
		if (!a_pbSeat.has_robot_action_buff())
		{
			return;
		}

		const PBSDRAction & pbBuffAction = a_pbSeat.robot_action_buff();
		if (pbBuffAction.act_type() == EN_SDR_ACTION_PASS)
		{
			iWaitringTime = 1;
		}

		if (a_pbTable.state() == EN_TABLE_STATE_WAIT_QIANG_DI_ZHU)
		{
			iWaitringTime = 1;
		}

		if (a_pbSeat.shou_cols_size() == 1)
		{
			iWaitringTime = 1;
		}

		a_pbTable.StartRobotTimer(iWaitringTime);
		VLogMsg(CLIB_LOG_LEV_DEBUG,"seat[%d] start robot timer int talbe[%ld]",a_pbSeat.index(),a_pbTable.tid());
	}
	else
	{
		if (a_pbSeat.is_trusteeship())
		{
			DoDefaultAction(a_pbTable, a_pbSeat);
		}
		else
		{
			NotifyOperationChoiceForSeat(a_pbTable, a_pbSeat);
		}
	}
}

/*
  获得房间内的机器人
*/
int TableLogic::GetRobotNum(const CPBGameTable & a_pbTable)
{
	int num = 0;
	for (int i = 0; i < a_pbTable.seats_size(); i++)
	{
		const PBSDRTableSeat & pbSeat = a_pbTable.seats(i);
		if (pbSeat.state() == EN_SEAT_STATE_NO_PLAYER)
		{
			continue;
		}

		if (pbSeat.user().acc_type() == EN_Account_Robot)
		{
			num++;
		}
	}
	return num;
}

/*
 * 等待其他玩家定时器
*/
void TableLogic::OnWaitinOtherTimeOut(CPBGameTable & a_pbTable)
{
	//已退休的游戏服不申请机器人
	if (TableManager::Instance()->_state == EN_Game_Service_State_Retired)
	{
		a_pbTable.StopWaitingOhterTimer();
		return;
	}

	/*
	一定要确保这个房间不会因为机器人进不来而无限制申请机器人！！！重要的事情说3遍
	申请机器人的策略
	不申请的情况：
		1.房间内全是机器人
		2.房间的人数已满（需在不满足1的条件下）
	*/
	int iPlayserNum = 0;
	int iPlayserNumExceptRobot = 0;
	for (auto iter = a_pbTable.seats().begin(); iter != a_pbTable.seats().end(); iter++)
	{
		const PBSDRTableSeat & pbSeat = *iter;
		if (pbSeat.state() == EN_SEAT_STATE_NO_PLAYER)
		{
			continue;
		}

		iPlayserNum++;

		if (pbSeat.user().acc_type() == EN_Account_Robot)
		{
			continue;
		}

		iPlayserNumExceptRobot++;
	}

	//除了机器人没有在房间，房间内全是机器人，不需要申请，解散房间
	if (iPlayserNumExceptRobot == 0)
	{
		a_pbTable.StopWaitingOhterTimer();

		//解散房间
		{
			PBCSMsg innermsg;
			SSInnerDissolveTable & ss_inner_dissolve_table = *innermsg.mutable_ss_inner_dissolve_table();
			ss_inner_dissolve_table.set_tid(a_pbTable.tid());
			GameHandlerProxy::Instance()->PushInnerMsg(innermsg);
		}
		return;
	}

	//房间的人数已满
	if (iPlayserNum == a_pbTable.config().seat_num())
	{
		a_pbTable.StopWaitingOhterTimer();
		return;
	}

	//向机器人服请求机器人
	PBCSMsg pbMsg;
	SSRequestRobotJoinMatch & pbRequest = *pbMsg.mutable_ss_request_robot_join_match();
	pbRequest.set_robot_num(1);
	PBSourceInfoRequestingRobot & pbSource = *pbRequest.mutable_source_info();
	pbSource.set_activity_type(EN_User_Game_Info_Coin_Match);
	pbSource.set_game_type(EN_Table_3Ren_DDZ_COIN);
	pbSource.set_level(a_pbTable.config().level());

	//置状态
	//a_pbTable.set_state(EN_TABLE_STATE_WAIT_ROBOT);

	//向机器人发送消息
	Message::PushInnerMsg(RouteManager::Instance()->GetRouteByRandom(), -1, pbMsg, EN_Node_Robot, 1);

	VLogMsg(CLIB_LOG_LEV_DEBUG, "table[%ld] request robot,plalyernum:[%d],seatnum:[%d],table.config.seatnum:[%d]", a_pbTable.tid(),
		iPlayserNum, iPlayserNumExceptRobot, a_pbTable.config().seat_num());
}

/*
检测该桌子上有无相同名字的机器人
*/
bool TableLogic::CheckTableHasTheSameRobot(const CPBGameTable & a_pbTable,string a_stUserName)
{
	for (auto iter = a_pbTable.seats().begin() ; iter != a_pbTable.seats().end() ; iter ++)
	{
		const PBSDRTableSeat & pbSeat = *iter;

		if (pbSeat.state() == EN_SEAT_STATE_NO_PLAYER)
		{
			continue;
		}

		const PBTableUser & pbUser = pbSeat.user();
		if (pbUser.acc_type() != EN_Account_Robot)
		{
			continue;
		}

		if (memcmp(a_stUserName.c_str(),pbUser.nick().c_str(), a_stUserName.size()) == 0)
		{
			return true;
		}
	}

	return false;
}

/*
机器人是否能够进入该房间
*/
bool TableLogic::CheckRobotAbleEnterTable(const CPBGameTable & a_pbTable, string a_stUserName)
{
	int iPlayserNum = 0;
	int iPlayserNumExceptRobot = 0;
	for (auto iter = a_pbTable.seats().begin(); iter != a_pbTable.seats().end(); iter++)
	{
		const PBSDRTableSeat & pbSeat = *iter;
		if (pbSeat.state() == EN_SEAT_STATE_NO_PLAYER)
		{
			continue;
		}

		iPlayserNum++;

		if (pbSeat.user().acc_type() == EN_Account_Robot)
		{
			continue;
		}

		iPlayserNumExceptRobot++;
	}

	//检查这个桌子是否全是机器人
	if (iPlayserNumExceptRobot == 0)
	{
		return false;
	}

	//桌子上的机器人最多只有2个
	if ((iPlayserNum - iPlayserNumExceptRobot) >= 2)
	{
		return false;
	}

	//检查这个桌子是否有与该机器人相同昵称的机器人
	if (CheckTableHasTheSameRobot(a_pbTable, a_stUserName))
	{
		return false;
	}

	return true;
}