#include "AnalyzeModle.h"

/*
* 分析出其玩家手牌的牌张
* return : 返回手数
*/
int AnalyzeModle::AnalyzeSeatPaiZhang(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat)
{
	RepeatedPtrField<PBShouColInfo> shouCards;
	RepeatedField<int> residualCards(a_pbSeat.hand_cards());

	//递归调用转化为手(循环就能解决)
	bool isEnd = false;
	while (!isEnd)
	{
		TryTransformShou(a_pbTable, a_pbSeat, shouCards, residualCards, isEnd);
	}

	//手能不能合并（在最后看看能不能通过3条和单牌组成3带1）

	//如果返回值>0,则正确把这些数据实装到玩家身上
	if (shouCards.size() > 0)
	{
		a_pbSeat.mutable_shou_cols()->CopyFrom(shouCards);
	}

	//返回手值
	return shouCards.size();
}

/*
* 循环将手牌置为手
* return : 手数
*/
int AnalyzeModle::TryTransformShou(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat,
	RepeatedPtrField<PBShouColInfo> & a_shouCards, RepeatedField<int> & a_residualCards, bool & a_bIsEnd)
{
	//定一个map，方便分析
	map<int, vector<int> > cards_map;
	for (int i = 0; i < a_residualCards.size(); i++)
	{
		int iCard = a_residualCards.Get(i);
		cards_map[PokerLogic::GetCardLogicVal(a_pbTable, iCard)].push_back(iCard);
	}

	switch (EN_POKER_TYPE_BOMB_OF_JOKER)
	{
		//先看有无双王
	case EN_POKER_TYPE_BOMB_OF_JOKER:
	{
		if (cards_map[17].size() == 1 && cards_map[16].size() == 1)
		{
			RepeatedField<int> cards;
			cards.Add(0x51);
			cards.Add(0x52);
			CreateShouFromCards(a_shouCards, a_residualCards,
				EN_POKER_TYPE_BOMB_OF_JOKER, PokerLogic::GetCardLogicVal(a_pbTable, 0x51), 1, cards);
			return 1;
		}
	}

	//再看有没有炸弹
	case EN_POKER_TYPE_BOMB:
	{
		map<int, vector<int> >::iterator iter = cards_map.begin();
		for (; iter != cards_map.end(); iter++)
		{
			if (iter->second.size() == 4)
			{
				RepeatedField<int> cards;
				vector<int> vect = iter->second;
				for (unsigned int i = 0; i < vect.size(); i++)
				{
					int iCard = vect.at(i);
					cards.Add(iCard);
				}
				CreateShouFromCards(a_shouCards, a_residualCards,
					EN_POKER_TYPE_BOMB, PokerLogic::GetCardLogicVal(a_pbTable, cards.Get(0)), 1, cards);
				return 1;
			}
		}
	}

	//确定3条
	case EN_POKER_TYPE_TRIPLE:
	{
		map<int, vector<int> >::iterator iter = cards_map.begin();
		for (; iter != cards_map.end(); iter++)
		{
			if (iter->second.size() == 3)
			{
				RepeatedField<int> cards;
				vector<int> vect = iter->second;
				for (unsigned int i = 0; i < vect.size(); i++)
				{
					int iCard = vect.at(i);
					cards.Add(iCard);
				}
				CreateShouFromCards(a_shouCards, a_residualCards,
					EN_POKER_TYPE_TRIPLE, PokerLogic::GetCardLogicVal(a_pbTable, cards.Get(0)), 1, cards);
				return 1;
			}
		}
	}

	//确定有没有3顺
	case EN_POKER_TYPE_STRAIGHT_3:
	{
		//遍历手的组
		for (int i = 0; i < a_shouCards.size(); i++)
		{
			vector<PBShouColInfo> delVect;

			//先排除不是3条的
			const PBShouColInfo & col_info = a_shouCards.Get(i);
			if (col_info.cardtype() != EN_POKER_TYPE_TRIPLE)
			{
				continue;
			}
			//然后排除值不满足的
			if (col_info.real() < 3 || col_info.real() > 13)
			{
				continue;
			}

			//首先这个值需要先添加
			delVect.push_back(col_info);

			//寻找它值+是否有(中途如果没有则终止)
			int NextValue = col_info.real() + 1;
			while (NextValue <= 14)
			{
				//如果发现了这个值得col，则添加到vect中
				PBShouColInfo tmpColInfo;//(容器)
				tmpColInfo.set_cardtype(col_info.cardtype());
				tmpColInfo.set_real(NextValue);
				if (FindNextColByValueAndType(a_shouCards, tmpColInfo))
				{
					delVect.push_back(tmpColInfo);
					NextValue++;
				}
				else
				{
					break;
				}
			}

			//如果有2个以上，则需要删除，然后退出，下一轮
			if (delVect.size() >= 2)
			{
				CreateShouFromOtherShou(a_shouCards, delVect,
					EN_POKER_TYPE_STRAIGHT_3, col_info.real(), delVect.size());
				return 1;
			}
		}
	}

	//确定有没有单顺
	case EN_POKER_TYPE_STRAIGHT:
	{
		map<int, vector<int> >::iterator iter = cards_map.begin();
		for (; iter != cards_map.end(); iter++)
		{
			vector<int> shunVect;
			int iValue = iter->first;
			int iEyeValue = iValue + 1;
			shunVect.push_back(iValue);

			if (iter->second.size() <= 0)
			{
				continue;
			}

			//遍历余下的值
			map<int, vector<int> >::iterator iter_E = iter;
			iter_E++;
			for (; iter_E != cards_map.end(); iter_E++)
			{
				if (iEyeValue == 15)
				{
					break;	//不能算2
				}

				if (iter_E->second.size() <= 0)
				{
					break;
				}

				if (iter_E->first == iEyeValue)
				{
					shunVect.push_back(iEyeValue);
					iEyeValue++;
				}
				else
				{
					break;
				}
			}

			//单顺要尽可能得多
			if (shunVect.size() >= 5)
			{
				//最小的为第一数
				int iMinValue = shunVect[0];
				RepeatedField<int> cards;
				for (unsigned int i = 0; i < shunVect.size(); i++)
				{
					int iValue = shunVect[i];
					if (cards_map[iValue].size() <= 0)
					{
						return -1;
					}

					cards.Add(cards_map[iValue][0]);
				}

				CreateShouFromCards(a_shouCards, a_residualCards,
					EN_POKER_TYPE_STRAIGHT, iMinValue, cards.size(), cards);

				return 1;
			}
		}
	}

	//看没有双顺（可以看前面完全吻合的单顺）
	case EN_POKER_TYPE_STRAIGHT_2:
	{
		//是否有原始的双顺
		map<int, vector<int> >::iterator iter = cards_map.begin();
		for (; iter != cards_map.end(); iter++)
		{
			vector<int> shunVect;
			int iValue = iter->first;
			int iEyeValue = iValue + 1;
			shunVect.push_back(iValue);

			if (iter->second.size() <= 1)
			{
				continue;
			}

			//遍历余下的值
			map<int, vector<int> >::iterator iter_E = iter;
			iter_E++;
			for (; iter_E != cards_map.end(); iter_E++)
			{
				if (iEyeValue == 15)
				{
					break;
				}

				if (iter_E->second.size() <= 1)
				{
					break;
				}

				if (iter_E->first == iEyeValue)
				{
					shunVect.push_back(iEyeValue);
					iEyeValue++;

					//取5的顺子
					if (shunVect.size() == 5)
					{
						break;
					}
				}
				else
				{
					break;
				}
			}

			if (shunVect.size() >= 3)
			{
				int iMinValue = shunVect[0];
				RepeatedField<int> cards;
				for (unsigned int i = 0; i < shunVect.size(); i++)
				{
					int iValue = shunVect[i];
					if (cards_map[iValue].size() <= 1)
					{
						return -1;
					}

					cards.Add(cards_map[iValue][0]);
					cards.Add(cards_map[iValue][1]);
				}

				CreateShouFromCards(a_shouCards, a_residualCards,
					EN_POKER_TYPE_STRAIGHT_2, iMinValue, cards.size() / 2, cards);
				return 1;
			}
		}

		//因为前面单顺到后来没有限制张数了，所以需要判定单顺的张数
		//real值可以不一样，num值可以不一样
		//需要取出他们相同的部分
		sort(a_shouCards.begin(), a_shouCards.end(), CompareShouCol);
		for (int i = 0; i < a_shouCards.size(); i++)
		{
			//先排除不是单顺的
			PBShouColInfo & col_info_A = *a_shouCards.Mutable(i);
			if (col_info_A.cardtype() != EN_POKER_TYPE_STRAIGHT || col_info_A.cards_size() < 5)
			{
				continue;
			}

			if (i == a_shouCards.size() - 1)
			{
				continue;
			}

			for (int j = i + 1; j < a_shouCards.size(); j++)
			{
				PBShouColInfo & col_info_B = *a_shouCards.Mutable(j);
				if (col_info_B.cardtype() != EN_POKER_TYPE_STRAIGHT || col_info_B.cards_size() < 5)
				{
					continue;
				}

				if (col_info_A.real() + col_info_A.num() - 1 < col_info_B.real() + 4)
				{
					continue;
				}

				sort(col_info_A.mutable_cards()->begin(), col_info_A.mutable_cards()->end(), CompareCardByValue);
				auto iter_A = col_info_A.mutable_cards()->begin();
				sort(col_info_B.mutable_cards()->begin(), col_info_B.mutable_cards()->end(), CompareCardByValue);
				auto iter_B = col_info_B.mutable_cards()->begin();

				//因为前面已经将其排序，所以是按照real值的重小到大来的
				if (PokerLogic::GetCardLogicVal(a_pbTable, *iter_A) > PokerLogic::GetCardLogicVal(a_pbTable, *iter_B))
				{
					continue;
				}

				while (PokerLogic::GetCardLogicVal(a_pbTable, *iter_A) < PokerLogic::GetCardLogicVal(a_pbTable, *iter_B))
				{
					iter_A++;
				}

				//这里itera和iterb已经相等，拿个值来记录他们相同的val
				vector<int> equelValVect;
				while (iter_A != col_info_A.mutable_cards()->end() && iter_B != col_info_B.mutable_cards()->end())
				{
					if (PokerLogic::GetCardLogicVal(a_pbTable, *iter_A) != PokerLogic::GetCardLogicVal(a_pbTable, *iter_B))
					{
						break;
					}

					equelValVect.push_back(PokerLogic::GetCardLogicVal(a_pbTable, *iter_A));
					iter_A++;
					iter_B++;
				}

				if (equelValVect.size() < 5)
				{
					continue;
				}

				//取得了相同的值，之后只需要合并到一起
				PBShouColInfo newShuangShunCol;
				newShuangShunCol.set_cardtype(EN_POKER_TYPE_STRAIGHT_2);
				newShuangShunCol.set_real(equelValVect.at(0));
				newShuangShunCol.set_num(equelValVect.size());
				//删除原来的，并添加到现col中
				for (unsigned int k = 0; k < equelValVect.size(); k++)
				{
					int iDelVal = equelValVect.at(k);
					int iDelCard = RemoveForVectByValue(a_pbTable, *col_info_A.mutable_cards(), iDelVal);
					newShuangShunCol.add_cards(iDelCard);

					iDelCard = RemoveForVectByValue(a_pbTable, *col_info_B.mutable_cards(), iDelVal);
					newShuangShunCol.add_cards(iDelCard);
				}

				//归还2个col中剩余的card
				a_residualCards.MergeFrom(*col_info_A.mutable_cards());
				a_residualCards.MergeFrom(*col_info_B.mutable_cards());

				//删除老的col，先删除大的那个，也就是b
				a_shouCards.DeleteSubrange(j, 1);
				a_shouCards.DeleteSubrange(i, 1);

				//添加到手中
				a_shouCards.Add()->CopyFrom(newShuangShunCol);

				return 1;
			}
		}
	}

	//确定对子
	case EN_POKER_TYPE_PAIR:
	{
		map<int, vector<int> >::iterator iter = cards_map.begin();
		for (; iter != cards_map.end(); iter++)
		{
			if (iter->second.size() == 2)
			{
				RepeatedField<int> cards;
				vector<int> vect = iter->second;
				for (unsigned int i = 0; i < vect.size(); i++)
				{
					int iCard = vect.at(i);
					cards.Add(iCard);
				}
				CreateShouFromCards(a_shouCards, a_residualCards,
					EN_POKER_TYPE_PAIR, PokerLogic::GetCardLogicVal(a_pbTable, cards.Get(0)), 1, cards);
				return 1;
			}
		}
	}

	//确定单牌
	case EN_POKER_TYPE_SINGLE_CARD:
	{
		map<int, vector<int> >::iterator iter = cards_map.begin();
		for (; iter != cards_map.end(); iter++)
		{
			if (iter->second.size() == 1)
			{
				RepeatedField<int> cards;
				vector<int> & vect = iter->second;
				for (unsigned int i = 0; i < vect.size(); i++)
				{
					int iCard = vect.at(i);
					cards.Add(iCard);
				}
				CreateShouFromCards(a_shouCards, a_residualCards,
					EN_POKER_TYPE_SINGLE_CARD, PokerLogic::GetCardLogicVal(a_pbTable, cards.Get(0)), 1, cards);
				return 1;
			}
		}
	}

	break;
	default:
		break;
	}

	a_bIsEnd = true;
	return 0;
}

/*
* 在vect中找到值和类型对应的col
*/
bool AnalyzeModle::FindNextColByValueAndType(const RepeatedPtrField<PBShouColInfo> & a_shouCards, PBShouColInfo & a_tmpColInfo)
{
	bool bIsFind = false;

	for (int i = 0; i < a_shouCards.size(); i++)
	{
		const PBShouColInfo & colInfo = a_shouCards.Get(i);
		if (colInfo.cardtype() != a_tmpColInfo.cardtype() || colInfo.real() != a_tmpColInfo.real())
		{
			continue;
		}

		a_tmpColInfo.CopyFrom(colInfo);
		bIsFind = true;
	}

	return bIsFind;
}

/*
* 将RepeatedPtrField中的一些手组合为一组手
*/
bool AnalyzeModle::CreateShouFromOtherShou(RepeatedPtrField<PBShouColInfo> & a_shouCards, const vector<PBShouColInfo> & a_srcVect,
	int a_cardtype, int a_real, int a_num)
{
	//删除老的手
	for (unsigned int i = 0; i < a_srcVect.size(); i++)
	{
		bool bIsFind = false;
		const PBShouColInfo & col_info_V = a_srcVect.at(i);
		for (int j = 0; j < a_shouCards.size(); j++)
		{
			const PBShouColInfo & col_info = a_shouCards.Get(j);
			if (col_info.cardtype() == col_info_V.cardtype() && col_info.real() == col_info_V.real())
			{
				a_shouCards.DeleteSubrange(j, 1);
				bIsFind = true;
				break;
			}
		}

		if (!bIsFind)
		{
			return false;
		}
	}

	//新手cards的添加
	RepeatedField<int> delCards;
	for (unsigned int i = 0; i < a_srcVect.size(); i++)
	{
		const PBShouColInfo & colInfo_S = a_srcVect.at(i);
		for (int j = 0; j < colInfo_S.cards_size(); j++)
		{
			int iCard = colInfo_S.cards(j);
			delCards.Add(iCard);
		}
	}
	sort(delCards.begin(), delCards.end());

	//新手的添加
	{
		PBShouColInfo newColInfo;
		newColInfo.set_cardtype(a_cardtype);
		newColInfo.set_real(a_real);
		newColInfo.set_num(a_num);
		newColInfo.mutable_cards()->CopyFrom(delCards);
		a_shouCards.Add()->CopyFrom(newColInfo);
	}

	return true;
}

/*
* 将一个col添加
*/
bool AnalyzeModle::CreateShouFromCards(RepeatedPtrField<PBShouColInfo> & a_shouCards, RepeatedField<int> & a_residualCards,
	int a_cardtype, int a_real, int a_num, const RepeatedField<int> & a_cards)
{
	//col_cards里添加
	PBShouColInfo shou_col;
	shou_col.set_cardtype(a_cardtype);
	shou_col.set_real(a_real);
	shou_col.set_num(a_num);
	shou_col.mutable_cards()->CopyFrom(a_cards);
	a_shouCards.Add()->CopyFrom(shou_col);

	//剩余牌的删除
	for (int i = 0; i < a_cards.size(); i++)
	{
		int iCard = a_cards.Get(i);
		RemoveForVect(a_residualCards, iCard);
	}

	return true;
}

/*
* 删除某个vect中的牌
*/
int AnalyzeModle::RemoveForVect(RepeatedField<int> & a_vect, int a_iCard)
{
	for (int i = 0; i < a_vect.size(); i++)
	{
		int iCard = a_vect.Get(i);
		if (iCard == a_iCard)
		{
			int del_card = 0;
			a_vect.ExtractSubrange(i, 1, &del_card);
			return iCard;
		}
	}

	return -1;
}

bool AnalyzeModle::ExecuteShouAction(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat, const PBShouColInfo & a_shouCol)
{
	//找到选项里面的type
	PBSDRAction * pChoiceAction = NULL;
	for (int i = 0; i < a_pbSeat.action_choice().choices_size(); i++)
	{
		PBSDRAction * pAtion = a_pbSeat.mutable_action_choice()->mutable_choices(i);
		if (pAtion->act_type() == EN_SDR_ACTION_CHUPAI)
		{
			pChoiceAction = pAtion;
		}
	}	

	if (!pChoiceAction)
	{
		return false;
	}

	pChoiceAction->set_cardtype(a_shouCol.cardtype());
	pChoiceAction->set_real(a_shouCol.real());
	pChoiceAction->set_num(a_shouCol.num());
	pChoiceAction->mutable_cards()->CopyFrom(a_shouCol.cards());
	pChoiceAction->mutable_col_info()->mutable_cards()->CopyFrom(pChoiceAction->cards());
	pChoiceAction->set_is_trustee_ship_auto_action(true);
	//发送

	a_pbSeat.mutable_robot_action_buff()->CopyFrom(*pChoiceAction);
	//TableLogic::ProcessAutoAction(a_pbTable, a_pbSeat, EN_SDR_ACTION_CHUPAI);
	VLogMsg(CLIB_LOG_LEV_DEBUG, "自动操作：uid[%ld] try do action act_type[%d] dest_card[%d]",
		a_pbSeat.user().uid(), pChoiceAction->act_type(), pChoiceAction->dest_card());

	return true;
}

/*
* 从3顺中拆一组3条
*/
bool AnalyzeModle::Separate3TiaoFrom3Shun(const PBSDRGameTable & a_pbTable, PBShouColInfo & a_Src3ShunCol, PBShouColInfo & a_Des3TiaoCol, int a_iMinValue)
{
	int iMinValue = -1;
	for (int i = 0; i < a_Src3ShunCol.cards_size(); i++)
	{
		int iCard = a_Src3ShunCol.cards(i);
		if (PokerLogic::GetCardLogicVal(a_pbTable, iCard) <= a_iMinValue)
		{
			continue;
		}

		if (iMinValue == -1)
		{
			iMinValue = PokerLogic::GetCardLogicVal(a_pbTable, iCard);
		}
		else
		{
			if (iMinValue > PokerLogic::GetCardLogicVal(a_pbTable, iCard))
			{
				iMinValue = PokerLogic::GetCardLogicVal(a_pbTable, iCard);
			}
		}
	}

	if (iMinValue == -1)
	{
		return false;
	}

	vector<int> delVect;
	a_Des3TiaoCol.set_real(iMinValue);
	a_Des3TiaoCol.set_num(1);
	for (int i = 0; i < a_Src3ShunCol.cards_size(); i++)
	{
		int iCard = a_Src3ShunCol.cards(i);
		if (PokerLogic::GetCardLogicVal(a_pbTable, iCard) != iMinValue)
		{
			continue;
		}

		a_Des3TiaoCol.add_cards(iCard);
		delVect.push_back(iCard);
	}

	for (unsigned int i = 0 ; i < delVect.size() ; i ++)
	{
		int iCard = delVect.at(i);
		RemoveForVect(*a_Src3ShunCol.mutable_cards(), iCard);
	}

	return true;
}

/*
*  将某个col中得cards全部添加到另一个col的cards中取
*/
void AnalyzeModle::AddCardsToDesFromSrc(const PBShouColInfo & a_Src3ShunCol, PBShouColInfo & a_Des3TiaoCol)
{
	for (int i = 0; i < a_Src3ShunCol.cards_size(); i++)
	{
		int iCard = a_Src3ShunCol.cards(i);
		a_Des3TiaoCol.add_cards(iCard);
	}
}

/*
* 执行某个动作
*/
bool AnalyzeModle::ExecuteAction(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat, int a_iActType)
{
	PBSDRAction * pChoiceAction = NULL;
	for (int i = 0; i < a_pbSeat.action_choice().choices_size(); i++)
	{
		PBSDRAction * pAtion = a_pbSeat.mutable_action_choice()->mutable_choices(i);
		if (pAtion->act_type() == a_iActType)
		{
			pChoiceAction = pAtion;
		}
	}

	if (!pChoiceAction)
	{
		return false;
	}

	pChoiceAction->set_is_trustee_ship_auto_action(true);
	a_pbSeat.mutable_robot_action_buff()->CopyFrom(*pChoiceAction);

	//TableLogic::ProcessAutoAction(a_pbTable, a_pbSeat, (ENSDRActionType)a_iActType);
	VLogMsg(CLIB_LOG_LEV_DEBUG, "自动操作：uid[%ld] try do action act_type[%d] dest_card[%d]",
		a_pbSeat.user().uid(), pChoiceAction->act_type(), pChoiceAction->dest_card());

	return true;
}

/*
* 删除某个vect中的值
*/
int AnalyzeModle::RemoveForVectByValue(const PBSDRGameTable & a_pbTable, RepeatedField<int> & a_vect, int a_iVal)
{
	for (int i = 0; i < a_vect.size(); i++)
	{
		int iCard = a_vect.Get(i);
		if (PokerLogic::GetCardLogicVal(a_pbTable, iCard) == a_iVal)
		{
			int del_card = 0;
			a_vect.ExtractSubrange(i, 1, &del_card);
			return iCard;
		}
	}

	return -1;
}