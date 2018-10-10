#include "HandlerTokenBasic.h"

CHandlerTokenBasic::CHandlerTokenBasic(CTCPSocketHandler * phandler)
{
	_phandler = phandler;
}

void CHandlerTokenBasic::release()
{
	delete this;
}
