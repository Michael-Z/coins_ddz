#ifndef QUICKMATCH_MANAGER_H_
#define QUICKMATCH_MANAGER_H_

#include <map>
#include <vector>

#include "type_def.h"
#include "singleton.h"
#include "poker_common.pb.h"
#include "json/json.h"
//#include "JsonHelper.h"

typedef std::pair<int/*table_type*/, int/*round*/> TableAndRoundPair;
typedef std::map<TableAndRoundPair, int/*discount*/> Table2DiscountMap;

class QuickMatchManager : public CSingleton<QuickMatchManager>
{
public:
    QuickMatchManager();
    ~QuickMatchManager();

public:
    bool Init();
public:
    int64 _GetQuickMatchTimeByTtype(int ttype,int64& starttimestamp, int64& stoptimestamp,int64& starttimestamp_day, int64& stoptimestamp_day, int& match_session);
    bool _CheckIsInMatchTimeForTtype(int ttype, int user_match_session);
    bool _CheckIsAfterMatchTimeForTtype(int ttype, int user_match_session);
    int _GetQuickMatchCostByTtype(int ttype,int& cost);
    int _GetQuickMatchTtype(int ttype);
    void _WriteUserMatchInfo(int match_session_id,long long uid, int user_state);
    int _GetUserMatchInfo(int match_session_id, long long uid);
    int _GetUserMatchState(long long uid);
    void _WriteUserMatchState(long long uid, int user_state);
};

#endif
