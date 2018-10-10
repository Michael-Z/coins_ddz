#pragma once
#include "TableModle.h"
#include "PokerLogic.h"

class AnalyzeModle
{
public:
	/*
	* ��������������Ƶ�����
	* return : ��������
	*/
	static int AnalyzeSeatPaiZhang(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat);

	/*
	* ѭ����������Ϊ��
	* return : ����
	*/
	static int TryTransformShou(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat,
		RepeatedPtrField<PBShouColInfo> & a_shouCards, RepeatedField<int> & a_residualCards, bool & a_bIsEnd);

	/*
	* ��һ��col���
	*/
	static bool CreateShouFromCards(RepeatedPtrField<PBShouColInfo> & a_shouCards, RepeatedField<int> & a_residualCards,
		int a_cardtype, int a_real, int a_num, const RepeatedField<int> & a_cards);

	/*
	* ��vect���ҵ�ֵ�����Ͷ�Ӧ��col
	*/
	static bool FindNextColByValueAndType(const RepeatedPtrField<PBShouColInfo> & a_shouCards, PBShouColInfo & a_tmpColInfo);

	/*
	* ��RepeatedPtrField�е�һЩ�����Ϊһ����
	*/
	static bool CreateShouFromOtherShou(RepeatedPtrField<PBShouColInfo> & a_shouCards, const vector<PBShouColInfo> & a_srcVect,
		int a_cardtype, int a_real, int a_num);

	/*
	* ɾ��ĳ��vect�е���
	*/
	static int RemoveForVect(RepeatedField<int> & a_vect, int a_iCard);

	/*
	* ɾ��ĳ��vect�е�ֵ
	*/
	static int RemoveForVectByValue(const PBSDRGameTable & a_pbTable, RepeatedField<int> & a_vect, int a_iVal);

	/*
	* ����ԭ��
	*/
	static bool CompareShouCol(const PBShouColInfo & a_shouCol_A, const PBShouColInfo & a_shouCol_B)
	{
		return a_shouCol_A.real() < a_shouCol_B.real();
	}

	/*
	* ��ֵ������ԭ��
	*/
	static bool CompareCardByValue(google::protobuf::int32 a_iCard_A, google::protobuf::int32 a_iCard_B)
	{
		bool bRet = false;
		PBSDRGameTable table;
		bRet = PokerLogic::GetCardLogicVal(table, a_iCard_A) < PokerLogic::GetCardLogicVal(table, a_iCard_B);
		return bRet;
	}

	/*
	* ִ��ĳһ�ֵĶ���
	*/
	static bool ExecuteShouAction(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat, const PBShouColInfo & a_shouCol);

	/*
	* ��3˳�в�һ��3��
	*/
	static bool Separate3TiaoFrom3Shun(const PBSDRGameTable & a_pbTable, PBShouColInfo & a_Src3ShunCol, PBShouColInfo & a_Des3TiaoCol, int a_iMinValue);

	/*
	* ��ĳ��col�е�cardsȫ����ӵ���һ��col��cards��ȡ
	*/
	static void AddCardsToDesFromSrc(const PBShouColInfo & a_Src3ShunCol, PBShouColInfo & a_Des3TiaoCol);

	/*
	* ִ��ĳ������
	*/
	static bool ExecuteAction(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat, int a_iActType);
};