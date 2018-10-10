#include "RobotMgr.h"
#include "PBConfigBasic.h"

bool RobotManager::Init()
{
	InitRobot();
	return true;
}

int RobotManager::ProcessOnTimerOut(int Timerid)
{
	switch (Timerid)
	{
	default:
		break;
	}
	return 0;
}


void RobotManager::InitRobot()
{
	int start_id = TGlobal::_group_id * 100000;
	char account[100] = {0};
	for (int i = 0; i < PokerPBRobotConfig::Instance()->robot_num(); i++)
	{
		memset(account, 0, sizeof(account));
		snprintf(account, sizeof(account), "RobotX%d", start_id + i);
		int detail_index = i % PokerPBRobotConfig::Instance()->detail_size();
		const PBRobotDetailConfig& detail = PokerPBRobotConfig::Instance()->detail(detail_index);
		string url = PokerPBRobotConfig::Instance()->url() + detail.pic();
		CRobot *probot = new CRobot();
		probot->SetInfo(account, url, detail.nick(), detail.play_num(), detail.win_num());
		_free_robot_list.push_back(probot);
	}
}

void RobotManager::RecycleRobot(CRobot* probot)
{
	RobotList::iterator iter = _use_robot_list.begin();
	for (; iter != _use_robot_list.end();)
	{
		CRobot* p = *iter;
		if (p == NULL)
		{
			_use_robot_list.erase(iter);
		}
		else
		{
			if (p->_account == probot->_account)
			{
				_use_robot_list.erase(iter);
				continue;
			}
		}
		iter++;
	}
	_free_robot_list.push_back(probot);
}

void RobotManager::PutRobot(int64 a_iMatchid, int a_iRobotNum, const PBSourceInfoRequestingRobot & a_pbSourceInfo)
{
	if (_free_robot_list.empty())
	{
		return;
	}

	ErrMsg("_free_robot_list size:[%d]", (int)_free_robot_list.size());

	std::random_shuffle(_free_robot_list.begin(), _free_robot_list.end());

	for (int i = 0; i < a_iRobotNum; i++)
	{
		CRobot* probot = _free_robot_list.back();
		_free_robot_list.pop_back();
		_use_robot_list.push_back(probot);

		if (!probot->Init())
		{
			probot->Recycle();
		}
		else
		{
			probot->Login(a_iMatchid, a_pbSourceInfo);
		}

		if (_free_robot_list.empty())
		{
			return;
		}
	}
}

CRobot* RobotManager::GetRobot(long long uid)
{
	RobotList::iterator iter = _use_robot_list.begin();
	for (; iter != _use_robot_list.end();)
	{
		CRobot* p = *iter;
		if (p == NULL)
		{
			_use_robot_list.erase(iter);
		}
		else
		{
			if (p->GetUid() == uid)
			{
				return p;
			}
			iter++;
		}
	}
	return NULL;
}