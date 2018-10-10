#pragma once
#include "poker_msg.pb.h"
#include <string>
#include <ostream>
#include <map>
#include <vector>
#include <algorithm>
#include "TableModle.h"
using google::protobuf::RepeatedField;
using namespace std;

class PokerLogic
{
public:
    static bool AnalyseFromCards(const CPBGameTable & table, int seat_index, RepeatedField<int> & cards,
                                 ENPokerType dest_poker_type, int start_val, int len);
    static int GetCardLogicVal(const PBSDRGameTable & table, int card);
protected:
    static bool PrioCompare(ENPokerType old_form, ENPokerType new_form, int old_val, int new_val,
                                 int old_size, int new_size);

    static bool CheckSingle(const CPBGameTable & table, RepeatedField<int> & cards, ENPokerType dest_poker_type, int start_val, int len);
    static bool CheckPair(const CPBGameTable & table, RepeatedField<int> & cards, ENPokerType dest_poker_type, int start_val, int len);
    static bool CheckTriple(const CPBGameTable & table, RepeatedField<int> & cards, ENPokerType dest_poker_type, int start_val, int len);
    static bool CheckTripleWithOne(const CPBGameTable & table, RepeatedField<int> & cards, ENPokerType dest_poker_type, int start_val, int len);
    static bool CheckTripleWithTwo(const CPBGameTable & table, RepeatedField<int> & cards, ENPokerType dest_poker_type, int start_val, int len);
    static bool CheckQuadrupleWithTwo(const CPBGameTable & table, RepeatedField<int> & cards, ENPokerType dest_poker_type, int start_val, int len);
    static bool CheckQuadrupleWithTwoPAIR(const CPBGameTable & table, RepeatedField<int> & cards, ENPokerType dest_poker_type, int start_val, int len);
    static bool CheckBomb(const CPBGameTable & table, RepeatedField<int> & cards, ENPokerType dest_poker_type, int start_val, int len);
    static bool CheckBombOfJoker(const CPBGameTable & table, RepeatedField<int> & cards, ENPokerType dest_poker_type, int start_val, int len);
    static bool CheckSoftBomb(const CPBGameTable & table, RepeatedField<int> & cards, ENPokerType dest_poker_type, int start_val, int len);
    static bool CheckSoftBombOfJoker(const CPBGameTable & table, RepeatedField<int> & cards, ENPokerType dest_poker_type, int start_val, int len);
    static bool CheckStraight(const CPBGameTable & table, RepeatedField<int> & cards, ENPokerType dest_poker_type, int start_val, int len);
    static bool CheckStraight_2(const CPBGameTable & table, RepeatedField<int> & cards, ENPokerType dest_poker_type, int start_val, int len);
    static bool CheckStraight_3_1(const CPBGameTable & table, RepeatedField<int> & cards, ENPokerType dest_poker_type, int start_val, int len);
    static bool CheckStraight_3(const CPBGameTable & table, RepeatedField<int> & cards, ENPokerType dest_poker_type, int start_val, int len);
    static bool CheckStraight_3_2(const CPBGameTable & table, RepeatedField<int> & cards, ENPokerType dest_poker_type, int start_val, int len);

    static int FindCardNumByVal(const CPBGameTable & table, RepeatedField<int> & cards, int val);
    static int FindLaiZiNum(const CPBGameTable & table, RepeatedField<int> & cards);
    static int FindJokerNum(const CPBGameTable & table, RepeatedField<int> & cards);
    static int Find2CardNum(const CPBGameTable & table, RepeatedField<int> & cards);

    static bool IsEqual(int val, int card);

    struct CompareObj : public std::binary_function<int const &, int const &, bool>
    {
    public:
        CompareObj(const CPBGameTable& table) : rtable(table)
        {

        }
        bool operator()(int card1, int card2)
        {
            if (TableLogic::IsLaiZi(rtable, card1))
            {
                return false;
            }
            if (TableLogic::IsLaiZi(rtable, card2))
            {
                return true;
            }

            return ((card1&0x0f) < (card2&0x0f));
        }
    public:
        const CPBGameTable& rtable;
    };
};

