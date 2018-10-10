#include "ActionBehaviorImp.h"

/*
* 出牌节点
*/
bool ChupaiBehaviorNode::Judge(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat)
{
	//在游戏中
	if (a_pbTable.state() != EN_TABLE_STATE_PLAYING)
	{
		return false;
	}

	//需要这把轮到他来操作
	//他得操作选项中有出牌操作
	if (a_pbSeat.action_choice().choices_size() == 0 || a_pbSeat.action_choice().is_determine())
	{
		return false;
	}

	bool bHasChupai = false;
	for (int i = 0; i < a_pbSeat.action_choice().choices_size(); i++)
	{
		const PBSDRAction & action = a_pbSeat.action_choice().choices(i);
		if (action.act_type() == EN_SDR_ACTION_CHUPAI)
		{
			bHasChupai = true;
		}
	}

	if (!bHasChupai)
	{
		return false;
	}

	//获得前一轮的actionflow，这个acion是pass，并且是单回结束
	const PBSDRGameTable & tableCopy = a_pbTable;
	const PBSDRAction * preChuAction = TableLogic::GetLastChuActionInFlowExceptEmptyChu(tableCopy);
	if (preChuAction == NULL)
	{
		return true;
	}
	else
	{
		if (preChuAction->seat_index() != a_pbSeat.index())
		{
			return false;
		}
	}

	return true;
}

/*
* 炸弹的出牌
*/
bool ChupaiNode::BombNode::Process(PBSDRGameTable &a_pbTable, PBSDRTableSeat &a_pbSeat)
{
	//手里只有其他一个手，持外全是炸弹
	//遍历查找
	int iOtherShou = 0;
	PBShouColInfo tmpShouCol;
	for (int i = 0; i < a_pbSeat.shou_cols_size(); i++)
	{
		const PBShouColInfo & shouCol = a_pbSeat.shou_cols(i);

		if (shouCol.cardtype() != EN_POKER_TYPE_BOMB || shouCol.cardtype() != EN_POKER_TYPE_BOMB_OF_JOKER)
		{
			iOtherShou++;
		}

		if (shouCol.cardtype() != EN_POKER_TYPE_BOMB)
		{
			continue;
		}

		if (!tmpShouCol.has_real())
		{
			tmpShouCol.CopyFrom(shouCol);
		}
		else if (tmpShouCol.real() > shouCol.real())
		{
			tmpShouCol.CopyFrom(shouCol);
		}
	}

	if (!tmpShouCol.has_cardtype())
	{
		return false;
	}

	if (iOtherShou > 1)
	{
		return false;
	}

	AnalyzeModle::ExecuteShouAction(a_pbTable, a_pbSeat, tmpShouCol);

	return true;
}

/*
* 双王炸弹的出牌
*/
bool ChupaiNode::BombOfJokerNode::Process(PBSDRGameTable &a_pbTable, PBSDRTableSeat &a_pbSeat)
{
	//手里只有其他一个手，持外全是炸弹
	//遍历查找
	int iOtherShou = 0;
	PBShouColInfo tmpShouCol;
	for (int i = 0; i < a_pbSeat.shou_cols_size(); i++)
	{
		const PBShouColInfo & shouCol = a_pbSeat.shou_cols(i);

		if (shouCol.cardtype() != EN_POKER_TYPE_BOMB || shouCol.cardtype() != EN_POKER_TYPE_BOMB_OF_JOKER)
		{
			iOtherShou++;
		}

		if (shouCol.cardtype() != EN_POKER_TYPE_BOMB_OF_JOKER)
		{
			continue;
		}

		tmpShouCol.CopyFrom(shouCol);
	}

	if (!tmpShouCol.has_cardtype())
	{
		return false;
	}

	if (iOtherShou > 1)
	{
		return false;
	}

	AnalyzeModle::ExecuteShouAction(a_pbTable, a_pbSeat, tmpShouCol);

	return true;
}

/*
* 3条的出牌节点
*/
bool ChupaiNode::TripleNode::Process(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat)
{
	//找到这个玩家身上是3条的手
	PBShouColInfo tmpShouCol;
	for (int i = 0; i < a_pbSeat.shou_cols_size(); i++)
	{
		const PBShouColInfo & shouCol = a_pbSeat.shou_cols(i);
		if (shouCol.cardtype() != EN_POKER_TYPE_TRIPLE)
		{
			continue;
			
		}

		if (!tmpShouCol.has_real())
		{
			tmpShouCol.CopyFrom(shouCol);
		}
		else if(tmpShouCol.real() > shouCol.real())
		{
			tmpShouCol.CopyFrom(shouCol);
		}
	}

	if (!tmpShouCol.has_cardtype())
	{
		return false;
	}

	//先看有没有单牌的手，取最小的
	{
		int iMinDanCard = -1;
		for (int i = 0; i < a_pbSeat.shou_cols_size(); i++)
		{
			const PBShouColInfo & shouCol = a_pbSeat.shou_cols(i);
			if (shouCol.cardtype() == EN_POKER_TYPE_SINGLE_CARD)
			{
				if (shouCol.cards_size() <= 0)
				{
					continue;
				}

				if (iMinDanCard == -1 && PokerLogic::GetCardLogicVal(a_pbTable, iMinDanCard) > PokerLogic::GetCardLogicVal(a_pbTable, shouCol.cards(0)))
				{
					iMinDanCard = shouCol.cards(0);
				}
			}
		}

		if (iMinDanCard != -1)
		{
			if (tmpShouCol.real() == 15)
			{
				int iOtherColNum = 0;
				for (int i = 0; i < a_pbSeat.shou_cols_size(); i++)
				{
					const PBShouColInfo & shouCol = a_pbSeat.shou_cols(i);

					if (shouCol.cardtype() == EN_POKER_TYPE_TRIPLE && shouCol.real() == 15)
					{
						continue;
					}

					if (shouCol.cardtype() == EN_POKER_TYPE_BOMB || shouCol.cardtype() == EN_POKER_TYPE_BOMB_OF_JOKER)
					{
						continue;
					}

					iOtherColNum++;
				}

				if (iOtherColNum <= 1)
				{
					tmpShouCol.set_cardtype(EN_POKER_TYPE_TRIPLE_WITH_ONE);
					tmpShouCol.add_cards(iMinDanCard);
					//执行3带1
					AnalyzeModle::ExecuteShouAction(a_pbTable, a_pbSeat, tmpShouCol);
					return true;
				}
			}
			else
			{
				tmpShouCol.set_cardtype(EN_POKER_TYPE_TRIPLE_WITH_ONE);
				tmpShouCol.add_cards(iMinDanCard);
				//执行3带1
				AnalyzeModle::ExecuteShouAction(a_pbTable, a_pbSeat, tmpShouCol);
				return true;
			}
		}
	}


	//看是否有双牌
	{
		int iMinCardValue = -1;
		vector<int> minShuangCardVect;
		for (int i = 0; i < a_pbSeat.shou_cols_size(); i++)
		{
			const PBShouColInfo & shouCol = a_pbSeat.shou_cols(i);
			if (shouCol.cardtype() == EN_POKER_TYPE_PAIR)
			{
				if (shouCol.cards_size() <= 1)
				{
					continue;
				}

				if (iMinCardValue == -1 && iMinCardValue > PokerLogic::GetCardLogicVal(a_pbTable, shouCol.cards(0)))
				{
					iMinCardValue = PokerLogic::GetCardLogicVal(a_pbTable, shouCol.cards(0));
					minShuangCardVect.clear();
					for (int i = 0; i < shouCol.cards_size(); i++)
					{
						minShuangCardVect.push_back(shouCol.cards(i));
					}
				}
			}
		}

		if (iMinCardValue != -1)
		{
			if (tmpShouCol.real() == 15)
			{
				int iOtherColNum = 0;
				for (int j = 0; j < a_pbSeat.shou_cols_size(); j++)
				{
					const PBShouColInfo & shouCol = a_pbSeat.shou_cols(j);

					if (shouCol.cardtype() == EN_POKER_TYPE_TRIPLE && shouCol.real() == 15)
					{
						continue;
					}

					if (shouCol.cardtype() == EN_POKER_TYPE_BOMB || shouCol.cardtype() == EN_POKER_TYPE_BOMB_OF_JOKER)
					{
						continue;
					}

					iOtherColNum++;
				}

				if (iOtherColNum <= 1)
				{
					for (unsigned int i = 0; i < minShuangCardVect.size(); i++)
					{
						tmpShouCol.set_cardtype(EN_POKER_TYPE_TRIPLE_WITH_TWO);
						tmpShouCol.add_cards(minShuangCardVect.at(i));
						//执行3带2
						AnalyzeModle::ExecuteShouAction(a_pbTable, a_pbSeat, tmpShouCol);
						return true;
					}
				}
			}
			else
			{
				for (unsigned int i = 0; i < minShuangCardVect.size(); i++)
				{
					tmpShouCol.set_cardtype(EN_POKER_TYPE_TRIPLE_WITH_TWO);
					tmpShouCol.add_cards(minShuangCardVect.at(i));
					//执行3带2
					AnalyzeModle::ExecuteShouAction(a_pbTable, a_pbSeat, tmpShouCol);
					return true;
				}
			}
		}
	}

	if (tmpShouCol.real() == 15)
	{
		int iOtherColNum = 0;
		for (int j = 0; j < a_pbSeat.shou_cols_size(); j++)
		{
			const PBShouColInfo & shouCol = a_pbSeat.shou_cols(j);

			if (shouCol.cardtype() == EN_POKER_TYPE_TRIPLE && shouCol.real() == 15)
			{
				continue;
			}

			if (shouCol.cardtype() == EN_POKER_TYPE_BOMB || shouCol.cardtype() == EN_POKER_TYPE_BOMB_OF_JOKER)
			{
				continue;
			}

			iOtherColNum++;
		}

		if (iOtherColNum <= 0)
		{
			AnalyzeModle::ExecuteShouAction(a_pbTable, a_pbSeat, tmpShouCol);
			return true;
		}
	}
	else
	{
		AnalyzeModle::ExecuteShouAction(a_pbTable, a_pbSeat, tmpShouCol);
		return true;
	}

	return false;
}


/*
* 3顺的出牌节点
*/
bool ChupaiNode::Straight_3Node::Process(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat)
{
	//找到3顺子（找到顺子最多的）
	PBShouColInfo tmpShouCol;
	unsigned int iMaxNum = 0;
	for (int i = 0; i < a_pbSeat.shou_cols_size(); i++)
	{
		const PBShouColInfo & shouCol = a_pbSeat.shou_cols(i);
		if (shouCol.cardtype() != EN_POKER_TYPE_STRAIGHT_3)
		{
			continue;
		}

		if (iMaxNum == 0 || (int)iMaxNum < shouCol.num())
		{
			tmpShouCol.CopyFrom(shouCol);
			iMaxNum = shouCol.num();
		}
	}

	if (iMaxNum == 0)
	{
		return false;
	}
	//找到对应的单牌
	vector<int> danVect;
	for (int i = 0; i < a_pbSeat.shou_cols_size(); i++)
	{
		const PBShouColInfo & shouCol = a_pbSeat.shou_cols(i);
		if (shouCol.cardtype() != EN_POKER_TYPE_SINGLE_CARD)
		{
			continue;
		}

		if (shouCol.cards_size() < 1)
		{
			continue;
		}

		danVect.push_back(shouCol.cards(0));
	}

	if (danVect.size() >= iMaxNum)
	{
		//排序
		sort(danVect.begin(), danVect.end());
		for (unsigned int i = 0; i < iMaxNum; i++)
		{
			tmpShouCol.add_cards(danVect[i]);
		}
		tmpShouCol.set_cardtype(EN_POKER_TYPE_STRAIGHT_3_1);
		AnalyzeModle::ExecuteShouAction(a_pbTable, a_pbSeat, tmpShouCol);

		return true;
	}

	//找到对应的对子,分为3带1的顺子，3带2的顺子
	map<int, vector<int> > shaungMap;
	for (int i = 0; i < a_pbSeat.shou_cols_size(); i++)
	{
		const PBShouColInfo & shouCol = a_pbSeat.shou_cols(i);
		if (shouCol.cardtype() != EN_POKER_TYPE_PAIR)
		{
			continue;
		}

		if (shouCol.cards_size() < 2)
		{
			continue;
		}

		vector<int> & tmpVect = shaungMap[PokerLogic::GetCardLogicVal(a_pbTable, shouCol.cards(0))];
		for (int j = 0; j < shouCol.cards_size(); j++)
		{
			tmpVect.push_back(shouCol.cards(j));
			sort(tmpVect.begin(), tmpVect.end());
		}
	}

	//3顺1的组合
	if (danVect.size() != 0 && (danVect.size() + shaungMap.size() * 2) >= iMaxNum)
	{
		int iNumShaung = iMaxNum / 2;
		int iIndex = 0;
		map<int, vector<int> >::iterator iter = shaungMap.begin();
		//对子
		for (; iter != shaungMap.end(); iter++)
		{
			for (unsigned int j = 0; j < iter->second.size(); j++)
			{
				tmpShouCol.add_cards(iter->second.at(j));
				iIndex++;
			}

			if (iIndex >= iNumShaung)
			{
				break;
			}
		}

		//看看是否还需要添加单牌
		int iNumDan = iMaxNum - iNumShaung * 2;
		if (iNumDan != 0)        //1
		{
			tmpShouCol.add_cards(danVect.at(0));
		}

		tmpShouCol.set_cardtype(EN_POKER_TYPE_STRAIGHT_3_1);
		AnalyzeModle::ExecuteShouAction(a_pbTable, a_pbSeat, tmpShouCol);

		return true;
	}
	//3顺2的组合
	else if (danVect.size() == 0 && shaungMap.size() >= iMaxNum)
	{
		//对子
		int iIndex = 0;
		map<int, vector<int> >::iterator iter = shaungMap.begin();
		for (; iter != shaungMap.end(); iter++)
		{
			for (unsigned int j = 0; j < iter->second.size(); j++)
			{
				tmpShouCol.add_cards(iter->second.at(j));
				iIndex++;
			}

			if (iIndex >= (int)iMaxNum)
			{
				break;
			}
		}

		tmpShouCol.set_cardtype(EN_POKER_TYPE_STRAIGHT_3_2);
		AnalyzeModle::ExecuteShouAction(a_pbTable, a_pbSeat, tmpShouCol);

		return true;
	}

	//执行默认
	{
		AnalyzeModle::ExecuteShouAction(a_pbTable, a_pbSeat, tmpShouCol);
	}

	return true;
}

/*
* 连牌的出牌
*/
bool ChupaiNode::StraightNode::Process(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat)
{
	//找到连牌
	PBShouColInfo tmpShouCol;
	for (int i = 0; i < a_pbSeat.shou_cols_size(); i++)
	{
		const PBShouColInfo & shouCol = a_pbSeat.shou_cols(i);
		if (shouCol.cardtype() != EN_POKER_TYPE_STRAIGHT)
		{
			continue;
		}

		if (!tmpShouCol.has_real())
		{
			tmpShouCol.CopyFrom(shouCol);
		}
		else if (tmpShouCol.real() > shouCol.real())
		{
			tmpShouCol.CopyFrom(shouCol);
		}
	}

	if (!tmpShouCol.has_cardtype())
	{
		return false;
	}

	AnalyzeModle::ExecuteShouAction(a_pbTable, a_pbSeat, tmpShouCol);

	return true;
}

/*
* 双顺的出牌
*/
bool ChupaiNode::Straight_2Node::Process(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat)
{
	PBShouColInfo tmpShouCol;
	for (int i = 0; i < a_pbSeat.shou_cols_size(); i++)
	{
		const PBShouColInfo & shouCol = a_pbSeat.shou_cols(i);
		if (shouCol.cardtype() != EN_POKER_TYPE_STRAIGHT_2)
		{
			continue;
		}

		if (!tmpShouCol.has_real())
		{
			tmpShouCol.CopyFrom(shouCol);
		}
		else if (tmpShouCol.real() > shouCol.real())
		{
			tmpShouCol.CopyFrom(shouCol);
		}
	}

	if (!tmpShouCol.has_cardtype())
	{
		return false;
	}

	AnalyzeModle::ExecuteShouAction(a_pbTable, a_pbSeat, tmpShouCol);

	return true;
}

/*
* 对子出牌
*/
bool ChupaiNode::PairNode::Process(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat)
{
	PBShouColInfo tmpShouCol;
	for (int i = 0; i < a_pbSeat.shou_cols_size(); i++)
	{
		const PBShouColInfo & shouCol = a_pbSeat.shou_cols(i);
		if (shouCol.cardtype() != EN_POKER_TYPE_PAIR)
		{
			continue;
		}

		if (!tmpShouCol.has_cardtype())
		{
			tmpShouCol.CopyFrom(shouCol);
		}
		else if (tmpShouCol.real() > shouCol.real())
		{
			tmpShouCol.CopyFrom(shouCol);
		}
	}

	if (!tmpShouCol.has_cardtype())
	{
		return false;
	}

	//对2不单出
	int iOtherColNum = 0;
	if (tmpShouCol.real() == 15)
	{
		for (int i = 0; i < a_pbSeat.shou_cols_size(); i++)
		{
			const PBShouColInfo & shouCol = a_pbSeat.shou_cols(i);
			if (shouCol.cardtype() == EN_POKER_TYPE_PAIR && shouCol.real() == 15)
			{
				continue;
			}

			if (shouCol.cardtype() == EN_POKER_TYPE_BOMB || shouCol.cardtype() == EN_POKER_TYPE_BOMB_OF_JOKER)
			{
				continue;
			}

			iOtherColNum++;
		}
	}
	
	if (iOtherColNum >= 1)
	{
		return false;
	}

	AnalyzeModle::ExecuteShouAction(a_pbTable, a_pbSeat, tmpShouCol);

	return true;
}

/*
* 单牌出牌
*/
bool ChupaiNode::SingleNode::Process(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat)
{
	enum
	{
		Chu_Da = 1,
		Chu_Xiao = 2
	};

	int bIncline = Chu_Xiao;
	//如果是庄家，且偏家只剩一张牌时，出大的
	//如果是偏家，而且庄家是下家，庄家只有一张牌时，出大的
	//如果是偏家，而且另一个偏家是下家，另一个偏家只有一张牌，出小的
	if (a_pbTable.dealer_index() == a_pbSeat.index())
	{
		bool bExitSeatHas1Card = false;
		for (int i = 0; i < a_pbTable.seats_size(); i++)
		{
			const PBSDRTableSeat & seat = a_pbTable.seats(i);
			if (seat.hand_cards_size() == 1)
			{
				bExitSeatHas1Card = true;
			}
		}

		if (bExitSeatHas1Card)
		{
			bIncline = Chu_Da;
		}
	}
	//不是地主
	else
	{
		int iNextIndex = (a_pbSeat.index() + 1) % a_pbTable.seats_size();
		const PBSDRTableSeat & pbNextSeat = a_pbTable.seats(iNextIndex);

		if (pbNextSeat.hand_cards_size() == 1)
		{
			if (a_pbTable.dealer_index() == pbNextSeat.index())
			{
				bIncline = Chu_Da;
			}
			else
			{
				bIncline = Chu_Xiao;
			}
		}
	}

	//按值排序
	PBShouColInfo tmpShouCol;
	sort(a_pbSeat.mutable_shou_cols()->begin(),a_pbSeat.mutable_shou_cols()->end(),AnalyzeModle::CompareShouCol);
	for (auto iter = a_pbSeat.mutable_shou_cols()->begin() ; iter != a_pbSeat.mutable_shou_cols()->end() ; iter ++)
	{
		const PBShouColInfo & shouCol = *iter;
		if (shouCol.cardtype() != EN_POKER_TYPE_SINGLE_CARD)
		{
			continue;
		}

		if (shouCol.cards_size() <= 0)
		{
			continue;
		}

		tmpShouCol.CopyFrom(shouCol);

		//如果是出小，就按照最小的来算
		if (bIncline == Chu_Xiao)
		{
			break;
		}
	}

	AnalyzeModle::ExecuteShouAction(a_pbTable, a_pbSeat, tmpShouCol);

	return true;
}

/*
* 跟牌节点判定
*/
bool GenpaiBehaviorNode::Judge(PBSDRGameTable &a_pbTable, PBSDRTableSeat &a_pbSeat)
{
	//在游戏中
	if (a_pbTable.state() != EN_TABLE_STATE_PLAYING)
	{
		return false;
	}

	//需要这把轮到他来操作
	//他得操作选项中有出牌操作
	if (a_pbSeat.action_choice().choices_size() == 0 || a_pbSeat.action_choice().is_determine())
	{
		return false;
	}

	bool bHasChupai = false;
	for (int i = 0; i < a_pbSeat.action_choice().choices_size(); i++)
	{
		const PBSDRAction & action = a_pbSeat.action_choice().choices(i);
		if (action.act_type() == EN_SDR_ACTION_CHUPAI)
		{
			bHasChupai = true;
		}
	}

	if (!bHasChupai)
	{
		return false;
	}

	//前一轮是别人出牌
	const PBSDRGameTable & tableCopy = a_pbTable;
	const PBSDRAction * preAction = TableLogic::GetLastChuActionInFlowExceptEmptyChu(tableCopy);
	if (preAction == NULL)
	{
		return false;
	}
	
	//不属于自己跟牌
	if (preAction->seat_index() == a_pbSeat.index())
	{
		return false;
	}

	//如果不是地主，上一家是地主，下一家不是地主，并且下家只有最后一手牌
	if (a_pbSeat.index() != a_pbTable.dealer_index())
	{
		int iPreIndex = (a_pbSeat.index() - 1 + a_pbTable.seats_size()) % a_pbTable.seats_size();
		int iNextIndex = (a_pbSeat.index() + 1 + a_pbTable.seats_size()) % a_pbTable.seats_size();

		if (iPreIndex == a_pbTable.dealer_index() && iNextIndex == preAction->seat_index())
		{		
			if (a_pbTable.seats(iNextIndex).shou_cols_size() <= 2)
			{
				return false;
			}
		}
	}

	return true;
}

/*
* 3条的跟牌
*/
bool GenPaiNode::TripleNode::Process(PBSDRGameTable &a_pbTable, PBSDRTableSeat &a_pbSeat)
{
	int iPreValue = -1;
	const PBSDRGameTable & tableCopy = a_pbTable;
	const PBSDRAction* preAction = TableLogic::GetLastChuActionInFlowExceptEmptyChu(tableCopy);
	if (preAction == NULL)
	{
		return false;
	}
	else
	{
		if (preAction->cardtype() != EN_POKER_TYPE_TRIPLE &&
			preAction->cardtype() != EN_POKER_TYPE_TRIPLE_WITH_ONE &&
			preAction->cardtype() != EN_POKER_TYPE_TRIPLE_WITH_TWO)
		{
			return false;
		}

		iPreValue = preAction->real();
	}

	//找到3条,3顺，对子，单牌
	PBShouColInfo tmpShouCol_3Tiao;
	PBShouColInfo tmpShouCol_3Shun;
	PBShouColInfo tmpShouCol_Pair;
	PBShouColInfo tmpShouCol_Single;
	for (int i = 0; i < a_pbSeat.shou_cols_size(); i++)
	{
		const PBShouColInfo & shouCol = a_pbSeat.shou_cols(i);
		if (shouCol.cardtype() == EN_POKER_TYPE_TRIPLE && shouCol.real() > iPreValue)
		{
			tmpShouCol_3Tiao.CopyFrom(shouCol);
		}
		else if (shouCol.cardtype() == EN_POKER_TYPE_STRAIGHT_3 && (shouCol.real() + shouCol.num() - 1) > iPreValue)
		{
			tmpShouCol_3Shun.CopyFrom(shouCol);
		}
		else if (shouCol.cardtype() == EN_POKER_TYPE_PAIR)
		{
			//尽量取小得值
			if (!tmpShouCol_Pair.has_real())
			{
				tmpShouCol_Pair.CopyFrom(shouCol);
			}
			else if (tmpShouCol_Pair.real() > shouCol.real())
			{
				tmpShouCol_Pair.CopyFrom(shouCol);
			}
		}
		else if (shouCol.cardtype() == EN_POKER_TYPE_SINGLE_CARD)
		{
			//尽量取小得值
			if (!tmpShouCol_Single.has_real())
			{
				tmpShouCol_Single.CopyFrom(shouCol);
			}
			else if (tmpShouCol_Single.real() > shouCol.real())
			{
				tmpShouCol_Single.CopyFrom(shouCol);
			}
		}
	}

	//找到3条
	if (!tmpShouCol_3Tiao.has_cardtype())
	{
		//没有则需要3顺来拆
		if (tmpShouCol_3Shun.has_cardtype())
		{
			AnalyzeModle::Separate3TiaoFrom3Shun(a_pbTable, tmpShouCol_3Shun, tmpShouCol_3Tiao, iPreValue);
		}
		else
		{
			return false;
		}
	}

	//根据前一手牌来操作
	switch (preAction->cardtype())
	{
	case EN_POKER_TYPE_TRIPLE:
	{
		AnalyzeModle::ExecuteShouAction(a_pbTable, a_pbSeat, tmpShouCol_3Tiao);
		return true;
	}

	case EN_POKER_TYPE_TRIPLE_WITH_ONE:
	{
		//3带1需要找单牌
		if (!tmpShouCol_Single.has_cardtype())
		{
			return false;
		}

		tmpShouCol_3Tiao.set_cardtype(EN_POKER_TYPE_TRIPLE_WITH_ONE);
		AnalyzeModle::AddCardsToDesFromSrc(tmpShouCol_Single, tmpShouCol_3Tiao);
		AnalyzeModle::ExecuteShouAction(a_pbTable, a_pbSeat, tmpShouCol_3Tiao);
		return true;
	}

	case EN_POKER_TYPE_TRIPLE_WITH_TWO:
	{
		//3带2需要找对子
		if (!tmpShouCol_Pair.has_cardtype())
		{
			return false;
		}

		tmpShouCol_3Tiao.set_cardtype(EN_POKER_TYPE_TRIPLE_WITH_TWO);
		AnalyzeModle::AddCardsToDesFromSrc(tmpShouCol_Pair, tmpShouCol_3Tiao);
		AnalyzeModle::ExecuteShouAction(a_pbTable, a_pbSeat, tmpShouCol_3Tiao);
		return true;
	}

	default:
		break;
	}

	return false;
}

/*
* 3顺的跟牌
*/
bool GenPaiNode::Straight_3Node::Process(PBSDRGameTable &a_pbTable, PBSDRTableSeat &a_pbSeat)
{
	int iPreValue = 0;
	int iPreNum = 0;
	const PBSDRGameTable & tableCopy = a_pbTable;
	const PBSDRAction* preAction = TableLogic::GetLastChuActionInFlowExceptEmptyChu(tableCopy);
	if (preAction == NULL)
	{
		return false;
	}
	else
	{
		if (preAction->cardtype() != EN_POKER_TYPE_STRAIGHT_3 &&
			preAction->cardtype() != EN_POKER_TYPE_STRAIGHT_3_1 &&
			preAction->cardtype() != EN_POKER_TYPE_STRAIGHT_3_2)
		{
			return false;
		}

		iPreValue = preAction->real();
		iPreNum = preAction->num();
	}

	PBShouColInfo tmpShouCol_3Shun;
	map<int, PBShouColInfo> tmpShouCol_PairMap;
	map<int, PBShouColInfo> tmpShouCol_SingleMap;
	for (int i = 0; i < a_pbSeat.shou_cols_size(); i++)
	{
		const PBShouColInfo & shouCol = a_pbSeat.shou_cols(i);
		if (shouCol.cardtype() == EN_POKER_TYPE_STRAIGHT_3 && shouCol.real() > iPreValue && shouCol.num() >= iPreNum)
		{
			tmpShouCol_3Shun.CopyFrom(shouCol);
		}
		else if (shouCol.cardtype() == EN_POKER_TYPE_PAIR)
		{
			tmpShouCol_PairMap[shouCol.real()] = shouCol;
		}
		else if (shouCol.cardtype() == EN_POKER_TYPE_SINGLE_CARD)
		{
			tmpShouCol_SingleMap[shouCol.real()] = shouCol;
		}
	}

	if (!tmpShouCol_3Shun.has_cardtype())
	{
		return false;
	}

	//检测3顺是否足够
	PBShouColInfo finalShunCol;
	int iMinReal = -1;
	for (int i = 0; i < iPreNum; i++)
	{
		PBShouColInfo tmpShouCol_3Tiao;
		AnalyzeModle::Separate3TiaoFrom3Shun(a_pbTable, tmpShouCol_3Shun, tmpShouCol_3Tiao, iPreValue);
		AnalyzeModle::AddCardsToDesFromSrc(tmpShouCol_3Tiao, finalShunCol);

		if (iMinReal == -1)
		{
			iMinReal = tmpShouCol_3Tiao.real();
		}
		else  if (iMinReal > tmpShouCol_3Tiao.real())
		{
			iMinReal = tmpShouCol_3Tiao.real();
		}
	}
	finalShunCol.set_cardtype(EN_POKER_TYPE_STRAIGHT_3);
	finalShunCol.set_real(iMinReal);
	finalShunCol.set_num(iPreNum);

	//根据上一手牌操作
	switch (preAction->cardtype())
	{
	case EN_POKER_TYPE_STRAIGHT_3:
	{
		AnalyzeModle::ExecuteShouAction(a_pbTable, a_pbSeat, finalShunCol);
		return true;
	}

	case EN_POKER_TYPE_STRAIGHT_3_2:
	{
		if ((int)tmpShouCol_PairMap.size() >= iPreNum)
		{
			int iIndex = 0;
			map<int, PBShouColInfo>::iterator iter = tmpShouCol_PairMap.begin();
			for (; iter != tmpShouCol_PairMap.end(); iter++)
			{
				if (iIndex >= iPreNum)
				{
					continue;
				}

				AnalyzeModle::AddCardsToDesFromSrc(iter->second, finalShunCol);
			}

			finalShunCol.set_cardtype(EN_POKER_TYPE_STRAIGHT_3_2);
			AnalyzeModle::ExecuteShouAction(a_pbTable, a_pbSeat, finalShunCol);
			return true;
		}
	}

	case EN_POKER_TYPE_STRAIGHT_3_1:
	{
		if ((int)tmpShouCol_SingleMap.size() >= iPreNum)
		{
			int iIndex = 0;
			map<int, PBShouColInfo>::iterator iter = tmpShouCol_SingleMap.begin();
			for (; iter != tmpShouCol_SingleMap.end(); iter++)
			{
				if (iIndex >= iPreNum)
				{
					continue;
				}

				AnalyzeModle::AddCardsToDesFromSrc(iter->second, finalShunCol);
				iIndex++;
			}

			finalShunCol.set_cardtype(EN_POKER_TYPE_STRAIGHT_3_1);
			AnalyzeModle::ExecuteShouAction(a_pbTable, a_pbSeat, finalShunCol);
			return true;
		}

		if ((int)tmpShouCol_SingleMap.size() < iPreNum && tmpShouCol_PairMap.size() >= 1)
		{
			AnalyzeModle::AddCardsToDesFromSrc(tmpShouCol_PairMap.begin()->second, finalShunCol);
			finalShunCol.set_cardtype(EN_POKER_TYPE_STRAIGHT_3_1);
			AnalyzeModle::ExecuteShouAction(a_pbTable, a_pbSeat, finalShunCol);
			return true;
		}
	}

	default:
		break;
	}

	return false;
}

/*
* 连牌的跟牌
*/
bool GenPaiNode::StraightNode::Process(PBSDRGameTable &a_pbTable, PBSDRTableSeat &a_pbSeat)
{
	int iPreValue = -1;
	int iPreNum = -1;
	const PBSDRGameTable & tableCopy = a_pbTable;
	const PBSDRAction* preAction = TableLogic::GetLastChuActionInFlowExceptEmptyChu(tableCopy);
	if (preAction == NULL)
	{
		return false;
	}
	else
	{
		if (preAction->cardtype() != EN_POKER_TYPE_STRAIGHT)
		{
			return false;
		}

		iPreValue = preAction->real();
		iPreNum = preAction->num();
	}

	//首先需要找到相同张数的单顺
	{
		PBShouColInfo tmpShouCol_LianPai;
		for (int i = 0; i < a_pbSeat.shou_cols_size(); i++)
		{
			const PBShouColInfo & shouCol = a_pbSeat.shou_cols(i);
			if (shouCol.cardtype() != EN_POKER_TYPE_STRAIGHT || shouCol.num() != iPreNum || shouCol.real() <= iPreValue)
			{
				continue;
			}

			if (!tmpShouCol_LianPai.has_real())
			{
				tmpShouCol_LianPai.CopyFrom(shouCol);
			}
			else if (tmpShouCol_LianPai.real() > shouCol.real())
			{
				tmpShouCol_LianPai.CopyFrom(shouCol);
			}
		}

		if (tmpShouCol_LianPai.has_real())
		{
			AnalyzeModle::ExecuteShouAction(a_pbTable, a_pbSeat, tmpShouCol_LianPai);
			return true;
		}
	}

	//没有则需要找到相同张数的双顺
	{
		PBShouColInfo tmpShouCol_LianPai;
		for (int i = 0; i < a_pbSeat.shou_cols_size(); i++)
		{
			const PBShouColInfo & shouCol = a_pbSeat.shou_cols(i);
			if (shouCol.cardtype() != EN_POKER_TYPE_STRAIGHT_2 || shouCol.num() != iPreNum || shouCol.real() <= iPreValue)
			{
				continue;
			}

			if (!tmpShouCol_LianPai.has_real())
			{
				tmpShouCol_LianPai.CopyFrom(shouCol);
			}
			else if (tmpShouCol_LianPai.real() > shouCol.real())
			{
				tmpShouCol_LianPai.CopyFrom(shouCol);
			}
		}

		if (tmpShouCol_LianPai.has_real())
		{
			//需要分离
			PBShouColInfo finalShunCol;
			finalShunCol.set_cardtype(EN_POKER_TYPE_STRAIGHT);
			finalShunCol.set_real(tmpShouCol_LianPai.real());
			finalShunCol.set_num(tmpShouCol_LianPai.num());
			for (int i = 0; i < tmpShouCol_LianPai.num(); i++)
			{
				int iCurrentVal = tmpShouCol_LianPai.real() + i;
				int iDelCard = AnalyzeModle::RemoveForVectByValue(a_pbTable, *tmpShouCol_LianPai.mutable_cards(), iCurrentVal);
				if (iDelCard != -1)
				{
					finalShunCol.add_cards(iDelCard);
				}
			}

			if (finalShunCol.cards_size() == iPreNum)
			{
				AnalyzeModle::ExecuteShouAction(a_pbTable, a_pbSeat, finalShunCol);
				return true;
			}
		}
	}

	//没有则需要找到相同张数的3顺
	{
		PBShouColInfo tmpShouCol_LianPai;
		for (int i = 0; i < a_pbSeat.shou_cols_size(); i++)
		{
			const PBShouColInfo & shouCol = a_pbSeat.shou_cols(i);
			if (shouCol.cardtype() != EN_POKER_TYPE_STRAIGHT_3 || shouCol.num() != iPreNum || shouCol.real() <= iPreValue)
			{
				continue;
			}

			if (!tmpShouCol_LianPai.has_real())
			{
				tmpShouCol_LianPai.CopyFrom(shouCol);
			}
			else if (tmpShouCol_LianPai.real() > shouCol.real())
			{
				tmpShouCol_LianPai.CopyFrom(shouCol);
			}
		}

		if (tmpShouCol_LianPai.has_real())
		{
			//需要分离
			PBShouColInfo finalShunCol;
			finalShunCol.set_cardtype(EN_POKER_TYPE_STRAIGHT);
			finalShunCol.set_real(tmpShouCol_LianPai.real());
			finalShunCol.set_num(tmpShouCol_LianPai.num());
			for (int i = 0; i < tmpShouCol_LianPai.num(); i++)
			{
				int iCurrentVal = tmpShouCol_LianPai.real() + i;
				int iDelCard = AnalyzeModle::RemoveForVectByValue(a_pbTable, *tmpShouCol_LianPai.mutable_cards(), iCurrentVal);
				if (iDelCard != -1)
				{
					finalShunCol.add_cards(iDelCard);
				}
			}

			if (finalShunCol.cards_size() == iPreNum)
			{
				AnalyzeModle::ExecuteShouAction(a_pbTable, a_pbSeat, finalShunCol);
				return true;
			}
		}
	}

	//再找不同张数的单顺
	{
		PBShouColInfo tmpShouCol_LianPai;
		for (int i = 0; i < a_pbSeat.shou_cols_size(); i++)
		{
			const PBShouColInfo & shouCol = a_pbSeat.shou_cols(i);
			if (shouCol.cardtype() != EN_POKER_TYPE_STRAIGHT || shouCol.num() < iPreNum || (shouCol.real() + shouCol.num() - iPreNum) < iPreValue)
			{
				continue;
			}

			if (!tmpShouCol_LianPai.has_real())
			{
				tmpShouCol_LianPai.CopyFrom(shouCol);
			}
			else if (tmpShouCol_LianPai.real() > shouCol.real())
			{
				tmpShouCol_LianPai.CopyFrom(shouCol);
			}
		}

		if (tmpShouCol_LianPai.has_real())
		{
			PBShouColInfo finalShunCol;
			finalShunCol.set_cardtype(EN_POKER_TYPE_STRAIGHT);
			finalShunCol.set_num(iPreNum);
			//需要分离
			for (int i = 0; i < iPreNum; i++)
			{
				int iCurrentVal = 0;
				if (tmpShouCol_LianPai.real() <= iPreValue)
				{
					iCurrentVal = iPreValue + 1 + i;
				}
				else
				{
					iCurrentVal = tmpShouCol_LianPai.real() + i;
				}
				int iDelCard = AnalyzeModle::RemoveForVectByValue(a_pbTable, *tmpShouCol_LianPai.mutable_cards(), iCurrentVal);
				if (iDelCard != -1)
				{
					finalShunCol.add_cards(iDelCard);
				}
				else
				{
					break;
				}
			}

			if (finalShunCol.cards_size() == iPreNum)
			{
				//将牌按照值来排序
				sort(finalShunCol.mutable_cards()->begin(), finalShunCol.mutable_cards()->end(), AnalyzeModle::CompareCardByValue);
				finalShunCol.set_real(PokerLogic::GetCardLogicVal(a_pbTable, finalShunCol.cards(0)));

				AnalyzeModle::ExecuteShouAction(a_pbTable, a_pbSeat, finalShunCol);
				return true;
			}
		}
	}

	//再找不同张数的双顺
	{
		PBShouColInfo tmpShouCol_LianPai;
		for (int i = 0; i < a_pbSeat.shou_cols_size(); i++)
		{
			const PBShouColInfo & shouCol = a_pbSeat.shou_cols(i);
			if (shouCol.cardtype() != EN_POKER_TYPE_STRAIGHT_2 || shouCol.num() < iPreNum || (shouCol.real() + shouCol.num() - iPreNum) < iPreValue)
			{
				continue;
			}

			if (!tmpShouCol_LianPai.has_real())
			{
				tmpShouCol_LianPai.CopyFrom(shouCol);
			}
			else if (tmpShouCol_LianPai.real() > shouCol.real())
			{
				tmpShouCol_LianPai.CopyFrom(shouCol);
			}
		}

		if (tmpShouCol_LianPai.has_real())
		{
			PBShouColInfo finalShunCol;
			finalShunCol.set_cardtype(EN_POKER_TYPE_STRAIGHT);
			finalShunCol.set_num(iPreNum);
			//需要分离
			for (int i = 0; i < iPreNum; i++)
			{
				int iCurrentVal = 0;
				if (tmpShouCol_LianPai.real() <= iPreValue)
				{
					iCurrentVal = iPreValue + 1 + i;
				}
				else
				{
					iCurrentVal = tmpShouCol_LianPai.real() + i;
				}
				int iDelCard = AnalyzeModle::RemoveForVectByValue(a_pbTable, *tmpShouCol_LianPai.mutable_cards(), iCurrentVal);
				if (iDelCard != -1)
				{
					finalShunCol.add_cards(iDelCard);
				}
				else
				{
					break;
				}
			}

			if (finalShunCol.cards_size() == iPreNum)
			{
				//将牌按照值来排序
				sort(finalShunCol.mutable_cards()->begin(), finalShunCol.mutable_cards()->end(), AnalyzeModle::CompareCardByValue);
				finalShunCol.set_real(PokerLogic::GetCardLogicVal(a_pbTable, finalShunCol.cards(0)));

				AnalyzeModle::ExecuteShouAction(a_pbTable, a_pbSeat, finalShunCol);
				return true;
			}	
		}
	}

	//再找不同张数的3顺
	{
		PBShouColInfo tmpShouCol_LianPai;
		for (int i = 0; i < a_pbSeat.shou_cols_size(); i++)
		{
			const PBShouColInfo & shouCol = a_pbSeat.shou_cols(i);
			if (shouCol.cardtype() != EN_POKER_TYPE_STRAIGHT_3 || shouCol.num() < iPreNum || (shouCol.real() + shouCol.num() - iPreNum) < iPreValue)
			{
				continue;
			}

			if (!tmpShouCol_LianPai.has_real())
			{
				tmpShouCol_LianPai.CopyFrom(shouCol);
			}
			else if (tmpShouCol_LianPai.real() > shouCol.real())
			{
				tmpShouCol_LianPai.CopyFrom(shouCol);
			}
		}

		if (tmpShouCol_LianPai.has_real())
		{
			PBShouColInfo finalShunCol;
			finalShunCol.set_cardtype(EN_POKER_TYPE_STRAIGHT);
			finalShunCol.set_num(iPreNum);
			//需要分离
			for (int i = 0; i < iPreNum; i++)
			{
				int iCurrentVal = 0;
				if (tmpShouCol_LianPai.real() <= iPreValue)
				{
					iCurrentVal = iPreValue + 1 + i;
				}
				else
				{
					iCurrentVal = tmpShouCol_LianPai.real() + i;
				}
				int iDelCard = AnalyzeModle::RemoveForVectByValue(a_pbTable, *tmpShouCol_LianPai.mutable_cards(), iCurrentVal);
				if (iDelCard != -1)
				{
					finalShunCol.add_cards(iDelCard);
				}
				else
				{
					break;
				}
			}

			if (finalShunCol.cards_size() == iPreNum)
			{
				//将牌按照值来排序
				sort(finalShunCol.mutable_cards()->begin(), finalShunCol.mutable_cards()->end(), AnalyzeModle::CompareCardByValue);
				finalShunCol.set_real(PokerLogic::GetCardLogicVal(a_pbTable, finalShunCol.cards(0)));

				AnalyzeModle::ExecuteShouAction(a_pbTable, a_pbSeat, finalShunCol);
				return true;
			}
		}

	}

	return false;
}

/*
* 双顺的跟牌
*/
bool GenPaiNode::Straight_2Node::Process(PBSDRGameTable &a_pbTable, PBSDRTableSeat &a_pbSeat)
{
	int iPreValue = -1;
	int iPreNum = -1;
	const PBSDRGameTable & tableCopy = a_pbTable;
	const PBSDRAction* preAction = TableLogic::GetLastChuActionInFlowExceptEmptyChu(tableCopy);
	if (preAction == NULL)
	{
		return false;
	}
	else
	{
		if (preAction->cardtype() != EN_POKER_TYPE_STRAIGHT_2)
		{
			return false;
		}

		iPreValue = preAction->real();
		iPreNum = preAction->num();
	}

	//只需要寻找有无现成的双顺（因为单顺已经被合并了）
	PBShouColInfo tmpShouCol_ShuangShun;
	for (int i = 0; i < a_pbSeat.shou_cols_size(); i++)
	{
		const PBShouColInfo & shouCol = a_pbSeat.shou_cols(i);
		if (shouCol.cardtype() != EN_POKER_TYPE_STRAIGHT_2 || shouCol.num() < iPreNum || (shouCol.real() + shouCol.num() - iPreNum) <= iPreValue)
		{
			continue;
		}

		if (!tmpShouCol_ShuangShun.has_real())
		{
			tmpShouCol_ShuangShun.CopyFrom(shouCol);
		}
		else if (tmpShouCol_ShuangShun.real() > shouCol.real())
		{
			tmpShouCol_ShuangShun.CopyFrom(shouCol);
		}
	}

	//这里只是得到大于或者等于num的，需要剔除与上次相等的num
	if (!tmpShouCol_ShuangShun.has_real())
	{
		return false;
	}

	//合成最终的连牌
	PBShouColInfo finalShunCol;
	finalShunCol.set_cardtype(EN_POKER_TYPE_STRAIGHT_2);
	finalShunCol.set_num(iPreNum);
	//只需要到num
	for (int i = 0; i < iPreNum; i++)
	{
		//需要查找的值
		int iCurrentVal = 0;
		if (tmpShouCol_ShuangShun.real() <= iPreValue)
		{
			iCurrentVal = iPreValue + 1 + i;
		}
		else
		{
			iCurrentVal = tmpShouCol_ShuangShun.real() + i;
		}

		//每次需要遍历2次
		for (int j = 0; j < 2; j++)
		{
			int iDelCard = AnalyzeModle::RemoveForVectByValue(a_pbTable, *tmpShouCol_ShuangShun.mutable_cards(), iCurrentVal);
			if (iDelCard != -1)
			{
				finalShunCol.add_cards(iDelCard);
			}
		}
	}
	//将牌按照值来排序
	sort(finalShunCol.mutable_cards()->begin(), finalShunCol.mutable_cards()->end(), AnalyzeModle::CompareCardByValue);
	finalShunCol.set_real(PokerLogic::GetCardLogicVal(a_pbTable, finalShunCol.cards(0)));

	AnalyzeModle::ExecuteShouAction(a_pbTable, a_pbSeat, finalShunCol);
	return true;
}

/*
* 对子的跟牌
*/
bool GenPaiNode::PairNode::Process(PBSDRGameTable &a_pbTable, PBSDRTableSeat &a_pbSeat)
{
	int iPreValue = -1;
	const PBSDRGameTable & tableCopy = a_pbTable;
	const PBSDRAction* preAction = TableLogic::GetLastChuActionInFlowExceptEmptyChu(tableCopy);
	if (preAction == NULL)
	{
		return false;
	}
	else
	{
		if (preAction->cardtype() != EN_POKER_TYPE_PAIR)
		{
			return false;
		}

		iPreValue = preAction->real();
	}

	PBShouColInfo tmpPairCol;
	for (int i = 0; i < a_pbSeat.shou_cols_size(); i++)
	{
		const PBShouColInfo & shouCol = a_pbSeat.shou_cols(i);
		if (shouCol.cardtype() != EN_POKER_TYPE_PAIR || shouCol.real() <= iPreValue)
		{
			continue;
		}

		if (!tmpPairCol.has_cardtype())
		{
			tmpPairCol.CopyFrom(shouCol);
		}
		else if (tmpPairCol.real() > shouCol.real())
		{
			tmpPairCol.CopyFrom(shouCol);
		}
	}

	if (!tmpPairCol.has_cardtype())
	{
		return false;
	}

	AnalyzeModle::ExecuteShouAction(a_pbTable, a_pbSeat, tmpPairCol);
	return true;
}

/*
* 单牌的跟牌
*/
bool GenPaiNode::SingleNode::Process(PBSDRGameTable &a_pbTable, PBSDRTableSeat &a_pbSeat)
{
	int iPreValue = -1;
	const PBSDRGameTable & tableCopy = a_pbTable;
	const PBSDRAction* preAction = TableLogic::GetLastChuActionInFlowExceptEmptyChu(tableCopy);
	if (preAction == NULL)
	{
		return false;
	}
	else
	{
		if (preAction->cardtype() != EN_POKER_TYPE_SINGLE_CARD)
		{
			return false;
		}

		iPreValue = preAction->real();
	}

	PBShouColInfo tmpPairCol;
	for (int i = 0; i < a_pbSeat.shou_cols_size(); i++)
	{
		const PBShouColInfo & shouCol = a_pbSeat.shou_cols(i);
		if (shouCol.cardtype() != EN_POKER_TYPE_SINGLE_CARD || shouCol.real() <= iPreValue)
		{
			continue;
		}

		if (!tmpPairCol.has_cardtype())
		{
			tmpPairCol.CopyFrom(shouCol);
		}
		else if (tmpPairCol.real() > shouCol.real())
		{
			tmpPairCol.CopyFrom(shouCol);
		}
	}

	if (!tmpPairCol.has_cardtype())
	{
		return false;
	}

	//如果前面那个人是队友，不出2王
	if (tmpPairCol.real() == 15 || tmpPairCol.real() == 16 || tmpPairCol.real() == 17)
	{
		if (a_pbSeat.index() != a_pbTable.dealer_index())
		{
			if (preAction->seat_index() != a_pbTable.dealer_index())
			{
				return false;
			}
		}
	}

	AnalyzeModle::ExecuteShouAction(a_pbTable, a_pbSeat, tmpPairCol);
	return true;
}

/*
* 炸弹的跟牌
*/
bool GenPaiNode::BombNode::Process(PBSDRGameTable &a_pbTable, PBSDRTableSeat &a_pbSeat)
{
	int iPreValue = -1;
	const PBSDRGameTable & tableCopy = a_pbTable;
	const PBSDRAction* preAction = TableLogic::GetLastChuActionInFlowExceptEmptyChu(tableCopy);
	if (preAction == NULL)
	{
		return false;
	}
	else
	{
		//如果前面那个人是队友，不出炸弹
		if (a_pbSeat.index() != a_pbTable.dealer_index())
		{
			if (preAction->seat_index() != a_pbTable.dealer_index())
			{
				return false;
			}
		}

		if (preAction->cardtype() == EN_POKER_TYPE_BOMB)
		{
			iPreValue = preAction->real();
		}

		if (preAction->cardtype() == EN_POKER_TYPE_BOMB_OF_JOKER)
		{
			iPreValue = 1000;
		}
	}

	PBShouColInfo tmpShouCol;
	for (int i = 0; i < a_pbSeat.shou_cols_size(); i++)
	{
		const PBShouColInfo & shouCol = a_pbSeat.shou_cols(i);

		if (shouCol.cardtype() != EN_POKER_TYPE_BOMB)
		{
			continue;
		}

		if (shouCol.real() < iPreValue)
		{
			continue;
		}

		if (!tmpShouCol.cardtype())
		{
			tmpShouCol.CopyFrom(shouCol);
		}
		else if (tmpShouCol.real() > shouCol.real())
		{
			tmpShouCol.CopyFrom(shouCol);
		}
	}

	if (tmpShouCol.has_cardtype())
	{
		AnalyzeModle::ExecuteShouAction(a_pbTable, a_pbSeat, tmpShouCol);
		return true;
	}

	return false;
}

/*
* 双王炸弹的跟牌
*/
bool GenPaiNode::BombOfJokerNode::Process(PBSDRGameTable &a_pbTable, PBSDRTableSeat &a_pbSeat)
{
	const PBSDRGameTable & tableCopy = a_pbTable;
	const PBSDRAction* preAction = TableLogic::GetLastChuActionInFlowExceptEmptyChu(tableCopy);
	if (preAction == NULL)
	{
		return false;
	}
	else
	{
		//如果前面那个人是队友，不出炸弹
		if (a_pbSeat.index() != a_pbTable.dealer_index())
		{
			if (preAction->seat_index() != a_pbTable.dealer_index())
			{
				return false;
			}
		}
	}

	PBShouColInfo tmpShouCol;
	for (int i = 0; i < a_pbSeat.shou_cols_size(); i++)
	{
		const PBShouColInfo & shouCol = a_pbSeat.shou_cols(i);

		if (shouCol.cardtype() != EN_POKER_TYPE_BOMB_OF_JOKER)
		{
			continue;
		}

		tmpShouCol.CopyFrom(shouCol);
	}

	if (tmpShouCol.has_cardtype())
	{
		AnalyzeModle::ExecuteShouAction(a_pbTable, a_pbSeat, tmpShouCol);
		return true;
	}

	return false;
}

/*
* 叫牌节点
*/
bool JiaoPaiBehaviorNode::Judge(PBSDRGameTable &a_pbTable, PBSDRTableSeat &a_pbSeat)
{
	//检查状态
	if (a_pbTable.state() != EN_TABLE_STATE_WAIT_QIANG_DI_ZHU)
	{
		return false;
	}

	if (a_pbTable.operation_index() != a_pbSeat.index())
	{
		return false;
	}

	//需要这把轮到他来操作
	//他得操作选项中有出牌操作
	if (a_pbSeat.action_choice().choices_size() == 0 || a_pbSeat.action_choice().is_determine())
	{
		return false;
	}

	bool bHasPass = false;
	for (int i = 0; i < a_pbSeat.action_choice().choices_size(); i++)
	{
		const PBSDRAction & action = a_pbSeat.action_choice().choices(i);
		if (action.act_type() == EN_SDR_ACTION_QIANG_DI_ZHU || action.act_type() == EN_SDR_ACTION_JIAO_DI_ZHU)
		{
			bHasPass = true;
		}
	}

	if (!bHasPass)
	{
		return false;
	}

	return true;
}

/*
* 叫牌
*/
bool JiaoPaiNode::JiaoNode::Process(PBSDRGameTable &a_pbTable, PBSDRTableSeat &a_pbSeat)
{
	//火箭为8分
	//炸弹为6分
	//大王为4分
	//小王为3分
	//一个2为2分
	enum Score
	{
		FireScore = 8,
		BombScore = 6,
		BigKingScore = 4,
		SmallKingScore = 3,
		Card2Score = 2
	};

	enum JiaoStand
	{
		QiangAndJiao = 7,
		JiaoButNotQiang = 5,
		NotJiaoAndNotQiang = 0
	};

	int iScore = 0;
	bool bHasFire = false;
	for (int i = 0; i < a_pbSeat.shou_cols_size(); i++)
	{
		const PBShouColInfo & shouCol = a_pbSeat.shou_cols(i);
		if (shouCol.cardtype() == EN_POKER_TYPE_BOMB_OF_JOKER)
		{
			bHasFire = true;
			iScore += FireScore;
		}
		else if (shouCol.cardtype() == EN_POKER_TYPE_BOMB)
		{
			iScore += BombScore;
		}
	}

	for (int i = 0; i < a_pbSeat.hand_cards_size(); i++)
	{
		int iCard = a_pbSeat.hand_cards(i);
		if (!bHasFire)
		{
			if (iCard == 0x51)
			{
				iScore += SmallKingScore;
			}
			else if (iCard == 0x52)
			{
				iScore += BigKingScore;
			}
		}

		if (PokerLogic::GetCardLogicVal(a_pbTable, iCard) == 15)
		{
			iScore += Card2Score;
		}
	}

	bool bHasJiao = false;
	for (int i = 0; i < a_pbTable.total_action_flows_size(); i++)
	{
		const PBSDRActionFlow & actionFlow = a_pbTable.total_action_flows(i);
		if (actionFlow.action().act_type() == EN_SDR_ACTION_JIAO_DI_ZHU)
		{
			bHasJiao = true;
		}
	}

	//查看选择是抢地主还是叫地主
	int iActType = 0;
	for (int i = 0; i < a_pbSeat.action_choice().choices_size(); i++)
	{
		const PBSDRAction & action = a_pbSeat.action_choice().choices(i);
		if (action.act_type() == EN_SDR_ACTION_JIAO_DI_ZHU || action.act_type() == EN_SDR_ACTION_QIANG_DI_ZHU)
		{
			iActType = action.act_type();
		}
	}

	if (iActType == 0)
	{
		return false;
	}

	//大于等于7分时叫地主并抢地主
	//大于等于5分时叫地主但不抢
	//小于5分不叫不抢
	if (iScore >= QiangAndJiao)
	{
		return AnalyzeModle::ExecuteAction(a_pbTable, a_pbSeat, iActType);
	}
	else if (iScore >= JiaoButNotQiang)
	{
		if (iActType == EN_SDR_ACTION_JIAO_DI_ZHU)
		{
			return AnalyzeModle::ExecuteAction(a_pbTable, a_pbSeat, EN_SDR_ACTION_JIAO_DI_ZHU);
		}
	}

	return false;
}

/*
* pass节点
*/
bool PassBehaviorNode::Judge(PBSDRGameTable &a_pbTable, PBSDRTableSeat &a_pbSeat)
{
	if (a_pbTable.operation_index() != a_pbSeat.index())
	{
		return false;
	}

	if (a_pbTable.state() == EN_TABLE_STATE_WAIT_QIANG_DI_ZHU)
	{
		return true;
	}

	//需要这把轮到他来操作
	//他得操作选项中有出牌操作
	if (a_pbSeat.action_choice().choices_size() == 0 || a_pbSeat.action_choice().is_determine())
	{
		return false;
	}

	bool bHasPass = false;
	for (int i = 0; i < a_pbSeat.action_choice().choices_size(); i++)
	{
		const PBSDRAction & action = a_pbSeat.action_choice().choices(i);
		if (action.act_type() == EN_SDR_ACTION_PASS)
		{
			bHasPass = true;
		}
	}

	if (!bHasPass)
	{
		return false;
	}

	return true;
}

/*
* 不叫pass
*/
bool PassNode::BuJiaoNode::Process(PBSDRGameTable &a_pbTable, PBSDRTableSeat &a_pbSeat)
{
	if (a_pbTable.state() != EN_TABLE_STATE_WAIT_QIANG_DI_ZHU)
	{
		return false;
	}

	int iJiaoType = 0;

	for (int i = 0; i < a_pbSeat.action_choice().choices_size(); i++)
	{
		const PBSDRAction & action = a_pbSeat.action_choice().choices(i);
		if (action.act_type() == EN_SDR_ACTION_BU_JIAO || action.act_type() == EN_SDR_ACTION_BU_QIANG)
		{
			iJiaoType = action.act_type();
		}
	}

	if (iJiaoType == 0)
	{
		return false;
	}

	return AnalyzeModle::ExecuteAction(a_pbTable, a_pbSeat, iJiaoType);
}

/*
* 不出pass
*/
bool PassNode::BuChuNode::Process(PBSDRGameTable &a_pbTable, PBSDRTableSeat &a_pbSeat)
{
	if (a_pbTable.state() != EN_TABLE_STATE_PLAYING)
	{
		return false;
	}

	return AnalyzeModle::ExecuteAction(a_pbTable, a_pbSeat, EN_SDR_ACTION_PASS);
}


