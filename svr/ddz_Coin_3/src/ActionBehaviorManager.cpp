#include "ActionBehaviorManager.h"

BehaviorManager * BehaviorManager::Instance()
{
    return CSingleton<BehaviorManager>::Instance();
}

/*
 * 设置跟节点
*/
void BehaviorManager::SetBeginNode(BehaviorBase* _a_Begin_node)
{
    _Begin_node = _a_Begin_node;
}

/*
 * 销毁
*/
void BehaviorManager::destory()
{
    _Begin_node->destory();
}

/*
 * 初始化
*/
bool BehaviorManager::init()
{
    SetTree();

    return true;
}

/*
 * 设计树
*/
void BehaviorManager::SetTree()
{
    //跟节点为选择节点
    BehaviorBase* beginNode = new SelectBehaviorNode();
	beginNode->SetName("begin");
    SetBeginNode(beginNode);

				//出牌节点
	*beginNode	
				< CREATE_NODE(ChupaiBehaviorNode)	
													< CREATE_NODE(ChupaiBehaviorNodeImp)	
																							< CREATE_NODE(ChupaiNode::BombNode) > 0
																							< CREATE_NODE(ChupaiNode::BombOfJokerNode) > 0
																							< CREATE_NODE(ChupaiNode::TripleNode) > 0
																							< CREATE_NODE(ChupaiNode::Straight_3Node) > 0
																							< CREATE_NODE(ChupaiNode::StraightNode) > 0
																							< CREATE_NODE(ChupaiNode::Straight_2Node) > 0
																							< CREATE_NODE(ChupaiNode::PairNode) > 0
																							< CREATE_NODE(ChupaiNode::SingleNode) > 0 
													> 0 
				> 0
				//跟牌节点
				< CREATE_NODE(GenpaiBehaviorNode)	
													< CREATE_NODE(GenpaiBehaviorNodeImp)	
																							< CREATE_NODE(GenPaiNode::TripleNode) > 0
																							< CREATE_NODE(GenPaiNode::Straight_3Node) > 0
																							< CREATE_NODE(GenPaiNode::StraightNode) > 0
																							< CREATE_NODE(GenPaiNode::Straight_2Node) > 0
																							< CREATE_NODE(GenPaiNode::PairNode) > 0
																							< CREATE_NODE(GenPaiNode::SingleNode) > 0
																							< CREATE_NODE(GenPaiNode::BombNode) > 0
																							< CREATE_NODE(GenPaiNode::BombOfJokerNode) > 0 
													> 0 
				> 0
				//叫牌节点
				< CREATE_NODE(JiaoPaiBehaviorNode)	
													< CREATE_NODE(JiaopaiBehaviorNodeImp)	
																							< CREATE_NODE(JiaoPaiNode::JiaoNode) > 0 
													> 0 
				> 0
				//pass节点
				< CREATE_NODE(PassBehaviorNode)		
													< CREATE_NODE(PassBehaviorNodeImp)		
																							< CREATE_NODE(PassNode::BuJiaoNode) > 0
																							< CREATE_NODE(PassNode::BuChuNode) > 0 
													> 0
				> 0;
}
 
void BehaviorManager::DoAction(PBSDRGameTable & a_CTable, PBSDRTableSeat & a_pbSeat)
{
	AnalyzeModle::AnalyzeSeatPaiZhang(a_CTable, a_pbSeat);
	_Begin_node->Process(a_CTable,a_pbSeat);
}



