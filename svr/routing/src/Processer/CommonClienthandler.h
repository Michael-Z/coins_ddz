#pragma once
#include "NewProcessor.h"

class CRegistInnerServer : public NewProcessor
{
public:
	virtual ENHandlerResult ProcessRequestMsg(CHandlerTokenBasic * ptoken,CSession * psession);
};

