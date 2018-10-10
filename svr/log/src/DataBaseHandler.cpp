#include "DataBaseHandler.h"
#include <cstdarg>

CDataBaseHandler::CDataBaseHandler()
{
	m_pMysqlConn = NULL;
	m_pStore = NULL;
}

CDataBaseHandler::~CDataBaseHandler()
{
	if (m_pMysqlConn)
	{
		delete m_pMysqlConn;
		m_pMysqlConn = NULL;
	}
	
	if (m_pStore)
	{
		delete m_pStore;
		m_pStore = NULL;
	}
}

int CDataBaseHandler::ConnectDB(Database_Param& stDbParam)
{
	try
	{
		m_pMysqlConn = new CMysqlConnect();
		if(!m_pMysqlConn->Connect(stDbParam.host, stDbParam.user, stDbParam.password, stDbParam.db, stDbParam.port, stDbParam.unix_socket, "utf8"))
		{
            return -1;
		}
		m_pStore = new CMysqlStore();
		m_pStore->SetTransAction(m_pMysqlConn);
		return 0;
	}
	catch (...)
	{
        return -1;
	}

	return -1;
}

void CDataBaseHandler::DisconnectDB()
{
    if (m_pMysqlConn)
        m_pMysqlConn->Disconnect();
}

int CDataBaseHandler::SQLOpporateDQL(const string & sSQL, const char * pszFmt,...)
{
	if (NULL == m_pMysqlConn->GetConnect())
	{
        return -1;
	}

	if (m_pStore->Query(sSQL))
	{
		unsigned long count = m_pStore->RowCount();
		if(count <= 0)
		{
            return -1;
		}

		if (NULL == pszFmt)
		{
			return 0;
		}

		va_list ap; 
		va_start (ap, pszFmt); 
		const char* p = NULL;
		int order = 0;
		for (p= pszFmt; *p; p++) 
		{ 
			if (*p != '%') 
			{
				continue; 
			}

			switch (*++p) 
			{ 
				case 'd':	//int
				{
					int* nVal= va_arg(ap, int*);
					(*nVal) = m_pStore->GetItemLong(0, order++);
					break;
				}

				case 's':	//char*
				{
					char* pVal = va_arg(ap, char*);
					strcpy(pVal, m_pStore->GetItemString(0, order++).c_str());
					break;
				}
				default:
				{
					break;
				}
			}
		}

		return 0;
	}

	return -1;
}

int CDataBaseHandler::SQLOpporateDML(const string & sSQL)
{
	if (NULL == m_pMysqlConn->GetConnect())
	{
        return -1;
	}

	if(m_pStore->Exec(sSQL))
	{
		return 0;
	}

	return -1;
}
