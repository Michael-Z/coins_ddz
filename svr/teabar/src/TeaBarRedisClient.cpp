#include "TeaBarRedisClient.h"
#include "PBConfigBasic.h"
#include "Common.h"

CTeaBarRedisClient::CTeaBarRedisClient()
{
}

CTeaBarRedisClient::~CTeaBarRedisClient()
{
}

bool CTeaBarRedisClient::Init(const string& ip, int port)
{
	if (Connect(ip, port) != 0)
	{
		VLogMsg(CLIB_LOG_LEV_ERROR, "failed to connect to tea bar redis[ip:%s,port:%d]",ip.c_str(), port);
		return false;
	}
	return true;
}

template <typename DATA_TYPE>
bool UnSerializeData(DATA_TYPE & data,int key,char * buff,int len)
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


bool CTeaBarRedisClient::QueryObject(const string & key, PBHashDataField & pb_hash_data_field)
{
	static char buff[10*1024*1024] = {0};
	int len;
	int iret = _QueryObject(key,buff,len);
    if (iret == REPLY_OK || iret == REPLY_NULL)
    {
        if (iret == REPLY_OK)
        {
        	switch(pb_hash_data_field.kv_union_case())
    		{
			case PBHashDataField::kTeaBarData:
				{
					return UnSerializeData(*pb_hash_data_field.mutable_tea_bar_data(),
						pb_hash_data_field.kv_union_case(), buff, len);
				}
			case PBHashDataField::kTeaBarTableDetailStatistics:
				{
					return UnSerializeData(*pb_hash_data_field.mutable_tea_bar_table_detail_statistics(),
						pb_hash_data_field.kv_union_case(), buff, len);
				}
			default:
				{
					ErrMsg("unserialize data[%d] failed.", pb_hash_data_field.kv_union_case());
					break;
				}
    		}		            
        }
		return true;
    }
	return false;
}

int CTeaBarRedisClient::_QueryObject(const string & key, char * buff, int & len)
{
	m_pRedisReply = RedisVCommand("GET %s", key.c_str());
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
				if (m_pRedisReply->len > 10* 1024 * 1024)
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

template <typename DATA_TYPE>
static bool	SerializeData(const DATA_TYPE & data,int key,PBRedisData & redis_data)
{
	static char _serialize_data_buffer[10*1024*1024] = {0};
	static int _byte_size;
	_byte_size = data.ByteSize();
	if(_byte_size == 0 )
	{
		return true;
	}
	if(_byte_size > 10*1024*1024)
	{
		ErrMsg("byte len exceed byte size %d",_byte_size);
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

bool CTeaBarRedisClient::UpdateObject(const string & key,const PBHashDataField & pb_hash_data_field)
{
	PBRedisData pb_redis_data;
	bool bret = false;
	switch(pb_hash_data_field.kv_union_case())
	{
		case PBHashDataField::kTeaBarData:
			{
				bret = SerializeData(pb_hash_data_field.tea_bar_data(),pb_hash_data_field.kv_union_case(),pb_redis_data);
				break;
			}
		case PBHashDataField::kTeaBarTableDetailStatistics:
			{
				bret = SerializeData(pb_hash_data_field.tea_bar_table_detail_statistics(), pb_hash_data_field.kv_union_case(), pb_redis_data);
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
		return (_UpdateObject(key, pb_redis_data) == REPLY_OK);
	}
	return false;
}



int CTeaBarRedisClient::_UpdateObject(const string & key, PBRedisData & data)
{
	m_pRedisReply = (redisReply *)RedisVCommand("SET %s %b",key.c_str(),data.buff().c_str(),data.buff().size());
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

int CTeaBarRedisClient::GetMaxTbid(int64& max_tbid)
{
	m_pRedisReply = (redisReply *)RedisVCommand("GET MAX_TBID");
	if (m_pRedisReply == NULL)
	{
		return REPLY_ERROR;
	}
	int iret = REPLY_OK;

	if (m_pRedisReply->type == REDIS_REPLY_NIL)
	{
		max_tbid = -1;
	}
	else if (m_pRedisReply->type == REDIS_REPLY_STRING)
	{
		max_tbid = atoll(m_pRedisReply->str);
	}
	else
	{
		iret = REPLY_ERROR;
	}

	freeReplyObject(m_pRedisReply);
	return iret;
}

int CTeaBarRedisClient::SetMaxTbid(int64 max_tbid)
{
	char sztbid[100] = {0};
	snprintf(sztbid, sizeof(sztbid), "%lld", max_tbid);
	m_pRedisReply = (redisReply *)RedisVCommand("SET MAX_TBID %s", sztbid);
	if (m_pRedisReply == NULL)
	{
		return REPLY_ERROR;
	}
	freeReplyObject(m_pRedisReply);
	return REPLY_OK;
}

bool CTeaBarRedisClient::DeleteObject(const string & key)
{
	m_pRedisReply = (redisReply *)RedisVCommand("DEL %s", key.c_str());
	if (m_pRedisReply == NULL)
	{
		return false;
	}
	freeReplyObject(m_pRedisReply);
	return true;
}