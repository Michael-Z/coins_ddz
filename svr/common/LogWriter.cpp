#include "LogWriter.h"
#include "global.h"

CLogWriter* CLogWriter::Instance()
{
	return CSingleton<CLogWriter>::Instance();
}

bool CLogWriter::Init()
{
    // 读取配置
    PBConfig config;
    if (!log_config.Init("../../conf/logsvrd.CFG", config))
    {
        ErrMsg("LogWriter load config failed");
        return false;
    }
    log_config.CopyFrom(config.logsvrd_config());
    // 连接logsvrd
    if (0 != log_handler.Connect(log_config.ip(), log_config.port()))
    {
        ErrMsg("LogWriter connect logsvrd failed");
        return false;
    }
    return true;
}

bool CLogWriter::Reload()
{
    // 断开连接
    //log_handler.DisConnect();

    //return Init();
    return true;
}

void CLogWriter::Send(const PBCSMsg& msg)
{
    // encode packet
	CPBOutputPacket output;
	output.EncodeProtoMsg(msg);
	output.End();

    // send
	log_handler.Send(output.packet_buf(),output.packet_size());
}


