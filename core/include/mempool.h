/******************************************************************************
  文 件 名   : mempool.h
  版 本 号   : v 0.0.1
  作    者   : BarretXia
  生成日期   : 2015年8月28日
  最近修改   :
  功能描述   : 内存池管理
  函数列表   :
  修改历史   :
  1.日    期   : 2015年8月28日
    作    者   : BarretXia
    修改内容   : 创建文件
******************************************************************************/

/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 外部变量说明                                 *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 外部函数原型说明                             *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 内部函数原型说明                             *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 全局变量                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 模块级变量                                   *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 常量定义                                     *
 *----------------------------------------------*/

/*----------------------------------------------*
 * 宏定义                                       *
 *----------------------------------------------*/

#pragma once
#include <map>
#include <list>
#include <set>

class CMemPool
{
public:
    CMemPool()
    {
        _allocated_size = 0;
        _water_mark = C_MAX_WATER_MARK;
    }
    ~CMemPool();

/*****************************************************************************
 函 数 名  : CMemPool.allocate
 功能描述  : 分配内存
 输入参数  : 
 		unsigned int size           所需要的内存大小
             unsigned int & result_size  	实际分配的内存大小
 输出参数  : 无
 返 回 值  : void* 
 成功:分配内存的地址,失败:NULL
 调用函数  : 
 被调函数  : 
 
 修改历史      :
  1.日    期   : 2015年8月28日
    作    者   : BarretXia
    修改内容   : 新生成函数

*****************************************************************************/
    void* allocate(unsigned int size, unsigned int & result_size);

/*****************************************************************************
 函 数 名  : CMemPool.recycle
 功能描述  : 回收内存
 输入参数  : 
 		void* mem          :待回收的内存地址
             unsigned mem_size  :内存大小
 输出参数  : 无
 返 回 值  : int
 成功:0,失败:非0
 调用函数  : 
 被调函数  : 
 
 修改历史      :
  1.日    期   : 2015年8月28日
    作    者   : BarretXia
    修改内容   : 新生成函数

*****************************************************************************/
    int recycle(void* mem, unsigned mem_size);
    
private:
    int extend(unsigned int size, std::list<void*>* l, std::set<void*>* s);
    int extend_new_size(unsigned int size);
    int release(unsigned int size, std::list<void*>* l, std::set<void*>* s);

    unsigned int release_size(unsigned int block_size, unsigned int free_count, unsigned int stub_count);
    unsigned int fit_block_size(unsigned int size)
    {
        unsigned int i = 10;
        size = (size>>10);
        for(; size; i++, size = (size>>1));
        return 1 << (i<10 ? 10 : i);
    }
    unsigned int fit_extend_set(unsigned size);
    int release_size(unsigned int mem_size);
    typedef std::map<unsigned int, std::list<void*>*> mml;
    typedef std::map<unsigned int, std::set<void*>*> mms;
    //空闲内存链表
    mml _free;
    //内存块存根
    mms _stub;
    //已分配内存总大小
    unsigned int _allocated_size;
    //内存分配最大水位
    unsigned int _water_mark;

    static const unsigned int C_MAX_POOL_SIZE = 2000 * (1 << 20); // 2G
    static const unsigned int C_MAX_WATER_MARK = 1 << 30; // 1G
};

