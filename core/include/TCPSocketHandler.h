#pragma once
#include "cache.h"
#include "DefaultStreamDecoder.h"
#include "TCP_Handler_Base.h"
//#include "Timer_Handler_Base.h"


#define MAX_WEB_RECV_LEN            1024*400

#define MAGIC_KEY "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

class CTCPSocketHandler  : public CTCP_Handler_Base
{
	public:
		CTCPSocketHandler();
		virtual ~CTCPSocketHandler();
 
	public:
		virtual int InputNotify();
		virtual int OutputNotify ();
		virtual int HangupNotify ();
		virtual int Send(const char * buff, int len);
		virtual int CreateNetfd();
		virtual int ConnectWithNetfd(const string & ip, int port);
		virtual int ConnectWithNetfd();
		virtual void bindDecoder(CDecoderBase* decode);
	protected:		
		virtual int OnClose() ;
		virtual int OnConnected();
		virtual int OnConnectedByRequest();
		virtual int OnReConnected();
		virtual int OnAttachPoller();
		virtual int OnPacketComplete(const char * data, int len);
	public:
		virtual void Reset();
	public:
		virtual int Connect();
		virtual int Connect(const string & ip, int port);
		virtual void Disconnect();
	protected:
		CConnState	_stage;
	public:
		CDecoderBase* _decode;

	private:
		virtual int handle_input();
		virtual int handle_output();

		CRawCache       _r;
		CRawCache       _w;
		bool _ever_been_connect;

};

class CLogSocketHandler  : public CTCP_Handler_Base
{
	public:
		CLogSocketHandler();
		virtual ~CLogSocketHandler();
 
	public:
		virtual int Init();
		virtual int InputNotify();
		virtual int OutputNotify ();
		virtual int HangupNotify ();
		int Send(const char * buff, int len);
	protected:
		virtual int OnClose() ;
		virtual int OnConnected() {return 0;};
		virtual int OnAttachPoller();
		virtual int OnPacketComplete(const char * data, int len);
		void Reset();
	public:
		int Connect();
		int Connect(const string & ip,int port);
        void DisConnect();
	protected:
		CConnState	_stage;

	private:
		int handle_input();
		int handle_output();

		CRawCache       _r;
		CRawCache       _w;

};

