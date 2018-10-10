#pragma once
#include "RedisServer.h"
#include <vector>
#include <string>
#include "poker_msg.pb.h"

using namespace std;

class CRecordRedisServer:public CRedisServer
{
public:
    CRecordRedisServer();
    ~CRecordRedisServer();

    bool QueryHashObject(const string & mainkey,const string & subkey, PBHashDataField & pb_hash_data_field );
    int _QueryHashObject(const string & mainkey,const string & subkey,char * buff,int & len);
    bool CheckExist(const string & mainkey,const string & subkey);
};

