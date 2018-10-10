#ifndef _ActivityMgr_h
#define _ActivityMgr_h
#include "ActivityRedisClient.h"

class CActivityMgr
{
public:
	static CActivityMgr * Instance();
	bool IsInActPeriod(int actid);
	bool IsGameTypeInStatistics(int game_type);
	bool IsInWhiteList(int uid);
	int ConvertHMSToInt(short hour, char min, char sec);
private:
	int GetDateZeroTimeStamp(const string& ymd);
	int GetHMS(const string& hms);
	
};
#endif