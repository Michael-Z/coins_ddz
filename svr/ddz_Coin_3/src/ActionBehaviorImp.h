#pragma once
#include "Behavior.h"
#include "TableModle.h"
#include "AnalyzeModle.h"
#include "PokerLogic.h"

/*
* 出牌节点（条件节点）
*/
class ChupaiBehaviorNode : public ConditionBehaviorNode
{
public:
	bool Judge(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat);
};

/*
* 出牌实现节点（选择节点）
*/
typedef SelectBehaviorNode ChupaiBehaviorNodeImp;

/*
* 出牌node
* 总的出牌类都写在这里面
*/
class ChupaiNode
{
public:
	/*
	* 炸弹的出牌
	*/
	class BombNode : public ActionBehaviorNode
	{
	public:
		virtual bool Process(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat);
	};

	/*
	* 双王炸弹的出牌
	*/
	class BombOfJokerNode : public ActionBehaviorNode
	{
	public:
		virtual bool Process(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat);
	};

	/*
	* 3条的出牌节点(行为节点)
	*/
	class TripleNode : public ActionBehaviorNode
	{
	public:
		virtual bool Process(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat);
	};

	/*
	* 3顺的出牌节点（行为节点）
	*/
	class Straight_3Node : public ActionBehaviorNode
	{
	public:
		virtual bool Process(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat);
	};

	/*
	* 连牌的出牌
	*/
	class StraightNode : public ActionBehaviorNode
	{
	public:
		virtual bool Process(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat);
	};

	/*
	* 双顺的出牌
	*/
	class Straight_2Node : public ActionBehaviorNode
	{
	public:
		virtual bool Process(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat);
	};

	/*
	* 对子出牌
	*/
	class PairNode : public ActionBehaviorNode
	{
	public:
		virtual bool Process(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat);
	};

	/*
	* 单牌出牌
	*/
	class SingleNode : public ActionBehaviorNode
	{
	public:
		virtual bool Process(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat);
	};
};

/*
* 跟牌节点（条件节点）
*/
class GenpaiBehaviorNode :public ConditionBehaviorNode
{
public:
	bool Judge(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat);
};

/*
* 跟牌实现节点（选择节点）
*/
typedef SelectBehaviorNode GenpaiBehaviorNodeImp;

class GenPaiNode
{
public:
	/*
	* 三条的跟牌
	*/
	class TripleNode : public ActionBehaviorNode
	{
	public:
		virtual bool Process(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat);
	};

	/*
	* 三顺的跟牌
	*/
	class Straight_3Node : public ActionBehaviorNode
	{
	public:
		virtual bool Process(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat);
	};

	/*
	* 连牌的跟牌
	*/
	class StraightNode : public ActionBehaviorNode
	{
	public:
		virtual bool Process(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat);
	};

	/*
	* 双顺的跟牌
	*/
	class Straight_2Node : public ActionBehaviorNode
	{
	public:
		virtual bool Process(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat);
	};

	/*
	* 对子的跟牌
	*/
	class PairNode : public ActionBehaviorNode
	{
	public:
		virtual bool Process(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat);
	};

	/*
	* 单牌的跟牌
	*/
	class SingleNode : public ActionBehaviorNode
	{
	public:
		virtual bool Process(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat);
	};

	/*
	* 炸弹的跟牌
	*/
	class BombNode : public ActionBehaviorNode
	{
	public:
		virtual bool Process(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat);
	};

	/*
	* 双王炸弹的跟牌
	*/
	class BombOfJokerNode : public ActionBehaviorNode
	{
	public:
		virtual bool Process(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat);
	};
};

/*
* 叫牌节点（条件节点）
*/
class JiaoPaiBehaviorNode : public ConditionBehaviorNode
{
public:
	bool Judge(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat);
};

/*
* 叫牌实现节点（选择节点）
*/
typedef SelectBehaviorNode JiaopaiBehaviorNodeImp;

/*
* 叫牌
*/
class JiaoPaiNode
{
public:
	/*
	* 叫牌
	*/
	class JiaoNode : public ActionBehaviorNode
	{
	public:
		virtual bool Process(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat);
	};
};

/*
* pass节点（条件节点）
*/
class PassBehaviorNode : public ConditionBehaviorNode
{
public:
	bool Judge(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat);
};

/*
* pass实现节点（选择节点）
*/
typedef SelectBehaviorNode PassBehaviorNodeImp;

class PassNode
{
public:
	/*
	* 不叫pass
	*/
	class BuJiaoNode : public ActionBehaviorNode
	{
	public:
		virtual bool Process(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat);
	};

	/*
	* 不出pass
	*/
	class BuChuNode : public ActionBehaviorNode
	{
	public:
		virtual bool Process(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat);
	};
};