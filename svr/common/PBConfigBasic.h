#pragma once
#include <errno.h>
#include <map>
#include "global.h"
#include "poker_config.pb.h"
#include "google/protobuf/text_format.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"

using namespace std;

template<typename PB_CONFIG_TYPE>
class PBConfigBasic : public PB_CONFIG_TYPE
{
public:
    typedef PB_CONFIG_TYPE conf_type;

public:
    PBConfigBasic() : _is_init(false) { }

    bool IsInit() const { return _is_init; }

    bool Init(char const * path,PBConfig & config)
    {
        int fd = open(path , O_RDONLY);
        if (fd < 0)
        {
            ErrMsg("open config err, path=%s, errstr=%s", path, strerror(errno));
            return false;
        }

        config.Clear();

        google::protobuf::io::FileInputStream fis(fd);
        fis.SetCloseOnDelete(true);
        if (!google::protobuf::TextFormat::Parse(&fis, &config))
        {
            ErrMsg("parse config err, path=%s, %s",
                   path, strerror(errno));
            return false;
        }

        config.DiscardUnknownFields();
        _is_init = true;
        return true;
    }

public:
    bool _is_init;
};

template <typename CONF_TYPE>
class PBConfigWrapperBasic
{
public:
    static bool Init(const char * path)
    {
        return _Instance()->Init(path);
    }
public:
    static CONF_TYPE * Instance()
    {
        return _Instance();
    }
protected:
    static CONF_TYPE * _Instance()
    {
        static CONF_TYPE _config;
        return & _config;
    }
};

class _PBRouteSvrdConfig : public PBConfigBasic<PBRouteSvrdConfig>
{
    typedef PBConfigBasic<PBRouteSvrdConfig> base_type;
public:
    bool Init(char const * path)
    {
        PBConfig config;
        if(!base_type::Init(path,config)) return false;
        base_type::CopyFrom(config.route_config());
        VLogMsg(CLIB_LOG_LEV_DEBUG,"init route config");
        return true;
    }
};

class _PBGlobalRedisConfig : public PBConfigBasic<PBGlobalRedisConfig>
{
    typedef PBConfigBasic<PBGlobalRedisConfig> base_type;
public:
    bool Init(char const * path)
    {
        PBConfig config;
        if(!base_type::Init(path,config)) return false;
        base_type::CopyFrom(config.global_redis_config());
        VLogMsg(CLIB_LOG_LEV_DEBUG,"init global redis config");
        return true;
    }

};

class _PBDBProxySvrdConfig : public PBConfigBasic<PBDBProxySvrdConfig>
{
    typedef PBConfigBasic<PBDBProxySvrdConfig> base_type;
public:
    bool Init(char const * path)
    {
        PBConfig config;
        if(!base_type::Init(path,config)) return false;
        base_type::CopyFrom(config.dbproxy_svrd_config());
        VLogMsg(CLIB_LOG_LEV_DEBUG,"init db proxy config");
        return true;
    }
};

class _PBWinScoreConfig : public PBConfigBasic<PBWinScoreConfig>
{
    typedef PBConfigBasic<PBWinScoreConfig> base_type;
    typedef map<int, int>   index_type;
public:
    bool Init(char const * path)
    {
        PBConfig config;
        if(!base_type::Init(path,config)) return false;
        base_type::CopyFrom(config.winscore_config());
        VLogMsg(CLIB_LOG_LEV_DEBUG,"init db win score config");
        for(int i=0;i<base_type::time_score_list_size();i++)
        {
            _index[base_type::time_score_list(i).time()] = base_type::time_score_list(i).score();
        }
        return true;
    }
    int GetScore(int time)
    {
        if(_index.find(time) == _index.end())
        {
            return -1;
        }
        return _index[time];
    }
public:
    index_type _index;
};

class _PBCostConfig : public PBConfigBasic<PBCostConfig>
{
    typedef PBConfigBasic<PBCostConfig> base_type;
    typedef map<int, int>   index_type;
public:
    bool Init(char const * path)
    {
        PBConfig config;
        if(!base_type::Init(path,config)) return false;
        base_type::CopyFrom(config.cost_config());
        VLogMsg(CLIB_LOG_LEV_DEBUG,"init cost config");
        for(int i=0;i<base_type::cost_item_list_size();i++)
        {
            _index[base_type::cost_item_list(i).round()] = base_type::cost_item_list(i).cost();
        }
        return true;
    }
    int GetCostByRound(int round)
    {
        if(_index.find(round) == _index.end())
        {
            return -1;
        }
        return _index[round];
    }
public:
    index_type _index;
};

class _PBMonitorConfig : public PBConfigBasic<PBMonitorConfig>
{
    typedef PBConfigBasic<PBMonitorConfig> base_type;
    typedef map<int, int>   index_type;
public:
    bool Init(char const * path)
    {
        PBConfig config;
        if(!base_type::Init(path,config)) return false;
        base_type::CopyFrom(config.monitor_config());
        VLogMsg(CLIB_LOG_LEV_DEBUG,"init monitor config");
        return true;
    }
};

class _PBTableMgrConfig : public PBConfigBasic<PBTableMgrConfig>
{
    typedef PBConfigBasic<PBTableMgrConfig> base_type;
    typedef map<int, int>   index_type;
public:
    bool Init(char const * path)
    {
        PBConfig config;
        if(!base_type::Init(path,config)) return false;
        base_type::CopyFrom(config.table_mgr_config());
        VLogMsg(CLIB_LOG_LEV_DEBUG,"init table mgr config");
        return true;
    }
};

class _PBConnectSvrdConfig : public PBConfigBasic<PBConnectSvrdConfig>
{
    typedef PBConfigBasic<PBConnectSvrdConfig> base_type;
    typedef map<int, int>   index_type;
public:
    bool Init(char const * path)
    {
        PBConfig config;
        if(!base_type::Init(path,config)) return false;
        base_type::CopyFrom(config.connect_svrd_config());
        VLogMsg(CLIB_LOG_LEV_DEBUG,"init connect svrd config");
        return true;
    }

    bool CheckPosition(int pos_type)
    {
        for (int i = 0; i < games_size(); i++)
        {
            if (games(i).pos_type() == pos_type)
            {
                return true;
            }
        }

        return false;
    }

    int GetNode(int pos_type)
    {
        for (int i = 0; i < games_size(); i++)
        {
            if (games(i).pos_type() == pos_type)
            {
                return games(i).node_type();
            }
        }

        return EN_Node_Unknown;
    }

    bool IsGameNode(int node_type)
        {
            for (int i = 0; i < games_size(); i++)
            {
                if (games(i).node_type() == node_type)
                {
                    return true;
                }
            }
            return false;
        }
};

class _PBRoomSvrdConfig : public PBConfigBasic<PBConnectSvrdConfig>
{
    typedef PBConfigBasic<PBConnectSvrdConfig> base_type;
    typedef map<int, int>   index_type;
public:
    bool Init(char const * path)
    {
        PBConfig config;
        if(!base_type::Init(path,config)) return false;
        base_type::CopyFrom(config.room_svrd_config());
        VLogMsg(CLIB_LOG_LEV_DEBUG,"init room svrd config");
        return true;
    }

};

class _PBTeaBarConfig : public PBConfigBasic<PBTeaBarConfig>
{
    typedef PBConfigBasic<PBTeaBarConfig> base_type;
public:
    bool Init(char const * path)
    {
        PBConfig config;
        if (!base_type::Init(path, config)) return false;
        base_type::CopyFrom(config.tea_bar_config());
        VLogMsg(CLIB_LOG_LEV_DEBUG, "init tea bar config");
        return true;
    }
};

class _PBRecordRedisConfig : public PBConfigBasic<PBGlobalRedisConfig>
{
    typedef PBConfigBasic<PBGlobalRedisConfig> base_type;
public:
    bool Init(char const * path)
    {
        PBConfig config;
        if (!base_type::Init(path, config)) return false;
        base_type::CopyFrom(config.record_redis_config());
        VLogMsg(CLIB_LOG_LEV_DEBUG, "init record redis config");
        return true;
    }

};

class _PBActivityConfig : public PBConfigBasic<PBActivityConfig>
{
	typedef PBConfigBasic<PBActivityConfig> base_type;
public:
	bool Init(char const * path)
	{
		PBConfig config;
		if (!base_type::Init(path, config)) return false;
		base_type::CopyFrom(config.activity_config());
		VLogMsg(CLIB_LOG_LEV_DEBUG, "init activity config");
		return true;
	}

	const PBActivityItem* GetActivityItem(int actid)
	{
		for (int i = 0; i < base_type::activity_item_size(); i++)
		{
			const PBActivityItem& item = base_type::activity_item(i);
			if (item.actid() == actid)
			{
				return &item;
			}
		}
		return NULL;
	}
};

class _PBHallSvrdConfig : public PBConfigBasic<PBHallSvrdConfig>
{
    typedef PBConfigBasic<PBHallSvrdConfig> base_type;
public:
    bool Init(char const * path)
    {
        PBConfig config;
        if(!base_type::Init(path,config)) return false;
        base_type::CopyFrom(config.hall_svrd_config());
        VLogMsg(CLIB_LOG_LEV_DEBUG,"init hall config");
        return true;
    }
};

class _PBFreeGameConfig : public PBConfigBasic<PBFreeGameConfig>{
	typedef PBConfigBasic<PBFreeGameConfig> base_type;
public:
    bool Init(char const * path)
    {
        PBConfig config;
        if(!base_type::Init(path,config)) return false;
        base_type::CopyFrom(config.free_game_config());
        VLogMsg(CLIB_LOG_LEV_DEBUG,"init free game config");
        return true;
    }

	const PBFreeGameConfItem * GetConfItem(int ttype)
	{
		for (int i = 0; i < base_type::items_size(); i++)
		{
			const PBFreeGameConfItem & item = base_type::items(i);
			if (item.ttype() == ttype)
			{
				return &item;
			}
		}
		return NULL;
	}
};

class _PBMatchTypeMapConfig : public PBConfigBasic<PBGameTypeMapConfig>
{
    typedef PBConfigBasic<PBGameTypeMapConfig> base_type;
    typedef map<int, int>   index_type;
public:
    bool Init(char const * path)
    {
        PBConfig config;
        if(!base_type::Init(path,config)) return false;
        base_type::CopyFrom(config.game_type_map_config());
        VLogMsg(CLIB_LOG_LEV_DEBUG,"init table type map config");
        return true;
    }

    ENNodeType GetNodeTypeByPosType(ENPlayerPositionType pos_type)
    {
        VLogMsg(CLIB_LOG_LEV_DEBUG,"base_type::game_type_size() %d",base_type::game_type_size());
            for (int i = 0; i < base_type::game_type_size(); i++)
            {
                const PBTableGameType& game = base_type::game_type(i);
                VLogMsg(CLIB_LOG_LEV_DEBUG,"base_type::game_type %d",int(game.pos_type()));
                VLogMsg(CLIB_LOG_LEV_DEBUG,"base_type::game_type %d",int(pos_type));

                    if (game.pos_type() == int(pos_type))
                    {
                            return game.node_type();
                    }
            }
            return EN_Node_Unknown;
    }

    ENNodeType GetNodeTypeByTableType(ENTableType ttype)
    {
            for (int i = 0; i < base_type::game_type().size(); i++)
            {
                    const PBTableGameType& game = base_type::game_type(i);
                    if (game.table_type() == ttype)
                    {
                            return game.node_type();
                    }
            }
            return EN_Node_Unknown;
    }
};

class _PBMatchTableMgrConfig : public PBConfigBasic<PBTableMgrConfig>
{
    typedef PBConfigBasic<PBTableMgrConfig> base_type;
    typedef map<int, int>   index_type;
public:
    bool Init(char const * path)
    {
        PBConfig config;
        if(!base_type::Init(path,config)) return false;
        base_type::CopyFrom(config.table_mgr_config());
        VLogMsg(CLIB_LOG_LEV_DEBUG,"init table mgr config");
        return true;
    }
};
class _PBActivityGameRoomConfig : public PBConfigBasic<PBActivityGameRoomConfig>
{
    typedef PBConfigBasic<PBActivityGameRoomConfig> base_type;
public:
    bool Init(char const * pPath)
    {
        PBConfig pbConfig;
        if(!base_type::Init(pPath,pbConfig))
        {
            return false;
        }

        base_type::CopyFrom(pbConfig.activity_game_room_config());
        VLogMsg(CLIB_LOG_LEV_DEBUG, "init skip match room config");
        ErrMsg("init skip match room config");

        return true;
    }

    int GetNodeTypeByTableType(int a_iType)
    {
        for(int i = 0 ; i < base_type::games_size() ; i++)
        {
            const PBTableNodePosConfig & pbGame = base_type::games(i);
            if(pbGame.ttype() == a_iType)
            {
                return pbGame.node_type();
            }
        }

        return -1;
    }

    int GetNodeTypeByPosType(int a_iPosType)
    {
        for(int i = 0 ; i < base_type::games_size() ; i ++)
        {
            const PBTableNodePosConfig & pbGame = base_type::games(i);
            if(pbGame.pos_type() == a_iPosType)
            {
                return pbGame.node_type();
            }
        }

        return -1;
    }

    int GetTableTypeByPosType(int a_iPosType)
    {
        for(int i = 0 ; i < base_type::games_size() ; i ++)
        {
            const PBTableNodePosConfig & pbGame = base_type::games(i);
            if(pbGame.pos_type() == a_iPosType)
            {
                return pbGame.ttype();
            }
        }

        return -1;
    }
};

class _PBFailedCostRedisConfig : public PBConfigBasic<PBFailedCostRedisConfig>
{
    typedef PBConfigBasic<PBFailedCostRedisConfig> base_type;

public:
    bool Init(char const * pPath)
    {
        PBConfig pbConfig;
        if(!base_type::Init(pPath,pbConfig))
        {
            return false;
        }

        base_type::CopyFrom(pbConfig.failed_cost_redis_config());
        VLogMsg(CLIB_LOG_LEV_DEBUG, "init failed_cost_redis_config");
        ErrMsg("init failed_cost_redis_config");

        return true;
    }
};

class _PBRobotConfig : public PBConfigBasic<PBRobotConfig>
{
	typedef PBConfigBasic<PBRobotConfig> base_type;
	typedef map<int, int>   index_type;
public:
	bool Init(char const * path)
	{
		PBConfig config;
		if (!base_type::Init(path, config)) return false;
		base_type::CopyFrom(config.robot_config());
		VLogMsg(CLIB_LOG_LEV_DEBUG, "init robot config");
		return true;
	}
};

class _PBAutoMatchRoomConfig : public PBConfigBasic<PBAutoMatchRoomConfig>
{
	typedef PBConfigBasic<PBAutoMatchRoomConfig> base_type;
public:
	bool Init(char const * path)
	{
		PBConfig config;
		if (!base_type::Init(path, config)) return false;
		base_type::CopyFrom(config.auto_match_room_config());
		VLogMsg(CLIB_LOG_LEV_DEBUG, "init auto match room config");
		ErrMsg("init auto match room config");
		return true;
	}

	PBAutoMatchRoomItem* GetConf(int ttype, int level)
	{
		for (int i = 0; i < base_type::items_size(); i++)
		{
			PBAutoMatchRoomItem * pitem = base_type::mutable_items(i);
			if (pitem->ttype() == ttype && pitem->level() == level)
			{
				return pitem;
			}
		}
		return NULL;
	}

	int GetNodeTypeByTableType(int ttype)
	{
		for (int i = 0; i < base_type::games_size(); i++)
		{
			const PBTableNodePosConfig& game = base_type::games(i);
			if (game.ttype() == ttype)
			{
				return game.node_type();
			}
		}
		return -1;
	}

	int GetNodeTypeByPosType(int pos_type)
	{
		for (int i = 0; i < base_type::games_size(); i++)
		{
			const PBTableNodePosConfig& game = base_type::games(i);
			if (game.pos_type() == pos_type)
			{
				return game.node_type();
			}
		}
		return -1;
	}

	int GetTableTypeByPosType(int pos_type)
	{
		for (int i = 0; i < base_type::games_size(); i++)
		{
			const PBTableNodePosConfig& game = base_type::games(i);
			if (game.pos_type() == pos_type)
			{
				return game.ttype();
			}
		}
		return -1;
	}

	int GetTableTypeByNodeType(int node_type)
	{
		for (int i = 0; i < base_type::games_size(); i++)
		{
			const PBTableNodePosConfig& game = base_type::games(i);
			if (game.node_type() == node_type)
			{
				return game.ttype();
			}
		}
		return -1;
	}
};

/////////////////////////////////////////////////////////////////////////
class PokerPBRouteSvrdConfig : public PBConfigWrapperBasic<_PBRouteSvrdConfig>{};
class PokerPBDBProxySvrdConfig : public PBConfigWrapperBasic<_PBDBProxySvrdConfig>{};
class PokerPBGlobalRedisConfig : public PBConfigWrapperBasic<_PBGlobalRedisConfig>{};
class PokerPBWinScoreConfig : public PBConfigWrapperBasic<_PBWinScoreConfig>{};
class PokerPBCostConfig : public PBConfigWrapperBasic<_PBCostConfig>{};
class PokerPBMonitorConfig : public PBConfigWrapperBasic<_PBMonitorConfig>{};
class PokerPBTableMgrConfig : public PBConfigWrapperBasic<_PBTableMgrConfig>{};
class PokerPBMatchTableMgrConfig : public PBConfigWrapperBasic<_PBMatchTableMgrConfig>{};
class PokerPBConnectSvrdConfig : public PBConfigWrapperBasic<_PBConnectSvrdConfig>{};
class PokerPBTeaBarConfig : public PBConfigWrapperBasic<_PBTeaBarConfig> {};
class PokerPBRecordRedisConfig : public PBConfigWrapperBasic<_PBRecordRedisConfig> {};
class PokerPBActivityConfig : public PBConfigWrapperBasic<_PBActivityConfig>{};
class PokerPBRoomSvrdConfig : public PBConfigWrapperBasic<_PBRoomSvrdConfig>{};
class PokerPBHallSvrdConfig : public PBConfigWrapperBasic<_PBHallSvrdConfig>{};
class PokerPBFreeGameConfig :public PBConfigWrapperBasic<_PBFreeGameConfig>{};
class PokerPBMatchTypeMapConfig : public PBConfigWrapperBasic<_PBMatchTypeMapConfig>{};
class PokerActivityGameRoomConfig : public PBConfigWrapperBasic<_PBActivityGameRoomConfig>{};
class PokerFailedCostRedisConfig : public PBConfigWrapperBasic<_PBFailedCostRedisConfig>{};
class PokerAutoMatchRoomConfig : public PBConfigWrapperBasic<_PBAutoMatchRoomConfig> {};

//»úÆ÷ÈË
class PokerPBRobotConfig : public PBConfigWrapperBasic<_PBRobotConfig> {};

