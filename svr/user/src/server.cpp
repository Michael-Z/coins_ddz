/*#include <stdio.h>
#include <signal.h>
#include "TCPSocketServer.h"
#include "Reactor.h"
#include "SessionManager.h"
#include "RouteManager.h"
#include "LogWriter.h"
#include "UserHandlerProxy.h"
#include "GlobalRedisClient.h"
#include "ProcesserManager.h"

void DaemonStart(void)
{
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGQUIT, &sa, NULL);
	sigaction(SIGHUP, &sa, NULL);
	signal(SIGPIPE,SIG_IGN); 
	signal(SIGCHLD,SIG_IGN); 
	sigset_t sset;
	sigemptyset(&sset);
	sigaddset(&sset, SIGSEGV);
	sigaddset(&sset, SIGBUS);
	sigaddset(&sset, SIGABRT);
	sigaddset(&sset, SIGILL);
	sigaddset(&sset, SIGCHLD);
	sigaddset(&sset, SIGFPE);
	sigprocmask(SIG_UNBLOCK, &sset, &sset);
	daemon (1, 1);
}


bool Global_Init(int argc,char ** argv)
{
	TGlobal::_svrd_type = EN_Node_User; 
	const char* option = "p:h:s:d:g:";
	int result;
	while((result = getopt(argc, argv, option)) != -1)
	{
		if (result=='h'){
			TGlobal::_ip = optarg;
		}
		else if(result=='p'){
			TGlobal::_port = atoi(optarg);
		}
		else if(result=='s'){
			TGlobal::_svid= atoi(optarg);
		}
		else if(result=='d'){
			TGlobal::_background = atoi(optarg) == 1 ? true : false;
		}
		else if(result=='g'){
			TGlobal::_group_id = atoi(optarg);
		}
	}
	if (TGlobal::_background){
		DaemonStart();
	}
	if (!RouteManager::Instance()->Init(UserHandlerProxy::Instance())) return false;
    if (!CLogWriter::Instance()->Init()) return false;
	if (!GlobalRedisClient::Instance()->Init()) return false;
    ProcesserManager::Instance()->Init();
	SessionManager::Instance()->Init();

    if (!PokerPBClubConfig::Instance()->Init("../../conf/club.CFG"))
    {
        VLogMsg(CLIB_LOG_LEV_ERROR,"failed to init club config");
        return false;
    }
    if (!PokerPBItemConfig::Instance()->Init("../../conf/item.CFG"))
    {
        VLogMsg(CLIB_LOG_LEV_ERROR,"failed to init item config");
        return false;
    }

	return true;
}

int main(int argc, char *argv[])
{
	CReactor::Instance()->Init();

	if (!Global_Init(argc,argv))
        return 0;

	CTCPSocketServer<CTCPSocketHandler> _server(UserHandlerProxy::Instance(),TGlobal::_ip.c_str(),TGlobal::_port);

	CReactor::Instance()->RegistServer(&_server);

	CReactor::Instance()->RunEventLoop();

	return 0;
}
*/
