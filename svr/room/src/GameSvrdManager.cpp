#include "GameSvrdManager.h"
#include "RouteManager.h"
#include "poker_msg.pb.h"
#include "Message.h"
#include "type_def.h"
#include <stdlib.h>

GameSvrdManager * GameSvrdManager::Instance (void)
{
	return CSingleton<GameSvrdManager>::Instance();
}

void GameSvrdManager::OnReportGame(int gtype,int gsvrd)
{
	GameSvrdMap & svrd_map = _game_type_map[gtype];
    svrd_map[gsvrd] = 1;
}

void GameSvrdManager::OnCloseGame(int gtype,int gsvrd)
{
	if(_game_type_map.find(gtype) != _game_type_map.end())
	{
		GameSvrdMap & svrd_map = _game_type_map[gtype];
		if(svrd_map.find(gsvrd) != svrd_map.end())
		{
			svrd_map.erase(gsvrd);
		}
	}
	if(_retired_game_type_map.find(gtype) != _retired_game_type_map.end())
	{
		GameSvrdMap & svrd_map = _retired_game_type_map[gtype];
		if(svrd_map.find(gsvrd) != svrd_map.end())
		{
			svrd_map.erase(gsvrd);
		}
	}
}

void GameSvrdManager::OnRetireGame(int gtype,int gsvrd)
{
	bool exist = false;
	if(_game_type_map.find(gtype) != _game_type_map.end())
	{
		GameSvrdMap & svrd_map = _game_type_map[gtype];
		if(svrd_map.find(gsvrd) != svrd_map.end())
		{
			svrd_map.erase(gsvrd);
			exist = true;
		}
	}
	if(exist == true)
	{
		GameSvrdMap & svrd_map = _retired_game_type_map[gtype];
		svrd_map[gsvrd] = 1;
	}
}


bool GameSvrdManager::IsRunningGameExist(int gtype,int gsvrd)
{
	if(_game_type_map.find(gtype) == _game_type_map.end())
	{
		return false;
	}
	GameSvrdMap & svrd_map = _game_type_map[gtype];
	if(svrd_map.find(gsvrd) == svrd_map.end())
	{
		return false;
	}
	return true;
}

bool GameSvrdManager::IsRetiredGameExist(int gtype,int gsvrd)
{
	if(_retired_game_type_map.find(gtype) == _retired_game_type_map.end())
	{
		return false;
	}
	GameSvrdMap & svrd_map = _retired_game_type_map[gtype];
	if(svrd_map.find(gsvrd) == svrd_map.end())
	{
		return false;
	}
	return true;
}


bool GameSvrdManager::IsGameExist(int gtype,int gsvrd)
{
	return IsRetiredGameExist(gtype,gsvrd) || IsRunningGameExist(gtype,gsvrd);
}

bool GameSvrdManager::GetGameIDByRandom(int gtype,int & gsvrd)
{
	gsvrd = -1;
	if(_game_type_map.find(gtype) == _game_type_map.end())
	{
		return false;
	}
	GameSvrdMap & svrd_map = _game_type_map[gtype];
	if(svrd_map.size() == 0)
	{
		return false;
	}
	int randomkey = random()%svrd_map.size();
	GameSvrdMap::iterator iter = svrd_map.begin();
	for(int i=0;iter!=svrd_map.end();iter++,i++)
	{
		if(i==randomkey)
		{
			gsvrd = iter->first;
			return true;
		}
	}
	return false;
}

bool GameSvrdManager::GetFirstGameID(int a_iGtype,int & a_iGsvrd)
{
    a_iGsvrd = -1;
    if(_game_type_map.find(a_iGtype) == _game_type_map.end())
    {
        return false;
    }
    GameSvrdMap & svrd_map = _game_type_map[a_iGtype];
    if(svrd_map.size() == 0)
    {
        return false;
    }

    GameSvrdMap::iterator iter = svrd_map.begin();
    a_iGsvrd = iter->first;

    return true;
}

void GameSvrdManager::NotifyRoomSvrdStart()
{
	PBCSMsg msg;
    msg.mutable_ss_notify_room_svrd();
    Message::BroadcastToGame(RouteManager::Instance()->GetRouteByRandom(), msg);
    // fpf
    Message::BroadcastToFpfGame(RouteManager::Instance()->GetRouteByRandom(), msg);
    // daer
    Message::BroadcastToPhzGame(RouteManager::Instance()->GetRouteByRandom(), msg, EN_Node_DAER);
}

int GameSvrdManager::GetGameID(int gtype)
{
    if (_game_type_map.find(gtype) == _game_type_map.end())
    {
        return -1;
    }
    GameSvrdMap & svrd_map = _game_type_map[gtype];
    if (svrd_map.size() == 0)
    {
        return -1;
    }
    vector<int> vec;
    GameSvrdMap::iterator iter = svrd_map.begin();
    for (int i = 0; iter != svrd_map.end(); iter++, i++)
    {
        {
            vec.push_back(iter->first);
        }
    }
    if (vec.empty())
    {
        return -1;
    }
    return vec[rand()%vec.size()];
}


