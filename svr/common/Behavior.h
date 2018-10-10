//创建者：名字 zhuke
//时间：2018-09-13

#pragma once
#include <iostream>
#include <list>
#include <string>
#include "poker_msg_basic.pb.h"

using namespace std;

/*
#defind FF_RUN(T, funname) (T)->##funname()  ##用于把参数粘在一起
*/
#define CREATE_NODE(T) (new T()) 

class BehaviorBase
{
public:
	BehaviorBase()
	{
		_parents = NULL;
		memset(_name, 0, sizeof(_name));
	}

	BehaviorBase(string a_pStr)
	{
		_parents = NULL;
		memset(_name, 0, sizeof(_name));
		memcpy(_name, a_pStr.c_str(), a_pStr.size());
	}

	~BehaviorBase()
	{
	
	}

	/*
	* 销毁函数
	*/
	virtual void destory()
	{
		delete this;
	}

	/*
	* 获得其父节点
	*/
	BehaviorBase* GetParent()
	{
		return _parents;
	}

	/*
	* 设置其父节点
	*/
	void SetParent(BehaviorBase * a_parents)
	{
		_parents = a_parents;
	}

	/*
	* 运行
	* return : true 为成功， false 为失败
	*/
	virtual bool Process(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat) = 0;

	/*
	* 设置name
	*/
	void SetName(string a_pStr)
	{
		memcpy(_name, a_pStr.c_str(), a_pStr.size());
	}

	/*
	添加节点
	*/
	BehaviorBase& AddChildPtr(BehaviorBase* a_pNewChild)
	{
		this->AddChild(a_pNewChild);

		return *a_pNewChild;
	}

	/*
	返回上一级
	*/
	BehaviorBase& Back()
	{
		if (this->_parents)
		{
			return *this->_parents;
		}

		return *this;
	}

	/*
	<重写(组合)
	*/
	BehaviorBase& operator<(BehaviorBase* a_pNewChild)
	{
		return AddChildPtr(a_pNewChild);
	}

	/*
	>重写
	*/
	BehaviorBase& operator>(int)
	{
		return Back();
	}

	/*
	* 父节点
	*/
	BehaviorBase * _parents;
	char _name[256];

private:
	/*
	* 添加叶子节点
	* return : true 为成功， false 为失败
	* 子类中结构不一样，方法需要重写
	*/
	virtual bool AddChild(BehaviorBase* a_child) = 0;
};

/*
* 组合行为类(用于其后有多个叶子节点)
*/
class CompositeClass : public BehaviorBase
{
public:

	CompositeClass()
	{
		_parents = NULL;
	}

	CompositeClass(string a_pStr)
	{
		_parents = NULL;
		memset(_name, 0, sizeof(_name));
		memcpy(_name, a_pStr.c_str(), a_pStr.size());
	}

	~CompositeClass()
	{

	}

	/*
	* 销毁函数，会销毁其叶子节点及其本身
	*/
	virtual void destory()
	{
		list<BehaviorBase*>::iterator iter = _childs.begin();
		for (; iter != _childs.end(); iter++)
		{
			BehaviorBase * node = *iter;
			node->destory();
			node = NULL;
		}

		delete this;
	}

	/*
	* 获得子类节点
	*/
	list<BehaviorBase*> & GetChild()
	{
		return _childs;
	}

	virtual bool Process(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat) = 0;

private:
	/*
	* 叶子节点list
	*/
	list<BehaviorBase*> _childs;

	/*
	* 添加叶子节点函数,只能AddChild在内部调用
	*/
	void addChildslist(BehaviorBase* a_child)
	{
		_childs.push_back(a_child);
		a_child->_parents = this;
	}

	/*
	* 重写添加叶子节点函数
	*/
	virtual bool AddChild(BehaviorBase* a_child)
	{
		addChildslist(a_child);

		return true;
	}
};


/*
* 单点节点（用户只有一个叶子节点）
*/
class SingleClass : public BehaviorBase
{
public:
	SingleClass()
	{
		_child = NULL;
	}

	SingleClass(string a_pStr)
	{
		_parents = NULL;
		memset(_name, 0, sizeof(_name));
		memcpy(_name, a_pStr.c_str(), a_pStr.size());
	}

	~SingleClass()
	{

	}

	/*
	* 销毁函数，会销毁其叶子节点及其本身
	*/
	virtual void destory()
	{
		if (_child)
		{
			_child->destory();
			_child = NULL;
		}
		delete this;
	}

	/*
	* 获得子类节点
	*/
	BehaviorBase* GetChild()
	{
		return _child;
	}

	virtual bool Process(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat) = 0;
private:
	BehaviorBase * _child;

	/*
	* 设置叶子节点函数,只能AddChild在内部调用
	*/
	void SetChild(BehaviorBase* a_child)
	{
		_child = a_child;
		a_child->_parents = this;
	}

	/*
	* 重写添加叶子节点函数
	*/
	virtual bool AddChild(BehaviorBase* a_child)
	{
		if (_child && _child != a_child)
		{
			return false;
		}

		SetChild(a_child);

		return true;
	}
};

/*
* 选择节点，只会执行其中一个，成功则会返回
*/
class SelectBehaviorNode : public CompositeClass
{
public:
	SelectBehaviorNode()
	{

	}

	SelectBehaviorNode(string a_pStr)
	{
		_parents = NULL;
		memset(_name, 0, sizeof(_name));
		memcpy(_name, a_pStr.c_str(), a_pStr.size());
	}

	~SelectBehaviorNode()
	{

	}

	/*
	* 重写运行函数
	* 会遍历执行其叶子节点写的所有函数，成功则会终止，并且放回true，否则返回失败
	* return : true 为成功， false 为失败
	*/
	virtual bool Process(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat)
	{
		list<BehaviorBase*> list_t = GetChild();
		list<BehaviorBase*>::iterator iter = list_t.begin();
		for (; iter != list_t.end(); iter++)
		{
			BehaviorBase* node = *iter;
			if (node->Process(a_pbTable, a_pbSeat))
			{
				return true;
			}

		}
		return false;
	}
};

/*
* 顺序节点，会执行其后所有的节点，其中一个失败则会所有返回失败
*/
class SequenceBehaviorNode : public CompositeClass
{
public:
	SequenceBehaviorNode()
	{

	}

	SequenceBehaviorNode(string a_pStr)
	{
		_parents = NULL;
		memset(_name, 0, sizeof(_name));
		memcpy(_name, a_pStr.c_str(), a_pStr.size());
	}

	~SequenceBehaviorNode()
	{
	
	}

	/*
	* 重写运行函数
	* 会遍历执行其叶子节点写的所有函数，成功则会终止，并且放回true，否则返回失败
	* return : true 为成功， false 为失败
	*/
	virtual bool Process(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat)
	{
		list<BehaviorBase*> & list_c = GetChild();
		list<BehaviorBase*>::iterator iter = list_c.begin();
		for (; iter != list_c.end(); iter++)
		{
			BehaviorBase* node = *iter;
			if (!node->Process(a_pbTable, a_pbSeat))
			{
				return false;
			}

		}
		return true;
	}
};

/*
* 条件节点,只会执行其后节点的操作
*/
class ConditionBehaviorNode : public SingleClass
{
public:
	ConditionBehaviorNode()
	{
	}

	ConditionBehaviorNode(string a_pStr)
	{
		_parents = NULL;
		memset(_name, 0, sizeof(_name));
		memcpy(_name, a_pStr.c_str(), a_pStr.size());
	}

	~ConditionBehaviorNode()
	{
	
	}

	/*
	* 重写运行函数
	* 会遍历执行其叶子节点写的所有函数，成功则会终止，并且放回true，否则返回失败
	* return : true 为成功， false 为失败
	*/
	virtual bool Process(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat)
	{
		if (Judge(a_pbTable, a_pbSeat))
		{
			return GetChild()->Process(a_pbTable, a_pbSeat);
		}

		return false;
	}

	/*
	* 判定条件是否满足
	* 修改它
	*/
	virtual bool Judge(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat) = 0;
};


/*
* 行为节点，最终的执行节点
*/
class ActionBehaviorNode : public SingleClass
{
public:
	ActionBehaviorNode()
	{

	}

	ActionBehaviorNode(string a_pStr)
	{
		_parents = NULL;
		memset(_name, 0, sizeof(_name));
		memcpy(_name, a_pStr.c_str(), a_pStr.size());
	}

	~ActionBehaviorNode()
	{
	
	}

	/*
	* 需要其后自己重写
	*/
	virtual bool Process(PBSDRGameTable & a_pbTable, PBSDRTableSeat & a_pbSeat) = 0;
};
