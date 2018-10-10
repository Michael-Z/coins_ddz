#include "HandlerManager.h"

void HandlerManager::BindHandlerToken(CTCPSocketHandler * phandler,CHandlerTokenBasic * ptoken)
{
	_handler_token_map[phandler] = ptoken;
	ptoken->_phandler = phandler;
}

void HandlerManager::UnBindHandlerToken(CTCPSocketHandler * phandler)
{
	HandlerTokenMap::iterator iter = _handler_token_map.find(phandler);
	if(iter != _handler_token_map.end())
	{
		CHandlerTokenBasic * ptoken = iter->second;
		if(ptoken!=NULL)
		{
			ptoken->release();
		}
		_handler_token_map.erase(iter);
	}
}

CHandlerTokenBasic * HandlerManager::GetHandlerToken(CTCPSocketHandler * phandler)
{
	HandlerTokenMap::iterator iter = _handler_token_map.find(phandler);
	if(iter != _handler_token_map.end())
	{
		return iter->second;
	}
	return NULL;
}


