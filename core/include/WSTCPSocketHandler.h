#pragma once
#include "cache.h"
#include "DecoderBase.h"
#include "TCPSocketHandler.h"
#include "base64.h"
#include "sha1.h"
#include <sstream>
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
//#include "Timer_Handler_Base.h"


#define MAX_WEB_RECV_LEN            1024*400

#define MAGIC_KEY "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

//htpp
class CWSTCPSocketHandler  : public CTCPSocketHandler
{
	typedef std::map<std::string, std::string> HEADER_MAP;

	public:
		CWSTCPSocketHandler();
		virtual ~CWSTCPSocketHandler();
 
	public:
		virtual int InputNotify ();
		virtual int OutputNotify ();
		virtual int HangupNotify ();
		virtual int Send(const char * buff, int len);
		virtual int CreateNetfd();
		virtual int ConnectWithNetfd(const string & ip,int port);
		virtual int ConnectWithNetfd();
		int handshark(const char * buff, int len);
		int fetch_http_info(const char * buff);
		void parse_str(char *request);
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
		virtual int Connect(const string & ip,int port);
		void Disconnect();
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
		HEADER_MAP _header_map;
};

//https
class CWSSTCPSocketHandler : public CTCPSocketHandler
{
	typedef std::map<std::string, std::string> HEADER_MAP;

public:
	CWSSTCPSocketHandler();
	virtual ~CWSSTCPSocketHandler();

public:
	virtual int InputNotify();
	virtual int OutputNotify();
	virtual int HangupNotify();
	virtual int Send(const char * buff, int len);
	virtual int CreateNetfd();
	virtual int ConnectWithNetfd(const string & ip, int port);
	virtual int ConnectWithNetfd();
	int handshark(const char * buff, int len);
	int fetch_http_info(const char * buff);
	void parse_str(char *request);
	virtual void bindDecoder(CDecoderBase* decode);
protected:
	virtual int OnClose();
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
	void Disconnect();
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
	HEADER_MAP _header_map;

//ssl
private:
	SSL * m_ssl;
	//SSL_CTX * m_ctx;
	bool m_sslConnected_;
	bool m_shaked;
};


