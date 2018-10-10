#include "ActionBehaviorImp.h"

/*
* ���ƽڵ�
*/
bool ChupaiBehaviorNode::Judge(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat)
{
	//����Ϸ��
	if (a_pbTable.state() != EN_TABLE_STATE_PLAYING)
	{
		return false;
	}

	//��Ҫ����ֵ���������
	//���ò���ѡ�����г��Ʋ���
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

	//���ǰһ�ֵ�actionflow�����acion��pass�������ǵ��ؽ���
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
* ը���ĳ���
*/
bool ChupaiNode::BombNode::Process(PBSDRGameTable &a_pbTable, PBSDRTableSeat &a_pbSeat)
{
	//����ֻ������һ���֣�����ȫ��ը��
	//��������
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
* ˫��ը���ĳ���
*/
bool ChupaiNode::BombOfJokerNode::Process(PBSDRGameTable &a_pbTable, PBSDRTableSeat &a_pbSeat)
{
	//����ֻ������һ���֣�����ȫ��ը��
	//��������
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
* 3���ĳ��ƽڵ�
*/
bool ChupaiNode::TripleNode::Process(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat)
{
	//�ҵ�������������3������
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

	//�ȿ���û�е��Ƶ��֣�ȡ��С��
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
					//ִ��3��1
					AnalyzeModle::ExecuteShouAction(a_pbTable, a_pbSeat, tmpShouCol);
					return true;
				}
			}
			else
			{
				tmpShouCol.set_cardtype(EN_POKER_TYPE_TRIPLE_WITH_ONE);
				tmpShouCol.add_cards(iMinDanCard);
				//ִ��3��1
				AnalyzeModle::ExecuteShouAction(a_pbTable, a_pbSeat, tmpShouCol);
				return true;
			}
		}
	}


	//���Ƿ���˫��
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
						//ִ��3��2
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
					//ִ��3��2
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
* 3˳�ĳ��ƽڵ�
*/
bool ChupaiNode::Straight_3Node::Process(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat)
{
	//�ҵ�3˳�ӣ��ҵ�˳�����ģ�
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
	//�ҵ���Ӧ�ĵ���
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
		//����
		sort(danVect.begin(), danVect.end());
		for (unsigned int i = 0; i < iMaxNum; i++)
		{
			tmpShouCol.add_cards(danVect[i]);
		}
		tmpShouCol.set_cardtype(EN_POKER_TYPE_STRAIGHT_3_1);
		AnalyzeModle::ExecuteShouAction(a_pbTable, a_pbSeat, tmpShouCol);

		return true;
	}

	//�ҵ���Ӧ�Ķ���,��Ϊ3��1��˳�ӣ�3��2��˳��
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

	//3˳1�����
	if (danVect.size() != 0 && (danVect.size() + shaungMap.size() * 2) >= iMaxNum)
	{
		int iNumShaung = iMaxNum / 2;
		int iIndex = 0;
		map<int, vector<int> >::iterator iter = shaungMap.begin();
		//����
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

		//�����Ƿ���Ҫ��ӵ���
		int iNumDan = iMaxNum - iNumShaung * 2;
		if (iNumDan != 0)        //1
		{
			tmpShouCol.add_cards(danVect.at(0));
		}

		tmpShouCol.set_cardtype(EN_POKER_TYPE_STRAIGHT_3_1);
		AnalyzeModle::ExecuteShouAction(a_pbTable, a_pbSeat, tmpShouCol);

		return true;
	}
	//3˳2�����
	else if (danVect.size() == 0 && shaungMap.size() >= iMaxNum)
	{
		//����
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

	//ִ��Ĭ��
	{
		AnalyzeModle::ExecuteShouAction(a_pbTable, a_pbSeat, tmpShouCol);
	}

	return true;
}

/*
* ���Ƶĳ���
*/
bool ChupaiNode::StraightNode::Process(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat)
{
	//�ҵ�����
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
* ˫˳�ĳ���
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
* ���ӳ���
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

	//��2������
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
* ���Ƴ���
*/
bool ChupaiNode::SingleNode::Process(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat)
{
	enum
	{
		Chu_Da = 1,
		Chu_Xiao = 2
	};

	int bIncline = Chu_Xiao;
	//�����ׯ�ң���ƫ��ֻʣһ����ʱ�������
	//�����ƫ�ң�����ׯ�����¼ң�ׯ��ֻ��һ����ʱ�������
	//�����ƫ�ң�������һ��ƫ�����¼ң���һ��ƫ��ֻ��һ���ƣ���С��
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
	//���ǵ���
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

	//��ֵ����
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

		//����ǳ�С���Ͱ�����С������
		if (bIncline == Chu_Xiao)
		{
			break;
		}
	}

	AnalyzeModle::ExecuteShouAction(a_pbTable, a_pbSeat, tmpShouCol);

	return true;
}

/*
* ���ƽڵ��ж�
*/
bool GenpaiBehaviorNode::Judge(PBSDRGameTable &a_pbTable, PBSDRTableSeat &a_pbSeat)
{
	//����Ϸ��
	if (a_pbTable.state() != EN_TABLE_STATE_PLAYING)
	{
		return false;
	}

	//��Ҫ����ֵ���������
	//���ò���ѡ�����г��Ʋ���
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

	//ǰһ���Ǳ��˳���
	const PBSDRGameTable & tableCopy = a_pbTable;
	const PBSDRAction * preAction = TableLogic::GetLastChuActionInFlowExceptEmptyChu(tableCopy);
	if (preAction == NULL)
	{
		return false;
	}
	
	//�������Լ�����
	if (preAction->seat_index() == a_pbSeat.index())
	{
		return false;
	}

	//������ǵ�������һ���ǵ�������һ�Ҳ��ǵ����������¼�ֻ�����һ����
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
* 3���ĸ���
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

	//�ҵ�3��,3˳�����ӣ�����
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
			//����ȡС��ֵ
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
			//����ȡС��ֵ
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

	//�ҵ�3��
	if (!tmpShouCol_3Tiao.has_cardtype())
	{
		//û������Ҫ3˳����
		if (tmpShouCol_3Shun.has_cardtype())
		{
			AnalyzeModle::Separate3TiaoFrom3Shun(a_pbTable, tmpShouCol_3Shun, tmpShouCol_3Tiao, iPreValue);
		}
		else
		{
			return false;
		}
	}

	//����ǰһ����������
	switch (preAction->cardtype())
	{
	case EN_POKER_TYPE_TRIPLE:
	{
		AnalyzeModle::ExecuteShouAction(a_pbTable, a_pbSeat, tmpShouCol_3Tiao);
		return true;
	}

	case EN_POKER_TYPE_TRIPLE_WITH_ONE:
	{
		//3��1��Ҫ�ҵ���
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
		//3��2��Ҫ�Ҷ���
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
* 3˳�ĸ���
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

	//���3˳�Ƿ��㹻
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

	//������һ���Ʋ���
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
* ���Ƶĸ���
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

	//������Ҫ�ҵ���ͬ�����ĵ�˳
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

	//û������Ҫ�ҵ���ͬ������˫˳
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
			//��Ҫ����
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

	//û������Ҫ�ҵ���ͬ������3˳
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
			//��Ҫ����
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

	//���Ҳ�ͬ�����ĵ�˳
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
			//��Ҫ����
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
				//���ư���ֵ������
				sort(finalShunCol.mutable_cards()->begin(), finalShunCol.mutable_cards()->end(), AnalyzeModle::CompareCardByValue);
				finalShunCol.set_real(PokerLogic::GetCardLogicVal(a_pbTable, finalShunCol.cards(0)));

				AnalyzeModle::ExecuteShouAction(a_pbTable, a_pbSeat, finalShunCol);
				return true;
			}
		}
	}

	//���Ҳ�ͬ������˫˳
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
			//��Ҫ����
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
				//���ư���ֵ������
				sort(finalShunCol.mutable_cards()->begin(), finalShunCol.mutable_cards()->end(), AnalyzeModle::CompareCardByValue);
				finalShunCol.set_real(PokerLogic::GetCardLogicVal(a_pbTable, finalShunCol.cards(0)));

				AnalyzeModle::ExecuteShouAction(a_pbTable, a_pbSeat, finalShunCol);
				return true;
			}	
		}
	}

	//���Ҳ�ͬ������3˳
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
			//��Ҫ����
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
				//���ư���ֵ������
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
* ˫˳�ĸ���
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

	//ֻ��ҪѰ�������ֳɵ�˫˳����Ϊ��˳�Ѿ����ϲ��ˣ�
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

	//����ֻ�ǵõ����ڻ��ߵ���num�ģ���Ҫ�޳����ϴ���ȵ�num
	if (!tmpShouCol_ShuangShun.has_real())
	{
		return false;
	}

	//�ϳ����յ�����
	PBShouColInfo finalShunCol;
	finalShunCol.set_cardtype(EN_POKER_TYPE_STRAIGHT_2);
	finalShunCol.set_num(iPreNum);
	//ֻ��Ҫ��num
	for (int i = 0; i < iPreNum; i++)
	{
		//��Ҫ���ҵ�ֵ
		int iCurrentVal = 0;
		if (tmpShouCol_ShuangShun.real() <= iPreValue)
		{
			iCurrentVal = iPreValue + 1 + i;
		}
		else
		{
			iCurrentVal = tmpShouCol_ShuangShun.real() + i;
		}

		//ÿ����Ҫ����2��
		for (int j = 0; j < 2; j++)
		{
			int iDelCard = AnalyzeModle::RemoveForVectByValue(a_pbTable, *tmpShouCol_ShuangShun.mutable_cards(), iCurrentVal);
			if (iDelCard != -1)
			{
				finalShunCol.add_cards(iDelCard);
			}
		}
	}
	//���ư���ֵ������
	sort(finalShunCol.mutable_cards()->begin(), finalShunCol.mutable_cards()->end(), AnalyzeModle::CompareCardByValue);
	finalShunCol.set_real(PokerLogic::GetCardLogicVal(a_pbTable, finalShunCol.cards(0)));

	AnalyzeModle::ExecuteShouAction(a_pbTable, a_pbSeat, finalShunCol);
	return true;
}

/*
* ���ӵĸ���
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
* ���Ƶĸ���
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

	//���ǰ���Ǹ����Ƕ��ѣ�����2��
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
* ը���ĸ���
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
		//���ǰ���Ǹ����Ƕ��ѣ�����ը��
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
* ˫��ը���ĸ���
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
		//���ǰ���Ǹ����Ƕ��ѣ�����ը��
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
* ���ƽڵ�
*/
bool JiaoPaiBehaviorNode::Judge(PBSDRGameTable &a_pbTable, PBSDRTableSeat &a_pbSeat)
{
	//���״̬
	if (a_pbTable.state() != EN_TABLE_STATE_WAIT_QIANG_DI_ZHU)
	{
		return false;
	}

	if (a_pbTable.operation_index() != a_pbSeat.index())
	{
		return false;
	}

	//��Ҫ����ֵ���������
	//���ò���ѡ�����г��Ʋ���
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
* ����
*/
bool JiaoPaiNode::JiaoNode::Process(PBSDRGameTable &a_pbTable, PBSDRTableSeat &a_pbSeat)
{
	//���Ϊ8��
	//ը��Ϊ6��
	//����Ϊ4��
	//С��Ϊ3��
	//һ��2Ϊ2��
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

	//�鿴ѡ�������������ǽе���
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

	//���ڵ���7��ʱ�е�����������
	//���ڵ���5��ʱ�е���������
	//С��5�ֲ��в���
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
* pass�ڵ�
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

	//��Ҫ����ֵ���������
	//���ò���ѡ�����г��Ʋ���
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
* ����pass
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
* ����pass
*/
bool PassNode::BuChuNode::Process(PBSDRGameTable &a_pbTable, PBSDRTableSeat &a_pbSeat)
{
	if (a_pbTable.state() != EN_TABLE_STATE_PLAYING)
	{
		return false;
	}

	return AnalyzeModle::ExecuteAction(a_pbTable, a_pbSeat, EN_SDR_ACTION_PASS);
}


