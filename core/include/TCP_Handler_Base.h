/******************************************************************************
  文 件 名   : TCP_Handler_Base.h
  版 本 号   : v 0.0.1
  作    者   : BarretXia
  生成日期   : 2015年8月28日
  最近修改   :
  功能描述   : Tcp句柄基类.提供基本的获取设置端口,重连状态.接口.子类需要实现-
               OnClose,OnConnected等事件
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

#include "poller.h"
#include <string>
using namespace std;

enum CConnState
{
	CONN_IDLE,
	CONN_HANDSHARKED,
	CONN_FATAL_ERROR,
	CONN_DATA_ERROR,
	CONN_CONNECTING,
	CONN_DISCONNECT,
	CONN_CONNECTED,
	CONN_DATA_SENDING,
	CONN_DATA_RECVING,
	CONN_SEND_DONE,
	CONN_RECV_DONE,
	CONN_APPEND_SENDING,
	CONN_APPEND_DONE,
	CONN_XML_POLICY
};


class HandlerProxyBasic;
class CTCP_Handler_Base : public CPollerObject 
{
	public:
		CTCP_Handler_Base(){_reconnect = false;_pproxy = NULL;_ctype= -1;};
		virtual ~CTCP_Handler_Base(){};
 
	public:
		virtual int OnClose()=0;				// 连接断开后调用 注意:如果返回1则不会删除对象只会关闭连接
		virtual int OnConnected()=0;		// 首次Accept到fd后调用
		virtual int OnPacketComplete(const char * data, int len)=0; // 需解析完整数据包时调用
		
	public:
		inline struct sockaddr GetIP(){return this->_ip;}; 
		inline string GetSIP(){return this->_sip;}
		inline void SetIP(sockaddr * ip){ memcpy(&_ip,ip,sizeof(sockaddr));}; 
		inline void SetSIP(const string & ip){this->_sip = ip;};
		inline uint16_t GetPort(){return this->_port;}; 
		inline void SetPort(uint16_t port){ this->_port = port;}; 

		inline int GetNetfd(){return this->netfd;}; 
		inline void SetNetfd(int netfd){ this->netfd = netfd;}; 
		inline void EnableReconnect(){_reconnect = true;}
		inline void DisableReconnect(){_reconnect = false;}
		inline bool GetReconnect(){return _reconnect;}
	public:
		struct sockaddr _ip;
		string _sip;
		uint16_t	_port;
		bool _reconnect;//
		int _ctype;
	public:
		HandlerProxyBasic * _pproxy;
};

