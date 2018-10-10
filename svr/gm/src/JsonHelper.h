#pragma once
#include <string>
#include "poker_msg.pb.h"
#include "Packet.h"
#include "json/json.h"
#include "TCPSocketHandler.h"

using namespace std;

typedef std::map<int, CTCPSocketHandler*> PHPSocketMap;

class JsonHelper
{
	enum ENJsonMsgId
	{
		EN_Json_Update_Chips = 0,
		EN_Json_Update_MultiChips = 1,
		EN_Json_Request_User_Info = 3,
		EN_Json_Push_Msessage = 4,
		EN_Json_Update_User_Info = 5,
		EN_Json_Update_Limit = 10,
		EN_Json_Request_Apply_Join_TeaBar = 11,
		EN_Json_Request_Del_TeaBar_Table = 30,
		EN_Json_Request_Transfer_TeaBar = 31,//转让茶馆
		EN_Json_Request_Table_Log = 6,// 牌局日志
		EN_Json_Request_Dissolve_Table = 7,// 解散牌桌
		EN_Json_Request_Query_TeaBar_User_List = 42,    //查询茶馆内用户列表
		EN_Json_Request_Update_User_Items = 19,
		EN_Json_Update_Diamond = 43,   //更新砖石
		EN_Json_Update_Bonus = 44,   //更新奖金
		EN_Json_Update_Activity_User_Game_Info = 45,   //更行用户的闯关属性（等级和状态）
		EN_Json_Notify_Share_Success = 46,   //通知分享成功
		EN_Json_Request_Activity_User_Game_Info = 47,   //拉取用户闯关信息
		EN_Json_Update_Coins = 48,	//更新金币
		EN_Json_Update_Coins_And_Diamond = 49	//同时更新金币和砖石，兑换功能，方便php调用
    };
public:
    static bool JsonToProto(InputPacket input, PBHead& head, PBCSMsg& msg, PBRoute& route);
private:
    static bool GenerateUpdateChipsMsg(const Json::Value& root, PBHead& head, PBCSMsg& msg, PBRoute& route);
	static bool GenerateUpdateLimitMsg(const Json::Value & root, PBHead & head, PBCSMsg & msg,PBRoute & route);
	static bool GenerateRequestUserInfo(const Json::Value& root, PBHead& head, PBCSMsg& msg, PBRoute& route);
	static bool GenerateRequestMultiChipsMsg(const Json::Value & root,PBHead & head,PBCSMsg & msg,PBRoute & route);
	static bool GeneratePushMessageMsg(const Json::Value& root, PBHead& head, PBCSMsg& msg, PBRoute& route);
	static bool GenerateUpdateUserInfo(const Json::Value& root, PBHead& head, PBCSMsg& msg, PBRoute& route);
	static bool GenerateRequestApplyJoinTeaBar(const Json::Value& root, PBHead& head, PBCSMsg& msg, PBRoute& route);
	static bool GenerateRequestDelTeaBarTable(const Json::Value& root, PBHead& head, PBCSMsg& msg, PBRoute& route);
	static bool GenerateRequestTransferTeaBar(const Json::Value& root, PBHead& head, PBCSMsg& msg, PBRoute& route);

    static bool GenerateRequestTableLogMsg(const Json::Value& root, PBHead& head, PBCSMsg& msg, PBRoute& route);
    static bool GenerateRequestDissolveTableMsg(const Json::Value& root, PBHead& head, PBCSMsg& msg, PBRoute& route);
    static bool GenerateRequestQueryTeaBarUserList(const Json::Value& root, PBHead& a_pbHead, PBCSMsg& a_pbMsg, PBRoute& a_pbRoute);
    static bool GenerateUpdateSkipMatchAttributeMsg(const Json::Value& a_jvRoot, PBHead & a_pbHead, PBCSMsg & a_pbMsg, PBRoute & a_pbRoute);
    static bool GenerateUpdateBonusMsg(const Json::Value& a_jvRoot, PBHead & a_pbHead, PBCSMsg & a_pbMsg, PBRoute & a_pbRoute);
    static bool GenerateUpdateDiamondMsg(const Json::Value& a_jvRoot, PBHead & a_pbHead, PBCSMsg & a_pbMsg, PBRoute & a_pbRoute);
    static bool GenerateNotifyShareSuccessMsg(const Json::Value& a_jvRoot, PBHead & a_pbHead, PBCSMsg & a_pbMsg, PBRoute & a_pbRoute);
public:
    static bool ProtoToJson(const PBHead& head, const PBCSMsg& msg, std::string& json_msg);
private:
    static bool GenerateUpdateRsp(const PBCSMsg& msg, std::string& json_msg);
    static bool GenerateGetUserDataRsp(const PBCSMsg& msg, std::string& json_msg);
    static bool GenerateRequestApplyJoinTeaBarRsp(const PBCSMsg& msg, std::string& json_msg);

    static bool GenerateTableLogRsp(const PBCSMsg& msg, std::string& json_msg);
    static bool GenerateDissolveTableRsp(const PBCSMsg& msg, std::string& json_msg);
    static bool GenerateResponseQueryTeaBarUserList(const PBCSMsg& msg, std::string& json_msg);
    static bool GenerateUpdateUserItems(const Json::Value& root, PBHead& head, PBCSMsg& msg, PBRoute& route);
    static bool GenerateRequestUserSkipMatchInfo(const Json::Value& root, PBHead& head, PBCSMsg& msg, PBRoute& route);
    static bool GenerateUpdateUserSkipMatchDataRsp(const PBCSMsg& a_pbMsg, std::string& a_szJsonMsg);
	/*
	更新金币
	*/
	static bool GenerateUpdateCoinsMsg(const Json::Value& a_jvRoot, PBHead & a_pbHead, PBCSMsg & a_pbMsg, PBRoute & a_pbRoute);

	/*
	兑换金币和砖石
	*/
	static bool GenerateUpdateCoinAndDiamond(const Json::Value& a_jvRoot, PBHead & a_pbHead, PBCSMsg & a_pbMsg, PBRoute & a_pbRoute);
public:
    static PHPSocketMap _socketmap;
};

