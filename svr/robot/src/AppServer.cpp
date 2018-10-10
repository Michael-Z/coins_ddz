#include "AppServer.h"
#include "NewReactor.h"
#include "RobotHandlerProxy.h"
#include "SessionManager.h"
#include "RouteManager.h"
#include "ProcesserManager.h"
#include "LogWriter.h"
#include "RobotMgr.h"

template <class T>
bool AppServer<T>::AppInit()
{
    if (!LoadConfig())
        return false;
	if (!RouteManager::Instance()->Init(RobotHandlerProxy::Instance()))
        return false;

    ProcesserManager::Instance()->Init();
	SessionManager::Instance()->Init();
	if (!RobotManager::Instance()->Init())
	{
		return false;
	}
	if (!CLogWriter::Instance()->Init())
        return false;
    return true;
}

template <class T>
bool AppServer<T>::LoadConfig()
{
	if (!PokerPBRobotConfig::Instance()->Init("../conf/robot.CFG"))
	{
		ErrMsg("failed to ini robot.CFG");
		return false;
	}

    return true;
}

template <class T>
bool AppServer<T>::Reload()
{
    return LoadConfig();
}

int main(int argc, char *argv[])
{
    AppServer<CTCPSocketHandler> server(RobotHandlerProxy::Instance());

    if (!CNewReactor::Instance()->Init(argc, argv, &server, EN_Node_Robot))
        return -1;

    CNewReactor::Instance()->RunEventLoop();

	return 0;
}

