#pragma once
#include "TableModle.h"
#include "PokerLogic.h"

class AnalyzeModle
{
public:
	/*
	* 分析出其玩家手牌的牌张
	* return : 返回手数
	*/
	static int AnalyzeSeatPaiZhang(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat);

	/*
	* 循环将手牌置为手
	* return : 手数
	*/
	static int TryTransformShou(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat,
		RepeatedPtrField<PBShouColInfo> & a_shouCards, RepeatedField<int> & a_residualCards, bool & a_bIsEnd);

	/*
	* 将一个col添加
	*/
	static bool CreateShouFromCards(RepeatedPtrField<PBShouColInfo> & a_shouCards, RepeatedField<int> & a_residualCards,
		int a_cardtype, int a_real, int a_num, const RepeatedField<int> & a_cards);

	/*
	* 在vect中找到值和类型对应的col
	*/
	static bool FindNextColByValueAndType(const RepeatedPtrField<PBShouColInfo> & a_shouCards, PBShouColInfo & a_tmpColInfo);

	/*
	* 将RepeatedPtrField中的一些手组合为一组手
	*/
	static bool CreateShouFromOtherShou(RepeatedPtrField<PBShouColInfo> & a_shouCards, const vector<PBShouColInfo> & a_srcVect,
		int a_cardtype, int a_real, int a_num);

	/*
	* 删除某个vect中的牌
	*/
	static int RemoveForVect(RepeatedField<int> & a_vect, int a_iCard);

	/*
	* 删除某个vect中的值
	*/
	static int RemoveForVectByValue(const PBSDRGameTable & a_pbTable, RepeatedField<int> & a_vect, int a_iVal);

	/*
	* 排序原则
	*/
	static bool CompareShouCol(const PBShouColInfo & a_shouCol_A, const PBShouColInfo & a_shouCol_B)
	{
		return a_shouCol_A.real() < a_shouCol_B.real();
	}

	/*
	* 牌值的排序原则
	*/
	static bool CompareCardByValue(google::protobuf::int32 a_iCard_A, google::protobuf::int32 a_iCard_B)
	{
		bool bRet = false;
		PBSDRGameTable table;
		bRet = PokerLogic::GetCardLogicVal(table, a_iCard_A) < PokerLogic::GetCardLogicVal(table, a_iCard_B);
		return bRet;
	}

	/*
	* 执行某一手的动作
	*/
	static bool ExecuteShouAction(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat, const PBShouColInfo & a_shouCol);

	/*
	* 从3顺中拆一组3条
	*/
	static bool Separate3TiaoFrom3Shun(const PBSDRGameTable & a_pbTable, PBShouColInfo & a_Src3ShunCol, PBShouColInfo & a_Des3TiaoCol, int a_iMinValue);

	/*
	* 将某个col中得cards全部添加到另一个col的cards中取
	*/
	static void AddCardsToDesFromSrc(const PBShouColInfo & a_Src3ShunCol, PBShouColInfo & a_Des3TiaoCol);

	/*
	* 执行某个动作
	*/
	static bool ExecuteAction(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat, int a_iActType);
};