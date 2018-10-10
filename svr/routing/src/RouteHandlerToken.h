#pragma once
#include "HandlerTokenBasic.h"
#include <string>
#include "type_def.h"

using namespace std;

class RouteHandlerToken:public CHandlerTokenBasic
{
public:
	RouteHandlerToken(CTCPSocketHandler * phandler)
		:CHandlerTokenBasic(phandler)
	{
		_handler_type = EN_Node_Client;
		_svid = -1;
		_stype = -1;
		_groupid = -1;
	}
	virtual void release();
public:
	int _handler_type;
	int _svid;
	int _stype;
	int _groupid;
};

