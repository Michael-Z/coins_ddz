#pragma once
#include "singleton.h"
#include <map>
#include <string>
#include "Timer_Handler_Base.h"
#include "ConnectHandlerToken.h"
#include "Session.h"

using namespace std;

#define  LOG_TIMER 1

class AccessUserManager: public CTimerOutListener
{
typedef map<string,ConnectHandlerToken *> AccountMap;			
typedef map<long long,ConnectHandlerToken *> TokenMap;	
typedef map<int, int> OnlineMap; 

public:
	static AccessUserManager * Instance(void);
    void Init();
	ConnectHandlerToken * GetHandlerTokenByAccount(const string & account);
	ConnectHandlerToken * GetHandlerTokenByUid(long long uid);
	void BindAccountHandlerToken(const string & account,ConnectHandlerToken * ptoken);
	void UnBindAccountHandler(const string & account);
	void BindUserHandlerToken(long long uid, int acc_type, int channel, ConnectHandlerToken * ptoken);
	void UnBindUserHandlerToken(const ConnectHandlerToken* token);
	void OnHandlerClosed(ConnectHandlerToken * ptoken);
	void OnHandlerRepeatedLogin(ConnectHandlerToken * ptoken, CSession * psession);
	void OnHandlerRepeatedLogin(int64 uid);
	void BroadCastMessage(CWSPBOutputPacket & output);
public:
	AccountMap _accountmap;
	TokenMap _tokenmap;
    OnlineMap _online_map;
private:
   	virtual int ProcessOnTimerOut(int Timerid);
	CTimer timer;
};

