#pragma once
#include "RedisServer.h"
#include <vector>
#include <string>
#include "poker_msg.pb.h"

using namespace std;

class CUserRedisServer:public CRedisServer
{
	public:
		CUserRedisServer();
		~CUserRedisServer();
		int GetNewUID(long long & uid);
        int CheckAccountToken(string account, int acc_type, string token);
		int QueryAccountUid(string account, int acc_type, long long & uid);
		int SetAccountUID(const string & account, int acc_type, long long uid);

		int _QueryDataField(long long uid, int key, char * buff,int & len);
		int _SaveDataField(long long uid, int key, const char* buff,int len);

		void _UpdateRankList(int rank_id, long long rank_key, long long rank_score);
};

