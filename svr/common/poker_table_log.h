#pragma once

#include <string>
#include <time.h>

#define    TIME_FORMAT     "[%H:%M:%S] "

template<int buf_size = 1024*4>
class TableLog
{
	char buf[buf_size];
    char tmpbuf[64];
	int pos;
    int level;
	
public:
	TableLog():pos(0),level(1){}
	
	int min_(int a, int b){return a<b?a:b;}
    
    TableLog &operator>>(int nLevel)
    {
        level = nLevel;
        if (0 == nLevel)
        {
            return *this;
        }
        memset(tmpbuf, 0, sizeof(tmpbuf));
        
        time_t rawtime;
        struct tm * timeinfo;
        
        time ( &rawtime );
        timeinfo = localtime ( &rawtime );
        
        strftime( tmpbuf, 64, TIME_FORMAT, timeinfo);
        
        int len = buf_size-pos-1;
        if(len <= 0){
            return *this;
        }
        len = min_(len, strlen(tmpbuf));
        memcpy(buf+pos, tmpbuf, len);
        pos += len;
        return *this;
    }
    
	TableLog &operator<<(int n)
	{
        if (0 == level)
        {
            return *this;
        }
        
		if(pos >= buf_size){
			return *this;
		}
		
		pos += snprintf(buf+pos, buf_size - pos - 1, "%d", n);
		return *this;
	}
	
	TableLog &operator<<(long long l)
	{
		if (0 == level)
		{
			return *this;
		}

		if(pos >= buf_size){
			return *this;
		}
		
		pos += snprintf(buf+pos, buf_size - pos - 1, "%lld", l);
		return *this;
	}

	TableLog &operator<<(long unsigned int l)
	{
		if (0 == level)
		{
			return *this;
		}

		if(pos >= buf_size){
			return *this;
		}
		
		pos += snprintf(buf+pos, buf_size - pos - 1, "%lu", l);
		return *this;
	}

	TableLog &operator<<(long l)
	{
		if (0 == level)
		{
			return *this;
		}

		if(pos >= buf_size){
			return *this;
		}
		
		pos += snprintf(buf+pos, buf_size - pos - 1, "%ld", l);
		return *this;
	}

	TableLog &operator<<(unsigned int n)
	{
		if (0 == level)
		{
			return *this;
		}

		if(pos >= buf_size){
			return *this;
		}
		
		pos += snprintf(buf+pos, buf_size - pos - 1, "%u", n);
		return *this;
	}
	
	TableLog &operator<<(const std::string &str)
	{
        if (0 == level)
        {
            return *this;
        }
		int len ;
		len = buf_size-pos-1;
		if(len <= 0){
			return *this;
		}
		len = min_(len, str.length());
		memcpy(buf+pos, str.c_str(), len);
		pos += len;
		return *this;
		
	}
	TableLog &operator<<(const char *str)
	{
        if (0 == level)
        {
            return *this;
        }
		int len ;
		if(str == NULL){
			return *this;
		}
		len = buf_size-pos-1;
		if(len <= 0){
			return *this;
		}
		len = min_(len, strlen(str));
		memcpy(buf+pos, str, len);
		pos += len;
		return *this;
		
	}
	
	int length() const
	{
		return pos;
	}

	void clear()
	{
		pos = 0;
	}

	void reset()
	{
		pos = 0;
		memset(buf, 0 ,sizeof(buf));
	}
	
	const char *c_str() const
	{
		return buf;
	}
};

