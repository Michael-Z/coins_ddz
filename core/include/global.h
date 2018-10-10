#pragma once

#include <malloc.h>
#include <string>
#include <map>
#include <vector>
#include <string>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include "clib_log.h"
#include "poker_msg.pb.h"
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>

using namespace std;

#define LogMsg(format,...) TGlobal::g_pDebugLog->logMsg("[%s %u][%s]"format,__FILE__,__LINE__,__FUNCTION__,##__VA_ARGS__)
#define ErrMsg(format,...) TGlobal::g_pErrorLog->logMsg("[%s %u][%s]"format,__FILE__,__LINE__,__FUNCTION__,##__VA_ARGS__)
#define VBootMsg(level,format,...) TGlobal::g_pBootLog->vwrite(level,"[%s %u][%s]"format,__FILE__,__LINE__,__FUNCTION__,##__VA_ARGS__)
#define BootMsg(format,...) TGlobal::g_pBootLog->logMsg("[%s %u][%s]"format,__FILE__,__LINE__,__FUNCTION__,##__VA_ARGS__)
#define VLogMsg(level,format,...) TGlobal::g_pDebugLog->vwrite(level,"[%s %u][%s]"format,__FILE__,__LINE__,__FUNCTION__,##__VA_ARGS__)

#define ProtoMsg(format,...) TGlobal::g_pProtoLog->logMsg(format, ##__VA_ARGS__)
#define StatsMsg(format,...) TGlobal::g_pStatsLog->logMsg(format, ##__VA_ARGS__)

class TGlobal
{
public:
	static clib_log* 		g_pErrorLog;
	static clib_log* 		g_pDebugLog;
	static clib_log*		g_pBootLog;
    static clib_log*        g_pProtoLog;
    static clib_log*        g_pStatsLog;
public:
	static SSL_CTX * g_ctx;
public:
	static string _ip;
	static int _port;
	static int _svid;
	static ENNodeType _svrd_type;
	static bool _background;
	static int _group_id;
	static int _zone_id;
	static int _argc;
	static char**  _pargv;
private: //no impl
    TGlobal (void);
    ~TGlobal (void);
    TGlobal (const TGlobal&);
    const TGlobal& operator= (const TGlobal&);
};

