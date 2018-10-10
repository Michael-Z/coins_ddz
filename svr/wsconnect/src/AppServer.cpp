#include "AppServer.h"
#include "NewReactor.h"
#include "ConnectHandlerProxy.h"
#include "ClientHandlerProxy.h"
#include "SessionManager.h"
#include "RouteManager.h"
#include "AccessUserManager.h"
#include "ProcesserManager.h"
#include "LogWriter.h"
#include "WSTCPSocketHandler.h"

template <class T>
bool AppServer<T>::AppInit()
{
    if (!LoadConfig())
        return false;
	if (!RouteManager::Instance()->Init(ConnectHandlerProxy::Instance()))
        return false;

    ProcesserManager::Instance()->Init();
	SessionManager::Instance()->Init();
    AccessUserManager::Instance()->Init();
	if (!CLogWriter::Instance()->Init())
        return false;

	SSL_load_error_strings();    // 错误信息的初始化
	SSL_library_init();    // 初始化SSL算法库函数( 加载要用到的算法 )

	TGlobal::g_ctx = SSL_CTX_new(SSLv23_method());
	if (TGlobal::g_ctx == NULL)
	{
		VBootMsg(CLIB_LOG_LEV_ERROR, "g_ctx == NULL");
		return false;
	}

	if (SSL_CTX_use_certificate_file(TGlobal::g_ctx, "../../conf/cert.pem", SSL_FILETYPE_PEM) != 1)
	{
		VBootMsg(CLIB_LOG_LEV_ERROR, "SSL_CTX_use_certificate_file failed");
		return false;
	}

	if (SSL_CTX_use_RSAPrivateKey_file(TGlobal::g_ctx, "../../conf/private.pem", SSL_FILETYPE_PEM) != 1)
	{
		VBootMsg(CLIB_LOG_LEV_ERROR, "SSL_CTX_use_RSAPrivateKey_file failed");
		return false;
	}

	if (SSL_CTX_check_private_key(TGlobal::g_ctx) != 1)
	{
		VBootMsg(CLIB_LOG_LEV_ERROR, "SSL_CTX_check_private_key failed");
		return false;
	}
    return true;
}

template <class T>
bool AppServer<T>::LoadConfig()
{
    if (!PokerPBConnectSvrdConfig::Instance()->Init("../../conf/connect_svrd_config.CFG"))
    {
        ErrMsg("failed to ini connect_svrd_config.CFG");
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
    AppServer<CWSSTCPSocketHandler> server(ClientHandlerProxy::Instance());

    if (!CNewReactor::Instance()->Init(argc, argv, &server, EN_Node_Connect))
        return -1;

    CNewReactor::Instance()->RunEventLoop();

	return 0;
}

