#include "ConnectHandlerToken.h"
#include "AccessUserManager.h"
#include "HandlerManager.h"

void ConnectHandlerToken::release()
{
	delete this;
}

int ConnectHandlerToken::ProcessOnTimerOut(int Timerid)
{
	VLogMsg(CLIB_LOG_LEV_DEBUG,"token[0x%lx] time out",(long)this);
	_heartbeat_timer.StopTimer();
	AccessUserManager::Instance()->OnHandlerClosed(this);
	if (_phandler != NULL)
	{
		CTCPSocketHandler * phandler = _phandler;
		phandler->Disconnect();
		delete phandler;
		HandlerManager::Instance()->UnBindHandlerToken(phandler);
	}
	return -1;
}