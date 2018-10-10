#include "PokerLogic.h"


bool PokerLogic::AnalyseFromCards(const CPBGameTable & table, int seat_index, RepeatedField<int> & cards,
                                  ENPokerType dest_poker_type, int start_val, int len)
{
    PokerLogic::CompareObj cmp(table);
    std::sort(cards.begin(), cards.end(), cmp);
    // 打印 cards
    {
        PBSDRAction action;
        action.mutable_col_info()->mutable_cards()->CopyFrom(cards);
        std::string strProto;
        google::protobuf::TextFormat::PrintToString(action, &strProto);
        VLogMsg(CLIB_LOG_LEV_DEBUG, "\n%s", strProto.c_str());
    }

    ENPokerType last_poker_type = EN_POKER_TYPE_UNKONWN;
    int last_poker_val = 0;
    int last_size = 0;

    const PBSDRAction* p_action = TableLogic::GetLastDeterminedActionChuPai(table);
    if (p_action != NULL && p_action->seat_index() != seat_index)
    {
       // 出牌必须要比上一次出牌的牌型或值要大才行
        last_poker_type = p_action->poker_type();
        last_poker_val = p_action->poker_val();
        last_size = p_action->col_info().cards_size();
    }

    if (cards.size() > 0)
    {
        VLogMsg(CLIB_LOG_LEV_DEBUG, "last_poker_type[%d] last_poker_val[%d] last_size[%d]", last_poker_type, last_poker_val, last_size);
        switch (dest_poker_type)
        {
            case EN_POKER_TYPE_SINGLE_CARD:
            {
                if (CheckSingle(table, cards, dest_poker_type, start_val, len))
                {
                    if (PrioCompare(last_poker_type, EN_POKER_TYPE_SINGLE_CARD,
                                    last_poker_val, start_val,
                                    last_size, cards.size()))
                    {
                        return true;
                    }
                }
                break;
            }
            case EN_POKER_TYPE_PAIR:
            {
                if (CheckPair(table, cards, dest_poker_type, start_val, len))
                {
                    if (PrioCompare(last_poker_type, EN_POKER_TYPE_PAIR,
                                    last_poker_val, start_val,
                                    last_size, cards.size()))
                    {
                        return true;
                    }
                }
                break;
            }
            case EN_POKER_TYPE_TRIPLE:
            {
                if (CheckTriple(table, cards, dest_poker_type, start_val, len))
                {
                    if (PrioCompare(last_poker_type, EN_POKER_TYPE_TRIPLE,
                                    last_poker_val, start_val,
                                    last_size, cards.size()))
                    {
                        return true;
                    }
                }
                break;
            }
            case EN_POKER_TYPE_TRIPLE_WITH_ONE:
            {
                if (CheckTripleWithOne(table, cards, dest_poker_type, start_val, len))
                {
                    if (PrioCompare(last_poker_type, EN_POKER_TYPE_TRIPLE_WITH_ONE,
                                    last_poker_val, start_val,
                                    last_size, cards.size()))
                    {
                        return true;
                    }
                }
                break;
            }
            case EN_POKER_TYPE_TRIPLE_WITH_TWO:
            {
                if (CheckTripleWithTwo(table, cards, dest_poker_type, start_val, len))
                {
                    if (PrioCompare(last_poker_type, EN_POKER_TYPE_TRIPLE_WITH_TWO,
                                    last_poker_val, start_val,
                                    last_size, cards.size()))
                    {
                        return true;
                    }
                }
                break;
            }
            case EN_POKER_TYPE_STRAIGHT:
            {
                if (CheckStraight(table, cards, dest_poker_type, start_val, len))
                {
                    if (PrioCompare(last_poker_type, EN_POKER_TYPE_STRAIGHT,
                                    last_poker_val, start_val,
                                    last_size, cards.size()))
                    {
                        return true;
                    }
                }
                break;
            }
            case EN_POKER_TYPE_STRAIGHT_2:
            {
                if (CheckStraight_2(table, cards, dest_poker_type, start_val, len))
                {
                    if (PrioCompare(last_poker_type, EN_POKER_TYPE_STRAIGHT_2,
                                    last_poker_val, start_val,
                                    last_size, cards.size()))
                    {
                        return true;
                    }
                }
                break;
            }
            case EN_POKER_TYPE_STRAIGHT_3:
            {
                if (CheckStraight_3(table, cards, dest_poker_type, start_val, len))
                {
                    if (PrioCompare(last_poker_type, EN_POKER_TYPE_STRAIGHT_3,
                                    last_poker_val, start_val,
                                    last_size, cards.size()))
                    {
                        return true;
                    }
                }
                break;
            }
            case EN_POKER_TYPE_STRAIGHT_3_1:
            {
                if (CheckStraight_3_1(table, cards, dest_poker_type, start_val, len))
                {
                    if (PrioCompare(last_poker_type, EN_POKER_TYPE_STRAIGHT_3_1,
                                    last_poker_val, start_val,
                                    last_size, cards.size()))
                    {
                        return true;
                    }
                }
            }
            case EN_POKER_TYPE_STRAIGHT_3_2:
            {
                if (CheckStraight_3_2(table, cards, dest_poker_type, start_val, len))
                {
                    if (PrioCompare(last_poker_type, EN_POKER_TYPE_STRAIGHT_3_2,
                                    last_poker_val, start_val,
                                    last_size, cards.size()))
                    {
                        return true;
                    }
                }
                break;
            }
            case EN_POKER_TYPE_QUADRUPLE_WITH_TWO:
            {
                if (CheckQuadrupleWithTwo(table, cards, dest_poker_type, start_val, len))
                {
                    if (PrioCompare(last_poker_type, EN_POKER_TYPE_QUADRUPLE_WITH_TWO,
                                    last_poker_val, start_val,
                                    last_size, cards.size()))
                    {
                        return true;
                    }
                }
                break;
            }
            case EN_POKER_TYPE_QUADRUPLE_WITH_TWO_PAIR:
            {
                if (CheckQuadrupleWithTwoPAIR(table, cards, dest_poker_type, start_val, len))
                {
                    if (PrioCompare(last_poker_type, EN_POKER_TYPE_QUADRUPLE_WITH_TWO_PAIR,
                                    last_poker_val, start_val,
                                    last_size, cards.size()))
                    {
                        return true;
                    }
                }
                break;
            }
            case EN_POKER_TYPE_BOMB:
            {
                if (CheckBomb(table, cards, dest_poker_type, start_val, len))
                {
                    if (PrioCompare(last_poker_type, EN_POKER_TYPE_BOMB,
                                    last_poker_val, start_val,
                                    last_size, cards.size()))
                    {
                        return true;
                    }
                }
                break;
            }
            case EN_POKER_TYPE_BOMB_OF_JOKER:
            {
                if (CheckBombOfJoker(table, cards, dest_poker_type, start_val, len))
                {
                    // 王炸最大
                    if (true)
                    {
                        return true;
                    }
                }
                return false;
            }
            default:
                break;
        }
    }
    else
    {
        ErrMsg("cards size 为 0");
    }

    return false;
}

bool PokerLogic::PrioCompare(ENPokerType old_form, ENPokerType new_form, int old_val, int new_val,
                             int old_size, int new_size)
{
    if (new_form > old_form && old_form == EN_POKER_TYPE_UNKONWN)
    {
        return true;
    }
    if (new_form > old_form && new_form >= EN_POKER_TYPE_SOFT_BOMB)
    {
        return true;
    }
    else if (new_form == old_form && new_size == old_size && new_val > old_val)
    {
        return true;
    }
    return false;
}

bool PokerLogic::IsEqual(int val, int card)
{
    return ((((card & 0xf0) >> 4) <= 4) && val == (card&0x0f));
}

int PokerLogic::FindCardNumByVal(const CPBGameTable & table, RepeatedField<int> & cards, int val)
{
    int cnt = 0;
    // 王牌
    if (val == 16)
    {
        cnt = std::count(cards.begin(), cards.end(), 0x51);
    }
    else if (val == 17)
    {
        cnt = std::count(cards.begin(), cards.end(), 0x52);
    }
    // 2点
    else if (val == 15)
    {
        cnt = std::count_if(cards.begin(), cards.end(), std::bind1st(ptr_fun(PokerLogic::IsEqual), 2));
    }
    else
    {
        cnt = std::count_if(cards.begin(), cards.end(), std::bind1st(ptr_fun(PokerLogic::IsEqual), val));
    }
    VLogMsg(CLIB_LOG_LEV_DEBUG, "val[%d] cnt[%d]", val, cnt);

    return cnt;
}

int PokerLogic::FindLaiZiNum(const CPBGameTable & table, RepeatedField<int> & cards)
{
    int cnt = std::count_if(cards.begin(), cards.end(), std::bind1st(ptr_fun(TableLogic::IsLaiZi), table));
    VLogMsg(CLIB_LOG_LEV_DEBUG, "laizi cnt[%d]", cnt);
    return cnt;
}
int PokerLogic::FindJokerNum(const CPBGameTable & table, RepeatedField<int> & cards)
{
    int cnt = std::count(cards.begin(), cards.end(), 0x51);
    cnt += std::count(cards.begin(), cards.end(), 0x52);
    return cnt;
}

int PokerLogic::Find2CardNum(const CPBGameTable & table, RepeatedField<int> & cards)
{
    int cnt = FindCardNumByVal(table, cards, 15);
    return cnt;
}

int PokerLogic::GetCardLogicVal(const PBSDRGameTable & table, int card)
{
    int poker_val = 0;
    // 王牌
    if (TableLogic::GetCardType(card) == 5)
    {
        poker_val = card == 0x51 ? 16 : 17;
    }
    // 2点
    else if (TableLogic::GetCardValue(card) == 2)
    {
        poker_val = 15;
    }
    // 癞子 算3点
    else if (TableLogic::IsLaiZi(table, card))
    {
        poker_val = 3;
    }
    else
    {
        poker_val = TableLogic::GetCardValue(card);
    }

    return poker_val;
}

// ============================================================================================================
bool PokerLogic::CheckSingle(const CPBGameTable & table, RepeatedField<int> & cards, ENPokerType dest_poker_type, int start_val, int len)
{
    // 检查size
    if (cards.size() != 1)
    {
        return false;
    }

    // 检测 poker_type
    if (dest_poker_type != EN_POKER_TYPE_SINGLE_CARD)
    {
        return false;
    }

    // 赖子若是当单牌出只能作为本身的值出
    if (TableLogic::GetLow(cards.Get(0)) == table.laizi_card()
            && start_val != table.laizi_card())
    {
        return false;
    }
    // 检测 start_val
    int lai_zi_num = FindLaiZiNum(table, cards);
    int card_num = FindCardNumByVal(table, cards, start_val);
    if (card_num+lai_zi_num < 1)
    {
        return false;
    }

    return true;
}

bool PokerLogic::CheckPair(const CPBGameTable & table, RepeatedField<int> & cards, ENPokerType dest_poker_type, int start_val, int len)
{
    // 检查size
    if (cards.size() != 2)
    {
        return false;
    }
    // 不能有王牌
    if (FindJokerNum(table, cards) > 0)
    {
        return false;
    }

    // 检测 poker_type
    if (dest_poker_type != EN_POKER_TYPE_PAIR)
    {
        return false;
    }

    // 若是两张赖子牌当一对儿出，只能是作为赖子牌本身的值出
    if (TableLogic::GetLow(cards.Get(0)) == table.laizi_card()
            && TableLogic::GetLow(cards.Get(1)) == table.laizi_card()
            && start_val != table.laizi_card())
    {
        return false;
    }

    // 检测 start_val
    int lai_zi_num = FindLaiZiNum(table, cards);
    int card_num = FindCardNumByVal(table, cards, start_val);
    if (card_num+lai_zi_num < 2)
    {
        return false;
    }

    return true;
}

bool PokerLogic::CheckTriple(const CPBGameTable & table, RepeatedField<int> & cards, ENPokerType dest_poker_type, int start_val, int len)
{
    // 检查size
    if (cards.size() != 3)
    {
        return false;
    }
    // 不能有王牌
    if (FindJokerNum(table, cards) > 0)
    {
        return false;
    }

    // 检测 poker_type
    if (dest_poker_type != EN_POKER_TYPE_TRIPLE)
    {
        return false;
    }

    // 若是三张赖子牌当三带零出，只能是作为赖子牌本身的值出
    if (TableLogic::GetLow(cards.Get(0)) == table.laizi_card()
            && TableLogic::GetLow(cards.Get(1)) == table.laizi_card()
            && TableLogic::GetLow(cards.Get(2)) == table.laizi_card()
            && start_val != table.laizi_card())
    {
        return false;
    }

    // 检测 start_val
    int lai_zi_num = FindLaiZiNum(table, cards);
    int card_num = FindCardNumByVal(table, cards, start_val);
    if (card_num+lai_zi_num < 3)
    {
        return false;
    }

    return true;
}

bool PokerLogic::CheckTripleWithOne(const CPBGameTable & table, RepeatedField<int> & cards, ENPokerType dest_poker_type, int start_val, int len)
{
    // 检查size
    if (cards.size() != 4)
    {
        return false;
    }
    // 只能有最多1张王牌
    if (FindJokerNum(table, cards) > 1)
    {
        return false;
    }

    // 检测 poker_type
    if (dest_poker_type != EN_POKER_TYPE_TRIPLE_WITH_ONE)
    {
        return false;
    }

    // 检测 start_val
    int lai_zi_num = FindLaiZiNum(table, cards);
    int card_num = FindCardNumByVal(table, cards, start_val);
    if (card_num+lai_zi_num < 3)
    {
        return false;
    }

    return true;
}

bool PokerLogic::CheckTripleWithTwo(const CPBGameTable & table, RepeatedField<int> & cards, ENPokerType dest_poker_type, int start_val, int len)
{
    // 检查size
    if (cards.size() != 5)
    {
        return false;
    }
    // 只能有最多1张王牌
    if (FindJokerNum(table, cards) > 1)
    {
        return false;
    }

    // 检测 poker_type
    if (dest_poker_type != EN_POKER_TYPE_TRIPLE_WITH_TWO)
    {
        return false;
    }

    // 检测带的必须是一对
    if (TableLogic::GetLow(cards.Get(2)) == TableLogic::GetLow(cards.Get(0)))
    {
        if (TableLogic::GetLow(cards.Get(3)) != TableLogic::GetLow(cards.Get(4)))
        {
            return false;
        }
    }
    else if (TableLogic::GetLow(cards.Get(2)) == TableLogic::GetLow(cards.Get(4)))
    {
        if (TableLogic::GetLow(cards.Get(0)) != TableLogic::GetLow(cards.Get(1)))
        {
            return false;
        }
    }

    // 检测 start_val
    int lai_zi_num = FindLaiZiNum(table, cards);
    int card_num = FindCardNumByVal(table, cards, start_val);
    if (card_num+lai_zi_num < 3)
    {
        return false;
    }

    return true;
}

bool PokerLogic::CheckQuadrupleWithTwoPAIR(const CPBGameTable & table, RepeatedField<int> & cards, ENPokerType dest_poker_type, int start_val, int len)
{
    // 检查size
    if (cards.size() != 8)
    {
        return false;
    }

    // 检测 poker_type
    if (dest_poker_type != EN_POKER_TYPE_QUADRUPLE_WITH_TWO_PAIR)
    {
        return false;
    }

    // 检测 start_val
    int lai_zi_num = FindLaiZiNum(table, cards);
    int card_num = FindCardNumByVal(table, cards, start_val);
    if (card_num+lai_zi_num < 4)
    {
        return false;
    }

    int iRemianingLaiZiNum = card_num >= 4 ? lai_zi_num : lai_zi_num - ( 4 - card_num ) ;
    // 检查2个对子
    vector<int> vtDuiVect;
    for(int i = 0 ; i < cards.size() ; i ++)
    {
        int iCard = cards.Get(i);

        //相等值的index
        int Equipindex = -1;
        for (unsigned int j = 0; j < vtDuiVect.size(); j++)
        {
            int iVerifyCard = vtDuiVect.at(j);
            if (TableLogic::GetCardValue(iCard) == TableLogic::GetCardValue(iVerifyCard))
            {
                Equipindex = j;
                break;
            }
        }

        if (Equipindex != -1)
        {
            vector<int>::iterator it = vtDuiVect.begin() + Equipindex;
            vtDuiVect.erase(it);
        }
        else
        {
            vtDuiVect.push_back(iCard);
        }
    }

    if(vtDuiVect.size() - iRemianingLaiZiNum > 0)
    {
        return false;
    }

    return true;
}

bool PokerLogic::CheckQuadrupleWithTwo(const CPBGameTable & table, RepeatedField<int> & cards, ENPokerType dest_poker_type, int start_val, int len)
{
    // 检查size
    if (cards.size() != 6)
    {
        return false;
    }

    // 检测 poker_type
    if (dest_poker_type != EN_POKER_TYPE_QUADRUPLE_WITH_TWO)
    {
        return false;
    }

    // 检测 start_val
    int lai_zi_num = FindLaiZiNum(table, cards);
    int card_num = FindCardNumByVal(table, cards, start_val);
    if (card_num+lai_zi_num < 4)
    {
        return false;
    }

    return true;
}

bool PokerLogic::CheckBomb(const CPBGameTable & table, RepeatedField<int> & cards, ENPokerType dest_poker_type, int start_val, int len)
{
    // 检查size
    if (cards.size() != 4)
    {
        return false;
    }
    // 不能有王牌
    if (FindJokerNum(table, cards) > 0)
    {
        return false;
    }

    // 检测 poker_type
    if (dest_poker_type != EN_POKER_TYPE_BOMB)
    {
        return false;
    }

    // 检测 start_val
    int lai_zi_num = FindLaiZiNum(table, cards);
    int card_num = FindCardNumByVal(table, cards, start_val);
    if (card_num+lai_zi_num < 4)
    {
        return false;
    }

    return true;
}

bool PokerLogic::CheckSoftBomb(const CPBGameTable & table, RepeatedField<int> & cards, ENPokerType dest_poker_type, int start_val, int len)
{
    // 检查size
    if (cards.size() != 4)
    {
        return false;
    }
    // 不能有王牌
    if (FindJokerNum(table, cards) > 0)
    {
        return false;
    }
    // 必须有赖子
    int lai_zi_num = FindLaiZiNum(table, cards);
    if (lai_zi_num < 1)
    {
        return false;
    }

    // 检测 poker_type
    if (dest_poker_type != EN_POKER_TYPE_SOFT_BOMB)
    {
        return false;
    }

    // 检测 start_val
    int card_num = FindCardNumByVal(table, cards, start_val);
    if (card_num+lai_zi_num < 4)
    {
        return false;
    }

    return true;
}


bool PokerLogic::CheckBombOfJoker(const CPBGameTable & table, RepeatedField<int> & cards, ENPokerType dest_poker_type, int start_val, int len)
{
    // 检查size
    if (cards.size() != 2)
    {
        return false;
    }
    // 不能有王牌
    if (FindJokerNum(table, cards) != 2)
    {
        return false;
    }

    // 检测 poker_type
    if (dest_poker_type != EN_POKER_TYPE_BOMB_OF_JOKER)
    {
        return false;
    }

    // 检测 start_val
    if (start_val != GetCardLogicVal(table, 0x51))
    {
        return false;
    }

    if (cards.Get(0) != 0x51 || cards.Get(1) != 0x52)
    {
        return false;
    }

    return true;
}

bool PokerLogic::CheckSoftBombOfJoker(const CPBGameTable & table, RepeatedField<int> & cards, ENPokerType dest_poker_type, int start_val, int len)
{
    // 检查size
    if (cards.size() != 2)
    {
        return false;
    }
    // 最多一张王牌
    if (FindJokerNum(table, cards) != 1)
    {
        return false;
    }
    // 必须有赖子
    int lai_zi_num = FindLaiZiNum(table, cards);
    if (lai_zi_num < 1)
    {
        return false;
    }

    // 检测 poker_type
    if (dest_poker_type != EN_POKER_TYPE_SOFT_BOMB_OF_JOKER)
    {
        return false;
    }

    // 检测 start_val
    if (start_val != 16 && start_val != 17)
    {
        return false;
    }

    return true;
}

bool PokerLogic::CheckStraight(const CPBGameTable & table, RepeatedField<int> & cards, ENPokerType dest_poker_type, int start_val, int len)
{
    // 检查size
    if (cards.size() < 5)
    {
        return false;
    }
    // 不能有王牌
    if (FindJokerNum(table, cards) > 0)
    {
        return false;
    }
    // 不能有2
    if (Find2CardNum(table, cards) > 0)
    {
        return false;
    }

    // 检测 poker_type
    if (dest_poker_type != EN_POKER_TYPE_STRAIGHT)
    {
        return false;
    }
    // 检测 len
    if (len != cards.size())
    {
         return false;
    }
    // 检测 start_val
    int lai_zi_num = FindLaiZiNum(table, cards);
    for (int i = 0; i < len; i++)
    {
        int card_num = FindCardNumByVal(table, cards, start_val+i);
        if (card_num > 1 && TableLogic::GetCardValue(table.laizi_card()) == (start_val+i))
        {
//            i += (card_num - 1);
            lai_zi_num--;
        }
        else if (card_num > 1 && TableLogic::GetCardValue(table.laizi_card()) != (start_val+i))
        {
            return false;
        }

        if (card_num == 0)
        {
            int need_laizi_num = 1 - card_num;
            if (lai_zi_num >= need_laizi_num)
            {
                lai_zi_num -= need_laizi_num;
            }
            else
            {
                return false;
            }
        }
    }

    return true;
}

bool PokerLogic::CheckStraight_2(const CPBGameTable & table, RepeatedField<int> & cards, ENPokerType dest_poker_type, int start_val, int len)
{
    // 检查size
    if (cards.size() < 6 || cards.size()%2 != 0)
    {
        return false;
    }
    // 不能有王牌
    if (FindJokerNum(table, cards) > 0)
    {
        return false;
    }
    // 不能有2
    if (Find2CardNum(table, cards) > 0)
    {
        return false;
    }

    // 检测 poker_type
    if (dest_poker_type != EN_POKER_TYPE_STRAIGHT_2)
    {
        return false;
    }
    // 检测 len
    if (len != cards.size()/2)
    {
         return false;
    }
    // 检测 start_val
    int lai_zi_num = FindLaiZiNum(table, cards);
    for (int i = 0; i < len; i++)
    {
        int card_num = FindCardNumByVal(table, cards, start_val+i);
        if (card_num > 2 && TableLogic::GetCardValue(table.laizi_card()) == (start_val+i))
        {
            lai_zi_num -= 2;
        }
        else if (card_num > 2 && TableLogic::GetCardValue(table.laizi_card()) != (start_val+i))
        {
            return false;
        }

        if (card_num < 2)
        {
            int need_laizi_num = 2 - card_num;
            if (lai_zi_num >= need_laizi_num)
            {
                lai_zi_num -= need_laizi_num;
            }
            else
            {
                return false;
            }
        }
    }

    return true;
}

// 三带一的顺子
bool PokerLogic::CheckStraight_3_1(const CPBGameTable & table, RepeatedField<int> & cards, ENPokerType dest_poker_type, int start_val, int len)
{
    // 检查size
    if (cards.size() < 8 || cards.size()%4 != 0)
    {
        return false;
    }

    // 检测 poker_type
    if (dest_poker_type != EN_POKER_TYPE_STRAIGHT_3_1)
    {
        return false;
    }
    // 检测 len
    if (len != cards.size()/4)
    {
         return false;
    }
    // 检测 start_val
    int lai_zi_num = FindLaiZiNum(table, cards);
    for (int i = 0; i < len; i++)
    {
        int card_num = FindCardNumByVal(table, cards, start_val+i);
        if (card_num > 3 && TableLogic::GetCardValue(table.laizi_card()) == (start_val+i))
        {
            lai_zi_num -= 3;
        }
//        else if (card_num > 3 && TableLogic::GetCardValue(table.laizi_card()) != (start_val+i))
//            return false;
        if (card_num < 3)
        {
            int need_laizi_num = 3 - card_num;
            if (lai_zi_num >= need_laizi_num)
            {
                lai_zi_num -= need_laizi_num;
            }
            else
            {
                return false;
            }
        }
    }

    return true;
}

// 三不带的顺子
bool PokerLogic::CheckStraight_3(const CPBGameTable & table, RepeatedField<int> & cards, ENPokerType dest_poker_type, int start_val, int len)
{
    // 检查size
    if (cards.size() < 6 || cards.size()%3 != 0)
    {
        return false;
    }

    // 检测 poker_type
    if (dest_poker_type != EN_POKER_TYPE_STRAIGHT_3)
    {
        return false;
    }
    // 检测 len
    if (len != cards.size()/3)
    {
         return false;
    }
    // 检测 start_val
    int lai_zi_num = FindLaiZiNum(table, cards);
    for (int i = 0; i < len; i++)
    {
        int card_num = FindCardNumByVal(table, cards, start_val+i);

        if (card_num > 3 && TableLogic::GetCardValue(table.laizi_card()) == (start_val+i))
        {
            lai_zi_num -= 3;
        }
        else if (card_num > 3 && TableLogic::GetCardValue(table.laizi_card()) != (start_val+i))
            return false;

        if (card_num <= 3)
        {
            int need_laizi_num = 3 - card_num;
            if (lai_zi_num >= need_laizi_num)
            {
                lai_zi_num -= need_laizi_num;
            }
            else
            {
                return false;
            }
        }
    }

    return true;
}

// 三带二的顺子
bool PokerLogic::CheckStraight_3_2(const CPBGameTable & table, RepeatedField<int> & cards, ENPokerType dest_poker_type, int start_val, int len)
{
    // 检查size
    if (cards.size() < 10 || cards.size()%5 != 0)
    {
        return false;
    }

    // 检测 poker_type
    if (dest_poker_type != EN_POKER_TYPE_STRAIGHT_3_2)
    {
        return false;
    }
    // 检测 len
    if (len != cards.size()/5)
    {
         return false;
    }

    // 检测带的必须是一对
    {
        std::map<int, int> card_map;
        for (int i = 0; i < cards.size(); i++)
        {
            card_map[TableLogic::GetLow(cards.Get(i))]++;
        }
        if (card_map.size() != 4)
        {
            return false;
        }
        for (std::map<int, int>::iterator it = card_map.begin();
             it != card_map.end(); ++it)
        {
            if (it->second != 2 && it->second != 3)
            {
                return false;
            }
        }
    }

    // 检测 start_val
    int lai_zi_num = FindLaiZiNum(table, cards);
    for (int i = 0; i < len; i++)
    {
        int card_num = FindCardNumByVal(table, cards, start_val+i);
        if (card_num > 3)
            return false;
        if (card_num != 3)
        {
            int need_laizi_num = 3 - card_num;
            if (lai_zi_num >= need_laizi_num)
            {
                lai_zi_num -= need_laizi_num;
            }
            else
            {
                return false;
            }
        }
    }

    return true;
}
