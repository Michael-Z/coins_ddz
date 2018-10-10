#include "Robot.h"
#include "Message.h"
#include "ClientHandlerProxy.h"
#include "RobotHandlerToken.h"
#include "RobotMgr.h"
#include "PBConfigBasic.h"
#include "HandlerManager.h"
#include "RobotHandlerProxy.h"
#include "ClientHandlerToken.h"

CRobot::CRobot()
{
	_account = "";
	_url = "";
	_nick = "";
	_uid = 0;
	_chips = 0;
	_phandler = NULL;
	_state = EN_Robot_State_Idle;
	_play_num = 0;
	_win_num = 0;
	_last_active_time_stamp = 0;
}

CRobot::~CRobot()
{
	if (_phandler)
	{
		delete _phandler;
		_phandler = NULL;
	}
	_heartbeat_timer.StopTimer();
}

void CRobot::SetInfo(string account, string url, string nick, int play_num, int win_num)
{
	_account = account;
	_url = url;
	_nick = nick;
	_play_num = play_num;
	_win_num = win_num;
}

bool CRobot::Init()
{
	ErrMsg("Init,account:[%s]", _account.c_str());
	_phandler = new CTCPSocketHandler();
	if (_phandler == NULL)
	{
		return false;
	}
	_phandler->_pproxy = ClientHandlerProxy::Instance();
	int index = rand() % PokerPBRobotConfig::Instance()->connect_size();
	if (_phandler->Connect(PokerPBRobotConfig::Instance()->connect(index).ip(), PokerPBRobotConfig::Instance()->connect(index).port()) != 0)
	{
		ErrMsg("failed to connect[%s,%d]", PokerPBRobotConfig::Instance()->connect(index).ip().c_str(), PokerPBRobotConfig::Instance()->connect(index).port());
		delete _phandler;
		_phandler = NULL;
		return false;
	}
	_iConenctId = PokerPBRobotConfig::Instance()->connect(index).svid_id();

	ClientHandlerToken * ptoken = (ClientHandlerToken *)HandlerManager::Instance()->GetHandlerToken(_phandler);
	if (ptoken == NULL)
	{
		VLogMsg(CLIB_LOG_LEV_ERROR, "fatal error.failed to find token for handler.");
		return false;
	}
	ptoken->_probot = this;
	_heartbeat_timer.SetTimeEventObj(this, HEART_BEAT_TIMER);
	_heartbeat_timer.StartTimer(60 * 1000, true);
	return true;
}

void CRobot::Login(int64 a_iMatchid, const PBSourceInfoRequestingRobot & a_pbSourceInfo)
{
	if (_phandler == NULL)
	{
		return;
	}

	PBCSMsg msg;
	CSRequestLogin& request = *msg.mutable_cs_request_login();
	request.set_acc_type(EN_Account_Robot);
	request.set_account(_account);
	request.set_pic_url(_url);
	request.set_nick(_nick);
	Message::SendRequestMsgAsClient(_phandler, msg);
	_state = EN_Robot_State_Wait_Login_Response;
	_last_active_time_stamp = (int)time(NULL);
	_matchid = a_iMatchid;
	_source_info.CopyFrom(a_pbSourceInfo);
}

void CRobot::Logout()
{
	Recycle();
}

void CRobot::SetUid(long long uid)
{
	_uid = uid;
}

void CRobot::SetChips(long long chips)
{
	_chips = chips;
}


void CRobot::ProcessState()
{
	switch (_state)
	{
	case EN_Robot_State_Wait_Login_Response:
		OnResponseLogin();
		break;
	case EN_Robot_State_Wait_Skip_Match:
		OnResponseSkipMatch();

	case EN_Robot_State_Wait_Coin_Match:
		OnResponseCoinMatch();

	case EN_Robot_State_Wait_Enter_Match:
		OnResponseEnterMatch();
		break;
	default:
		break;
	}
}

void CRobot::OnResponseLogin()
{
	const CSResponseLogin & cs_response_login = _response_msg.cs_response_login();
	if (cs_response_login.result() != EN_MESSAGE_ERROR_OK)
	{
		ErrMsg("failed to login for robot[%s].result[%d]", _account.c_str(), cs_response_login.result());
		Recycle();
	}
	else
	{
		/*此处根据需求重写
		*/
		const PBUser & user = cs_response_login.user();
		const PBUserGameData & user_game_data = cs_response_login.user_game_data();
		//如果是活动游戏服
		if (_source_info.has_activity_type())
		{
			switch (_source_info.activity_type())
			{
			case EN_User_Game_Info_Skip_Match:
			{
				PBBPlayerPositionInfo pbPos;

				//先检测位置
				if (!user.has_skipmatch_pos())
				{
					pbPos.set_pos_type(EN_Position_Hall);
					//break;
				}
				else
				{
					pbPos.CopyFrom(user.skipmatch_pos());
				}

				//如果不在大厅，刷新活跃时间
				if (pbPos.pos_type() != EN_Position_Hall)
				{
					_last_active_time_stamp = (int)time(NULL);
				}
				else
				{
					int iLevel = 1;	//默认为1
					const PBUserSkipMatchInfo & pbSkipmatchInfo = user_game_data.user_skip_match_info();
					for (auto iter = pbSkipmatchInfo.skip_match_info_item().begin() ; iter != pbSkipmatchInfo.skip_match_info_item().end(); iter ++)
					{
						if (iter->skipmatch_type() == _source_info.game_type())
						{
							iLevel = iter->skipmatch_level();
						}
					}

					PBCSMsg pbMsg;
					CSRequestSkipMatchGame & pbRequest = *pbMsg.mutable_cs_request_skip_match_game();
					pbRequest.set_ttype(_source_info.game_type());
					pbRequest.set_connect_id(_iConenctId);
					pbRequest.set_level(iLevel);

					Message::SendRequestMsgAsClient(_phandler, pbMsg);
					_last_active_time_stamp = (int)time(NULL);
					_state = EN_Robot_State_Wait_Skip_Match;
				}
			}
			case EN_User_Game_Info_Coin_Match:
			{
				//检查位置
				PBBPlayerPositionInfo pbPos;
				if (!user.has_coin_pos())
				{
					pbPos.set_pos_type(EN_Position_Hall);
				}
				else
				{
					pbPos.CopyFrom(user.coin_pos());
				}

				//如果不在大厅，刷新活跃时间
				if (pbPos.pos_type() != EN_Position_Hall)
				{
					_last_active_time_stamp = (int)time(NULL);
				}
				//else
				{
					PBCSMsg pbMsg;
					CSRequestCoinMatchGame & pbRequest = *pbMsg.mutable_cs_request_coin_match_game();
					pbRequest.set_ttype(_source_info.game_type());
					pbRequest.set_connect_id(_iConenctId);
					pbRequest.set_level(_source_info.level());

					Message::SendRequestMsgAsClient(_phandler, pbMsg);
					_last_active_time_stamp = (int)time(NULL);
					_state = EN_Robot_State_Wait_Coin_Match;
				}
			}

			default:
				break;
			}
		}
		//当然也可以是其他信息
		else if (false)
		{
			return;
		}
	}
}

int CRobot::ProcessOnTimerOut(int TimerID)
{
	switch (TimerID)
	{
	case HEART_BEAT_TIMER:
	{
		PBCSMsg msg;
		msg.mutable_cs_request_heart_beat();
		Message::SendRequestMsgAsClient(_phandler, msg);
	}
	break;
	default:
		break;
	}
	int now = (int)time(NULL);
	if (_state != EN_Robot_State_Idle && _state != EN_Robot_State_Enter_Match_Succ)
	{
		if (now - _last_active_time_stamp >= 5)
		{
			Recycle();
		}
	}
	else if (_state == EN_Robot_State_Enter_Match_Succ)
	{
		if (now - _last_active_time_stamp >= 60 * 60 * 24)
		{
			Recycle();
		}
	}
	return 0;
}

void CRobot::OnHandlerClosed()
{
	VLogMsg(CLIB_LOG_LEV_ERROR, "robot[%s,%lld] handler closed", _account.c_str(), _uid);
	Recycle();
}

void CRobot::Recycle()
{
	ErrMsg("Recycle,account:[%s]", _account.c_str());
	_uid = 0;
	_chips = 0;
	_state = EN_Robot_State_Idle;
	if (_phandler)
	{
		delete _phandler;
		_phandler = NULL;
	}
	_pos_type = 0;
	_gamesvid = 0;
	_tid = 0;

	_heartbeat_timer.StopTimer();
	RobotManager::Instance()->RecycleRobot(this);
}

void CRobot::OnResponseEnterMatch()
{
	_state = EN_Robot_State_Enter_Match_Succ;
	_last_active_time_stamp = (int)time(NULL);
}

void CRobot::OnResponseSkipMatch()
{
	const CSResponseSkipMatchGame & pbResponse = _response_msg.cs_response_skip_match_game();
	if (pbResponse.result() != EN_MESSAGE_ERROR_OK)
	{
		ErrMsg("failed to login for robot[%s].result[%d]", _account.c_str(), pbResponse.result());
		Recycle();
	}

	int iTid = pbResponse.tid();
	int iSkipSvidId = pbResponse.skipmatch_game_svid();
	int iPosType = pbResponse.pos_type();

	//发送数据
	PBCSMsg pbMsg;
	CSRequestSdrEnterTable & pbRequest = *pbMsg.mutable_cs_request_sdr_enter_table();
	pbRequest.set_tid(iTid);
	pbRequest.set_connect_id(_iConenctId);
	pbRequest.set_skipmatch_game_svid(iSkipSvidId);
	pbRequest.set_pos_type(iPosType);

	Message::SendRequestMsgAsClient(_phandler, pbMsg);
	_state = EN_Robot_State_Wait_Enter_Match;
	_last_active_time_stamp = (int)time(NULL);
}

void CRobot::OnResponseCoinMatch()
{
	const CSResponseCoinMatchGame & pbResponse = _response_msg.cs_response_coin_match_game();
	if (pbResponse.result() != EN_MESSAGE_ERROR_OK)
	{
		ErrMsg("failed to login for robot[%s].result[%d]", _account.c_str(), pbResponse.result());
		Recycle();
	}

	int iTid = pbResponse.tid();
	int iCoinSvidId = pbResponse.coin_game_svid();
	int iPosType = pbResponse.pos_type();

	//发送数据
	PBCSMsg pbMsg;
	CSRequestSdrEnterTable & pbRequest = *pbMsg.mutable_cs_request_sdr_enter_table();
	pbRequest.set_tid(iTid);
	pbRequest.set_connect_id(_iConenctId);
	pbRequest.set_coinmatch_game_svid(iCoinSvidId);
	pbRequest.set_coin_match_level(_source_info.level());
	pbRequest.set_pos_type(iPosType);

	Message::SendRequestMsgAsClient(_phandler, pbMsg);
	_state = EN_Robot_State_Wait_Enter_Match;
	_last_active_time_stamp = (int)time(NULL);
}