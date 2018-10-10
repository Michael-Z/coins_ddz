#include "RecordRedisClient.h"
#include "PBConfigBasic.h"
#include "Common.h"

RecordRedisClient * RecordRedisClient::Instance()
{
    return CSingleton<RecordRedisClient>::Instance();
}

bool RecordRedisClient::Init()
{
    if ( !PokerPBRecordRedisConfig::Instance()->Init("../../conf/record_redis.CFG") )
    {
        VLogMsg(CLIB_LOG_LEV_ERROR,"failed to load global redis config");
        return false;
    }
    if (Connect(PokerPBRecordRedisConfig::Instance()->ip(),PokerPBRecordRedisConfig::Instance()->port()) != 0 )
    {
        VLogMsg(CLIB_LOG_LEV_ERROR,"failed to connect to record redis[ip:%s,port:%d]",
                PokerPBRecordRedisConfig::Instance()->ip().c_str(),PokerPBRecordRedisConfig::Instance()->port());
        return false;
    }
    return true;
}

bool RecordRedisClient::GetHashMapValue(const string & mainkey,const string & subkey,string & value)
{
    m_pRedisReply = RedisVCommand("hget %s %s",mainkey.c_str(),subkey.c_str());
    if (!m_pRedisReply || m_pRedisReply->type != REDIS_REPLY_STRING)
    {
        if (m_pRedisReply) freeReplyObject(m_pRedisReply);
        return false;
    }
    value = m_pRedisReply->str;
    if (m_pRedisReply) freeReplyObject(m_pRedisReply);
    return true;
}

bool RecordRedisClient::GetHashMapValue(const string & mainkey,const string & subkey,int64 & value)
{
    m_pRedisReply = RedisVCommand("hget %s %s",mainkey.c_str(),subkey.c_str());
    if (!m_pRedisReply)
    {
        if (m_pRedisReply) freeReplyObject(m_pRedisReply);
        return false;
    }
    if(m_pRedisReply->type == REDIS_REPLY_STRING)
    {
        value = atoll(m_pRedisReply->str);
    }
    else if(m_pRedisReply->type == REDIS_REPLY_INTEGER)
    {
        value = m_pRedisReply->integer;
    }
    else
    {
        if (m_pRedisReply) freeReplyObject(m_pRedisReply);
        return false;
    }
    if (m_pRedisReply) freeReplyObject(m_pRedisReply);
    return true;
}

bool RecordRedisClient::SetHashMapValue(const string & mainkey,const string & subkey,int64 value)
{
    m_pRedisReply = RedisVCommand("hset %s %s %lld",mainkey.c_str(),subkey.c_str(),value);
    if (!m_pRedisReply)
    {
        if (m_pRedisReply) freeReplyObject(m_pRedisReply);
        return false;
    }
    if (m_pRedisReply) freeReplyObject(m_pRedisReply);
    return true;
}

bool RecordRedisClient::SetHashMapValue(const string & mainkey,const string & subkey,const string & value)
{
    m_pRedisReply = RedisVCommand("hset %s %s %s",mainkey.c_str(),subkey.c_str(),value.c_str());
    if (!m_pRedisReply)
    {
        if (m_pRedisReply) freeReplyObject(m_pRedisReply);
        return false;
    }
    if (m_pRedisReply) freeReplyObject(m_pRedisReply);
    return true;
}

bool RecordRedisClient::DelHashMapKey(const string & mainkey,const string & subkey)
{
    m_pRedisReply = RedisVCommand("hdel %s %s",mainkey.c_str(),subkey.c_str());
    if (!m_pRedisReply)
    {
        if (m_pRedisReply) freeReplyObject(m_pRedisReply);
        return false;
    }
    if (m_pRedisReply) freeReplyObject(m_pRedisReply);
    return true;
}

bool RecordRedisClient::GetAllHashMapKey(const string & mainkey,vector<string> & strvect)
{
    strvect.clear();
    m_pRedisReply = RedisVCommand("HKEYS %s",mainkey.c_str());

    if (!m_pRedisReply)
    {
        if (m_pRedisReply) freeReplyObject(m_pRedisReply);
        return false;
    }

    if(m_pRedisReply->type == REDIS_REPLY_NIL)
    {
        if (m_pRedisReply) freeReplyObject(m_pRedisReply);
        return true;
    }
    else if(m_pRedisReply->type == REDIS_REPLY_ARRAY)
    {
        for(unsigned int i=0;i<m_pRedisReply->elements;i++)
        {
            redisReply * ele = m_pRedisReply->element[i];
            if(ele->type == REDIS_REPLY_STRING)
            {
                strvect.push_back(ele->str);
            }
            else
            {
                ErrMsg("invalid hash sub key type[%d] for main key[%s]",ele->type,mainkey.c_str());
            }
        }
    }
    if (m_pRedisReply) freeReplyObject(m_pRedisReply);
    return true;
}


bool RecordRedisClient::GetAllHashMapKey(const string & mainkey,vector<int64> & tidvect)
{
    tidvect.clear();
    m_pRedisReply = RedisVCommand("HKEYS %s",mainkey.c_str());

    if (!m_pRedisReply)
    {
        if (m_pRedisReply) freeReplyObject(m_pRedisReply);
        return false;
    }

    if(m_pRedisReply->type == REDIS_REPLY_NIL)
    {
        if (m_pRedisReply) freeReplyObject(m_pRedisReply);
        return true;
    }
    else if(m_pRedisReply->type == REDIS_REPLY_ARRAY)
    {
        for(int i=0;i<m_pRedisReply->elements;i++)
        {
            redisReply * ele = m_pRedisReply->element[i];
            if(ele->type == REDIS_REPLY_INTEGER)
            {
                tidvect.push_back(ele->integer);
            }
            else if(ele->type == REDIS_REPLY_STRING)
            {
                tidvect.push_back(atoll(ele->str));
            }
            else
            {
                ErrMsg("invalid hash sub key type[%d] for main key[%s]",ele->type,mainkey.c_str());
            }
        }
    }
    if (m_pRedisReply) freeReplyObject(m_pRedisReply);
    return true;
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

bool RecordRedisClient::QueryHashObject(const string & mainkey,const string & subkey,PBHashDataField & pb_hash_data_field)
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

int RecordRedisClient::_QueryHashObject(const string & mainkey,const string & subkey,char * buff,int & len)
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

template <typename GLOBAL_DATA_TYPE>
static bool	SerializeData(const GLOBAL_DATA_TYPE & data,int key,PBRedisData & redis_data)
{
    static char _serialize_data_buffer[102400] = {0};
    static int _byte_size;
    _byte_size = data.ByteSize();
    if(_byte_size == 0 )
    {
        return true;
    }
    if(_byte_size > 102400)
    {
        ErrMsg("byte len exceed 10240.byte size %d",_byte_size);
        return false;
    }
    if (!data.SerializeWithCachedSizesToArray((google::protobuf::uint8*)_serialize_data_buffer))
    {
        ErrMsg("protobuf serialize failed, %d",key);
        return false;
    }
    redis_data.set_key(key);
    redis_data.set_buff(_serialize_data_buffer,_byte_size);
    return true;
}

bool RecordRedisClient::UpdateHashObject(const string & mainkey,const string & subkey,const PBHashDataField & pb_hash_data_field)
{
    PBRedisData pb_redis_data;
    bool bret = false;
    switch(pb_hash_data_field.kv_union_case())
    {
        case PBHashDataField::kLzmjTableFlowRecordItem:
        {
            bret = SerializeData(pb_hash_data_field.lzmj_table_flow_record_item(),pb_hash_data_field.kv_union_case(),pb_redis_data);
            break;
        }
        case PBHashDataField::kDssTableFlowRecordItem:
        {
            bret = SerializeData(pb_hash_data_field.dss_table_flow_record_item(),pb_hash_data_field.kv_union_case(),pb_redis_data);
            break;
        }
        case PBHashDataField::kSdrTableFlowRecordItem:
        {
            bret = SerializeData(pb_hash_data_field.sdr_table_flow_record_item(),pb_hash_data_field.kv_union_case(),pb_redis_data);
            break;
        }
        default:
        {
            VLogMsg(CLIB_LOG_LEV_ERROR,"invalid data field type:%d",pb_hash_data_field.kv_union_case());
            break;
        }
    }
    if (bret)
    {
        return _UpdateHashObject(mainkey,subkey,pb_redis_data);
    }
    return false;
}



int RecordRedisClient::_UpdateHashObject(const string & mainkey,const string & subkey,PBRedisData & data)
{
    m_pRedisReply = (redisReply *)RedisVCommand("HSET %s %s %b",mainkey.c_str(),subkey.c_str(),data.buff().c_str(),data.buff().size());
    if(m_pRedisReply == NULL)
    {
        return REPLY_ERROR;
    }

    int iret = REPLY_OK;

    if(m_pRedisReply != NULL)
    {
        freeReplyObject(m_pRedisReply);
    }
    return iret;
}

void RecordRedisClient::IncreaseHashObject(const string & mainkey,const string & subkey, int64_t inc)
{
    m_pRedisReply = (redisReply *)RedisVCommand("HINCRBY %s %s %lld", mainkey.c_str(), subkey.c_str(), inc);
    if (m_pRedisReply)
        freeReplyObject(m_pRedisReply);
}

int64_t RecordRedisClient::GetHashObject(const string & mainkey,const string & subkey)
{
    int64_t val = 0;
    m_pRedisReply = (redisReply *)RedisVCommand("HGET %s %s", mainkey.c_str(), subkey.c_str());
    if (m_pRedisReply && m_pRedisReply->type == REDIS_REPLY_STRING)
    {
        val = atoll(m_pRedisReply->str);
        freeReplyObject(m_pRedisReply);
    }
    return val;
}

bool RecordRedisClient::CheckExist(const string & mainkey,const string & subkey)
{
    m_pRedisReply = (redisReply *)RedisVCommand("HEXISTS %s %s", mainkey.c_str(), subkey.c_str());
    if (m_pRedisReply && m_pRedisReply->type == REDIS_REPLY_INTEGER)
    {
        freeReplyObject(m_pRedisReply);
        return (m_pRedisReply->integer == 1 ? true : false);
    }
    return false;
}
