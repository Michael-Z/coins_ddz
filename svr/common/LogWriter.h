#pragma once
#include "TCPSocketHandler.h"
#include "singleton.h"
#include "PBConfigBasic.h"
#include "PBPacket.h"

class CLogWriter
{
public:
	static CLogWriter* Instance(void);
    bool Init();
    bool Reload();
    void Send(const PBCSMsg& msg);
private:
    CLogSocketHandler log_handler;
    PBConfigBasic<PBLogsvrdConfig> log_config;
};

