#pragma once
#include "Behavior.h"
#include "TableModle.h"
#include "AnalyzeModle.h"
#include "PokerLogic.h"

/*
* ���ƽڵ㣨�����ڵ㣩
*/
class ChupaiBehaviorNode : public ConditionBehaviorNode
{
public:
	bool Judge(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat);
};

/*
* ����ʵ�ֽڵ㣨ѡ��ڵ㣩
*/
typedef SelectBehaviorNode ChupaiBehaviorNodeImp;

/*
* ����node
* �ܵĳ����඼д��������
*/
class ChupaiNode
{
public:
	/*
	* ը���ĳ���
	*/
	class BombNode : public ActionBehaviorNode
	{
	public:
		virtual bool Process(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat);
	};

	/*
	* ˫��ը���ĳ���
	*/
	class BombOfJokerNode : public ActionBehaviorNode
	{
	public:
		virtual bool Process(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat);
	};

	/*
	* 3���ĳ��ƽڵ�(��Ϊ�ڵ�)
	*/
	class TripleNode : public ActionBehaviorNode
	{
	public:
		virtual bool Process(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat);
	};

	/*
	* 3˳�ĳ��ƽڵ㣨��Ϊ�ڵ㣩
	*/
	class Straight_3Node : public ActionBehaviorNode
	{
	public:
		virtual bool Process(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat);
	};

	/*
	* ���Ƶĳ���
	*/
	class StraightNode : public ActionBehaviorNode
	{
	public:
		virtual bool Process(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat);
	};

	/*
	* ˫˳�ĳ���
	*/
	class Straight_2Node : public ActionBehaviorNode
	{
	public:
		virtual bool Process(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat);
	};

	/*
	* ���ӳ���
	*/
	class PairNode : public ActionBehaviorNode
	{
	public:
		virtual bool Process(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat);
	};

	/*
	* ���Ƴ���
	*/
	class SingleNode : public ActionBehaviorNode
	{
	public:
		virtual bool Process(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat);
	};
};

/*
* ���ƽڵ㣨�����ڵ㣩
*/
class GenpaiBehaviorNode :public ConditionBehaviorNode
{
public:
	bool Judge(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat);
};

/*
* ����ʵ�ֽڵ㣨ѡ��ڵ㣩
*/
typedef SelectBehaviorNode GenpaiBehaviorNodeImp;

class GenPaiNode
{
public:
	/*
	* �����ĸ���
	*/
	class TripleNode : public ActionBehaviorNode
	{
	public:
		virtual bool Process(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat);
	};

	/*
	* ��˳�ĸ���
	*/
	class Straight_3Node : public ActionBehaviorNode
	{
	public:
		virtual bool Process(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat);
	};

	/*
	* ���Ƶĸ���
	*/
	class StraightNode : public ActionBehaviorNode
	{
	public:
		virtual bool Process(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat);
	};

	/*
	* ˫˳�ĸ���
	*/
	class Straight_2Node : public ActionBehaviorNode
	{
	public:
		virtual bool Process(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat);
	};

	/*
	* ���ӵĸ���
	*/
	class PairNode : public ActionBehaviorNode
	{
	public:
		virtual bool Process(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat);
	};

	/*
	* ���Ƶĸ���
	*/
	class SingleNode : public ActionBehaviorNode
	{
	public:
		virtual bool Process(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat);
	};

	/*
	* ը���ĸ���
	*/
	class BombNode : public ActionBehaviorNode
	{
	public:
		virtual bool Process(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat);
	};

	/*
	* ˫��ը���ĸ���
	*/
	class BombOfJokerNode : public ActionBehaviorNode
	{
	public:
		virtual bool Process(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat);
	};
};

/*
* ���ƽڵ㣨�����ڵ㣩
*/
class JiaoPaiBehaviorNode : public ConditionBehaviorNode
{
public:
	bool Judge(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat);
};

/*
* ����ʵ�ֽڵ㣨ѡ��ڵ㣩
*/
typedef SelectBehaviorNode JiaopaiBehaviorNodeImp;

/*
* ����
*/
class JiaoPaiNode
{
public:
	/*
	* ����
	*/
	class JiaoNode : public ActionBehaviorNode
	{
	public:
		virtual bool Process(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat);
	};
};

/*
* pass�ڵ㣨�����ڵ㣩
*/
class PassBehaviorNode : public ConditionBehaviorNode
{
public:
	bool Judge(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat);
};

/*
* passʵ�ֽڵ㣨ѡ��ڵ㣩
*/
typedef SelectBehaviorNode PassBehaviorNodeImp;

class PassNode
{
public:
	/*
	* ����pass
	*/
	class BuJiaoNode : public ActionBehaviorNode
	{
	public:
		virtual bool Process(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat);
	};

	/*
	* ����pass
	*/
	class BuChuNode : public ActionBehaviorNode
	{
	public:
		virtual bool Process(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat);
	};
};