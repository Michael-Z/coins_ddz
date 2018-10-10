#pragma once
#include "singleton.h"
#include "type_def.h"
#include "Timer_Handler_Base.h"
#include <map>
#include <vector>

using namespace std;

#define _RECYCLE_TIMER 1000

class ReplayCodeManager : public CTimerOutListener
{
public:
    static ReplayCodeManager * Instance();
	bool Init();
    int64 AllocNewCode(const string& mainkey, const string& subkey);
    bool CheckAllocedCode(int64 code);
    bool GetReplayForCode(int64 code, string& mainkey, string& subkey);
    bool ReleaseCode(int64 code);
	virtual int ProcessOnTimerOut(int Timerid);
protected:
    bool BindCodeForReplay(int64 code, const string& mainkey, const string& subkey);
    bool UnBindCodeForReplay(int64 code);
protected:
	vector<int64> _alloced;
    map<int64, int> _alloced_map;    // <code, timestamp>
	vector<int64> _free;
    CTimer _recycle_timer;
    map<string, int> _key_map;    // <subkey, code>
};
