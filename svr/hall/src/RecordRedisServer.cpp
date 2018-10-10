#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>
#include "RecordRedisServer.h"
#include "global.h"

CRecordRedisServer::CRecordRedisServer()
{
}

CRecordRedisServer::~CRecordRedisServer()
{
}


template <typename GLOBAL_DATA_TYPE>
bool UnSerializeData(GLOBAL_DATA_TYPE & data,int key,char * buff,int len)
{
    PBRedisData redis_data;
    redis_data.set_key(key);
    if(len != 0)
    {
        redis_data.set_buff(buff,len);
    }
    if ( redis_data.has_buff() &&
         data.ParseFromArray(buff,len) )
    {
        return true;
    }
    else if(redis_data.has_buff())
    {
        //解析失败
        ErrMsg("fatal error.parse from array failed.");
    }
    else
    {
        //为空
        return true;
    }
    return false;
}

bool CRecordRedisServer::QueryHashObject(const string & mainkey,const string & subkey,PBHashDataField & pb_hash_data_field)
{
    char buff[102400] = {0};
    int len;
    int iret = _QueryHashObject(mainkey,subkey,buff,len);
    if (iret == REPLY_OK /*|| iret == REPLY_NULL*/)
    {
        if (iret == REPLY_OK)
        {
            switch(pb_hash_data_field.kv_union_case())
            {
                case PBHashDataField::kLzmjTableFlowRecordItem:
                {
                    return UnSerializeData(*pb_hash_data_field.mutable_lzmj_table_flow_record_item(),
                                           pb_hash_data_field.kv_union_case(),buff,len);
                }
                case PBHashDataField::kDssTableFlowRecordItem:
                {
                    return UnSerializeData(*pb_hash_data_field.mutable_dss_table_flow_record_item(),
                                           pb_hash_data_field.kv_union_case(),buff,len);
                }
                case PBHashDataField::kSdrTableFlowRecordItem:
                {
                    return UnSerializeData(*pb_hash_data_field.mutable_sdr_table_flow_record_item(),
                                           pb_hash_data_field.kv_union_case(),buff,len);
                }
                default:
                {
                    ErrMsg("unserialize data[%d] failed.",pb_hash_data_field.kv_union_case());
                    break;
                }
            }
            //if (UnSerializeData(&,int key,const PBRedisData & redis_data))
        }
        return true;
    }
    return false;
}

int CRecordRedisServer::_QueryHashObject(const string & mainkey,const string & subkey,char * buff,int & len)
{
    m_pRedisReply = RedisVCommand("hget %s %s",mainkey.c_str(),subkey.c_str());
    if(m_pRedisReply == NULL)
    {
        return REPLY_ERROR;
    }
    int iret = REPLY_OK;
    switch(m_pRedisReply->type)
    {
        case REDIS_REPLY_NIL:
        {
            iret = REPLY_NULL;
        }
            break;
        case REDIS_REPLY_STRING:
        {
            if(m_pRedisReply->len == 0)
            {
                iret = REPLY_NULL;
            }
            if(m_pRedisReply->len > 102400)
            {
                iret = REPLY_TYPE_STR_EXCEED_LEN;
            }
            else
            {
                memcpy(buff,m_pRedisReply->str,m_pRedisReply->len);
                len = m_pRedisReply->len;
                iret = REPLY_OK;
            }
        }
            break;
        case REDIS_REPLY_ARRAY:
        case REDIS_REPLY_INTEGER:
        case REDIS_REPLY_STATUS:
        {
            iret = REPLY_TYPE_ERROR;
        }
        case REDIS_REPLY_ERROR:
        {
            iret = REPLY_ERROR;
        }
    }
    if(m_pRedisReply != NULL)
    {
        freeReplyObject(m_pRedisReply);
    }
    return iret;
}

bool CRecordRedisServer::CheckExist(const string & mainkey,const string & subkey)
{
    m_pRedisReply = (redisReply *)RedisVCommand("HEXISTS %s %s", mainkey.c_str(), subkey.c_str());
    if (m_pRedisReply && m_pRedisReply->type == REDIS_REPLY_INTEGER)
    {
        freeReplyObject(m_pRedisReply);
        return (m_pRedisReply->integer == 1 ? true : false);
    }
    return false;
}



