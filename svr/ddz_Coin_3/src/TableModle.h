#pragma once

#include "Table.h"
#include "type_def.h"
#include <vector>
#include <map>
#include <string>
#include <math.h>
#include <algorithm>
#include "poker_msg.pb.h"
#include "global.h"
#include "Message.h"
#include "RouteManager.h"

using google::protobuf::RepeatedField;
using google::protobuf::RepeatedPtrField;

using namespace std;

#define MIN(X,Y) ((X)>(Y)?(Y):(X))

typedef pair<int,int> PAIR;
class TableLogic
{
public:
    //玩家与座位
    static PBSDRTableSeat * FindEmptySeatInTable(CPBGameTable & table);
    static PBTableUser * FindUserInTable(CPBGameTable & table ,long long uid);
    static int FindIndexByUid(const CPBGameTable & table,long long uid);
    static void OnUserReconnect(CPBGameTable & table,long long uid, int connect_id = 1);
    static void SitDownOnTable(CPBGameTable & table,PBSDRTableSeat & seat,const PBUser & user, int connect_id = 1);
    static void LogoutTable(CPBGameTable & table,int index);
    // fpf
    static bool IsPlayerAbleDoAction(const PBSDRTableSeat & seat,PBSDRAction & action,int token);
    static int GetLastOperateSeatIndex(const CPBGameTable & table);
    static bool UpdateTableFlowRecord(CPBGameTable & table);
public:
    //牌桌状态机
    static void ProcessTable(CPBGameTable & table);
    static void GameStart_1(CPBGameTable & table);
    static void GameStart_1_2(CPBGameTable & table);
    static void GameStart_1_3(CPBGameTable & table);
    static void GameStart_1_4(CPBGameTable & table);
    static void GameStart_2(CPBGameTable & table);
    static void GameStart_2_2(CPBGameTable & table);
    static void GameStart_3(CPBGameTable & table);
    static void GameStart_4(CPBGameTable & table);
    static void GameStart_5(CPBGameTable & table);
    static void GameOver(CPBGameTable & table);
    static void GameFinish(CPBGameTable & table);
    static void OnDissolveTableTimeOut(CPBGameTable & table);
    static void OnWaitKouPaiTimeOut(CPBGameTable & table);
    static void OnWaitNaPaiTimeOut(CPBGameTable & table);
    // FPF
    static void AppendActionFlow(CPBGameTable & table,PBSDRTableSeat & seat, const PBSDRAction & action);
    static void AppendActionFlowForReplay(CPBGameTable & table,PBSDRTableSeat & seat, const PBSDRAction & action);
    static bool PlayerDoAction(CPBGameTable & table,PBSDRTableSeat & seat, PBSDRAction & request_action);
    static bool PlayerDoActionChupai(CPBGameTable & table,PBSDRTableSeat & seat, PBSDRAction & action);
    static bool PlayerDoActionPass(CPBGameTable & table,PBSDRTableSeat & seat, PBSDRAction & action);
    static bool PlayerDoActionMaiPai(CPBGameTable & table,PBSDRTableSeat & seat, PBSDRAction & action);
    static bool PlayerDoActionJiaBei(CPBGameTable & table,PBSDRTableSeat & seat, PBSDRAction & action);
    static bool PlayerDoActionQiangDiZhu(CPBGameTable & table,PBSDRTableSeat & seat, PBSDRAction & action);
    static bool PlayerDoActionMianZhan(CPBGameTable & table,PBSDRTableSeat & seat, PBSDRAction & action);
    static bool PlayerDoActionFanDiPai(CPBGameTable & table,PBSDRTableSeat & seat, PBSDRAction & action);
    static bool PlayerDoActionMingPai(CPBGameTable & table,PBSDRTableSeat & seat, PBSDRAction & action);
    static int DetermineAction(CPBGameTable & table,PBSDRTableSeat & seat,const PBSDRAction & request_action);
    static bool NeedWaitingOtherOpera(CPBGameTable & table,PBSDRTableSeat & request_seat,const PBSDRAction & request_action);
    // 下一个玩家拿牌
    static int ProcessChuPaiForCurrentSeat(CPBGameTable & table);
    static int ProcessChuPaiForNextSeat(CPBGameTable & table);

    static void RemoveActionFlow(CPBGameTable & table);
    static void RemoveLastActionFlow(CPBGameTable & table, int type);
    static PBSDRAction& GetLastActionInFlow(CPBGameTable & table);
    static const PBSDRAction* GetLastActionInFlow(const CPBGameTable & table);
    static const PBSDRAction* GetLastActionPtrInFlow(const CPBGameTable & table);
    // 判断自摸和他摸和他出 专用
    static const PBSDRAction* GetLastActionInFlow_ForXPai(const CPBGameTable & table);
    static const PBSDRAction* GetLastActionPtrInFlow(const CPBGameTable & table, int index);
    static const PBSDRAction* GetLastActionPtrInFlow(const CPBGameTable & table, ENSDRActionType type);
    // 预操作处理 有多个玩家可以操作时
    static bool ProcessAutoActionOnPassHu(CPBGameTable & table,PBSDRTableSeat & seat, PBSDRAction & action);
	static int ProcessAutoAction(PBSDRGameTable & table, const PBSDRTableSeat & seat, ENSDRActionType act_type);
    static int ProcessAutoActionX(CPBGameTable & table, const PBSDRTableSeat & seat, ENSDRActionType act_type);
public:
    //座位判断与状态统计
    static int GetPlayerNumByState(const CPBGameTable & table,ENSeatState expected_state);
public:
    //广播
    static void SendGameStartNotify(const CPBGameTable & table);
    static void SendGameFinishNotify(CPBGameTable & table);

    static void BroadcastActionFlow(const CPBGameTable & table);
    static void BroadcastActionFlow(const CPBGameTable & table, ENSDRActionType type);
    static void BroadcastActionFlowForReplay(const CPBGameTable & table);
    static void BroadcastTableMsg(const CPBGameTable & table,PBCSMsg & notify,int expected_uid = 0);
    static void CopyTableToNotify(const CPBGameTable & table,CSNotifyTableInfo & info,int64 uid);
    // fpf
    static void NotifyOperationChoiceForSeat(CPBGameTable & table,const PBSDRTableSeat & seat);
    static void BroadcastNextOperation(const CPBGameTable & table, int seat_index);
public:
    // 放炮罚
    static void ShuffleCards(CPBGameTable& table);
    static void DealCards(RepeatedField<int>& src, RepeatedField<int>& des, int num = 1);
    static void DealHandCards(CPBGameTable& table);
    static void SortHandCards(CPBGameTable& table);

    // 添加操作到列表
    static void AddChoise(PBSDRTableSeat & seat, const PBSDRAction & action);
    // 移除牌
    static bool RemoveCardForSeat(PBSDRTableSeat & seat, int card);

    static void ResetPlayerChoice(CPBGameTable & table);
    static void ResetPlayerChoice(CPBGameTable & table, int seat_index);

    // 出牌操作
    static void AddChupaiChoiceForSeat(CPBGameTable & table, PBSDRTableSeat & seat);

    // 获取牌的类型
    static int GetCardType(int card)
    {
        return GetHigh(card);
    }
    // 获取牌的大小
    static int GetCardValue(int card)
    {
        return GetLow(card);
    }

    static int GetHigh(int card)
    {
        return (card & 0xf0) >> 4;
    }
    static int GetLow(int card)
    {
        return (card & 0x0f);
    }

    // 合成牌
    static int ComposeCard(int h, int l)
    {
        int ret = (h << 4) | l;
        return ret;
    }
    static int CalcChoiceLevel(const PBSDRAction & action);

    // 是否为赖子
    static bool IsLaiZi(const PBSDRGameTable & table, int card);
    // 是否必抓
    static bool IsBiZhua(const CPBGameTable & table, const PBSDRTableSeat & seat);

    static string CardToString(int src);
    static string CardsToString(const RepeatedField<int> & src);
    static string ActionToString(const PBSDRAction & action);
    static void TestHu_3(CPBGameTable & table);
    static void GetLastXPai(const CPBGameTable& table, const PBSDRTableSeat& seat, bool& is_zimo, bool& is_tamo, bool& is_tachu);
    // 排行榜
    static void SendUpdateRankMsg(int rank_id, long long rank_key, long long rank_score);
    static bool NeedPayForTable(const CPBGameTable& table);
    static bool NeedPayForTable(int round, int state);
    static void SendInterEventOnDoActionOver(CPBGameTable& table,int type, const PBSDRAction& action);
    static const PBSDRAction* GetLastDeterminedActionChuPai(const CPBGameTable & table);
    // 检测单回结束
    static bool CheckSingleOver(const CPBGameTable& table, const PBSDRTableSeat& seat);
    static void AnalyzeHuStyles(CPBGameTable& table, PBSDRTableSeat & seat);

    /*
     * 创关
    */
    static int GetRealValue(int card)
    {
        if(GetCardValue(card) == 2)
        {
            return 15;
        }

        return GetCardValue(card);
    }

    /*
     * 获得桌子上玩家的数量
    */
    static int GetPlayerNum(const CPBGameTable & table);

    /*
     * 执行默认的动作
    */
    static void DoDefaultAction(CPBGameTable & a_CTable,PBSDRTableSeat & a_pbSeat);

    /*
     * 操作定时器到达后操作
    */
    static void OnOperationTimeOut(CPBGameTable & a_CTable);

    /*
     * 检查是否所有人托管结束
    */
    static bool CheckIsTrusteeshipOver(CPBGameTable & a_CTable);

    /*
     * 注册所有动作的延迟时间
    */
    static void RegisterAllActionWaitingTime(CPBGameTable & a_CTable);

    /*
     * 注册特定动作的延迟时间
     * a_iType : 动作类型
     * a_iMilliSecond ： 毫秒级时间
    */
    static void RegisterActionWaitingTime(CPBGameTable & a_CTable,int a_iType,int a_iMilliSecond);

    /*
     * 获得某个动作完成后等待时间
     * a_iType ： 动作类型
    */
    static int GetWaitingTime(CPBGameTable & a_CTable,int a_iType);

    /*
     * 将table上暂存的消息发送出去
    */
    static void OnActionWaitingTimerOut(CPBGameTable & a_CTable);

    /*
     * 检测用户是否离线定时器到达后
     * 自动做动作，并且置托管状态
    */
    static void OnCheckUserOffTimerOut(CPBGameTable & a_CTable);

    /*
     * 动作操作后处理托管状态
     * 返回true为结束
     * 返回false为继续当前行为
    */
    static bool DealWithTrusteeshipAfterAct(CPBGameTable & a_CTable,PBSDRTableSeat & a_pbSeat,const PBSDRAction & action);

    /*
     * 检查这个操作是否是过操作
    */
    static bool CheckActionIsPassAction(int a_iActType);

    /*
     * 进入托管状态
    */
    static void EnableTrusteeship(CPBGameTable & a_CTable,PBSDRTableSeat & a_pbSeat);

    /*
     * 离开托管状态
    */
    static void DisableTrusteeship(CPBGameTable & a_CTable,PBSDRTableSeat & a_pbSeat);

    /*
     * 清理人员
    */
    static void OnTableRecycleTimerOut(const CPBGameTable & a_CTable);


    //写牌
    /*
    * 判定写牌类型
    * bImComSetPileCards ： 是否是不完全写牌（同时写玩家和牌堆，或者只写玩家）
    * bComSetPileCards  ： 是否是完全写牌（只写牌堆）
    */
    static void JugWriteCardsType(CPBGameTable & table,bool & bImComSetPileCards,bool & bComSetPileCards);

    /*
    * 不完全写牌
    */
    static void ImComSetPileCards(CPBGameTable & table,RepeatedField<int> & rfTableCardCopy,int iHandNumMax);

    /*
    * 完全写牌
    */
    static void ComSetPileCards(CPBGameTable & table,RepeatedField<int> & rfTableCardCopy,int iHandNumMax);

    /*
    * 根据写牌类型，修改牌型，设置庄家，将牌
    */
    static void InterRoundData(CPBGameTable & table,RepeatedField<int> & rfTableCardCopy,int iHandNumMax,
                               bool bImComSetPileCards,bool bComSetPileCards);

    /*
    * 写牌总入口
    */
    static void WriteCard(CPBGameTable & table,RepeatedField<int> & rfTableCardCopy);

    static int DealCardByValue(RepeatedField < int > & rfSrc,RepeatedField < int > & rfDes,int iValue);

    static int FindCardFromVect(RepeatedField < int > & pfSrc,int iCard);

    /*
     * 未准备玩家将会被踢出
    */
    static void OnReadTimerTimeOut(CPBGameTable & a_pbTable);

	/*
	* 获得在添加出牌action_flow前的一个action_flow
	*/
	static const PBSDRAction* GetLastChuActionInFlowExceptEmptyChu(const PBSDRGameTable &a_pbTable);

	/*
	 * 机器人定时器
	*/
	static void OnRobotTimerTimeOut(CPBGameTable & a_pbTable);

	/*
	 * 在添加操作后的动作
	 * 如果是机器人则开启机器人定时器
	 * 如果是玩家则根据状态来执行后面的行为
	*/
	static void OnOperateAfterAddChoice(CPBGameTable & a_pbTable,PBSDRTableSeat & a_pbSeat);
	
	/*
	获得房间内的机器人
	*/
	static int GetRobotNum(const CPBGameTable & a_pbTable);

	/*
	* 等待其他玩家定时器
	*/
	static void OnWaitinOtherTimeOut(CPBGameTable & a_pbTable);

	/*
	检测该桌子上有无相同名字的机器人
	*/
	static bool CheckTableHasTheSameRobot(const CPBGameTable & a_pbTable, string a_stUserName);

	/*
	机器人是否能够进入该房间
	*/
	static bool CheckRobotAbleEnterTable(const CPBGameTable & a_pbTable, string a_stUserName);
};

