#ifndef _DATAHANDLER_H_
#define _DATAHANDLER_H_

#include "DataBase.h"

typedef struct 					//数据库参数
{
	string host;				//主机名
	string user;				//用户名
	string password;			//密码
	string db;					//数据库名
	unsigned int port;			//端口，一般为0
	string unix_socket;			//套接字，一般为NULL
	unsigned int client_flag;	//一般为0
}Database_Param;

class CMysqlStore;
class CConnect;
class CDataBaseHandler
{
public:
	CDataBaseHandler();
	
	virtual ~CDataBaseHandler();
	
	int ConnectDB(Database_Param& stDbParam);

    void DisconnectDB();

	int SQLOpporateDQL(const string& sSQL, const char* pszFmt, ...);

	int SQLOpporateDML(const string& sSQL);

    string What() { return m_pMysqlConn->What(); }

public:
	CConnect*		m_pMysqlConn;
	CMysqlStore*	m_pStore;
};
#endif
