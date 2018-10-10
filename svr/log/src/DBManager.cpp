#include "DBManager.h"

void* DBThreadFunc(void* para)
{
    // unjoinable
    pthread_detach(pthread_self());

    CThreadPara* pthread_para = (CThreadPara*)para;
    CDataBaseHandler db_handler;

    Database_Param dbParam;
    dbParam.host = pthread_para->log_config.db_host();
    dbParam.port = pthread_para->log_config.db_port();
    dbParam.user = pthread_para->log_config.db_user();
    dbParam.password = pthread_para->log_config.db_pwd();
    dbParam.db = pthread_para->log_config.db_name();

    if (0 == db_handler.ConnectDB(dbParam))
    {
        vector<string>::iterator iter = pthread_para->sql_vec.begin();
        for (; iter != pthread_para->sql_vec.end(); ++iter)
        {
        	//todo 多条sql语句拼接在一起 一次性执行多条
            db_handler.SQLOpporateDML(*iter);
        }
    }

    db_handler.DisconnectDB();
    delete pthread_para;
    pthread_exit((void*)0);
}

CDBManager* CDBManager::Instance (void)
{
	return CSingleton<CDBManager>::Instance();
}

bool CDBManager::Init()
{
    // 读取配置
    PBConfig config;
    if (!log_config.Init("../../conf/logsvrd.CFG", config))
    {
        ErrMsg("LogSvrd load config failed");
        return false;
    }
    log_config.CopyFrom(config.logsvrd_config());

    timer.SetTimeEventObj(this, LOG_TIMER);
    timer.StartTimerBySecond(60, true);

    Database_Param dbParam;
    dbParam.host = log_config.db_host();
    dbParam.port = log_config.db_port();
    dbParam.user = log_config.db_user();
    dbParam.password = log_config.db_pwd();
    dbParam.db = log_config.db_name();

    if (0 != db_handler.ConnectDB(dbParam))
    {
        ErrMsg("LogSvrd connect db failed");
        return false;
    }

    play = 0;
    play_robot = 0;
    sql_vec.reserve(log_config.max_sql_num());
	bj_play = 0;
    return true;
}

int CDBManager::ProcessOnTimerOut(int Timerid)
{
    int online = 0;
    for (std::map<int, int>::iterator iter = online_map.begin(); iter != online_map.end(); ++iter)
        online += iter->second;

	char szSql[10240] = {0};
    sprintf(szSql, "INSERT INTO lzmj_online(online, play, time_stamp)"
        "VALUES(%d, %d, %d)",
        online, play, (int)time(NULL));
	string sql = szSql;

	if (0 != db_handler.SQLOpporateDML(sql))
	{
        ErrMsg("db err, sql: %s, err: %s\n", szSql, db_handler.What().c_str());
	}

    // 记录各游戏在玩
    {
        for (std::map<int, std::map<int, int> >::iterator it = atplay_map.begin(); it != atplay_map.end(); ++it)
        {
            int game_type = it->first;
            int at_play = 0;
            std::map<int, int>& sub_map = it->second;
            for (std::map<int, int>::iterator it2 = sub_map.begin(); it2 != sub_map.end(); ++it2)
            {
                at_play += it2->second;
            }

            char szSql[10240] = {0};
            sprintf(szSql, "INSERT INTO lzmj_atplay(play, game_type, time_stamp)"
                "VALUES(%d, %d, %d)",
                at_play, game_type, (int)time(NULL));
            string sql = szSql;

            if (0 != db_handler.SQLOpporateDML(sql))
            {
                ErrMsg("db err, sql: %s, err: %s\n", szSql, db_handler.What().c_str());
            }
        }
        atplay_map.clear();
    }
	return 0;
}

void CDBManager::AddSql(string sql)
{
    sql_vec.push_back(sql);
    if (sql_vec.size() >= log_config.max_sql_num())
    {
        // 线程写db
        CThreadPara* pthread_para = new CThreadPara;
        pthread_para->log_config.CopyFrom(log_config);
        pthread_para->sql_vec = sql_vec;
        pthread_t id;
        int ret = pthread_create(&id, NULL, DBThreadFunc, pthread_para);
        if (0 != ret)
        {
            delete pthread_para;
            ErrMsg("db thread create err: %d\n", ret);
        }
        sql_vec.clear();
    }
}

