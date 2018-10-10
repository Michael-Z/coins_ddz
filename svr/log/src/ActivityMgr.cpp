#include "ActivityMgr.h"
#include "time.h"
#include "PBConfigBasic.h"

CActivityMgr * CActivityMgr::Instance()
{
	return CSingleton<CActivityMgr>::Instance();
}

bool CActivityMgr::IsInActPeriod(int actid)
{
	const PBActivityItem* pItem = PokerPBActivityConfig::Instance()->GetActivityItem(actid);
	if (pItem == NULL)
	{
		return false;
	}
	int startstamp = GetDateZeroTimeStamp(pItem->startymd());
	int endstamp = GetDateZeroTimeStamp(pItem->endymd()) + 24*60*60;
	time_t now = time(NULL);
	if (now < startstamp || now > endstamp)
	{
		return false;
	}

	struct tm* ptm = localtime(&now);
	int starthms = GetHMS(pItem->starthms());
	int endhms = GetHMS(pItem->endhms());
	int nowhms = ConvertHMSToInt((short)ptm->tm_hour, (char)ptm->tm_min, (char)ptm->tm_sec);
	if (nowhms < starthms || nowhms > endhms)
	{
		return false;
	}
	return true;
}

int CActivityMgr::GetDateZeroTimeStamp(const string& ymd)
{
	int year = 0;
	int mon = 0;
	int day = 0;
	sscanf(ymd.c_str(), "%d-%d-%d", &year, &mon, &day);
	struct tm tmp;
	tmp.tm_year = year - 1900;
	tmp.tm_mon = mon - 1;
	tmp.tm_mday = day;
	tmp.tm_hour = 0;
	tmp.tm_min = 0;
	tmp.tm_sec = 0;
	return (int)mktime(&tmp);
}

int CActivityMgr::GetHMS(const string& hms)
{
	int hour = 0;
	int min = 0;
	int sec = 0;
	sscanf(hms.c_str(), "%d:%d:%d", &hour, &min, &sec);
	return ConvertHMSToInt((short)hour, (char)min, (char)sec);
}


int CActivityMgr::ConvertHMSToInt(short hour, char min, char sec)
{
	return ((hour & 0xffff) << 16) + ((min & 0xff) << 8) + sec;
}

bool CActivityMgr::IsGameTypeInStatistics(int game_type)
{
	for (int i = 0; i < PokerPBActivityConfig::Instance()->game_type_size(); i++)
	{
		if (PokerPBActivityConfig::Instance()->game_type(i) == game_type)
		{
			return true;
		}
	}
	return false;
}

bool CActivityMgr::IsInWhiteList(int uid)
{
	if (!PokerPBActivityConfig::Instance()->if_use_white_list())
	{
		return true;
	}
	for (int i = 0; i < PokerPBActivityConfig::Instance()->white_uid_list_size(); i++)
	{
		if (PokerPBActivityConfig::Instance()->white_uid_list(i) == uid)
		{
			return true;
		}
	}
	return false;
}