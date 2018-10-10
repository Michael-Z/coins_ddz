#ifndef COST_MANAGER_H_
#define COST_MANAGER_H_

#include <map>
#include <vector>

#include "type_def.h"
#include "singleton.h"
#include "poker_common.pb.h"

typedef std::pair<int/*table_type*/, int/*round*/> TableAndRoundPair;
typedef std::map<TableAndRoundPair, int/*discount*/> Table2DiscountMap;

class CostManager : public CSingleton<CostManager>
{
public:
    CostManager();
    ~CostManager();

public:
    bool Init();
    int GetDiscountVal(int table_type, int round);

private:
    bool _NeedUpdate();
    int64 _GetDiscountTimestamp();
    void _UpdateDiscountTimestamp();
    bool _IsInDiscountTime();
    void _UpdateLastTimestamp();
    int _GetDiscountVal(int table_type, int round);

private:
    int64 last_timestamp_;
    char discount_val_key_[128];
    int64 start_timestamp_;
    int64 end_timestamp_;
    int64 start_min_;
    int64 end_min_;
    std::vector<TableAndRoundPair> updated_vec;
    Table2DiscountMap table_2_discount_map_;
};

#endif // COST_MANAGER_H_
