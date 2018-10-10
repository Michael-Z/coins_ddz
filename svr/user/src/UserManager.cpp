#include "UserManager.h"

#include "Common.h"
#include "Session.h"
#include "Message.h"
#include "clib_time.h"
#include "LogWriter.h"
#include "RouteManager.h"

#include "UserHandlerProxy.h"

UserManager * UserManager::Instance()
{
    return CSingleton<UserManager>::Instance();
}

void UserManager::Init()
{
    _lru_timer.SetTimeEventObj(this, LRU_TIMER);
    _lru_timer.StartTimerBySecond(60, true);
}
void UserManager::OnUserLogin(const string & account, const PBUserData & user_record)
{
    long long uid = user_record.user_info().uid();
    int current_stamp = time(NULL);
    _accountmap[account] = uid;
    _userdatamap[uid] = user_record;
    _account_lru_map[account] = current_stamp;
    _userdata_lru_map[uid] = current_stamp;
}

PBUser* UserManager::GetUserInfo(long long uid)
{
    PBUserData* user = GetUserData(uid);
    if (!user || !user->has_user_info())
    {
        return NULL;
    }
    return user->mutable_user_info();
}

PBUserRecord * UserManager::GetUserRecord(long long uid)
{
    PBUserData* puser = GetUserData(uid);
    if (!puser || !puser->has_user_record())
    {
        return NULL;
    }
    return puser->mutable_user_record();
}

PBUserTableInfo * UserManager::GetUserTableInfo(long long uid)
{
    PBUserData* puser = GetUserData(uid);
    if (!puser || !puser->has_user_table_info())
    {
        return NULL;
    }
    return puser->mutable_user_table_info();
}

PBUserTeaBarData * UserManager::GetUserTeaBarData(long long uid)
{
    PBUserData* puser = GetUserData(uid);
    if (!puser || !puser->has_user_tea_bar_data())
    {
        return NULL;
    }
    return puser->mutable_user_tea_bar_data();
}

void UserManager::UpdateUserData(long long uid, const PBUserData& user_record)
{
    _userdatamap[uid] = user_record;
    int current_stamp = time(NULL);
    _userdata_lru_map[uid] = current_stamp;
}

PBUserData* UserManager::GetUserData(long long uid)
{
    UserDataMap::iterator iter = _userdatamap.find(uid);
    if (iter != _userdatamap.end())
    {
        return &iter->second;
    }
    int current_stamp = time(NULL);
    _userdata_lru_map[uid] = current_stamp;
    return NULL;
}

PBUserData* UserManager::GetUserDataByAcc(string account)
{
    AccountMap::iterator iter_acc = _accountmap.find(account);
    if (iter_acc == _accountmap.end())
    {
        return NULL;
    }
    int64 uid = iter_acc->second;
    return GetUserData(uid);
}

ENMessageError UserManager::OnParseUserData(const PBDataSet& data_set)
{
    PBUserData& user_data = _userdatamap[data_set.uid()];
    for (int i = 0; i < data_set.key_list_size(); ++i)
    {
        const PBRedisData& redis_data = data_set.key_list(i);
        if (!redis_data.result()) continue;
        switch (redis_data.key())
        {
        case PBUserDataField::kUserInfo:
        {
            // user_info 在注册时一定生成
            PBUser user;
            if (redis_data.has_buff() &&
                user.ParseFromArray(redis_data.buff().c_str(), redis_data.buff().size()))
            {
                user_data.mutable_user_info()->CopyFrom(user);
            }
            break;
        }
        case PBUserDataField::kUserRecord:
        {
            PBUserRecord record;
            if (redis_data.has_buff())
            {
                if (record.ParseFromArray(redis_data.buff().c_str(), redis_data.buff().size()))
                {
                    user_data.mutable_user_record()->CopyFrom(record);
                }
            }
            else
            {
                user_data.mutable_user_record();
            }
            break;
        }
        case PBUserDataField::kUserTableInfo:
        {
            PBUserTableInfo tableinfo;
            if (redis_data.has_buff())
            {
                if (tableinfo.ParseFromArray(redis_data.buff().c_str(), redis_data.buff().size()))
                {
                    user_data.mutable_user_table_info()->CopyFrom(tableinfo);
                }
            }
            else
            {
                user_data.mutable_user_table_info();
            }
            break;
        }
        case  PBUserDataField::kUserTeaBarData:
        {
            PBUserTeaBarData teabardata;
            if (redis_data.has_buff())
            {
                if (teabardata.ParseFromArray(redis_data.buff().c_str(), redis_data.buff().size()))
                {
                    user_data.mutable_user_tea_bar_data()->CopyFrom(teabardata);
                }
            }
            else
            {
                user_data.mutable_user_tea_bar_data();
            }
            break;
        }
            case PBUserDataField::kUserGameData:
            {
                PBUserGameData pbUserGameData;
                if(redis_data.has_buff())
                {
                    if(pbUserGameData.ParseFromArray(redis_data.buff().c_str(), redis_data.buff().size()))
                    {
                        user_data.mutable_user_game_data()->CopyFrom(pbUserGameData);
                    }
                }
                else
                {
                    user_data.mutable_user_game_data();
                }
            }
            default : break;

        }
    }
    return EN_MESSAGE_ERROR_OK;
}

void UserManager::UpdateUserLastWeekGameInfo(long long uid, const PBGameRecord& game_record, PBUserRecord* user_record)
{
    auto current_time = time(NULL);

    if (!user_record->has_user_current_week_game_info())
    {
        user_record->mutable_user_current_week_game_info()->set_time_stamp(current_time);
    }

    if (!user_record->has_user_last_week_game_info())
    {
        user_record->mutable_user_last_week_game_info()->set_time_stamp(current_time);
        user_record->mutable_user_last_week_game_info()->set_total_rounds(0);
        user_record->mutable_user_last_week_game_info()->set_win_rounds(0);
    }

    auto current_week_game_info = user_record->mutable_user_current_week_game_info();

    if (!CTimeHelper::IsInSameWeekBeginOfMon(user_record->user_current_week_game_info().time_stamp(), current_time))
    {
        user_record->mutable_user_last_week_game_info()->CopyFrom(*current_week_game_info);
        current_week_game_info->Clear();
        current_week_game_info->set_time_stamp(current_time);
    }

    for (auto it = game_record.round_results().begin(); it != game_record.round_results().end(); ++it)
    {
        current_week_game_info->set_total_rounds(current_week_game_info->total_rounds() + 1);
        for (auto score_it = it->scores().begin(); score_it != it->scores().end(); ++score_it)
        {
            if (score_it->uid() == uid && score_it->score() > 0)
            {
                current_week_game_info->set_win_rounds(current_week_game_info->win_rounds() + 1);
            }
        }
    }
}

bool UserManager::OnUpdateUserTableInfo(int64 uid, const PBUpdateData & update_key, PBRedisData & redis_data)
{
    PBUserTableInfo * puser_table_info = GetUserTableInfo(uid);
    if (!puser_table_info)
    {
        return false;
    }
    for (int i = 0; i < update_key.field_list_size(); i++)
    {
        const PBDBAtomicField& field_info = update_key.field_list(i);
        ENUpdateStrategy strategy = field_info.strategy();
        switch (field_info.field())
        {
        case EN_DB_Field_Table_Running:
        {
            if (strategy == EN_Update_Strategy_Add)
            {
                const PBDelegateTableInfo & new_table_info = field_info.tableinfo();
                bool exist = false;
                for (int i = 0; i < puser_table_info->running_table_list_size(); i++)
                {
                    PBDelegateTableInfo & table_info = *puser_table_info->mutable_running_table_list(i);
                    if (table_info.tid() == new_table_info.tid())
                    {
                        exist = true;
                    }
                }
                if (exist == false)
                {
                    //新纪录
                    puser_table_info->add_running_table_list()->CopyFrom(field_info.tableinfo());
                    if (puser_table_info->running_table_list_size() > 10)
                    {
                        puser_table_info->mutable_running_table_list()->DeleteSubrange(0, puser_table_info->running_table_list_size() - 10);
                    }
                }
            }
            else if (strategy == EN_Update_Strategy_Replace)
            {
                const PBDelegateTableInfo & new_table_info = field_info.tableinfo();
                bool exist = false;
                for (int i = 0; i < puser_table_info->running_table_list_size(); i++)
                {
                    PBDelegateTableInfo & table_info = *puser_table_info->mutable_running_table_list(i);
                    if (table_info.tid() == new_table_info.tid())
                    {
                        exist = true;
                        table_info.CopyFrom(new_table_info);
                    }
                }
            }
            else if (strategy == EN_Update_Strategy_Del)
            {
                const PBDelegateTableInfo & new_table_info = field_info.tableinfo();
                for (int i = 0; i < puser_table_info->running_table_list_size(); i++)
                {
                    PBDelegateTableInfo & table_info = *puser_table_info->mutable_running_table_list(i);
                    if (table_info.tid() == new_table_info.tid())
                    {
                        puser_table_info->mutable_running_table_list()->DeleteSubrange(i, 1);
                    }
                }
            }
            break;
        }
        case EN_DB_Field_Table_Closed:
        {
            const PBDelegateTableInfo & new_table_info = field_info.tableinfo();
            bool exist = false;
            for (int i = 0; i < puser_table_info->closed_table_list_size(); i++)
            {
                PBDelegateTableInfo & table_info = *puser_table_info->mutable_closed_table_list(i);
                if (table_info.tid() == new_table_info.tid())
                {
                    exist = true;
                }
            }
            if (exist == false)
            {
                //新纪录
                puser_table_info->add_closed_table_list()->CopyFrom(field_info.tableinfo());
                if (puser_table_info->closed_table_list_size() > 20)
                {
                    puser_table_info->mutable_closed_table_list()->DeleteSubrange(0, puser_table_info->closed_table_list_size() - 20);
                }
            }
            break;
        }
        default: return false; break;
        }
    }
    return SerializeUserData(*puser_table_info, uid, PBUserDataField::kUserTableInfo, redis_data);
}

bool UserManager::OnUpdateUserRecord(int64 uid, const PBUpdateData & update_key, PBRedisData & redis_data)
{
    PBUserRecord * puser_record = GetUserRecord(uid);
    if (!puser_record)
    {
        return false;
    }
    for (int i = 0; i < update_key.field_list_size(); i++)
    {
        const PBDBAtomicField& field_info = update_key.field_list(i);
        ENUpdateStrategy strategy = field_info.strategy();
        switch (field_info.field())
        {
        case EN_DB_Field_Record:
        {
            if (strategy == EN_Update_Strategy_Add)
            {
                const PBGameRecord & game_record = field_info.record();
                bool exist = false;
                for (int i = 0; i < puser_record->records_size(); i++)
                {
                    PBGameRecord & record = *puser_record->mutable_records(i);
                    if (record.recordid() == game_record.recordid())
                    {
                        //附加到现有的记录中
                        exist = true;
                        for (int j = 0; j < game_record.round_results_size(); j++)
                        {
                            record.add_round_results()->CopyFrom(game_record.round_results(j));
                        }
                        //更新总记录
                        record.clear_final_user_scores();
                        record.mutable_final_user_scores()->CopyFrom(game_record.final_user_scores());
                    }
                }
                if (!exist)
                {
                    //新纪录
                    puser_record->add_records()->CopyFrom(field_info.record());
                    if (puser_record->records_size() > 10)
                    {
                        puser_record->mutable_records()->DeleteSubrange(0, puser_record->records_size() - 10);
                    }
                }
                SendUpdateRankMsg(EN_Rank_Type_LZMJ_WEEK_ROUND, uid, 1);
            }

            break;
        }
        case EN_DB_Field_Fpf_Record:
        {
            if (strategy == EN_Update_Strategy_Add)
            {
                const PBGameRecord& game_record = field_info.record();
                puser_record->add_fpf_records()->CopyFrom(game_record);
                if (puser_record->fpf_records_size() > 20)
                {
                    puser_record->mutable_fpf_records()->DeleteSubrange(0, puser_record->fpf_records_size() - 20);
                }
            }
            break;
        }
        case EN_DB_Field_Dss_Record:
        {
            if (strategy == EN_Update_Strategy_Add)
            {
                const PBGameRecord& game_record = field_info.record();
                puser_record->add_dss_records()->CopyFrom(game_record);
                if (puser_record->dss_records_size() > 20)
                {
                    puser_record->mutable_dss_records()->DeleteSubrange(0, puser_record->dss_records_size() - 20);
                }
            }
            break;
        }
        case EN_DB_Field_Sdr_Record:
        {
            if (strategy == EN_Update_Strategy_Add)
            {
                const PBGameRecord& game_record = field_info.record();
                puser_record->add_sdr_records()->CopyFrom(game_record);
                if (puser_record->sdr_records_size() > 20)
                {
                    puser_record->mutable_sdr_records()->DeleteSubrange(0, puser_record->sdr_records_size() - 20);
                }

                UpdateUserLastWeekGameInfo(uid, game_record, puser_record);
            }
            break;
        }
        default: return false; break;
        }
    }
    return SerializeUserData(*puser_record, uid, PBUserDataField::kUserRecord, redis_data);
}

bool UserManager::OnUpdateUserInfo(int64 uid, const PBUpdateData& update_key, PBRedisData& redis_data)
{
    PBUser* puser_info = GetUserInfo(uid);
    if (!puser_info)
    {
        return false;
    }

    for (int i = 0; i < update_key.field_list_size(); ++i)
    {
        const PBDBAtomicField& field_info = update_key.field_list(i);
        //ENUpdateStrategy strategy = field_info.strategy();
        switch (field_info.field())
        {
        case EN_DB_Field_Hsvid:			puser_info->set_hallsvid(field_info.intval()); break;
        case EN_DB_Field_Nick:          puser_info->set_nick(field_info.strvalue()); break;
        case EN_DB_Field_Pic_Url:       puser_info->set_pic_url(field_info.strvalue()); break;
        case EN_DB_Field_Gender:        puser_info->set_gender(field_info.intval()); break;
        case EN_DB_Field_RoleType:      puser_info->set_roletype(field_info.intval()); break;
        case EN_DB_Field_Create_Table:  puser_info->set_create_table_id(field_info.intval()); break;
        case EN_DB_Field_Items_Info:    puser_info->set_items_info(field_info.strvalue()); break;
        case EN_DB_Field_POS:
        {
            if (puser_info->pos().pos_type() == field_info.pos().pos_type()
                && puser_info->pos().gamesvrd_id() == field_info.pos().gamesvrd_id()
                && puser_info->pos().table_id() == field_info.pos().table_id())
            {
                break;
            }
            if (puser_info->pos().pos_type() != EN_Position_Hall &&
                field_info.pos().pos_type() != EN_Position_Hall)
                return false;
            puser_info->mutable_pos()->CopyFrom(field_info.pos()); break;
        }
        case EN_DB_Field_Limit:
        {
            puser_info->set_limit(field_info.intval());
            break;
        }
        case EN_DB_Field_IP:
        {
            puser_info->set_last_login_ip(field_info.strvalue()); break;
        }
        case EN_DB_Field_Conf:
        {
            puser_info->mutable_tconf()->CopyFrom(field_info.conf()); break;
        }
        case EN_DB_Field_Chips:
        {
            if (0 == field_info.intval()) break;
            if (!field_info.has_reason()) return false;
            VLogMsg(CLIB_LOG_LEV_DEBUG, "befor update user[%ld] chips[%ld],intval :%ld", puser_info->uid(), puser_info->chips(), field_info.intval());
            if (puser_info->chips() + field_info.intval() < 0) return false;
            puser_info->set_chips(puser_info->chips() + field_info.intval());
            // 通知客户端
            SendChipChangeMsg(*puser_info);
            // 金币流水上报logsvrd
            SendChipJournalLog(*puser_info, field_info.intval(), field_info.reason());
            VLogMsg(CLIB_LOG_LEV_DEBUG, "after update user[%ld] chips[%ld]", puser_info->uid(), puser_info->chips());
            break;
        }
        case EN_DB_Field_Fpf_Conf:
        {
            puser_info->mutable_fpf_conf()->CopyFrom(field_info.fpf_conf()); break;
        }
        case EN_DB_Field_Dss_Conf:
        {
            puser_info->mutable_dss_conf()->CopyFrom(field_info.dss_conf()); break;
        }
        case EN_DB_Field_Sdr_Conf:
        {
            puser_info->mutable_sdr_conf()->CopyFrom(field_info.sdr_conf()); break;
        }

            //玩家闯关位置信息
            case EN_DB_Field_Skipmatch_POS:
            {
                if(puser_info->skipmatch_pos().pos_type() == field_info.pos().pos_type()
                        && puser_info->skipmatch_pos().gamesvrd_id() == field_info.pos().gamesvrd_id()
                        && puser_info->skipmatch_pos().table_id() == field_info.pos().table_id())
                {
                    break;
                }
                //位置改变中，必须有个hall，不能跨
                if(puser_info->skipmatch_pos().pos_type() != EN_Position_Hall &&
                        field_info.pos().pos_type() != EN_Position_Hall)
                {
                    return false;
                }
                puser_info->mutable_skipmatch_pos()->CopyFrom(field_info.pos()); break;
                break;
            }
            //砖石
            case EN_DB_Field_Diamond:
            {
                if(0 == field_info.intval())
                {
                    break;
                }
                if(!field_info.has_reason())
                {
                    return false;
                }
                if(puser_info->diamond() + field_info.intval() < 0)
                {
                    return false;
                }
                puser_info->set_diamond(puser_info->diamond() + field_info.intval());

                //通知客户端
                SendDiamondChangeMsg(*puser_info);
                //砖石流水上报
                SendDiamondFlowLog(*puser_info,field_info.intval(), field_info.reason());
                VLogMsg(CLIB_LOG_LEV_DEBUG,"update user[%ld] diamond[%ld] total[%ld]",puser_info->uid(),field_info.intval(),puser_info->diamond());

                break;
            }
            //奖金
            case EN_DB_Field_Bonus:
            {
                if(0 == field_info.floatval())
                {
                    break;
                }
                if(!field_info.has_reason())
                {
                    return false;
                }
                if(puser_info->bonus() + field_info.floatval() < 0)
                {
                    return false;
                }

                //历史总奖金累加
                if(field_info.floatval() > 0)
                {
                    puser_info->set_total_bonus(puser_info->total_bonus() + field_info.floatval());

                    SendToTalBonusLog(*puser_info,field_info.floatval(),field_info.reason());
                }
                puser_info->set_bonus(puser_info->bonus() + field_info.floatval());
                //通知客户端
                SendBonusChangeMsg(*puser_info);
                //奖金流水上报
                SendBonusFlowLog(*puser_info,field_info.floatval(), field_info.reason());
                VLogMsg(CLIB_LOG_LEV_DEBUG,"update user[%ld] diamond[%f] total[%f]",puser_info->uid(),field_info.floatval(),puser_info->bonus());
                break;
            }
			case EN_DB_Field_Coin_POS:
			{
				if (puser_info->coin_pos().pos_type() == field_info.pos().pos_type()
					&& puser_info->coin_pos().gamesvrd_id() == field_info.pos().gamesvrd_id()
					&& puser_info->coin_pos().table_id() == field_info.pos().table_id())
				{
					break;
				}
				//位置改变中，必须有个hall，不能跨
				if (puser_info->coin_pos().pos_type() != EN_Position_Hall &&
					field_info.pos().pos_type() != EN_Position_Hall)
				{
					return false;
				}
				puser_info->mutable_coin_pos()->CopyFrom(field_info.pos()); break;
				break;
			}
			case EN_DB_Field_Coin:
			{
				if (0 == field_info.intval())
				{
					break;
				}
				if (!field_info.has_reason())
				{
					return false;
				}

				int64 iIntVal = field_info.intval();
				//如果没有钱出房费了，不出
				if (field_info.reason() == EN_Reason_Coin_Game_Fee)
				{
					if (puser_info->coins() + field_info.intval() < 0)
					{
						iIntVal = (-puser_info->coins());
					}
				}

				if (puser_info->coins() + iIntVal < 0)
				{
					return false;
				}

				puser_info->set_coins(puser_info->coins() + iIntVal);

				//通知客户端
				SendCoinsChangeMsg(*puser_info);
				//金币流水上报
				SendCoinsFlowLog(*puser_info, iIntVal, field_info.reason());
			}
            default:
                return false;
            break;
        }
    }

    return SerializeUserData(*puser_info, uid, PBUserDataField::kUserInfo, redis_data);
}

bool UserManager::OnUpdateUserTeaBarData(int64 uid, const PBUpdateData& update_key, PBRedisData& redis_data)
{
    PBUserTeaBarData* pteabardata = GetUserTeaBarData(uid);
    if (!pteabardata)
    {
        return false;
    }
    for (int i = 0; i < update_key.field_list_size(); ++i)
    {
        const PBDBAtomicField& field_info = update_key.field_list(i);
        // ENUpdateStrategy strategy = field_info.strategy();
        switch (field_info.field())
        {
        case EN_DB_Field_TeaBar_Brief:
        {
            if (field_info.strategy() == EN_Update_Strategy_Add)
            {
                TeaBarBriefData& brief = *pteabardata->add_brief_data();
                brief.CopyFrom(field_info.tea_bar_brief());
            }
            else if (field_info.strategy() == EN_Update_Strategy_Del)
            {
                int find_index = -1;
                for (int i = 0; i < pteabardata->brief_data_size(); i++)
                {
                    const TeaBarBriefData& brief = pteabardata->brief_data(i);
                    if (brief.tbid() == field_info.tea_bar_brief().tbid())
                    {
                        find_index = i;
                        break;
                    }
                }
                if (find_index != -1)
                {
                    const TeaBarBriefData& brief = pteabardata->brief_data(find_index);
                    //通知玩家被移除茶馆
                    SendUserRemoveOutTeaBar(uid, brief);

                    pteabardata->mutable_brief_data()->DeleteSubrange(find_index, 1);
                }
            }
            break;
        }
        case EN_DB_Field_TeaBar_Chips:
        {
            if (0 == field_info.tea_bar_chips_flow().chips()) break;
            if (!field_info.tea_bar_chips_flow().has_reason()) return false;
            TeaBarBriefData* pbrief = NULL;
            for (int i = 0; i < pteabardata->brief_data_size(); i++)
            {
                TeaBarBriefData& brief = *pteabardata->mutable_brief_data(i);
                if (brief.tbid() == field_info.tea_bar_chips_flow().tbid())
                {
                    pbrief = &brief;
                    break;
                }
            }
            if (pbrief == NULL)
            {
                return true;
            }
            VLogMsg(CLIB_LOG_LEV_DEBUG, "befor update teabar[%ld] chips[%ld],intval:[%d]", pbrief->tbid(), pbrief->chips(), field_info.tea_bar_chips_flow().chips());
            if (pbrief->chips() + field_info.tea_bar_chips_flow().chips() < 0) return false;
            pbrief->set_chips(pbrief->chips() + field_info.tea_bar_chips_flow().chips());
            // 金币流水上报logsvrd
            SendTeaBarChipLog(uid, *pbrief, field_info.tea_bar_chips_flow());
            VLogMsg(CLIB_LOG_LEV_DEBUG, "after update teabar[%ld] chips[%ld]", pbrief->tbid(), pbrief->chips());
        }
        break;
        case EN_DB_Field_TeaBar_Wait_Table_Num:
        {
            TeaBarBriefData* pbrief = NULL;
            for (int i = 0; i < pteabardata->brief_data_size(); i++)
            {
                TeaBarBriefData& brief = *pteabardata->mutable_brief_data(i);
                if (brief.tbid() == field_info.tea_bar_wait_table_data().tbid())
                {
                    pbrief = &brief;
                    break;
                }
            }
            if (pbrief == NULL)
            {
                return false;
            }
            pbrief->set_wait_table_num(field_info.tea_bar_wait_table_data().wait_table_num());
        }
        break;
        case EN_DB_Field_TeaBar_Pay_Type:
        {
            TeaBarBriefData* pbrief = NULL;
            for (int i = 0; i < pteabardata->brief_data_size(); i++)
            {
                TeaBarBriefData& brief = *pteabardata->mutable_brief_data(i);
                if (brief.tbid() == field_info.tea_bar_pay_type_data().tbid())
                {
                    pbrief = &brief;
                    break;
                }
            }
            if (pbrief == NULL)
            {
                return false;
            }
            pbrief->set_pay_type(field_info.tea_bar_pay_type_data().pay_type());
        }
        break;
        default:break;
        }
    }
    return SerializeUserData(*pteabardata, uid, PBUserDataField::kUserTeaBarData, redis_data);
}

void UserManager::SendChipChangeMsg(const PBUser& user_info)
{
    PBCSMsg msg;
    CSNotifyChipChange& notify = *msg.mutable_cs_notify_chip_change();
    notify.set_cur_chip(user_info.chips());
    //notify.set_cur_gold(user_info.gold());

    Message::PushMsg(RouteManager::Instance()->GetRouteByRandom(), user_info.uid(), msg, EN_Node_Connect, user_info.hallsvid());
}

void UserManager::SendUpdateRankMsg(int rank_id, long long rank_key, long long rank_score)
{
    PBCSMsg msg;
    SSRequestUpdateRankList & request = *msg.mutable_ss_request_update_rank_list();
    request.set_rank_id(rank_id);
    request.set_rank_key(rank_key);
    request.set_rank_score(rank_score);

    Message::PushInnerMsg(RouteManager::Instance()->GetRouteByRandom(), rank_id, msg, EN_Node_DBProxy, rank_id, EN_Route_hash);
}

void UserManager::SendChipJournalLog(const PBUser& user_info, long long act_num, int reason)
{
    PBCSMsg msg;
    LogChipJournal& log = *msg.mutable_log_chip_journal();
    log.set_uid(user_info.uid());
    log.set_act_num(act_num);
    log.set_total_num(user_info.chips());
    log.set_reason(reason);
    log.set_time_stamp(time(NULL));
    log.set_acc_type(user_info.acc_type());
    log.set_channel(user_info.channel());

    CLogWriter::Instance()->Send(msg);
}

int UserManager::ProcessOnTimerOut(int Timerid)
{
    int current_stamp = time(NULL);
    AccountLruMap::iterator iter = _account_lru_map.begin();
    vector<string> timeout_account_vect;
    for (; iter != _account_lru_map.end(); iter++)
    {
        if ((current_stamp - iter->second) > 30 * 60)
        {
            timeout_account_vect.push_back(iter->first);
        }
    }

    for (unsigned int i = 0; i < timeout_account_vect.size(); i++)
    {
        const string & account = timeout_account_vect[i];
        VLogMsg(CLIB_LOG_LEV_ERROR, "account[%s] timeout", account.c_str());
        if (_account_lru_map.find(account) != _account_lru_map.end())
        {
            _account_lru_map.erase(account);
        }
        if (_accountmap.find(account) != _accountmap.end())
        {
            _accountmap.erase(account);
        }
    }

    UserDataLruMap::iterator uiter = _userdata_lru_map.begin();
    vector<int64> timeout_userdata_vect;
    for (; uiter != _userdata_lru_map.end(); uiter++)
    {
        if ((current_stamp - uiter->second) > 30 * 60)
        {
            timeout_userdata_vect.push_back(uiter->first);
        }
    }
    for (unsigned int i = 0; i < timeout_userdata_vect.size(); i++)
    {
        int64 uid = timeout_userdata_vect[i];
        VLogMsg(CLIB_LOG_LEV_ERROR, "uid[%lld] timeout", uid);
        if (_userdata_lru_map.find(uid) != _userdata_lru_map.end())
        {
            _userdata_lru_map.erase(uid);
        }
        if (_userdatamap.find(uid) != _userdatamap.end())
        {
            _userdatamap.erase(uid);
        }
    }
    return 0;
}

void UserManager::SendTeaBarChipLog(int64 uid, TeaBarBriefData& brief, const TeaBarChipsFlow& flow)
{
    PBCSMsg msg;
    LogTeaBarChipsFlow& log = *msg.mutable_log_tea_bar_chips_flow();
    log.set_tbid(brief.tbid());
    log.set_act_num(flow.chips());
    log.set_total_num(brief.chips());
    log.set_reason(flow.reason());
    log.set_time_stamp(time(NULL));
    log.set_master_uid(uid);
    CLogWriter::Instance()->Send(msg);

    if (flow.reason() == EN_Reason_Create_Tea_Bar_Table || flow.reason() == EN_Reason_Dissolve_Tea_Bar_Table)
    {
        //茶馆流水
        Message::PushInnerMsg(RouteManager::Instance()->GetRouteByRandom(), 0, msg, EN_Node_TeaBar, 1);
    }
}

void UserManager::SendUserRemoveOutTeaBar(int64 uid, const TeaBarBriefData& brief)
{
    PBUser* puser = GetUserInfo(uid);
    if (puser == NULL)
    {
        return;
    }
    PBCSMsg msg;
    CSNotifyUserRemoveOutTeaBar& cs_notify_user_remove_out_tea_bar = *msg.mutable_cs_notify_user_remove_out_tea_bar();
    cs_notify_user_remove_out_tea_bar.set_tbid(brief.tbid());
    cs_notify_user_remove_out_tea_bar.set_tbname(brief.tbname());
    Message::PushInnerMsg(RouteManager::Instance()->GetRouteByRandom(), uid, msg, EN_Node_Connect, puser->hallsvid());
}


void UserManager::SendDiamondChangeMsg(const PBUser& user_info)
{
    PBCSMsg msg;
    CSNotifyDiamondChange& notify = *msg.mutable_cs_notify_diamond_change();
    notify.set_cur_diamond(user_info.diamond());
    Message::PushMsg(RouteManager::Instance()->GetRouteByRandom(), user_info.uid(), msg, EN_Node_Connect, user_info.hallsvid());
}

void UserManager::SendDiamondFlowLog(const PBUser& user_info, long long act_num, int reason)
{
    PBCSMsg msg;
    LogDiamondFlow &log = *msg.mutable_log_diamond_flow();
    log.set_uid(user_info.uid());
    log.set_act_num(act_num);
    log.set_total_num(user_info.diamond());
    log.set_reason(reason);
    log.set_time_stamp(time(NULL));
    log.set_acc_type(user_info.acc_type());
    log.set_channel(user_info.channel());
    CLogWriter::Instance()->Send(msg);
}

/*
推送金币改变
*/
void UserManager::SendCoinsChangeMsg(const PBUser& user_info)
{
	PBCSMsg msg;
	CSNotifyCoinsChange	& notify = *msg.mutable_cs_notify_coins_change();
	notify.set_cur_coins(user_info.coins());
	Message::PushMsg(RouteManager::Instance()->GetRouteByRandom(), user_info.uid(), msg, EN_Node_Connect, user_info.hallsvid());
}

void UserManager::SendBonusChangeMsg(const PBUser& user_info)
{
    PBCSMsg msg;
    CSNotifyBonusChange & notify = *msg.mutable_cs_notify_bonus_change();
    notify.set_cur_bonus(user_info.bonus());
    Message::PushMsg(RouteManager::Instance()->GetRouteByRandom(), user_info.uid(), msg, EN_Node_Connect, user_info.hallsvid());
}

void UserManager::SendBonusFlowLog(const PBUser& user_info,float act_num, int reason)
{
    PBCSMsg msg;
    LogBonusFlow &log = *msg.mutable_log_bonus_flow();
    log.set_uid(user_info.uid());
    log.set_act_num(act_num);
    log.set_total_num(user_info.bonus());
    log.set_reason(reason);
    log.set_time_stamp(time(NULL));
    log.set_acc_type(user_info.acc_type());
    log.set_channel(user_info.channel());
    CLogWriter::Instance()->Send(msg);
}

/*
发送金币日志流水
*/
void UserManager::SendCoinsFlowLog(const PBUser& user_info, int64 act_num, int reason)
{
	PBCSMsg msg;
	LogCoinsFlow & log = *msg.mutable_log_coins_flow();
	log.set_uid(user_info.uid());
	log.set_act_num(act_num);
	log.set_total_num(user_info.coins());
	log.set_reason(reason);
	log.set_time_stamp(time(NULL));
	log.set_acc_type(user_info.acc_type());
	log.set_channel(user_info.channel());
	CLogWriter::Instance()->Send(msg);
}


//void UserManager::SendUpdateSkipMatchResult(const PBUser& a_UserInfo,long long a_iSkipSessionId,int a_iSkipLevelChangeVal,int a_iSkipLevel,
//                                            int a_iSkipState,int a_iReason)
//{
//    //是否需要结果验证
//    PBCSMsg pbResultNotify;
//    SSUpdateDBSkipMatchResult & ss_update_skip_match_result = *pbResultNotify.mutable_ss_update_db_skip_match_result();
//    ss_update_skip_match_result.set_uid(a_UserInfo.uid());
//    ss_update_skip_match_result.set_skip_match_session_id(a_iSkipSessionId);
//    ss_update_skip_match_result.set_skipmatch_level_change_val(a_iSkipLevelChangeVal);
//    ss_update_skip_match_result.set_skipmatch_level(a_iSkipLevel);
//    ss_update_skip_match_result.set_skipmatch_state(a_iSkipState);
//    ss_update_skip_match_result.set_reason(a_iReason);

//    Message::PushInnerMsg(RouteManager::Instance()->GetRouteByRandom(),a_UserInfo.uid(),pbResultNotify,EN_Node_DBProxy,1,EN_Route_hash);
//}

void UserManager::SendToTalBonusLog(const PBUser& pbUserInfo, long long a_iActNum, int a_iReason)
{
    PBCSMsg msg;
    LogTotalBonus &log = *msg.mutable_log_total_bonus();
    log.set_uid(pbUserInfo.uid());
    log.set_act_num(a_iActNum);
    log.set_total_num(pbUserInfo.total_bonus());
    log.set_reason(a_iReason);

    CLogWriter::Instance()->Send(msg);
}

/*
 * 修改用户游戏数据
*/
bool UserManager::OnUpdateUserGameData(int64 a_iUid, const PBUpdateData& a_pbUpdateKey, PBRedisData& a_pbRedisData)
{
    PBUserGameData * ppbUserGameData = GetUserGameData(a_iUid);
    if(!ppbUserGameData)
    {
        VLogMsg(CLIB_LOG_LEV_DEBUG,"failed to find user[%lld] game data",a_iUid);
        return false;
    }

    PBUser* ppbUserInfo = GetUserInfo(a_iUid);
    if (!ppbUserInfo)
    {
        VLogMsg(CLIB_LOG_LEV_DEBUG,"failed to find user[%lld] user infos",a_iUid);
        //return false;
    }

    for(int i = 0 ; i < a_pbUpdateKey.field_list_size() ; ++ i)
    {
        const PBDBAtomicField & pbFieldInfo = a_pbUpdateKey.field_list(i);
        int iActivityType = pbFieldInfo.activity_type();
        int iField = pbFieldInfo.field();
        int iGameType = pbFieldInfo.game_type();
        ENUpdateStrategy iStrategy = pbFieldInfo.strategy();
        switch (iActivityType)
        {
            case EN_User_Game_Info_Skip_Match:
            {
                PBUserSkipMatchInfo &  pbUserSkipMatchInfo = *ppbUserGameData->mutable_user_skip_match_info();
                //得到某个游戏的信息
                PBUserSkipMatchInfoItem * ppbInfoItem = GetItemFromSkipMatchInfo(iGameType,pbUserSkipMatchInfo);
                if(!ppbInfoItem)
                {
                    ppbInfoItem = pbUserSkipMatchInfo.add_skip_match_info_item();
                    InitSkipMatchInfoItemByType(ppbInfoItem,iGameType);
                }

                switch (iField)
                {
                    //数据初始化，什么都不干
                    case EN_DB_Field_Skip_Match_Info_Init:
                        break;

                    case EN_DB_Field_Skip_Match_Info_Level:
                        if(0 == pbFieldInfo.intval())
                        {
                            break;
                        }
                        if(!pbFieldInfo.has_reason())
                        {
                            VLogMsg(CLIB_LOG_LEV_DEBUG,"failed to save user[%lld] level : no reason",a_iUid);
                            return false;
                        }

                        //策略为替换
                        if(iStrategy == EN_Update_Strategy_Replace)
                        {
                            if(pbFieldInfo.intval() < 0 || pbFieldInfo.intval() > 7)
                            {
                                VLogMsg(CLIB_LOG_LEV_DEBUG,"failed to save user[%lld] level : level error",a_iUid);
                                return false;
                            }

                            int iOldLevel = ppbInfoItem->skipmatch_level();

                            //添加跳级记录
                            if(iOldLevel == 7 && pbFieldInfo.intval() == 1 && pbFieldInfo.reason() == EN_Reason_LevelAndBonus_Game_Over_Settle)
                            {
                                ppbInfoItem->set_skipmatch_success_skip_level_num(ppbInfoItem->skipmatch_success_skip_level_num() + 1);
                            }

                            //尝试增加游戏的次数
                            TryAddSkipMatchGameNum(ppbInfoItem,pbFieldInfo);

                            ppbInfoItem->set_old_skipmatch_level(iOldLevel);
                            ppbInfoItem->set_skipmatch_level(pbFieldInfo.intval());
                            //设置最近一次的sessionid
                            ppbInfoItem->set_lately_session_id(pbFieldInfo.skipmatch_session_id());
                            //更新结果流
                            UpdateUserSkipMatchResultFlow(*ppbInfoItem,pbFieldInfo.skipmatch_session_id(),pbFieldInfo.intval() - iOldLevel,
                                                          ppbInfoItem->skipmatch_state(),pbFieldInfo.reason());

                            if(ppbUserInfo)
                            {
                                //向log发消息，更新
                                SendSkipMatchLevelAndStateFlowLog(*ppbUserInfo,*ppbInfoItem,pbFieldInfo.intval() - iOldLevel, pbFieldInfo.reason(),pbFieldInfo.game_type());
                            }
                        }
                        //非替换
                        else
                        {
                            if(ppbInfoItem->skipmatch_level() + pbFieldInfo.intval() < 1
                                    || ppbInfoItem->skipmatch_level() + pbFieldInfo.intval() > 7)    //这里7需要做成配置？
                            {
                                VLogMsg(CLIB_LOG_LEV_DEBUG,"failed to save user[%lld] level : level error",a_iUid);
                                return false;
                            }

                            if(pbFieldInfo.intval() > 0 && pbFieldInfo.reason() == EN_Reason_LevelAndBonus_Game_Over_Settle)
                            {
                                ppbInfoItem->set_skipmatch_success_skip_level_num(ppbInfoItem->skipmatch_success_skip_level_num() + 1);
                            }

                            int iOldLevel = ppbInfoItem->skipmatch_level();

                            //尝试增加游戏的次数
                            TryAddSkipMatchGameNum(ppbInfoItem,pbFieldInfo);
                            ppbInfoItem->set_old_skipmatch_level(iOldLevel);
                            ppbInfoItem->set_skipmatch_level(ppbInfoItem->skipmatch_level() + pbFieldInfo.intval());
                            //设置最近一次的sessionid
                            ppbInfoItem->set_lately_session_id(pbFieldInfo.skipmatch_session_id());
                            //更新结果流
                            UpdateUserSkipMatchResultFlow(*ppbInfoItem,pbFieldInfo.skipmatch_session_id(),pbFieldInfo.intval(),
                                                          ppbInfoItem->skipmatch_state(),pbFieldInfo.reason());

                            if(ppbUserInfo)
                            {
                                //向log发消息，更新
                                SendSkipMatchLevelAndStateFlowLog(*ppbUserInfo,*ppbInfoItem,pbFieldInfo.intval(), pbFieldInfo.reason(),pbFieldInfo.game_type());
                            }
                        }
                        break;

                    //玩家闯关状态 ，0为初始状态， 1为失败状态
                    case EN_DB_Field_Skip_Match_Info_State:
//                        if(ppbInfoItem->skipmatch_state() == pbFieldInfo.intval())
//                        {
//                            break;
//                        }
                        if(!pbFieldInfo.has_reason())
                        {
                            VLogMsg(CLIB_LOG_LEV_DEBUG,"failed to save user[%lld] state : no reason",a_iUid);
                            return false;
                        }
                        if(pbFieldInfo.intval() != EN_Skip_Match_State_Initial && pbFieldInfo.intval() != EN_Skip_Match_State_Failed)
                        {
                            VLogMsg(CLIB_LOG_LEV_DEBUG,"failed to save user[%lld] state : state reason",a_iUid);
                            return false;
                        }

                        //老局等级的修改（给客户端判定前一局的等级），只有在失败的时候才去修改
                        if(pbFieldInfo.intval() == EN_Skip_Match_State_Failed)
                        {
                            int iOldLevel = ppbInfoItem->skipmatch_level();
                            ppbInfoItem->set_old_skipmatch_level(iOldLevel);
                        }

                        //设置闯关状态
                        ppbInfoItem->set_skipmatch_state(pbFieldInfo.intval());
                        //需要的奖金数
                        if(pbFieldInfo.has_skipmatch_need_share_num())
                        {
                            ppbInfoItem->set_skipmatch_need_share_num(pbFieldInfo.skipmatch_need_share_num());
                        }
                        //需要的砖石数
                        if(pbFieldInfo.has_skipmatch_need_diamond_num())
                        {
                            ppbInfoItem->set_skipmatch_need_diamond_num(pbFieldInfo.skipmatch_need_diamond_num());
                        }
                        //是否在这局中有输赢结果,默认情况下为没有，等于true
                        if(pbFieldInfo.has_lately_session_has_win_or_lose())
                        {
                            ppbInfoItem->set_lately_session_has_win_or_lose(pbFieldInfo.lately_session_has_win_or_lose());
                        }
                        else
                        {
                            ppbInfoItem->set_lately_session_has_win_or_lose(true);
                        }

                        //尝试增加游戏的次数
                        TryAddSkipMatchGameNum(ppbInfoItem,pbFieldInfo);
                        //设置最近一次的sessionid
                        ppbInfoItem->set_lately_session_id(pbFieldInfo.skipmatch_session_id());

                        //更新结果流
                        UpdateUserSkipMatchResultFlow(*ppbInfoItem,pbFieldInfo.skipmatch_session_id(),0,
                                                      pbFieldInfo.intval(),pbFieldInfo.reason());
                        if(ppbUserInfo)
                        {
                            //向log发消息，更新
                            SendSkipMatchLevelAndStateFlowLog(*ppbUserInfo,*ppbInfoItem,0, pbFieldInfo.reason(),pbFieldInfo.game_type());
                        }
                        break;
                        //总的赢的局数
                    case EN_DB_Field_Skip_Match_Info_Total_Win_Num:
                    {
                        if(0 == pbFieldInfo.intval())
                        {
                            break;
                        }

                        ppbInfoItem->set_skipmatch_total_win_num_on_type(ppbInfoItem->skipmatch_total_win_num_on_type() + pbFieldInfo.intval());
                        pbUserSkipMatchInfo.set_skipmatch_total_win_num(pbUserSkipMatchInfo.skipmatch_total_win_num() + pbFieldInfo.intval());
                        SendToTalWinNumLog(a_iUid,pbUserSkipMatchInfo,pbFieldInfo.intval(),pbFieldInfo.reason());

                        break;
                    }
                default:
                    break;
                }
                break;
            }
        default:
            break;
        }
    }
    return SerializeUserData(*ppbUserGameData, a_iUid, PBUserDataField::kUserGameData, a_pbRedisData);
}

/*
 * 获得用户的游戏数据
 * 返回值为指针
*/
PBUserGameData * UserManager::GetUserGameData(long long a_iUid)
{
    PBUserData* puser = GetUserData(a_iUid);
    if (!puser || !puser->has_user_game_data())
    {
        return NULL;
    }
    return puser->mutable_user_game_data();
}

/*
 * 初始化某个闯关游戏的记录
*/
bool UserManager::InitSkipMatchInfoItemByType(PBUserSkipMatchInfoItem * a_ppbInfoItem,int a_iSkipmatchType)
{
    a_ppbInfoItem->set_skipmatch_type(a_iSkipmatchType);
    a_ppbInfoItem->set_skipmatch_level(1);
    a_ppbInfoItem->set_skipmatch_state(EN_Skip_Match_State_Initial);
    a_ppbInfoItem->set_old_skipmatch_level(1);
    a_ppbInfoItem->set_skipmatch_success_skip_level_num(0);
    a_ppbInfoItem->set_skipmatch_game_num(0);
    a_ppbInfoItem->set_skipmatch_total_win_num_on_type(0);

    return true;
}

/*
 * 获得某个游戏的闯关信息从总的闯关信息中
*/
PBUserSkipMatchInfoItem * UserManager::GetItemFromSkipMatchInfo(int a_iGameType,PBUserSkipMatchInfo & a_pbUserSkipMatcahInfo)
{
    for(int i = 0 ; i < a_pbUserSkipMatcahInfo.skip_match_info_item_size() ; i ++)
    {
        PBUserSkipMatchInfoItem * pbItem = a_pbUserSkipMatcahInfo.mutable_skip_match_info_item(i);
        if(pbItem->skipmatch_type() == a_iGameType)
        {
            return pbItem;
        }
    }

    return NULL;
}

/*
 * 尝试增加游戏的次数
*/
void UserManager::TryAddSkipMatchGameNum(PBUserSkipMatchInfoItem * a_ppbInfoItem,const PBDBAtomicField & a_pbFieldInfo)
{
    bool bHasTheSameFlow = false;
    for(int i = 0 ; i < a_ppbInfoItem->result_flows_size() ; i ++)
    {
        const PBUserSkipMatchResultFlow & pbResultFlow = a_ppbInfoItem->result_flows(i);
        if(pbResultFlow.session_id() == a_pbFieldInfo.skipmatch_session_id())
        {
            bHasTheSameFlow = true;
        }
    }

    if(!bHasTheSameFlow)
    {
        a_ppbInfoItem->set_skipmatch_game_num(a_ppbInfoItem->skipmatch_game_num() + 1);
    }
}

/*
 * 发送闯关状态变更
*/
void UserManager::SendSkipMatchLevelAndStateFlowLog(const PBUser & user_info,const PBUserSkipMatchInfoItem & a_pbUserSkipInfoItem,int a_iLevelActNum,int a_iReason,int a_iTableType)
{
    PBCSMsg pbMsg;
    LogSkipMatchLevelAndStateFlow & pbLog = *pbMsg.mutable_log_skipmatch_level_and_state_flow();
    pbLog.set_uid(user_info.uid());
    pbLog.set_level_act_val(a_iLevelActNum);
    pbLog.set_level_after_change(a_pbUserSkipInfoItem.skipmatch_level());
    pbLog.set_state_after_change(a_pbUserSkipInfoItem.skipmatch_state());
    pbLog.set_reason(a_iReason);
    pbLog.set_time_stamp(time(NULL));
    pbLog.set_game_type(a_iTableType);
    pbLog.set_channel(user_info.channel());
    //如果是一个等级重置，并且原因是通过游戏结束，则置为一次成功的闯关
    if(a_iLevelActNum == -6 && a_iReason == EN_Reason_LevelAndBonus_Game_Over_Settle)
    {
        pbLog.set_is_finished_skip(true);
    }

    CLogWriter::Instance()->Send(pbMsg);
}

/*
 * 更新结果操作流
*/
void UserManager::UpdateUserSkipMatchResultFlow(PBUserSkipMatchInfoItem & a_pbUserSkipInfoItem,int64 a_iSessionId,int a_iLevelChangeVal,int a_iState
                                                ,int a_iReason)
{
    PBUserSkipMatchResultFlow pbResultFlow;
    pbResultFlow.set_session_id(a_iSessionId);
    pbResultFlow.set_skipmatch_level_change_val(a_iLevelChangeVal);
    pbResultFlow.set_skipmatch_level_after_change(a_pbUserSkipInfoItem.skipmatch_level());
    pbResultFlow.set_skipmatch_state(a_iState);
    pbResultFlow.set_reason(a_iReason);
    //校验这个操作流的原因
    if(EN_Reason_LevelAndBonus_Choice_Reset == a_iReason ||
            EN_Reason_LevelAndBonus_Share_Success == a_iReason ||
            EN_Reason_LevelAndBonus_Use_Diamond == a_iReason)
    {
        pbResultFlow.set_is_over(true);
    }

    a_pbUserSkipInfoItem.add_result_flows()->CopyFrom(pbResultFlow);

    //需要删除以前教老的流
    if(a_pbUserSkipInfoItem.result_flows_size() <= 5)
    {
        return ;
    }

    RemoveNeedlessResultFlow(a_pbUserSkipInfoItem);
}

/*
 * 删除多余的流
*/
bool UserManager::RemoveNeedlessResultFlow(PBUserSkipMatchInfoItem & a_pbUserSkipInfoItem)
{
    vector<int> del_vect;
    for(int i = 0 ; i < a_pbUserSkipInfoItem.result_flows_size() ; i ++)
    {
        if(i >= a_pbUserSkipInfoItem.result_flows_size() - 5)
        {
            break;
        }

        del_vect.push_back(i);
    }

    for(unsigned int i = 0 ; i < del_vect.size() ; i ++)
    {
        int iDelIndex = del_vect[i];
        a_pbUserSkipInfoItem.mutable_result_flows()->DeleteSubrange(iDelIndex,1);
    }

    return true;
}

/*
 * 发送总的获胜局数变更信息
*/
void UserManager::SendToTalWinNumLog(int64 a_iUid,const PBUserSkipMatchInfo & a_pbUserSkipInfo,int a_iActNum,int a_iReason)
{
    PBCSMsg msg;
    LogSkipMatchTotalWinNum &log = *msg.mutable_log_skipmatch_total_win_num();
    log.set_uid(a_iUid);
    log.set_act_num(a_iActNum);
    log.set_total_num(a_pbUserSkipInfo.skipmatch_total_win_num());
    log.set_reason(a_iReason);

    CLogWriter::Instance()->Send(msg);
}
