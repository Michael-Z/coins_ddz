#pragma once
#include "singleton.h"
#include <map>
#include <vector>
using namespace std;

typedef map<int,int> GameSvrdMap;
typedef map<int,GameSvrdMap> GameTypeMap;


class GameSvrdManager
{
public:
	static GameSvrdManager * Instance();
    void OnReportGame(int gtype,int gsvrd);
	void OnCloseGame(int gtype,int gsvrd);
	void OnRetireGame(int gtype,int gsvrd);
	bool IsGameExist(int gtype,int gsvrd);
	bool IsRetiredGameExist(int gtype,int gsvrd);
	bool IsRunningGameExist(int gtype,int gsvrd);
	bool GetGameIDByRandom(int gtype,int & gsvrd);
	void NotifyRoomSvrdStart();
    int GetGameID(int gtype);
    bool GetFirstGameID(int a_iGtype,int & a_iGsvrd);
public:
	GameTypeMap _game_type_map;
	GameTypeMap _retired_game_type_map;
};


