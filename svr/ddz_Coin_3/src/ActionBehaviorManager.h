#pragma once
#include "Behavior.h"
#include "TableModle.h"
#include "singleton.h"
#include "PokerLogic.h"
#include "ActionBehaviorImp.h"

class BehaviorManager
{
public:
    static BehaviorManager * Instance();

    BehaviorManager(){_Begin_node = NULL;}
    ~BehaviorManager(){}

    /*
     * 设置跟节点
    */
    void SetBeginNode(BehaviorBase* _a_Begin_node);

    /*
     * 销毁
    */
    void destory();

    /*
     * 设计树
    */
    void SetTree();

    /*
     * 初始化
    */
    bool init();

	/*
	* 执行动作
	*/
	void DoAction(PBSDRGameTable & a_CTable, PBSDRTableSeat & a_pbSeat);
private:

    /*
     * 树的跟节点
    */
    BehaviorBase* _Begin_node;
};


