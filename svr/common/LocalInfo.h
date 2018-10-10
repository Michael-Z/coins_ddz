//#pragma once
//
//#include <string>
//#include <iostream>
//#include <sys/ioctl.h>
//#include <arpa/inet.h>
//#include <netinet/in.h>
//#include "global.h"
//#include "sys/socket.h"
//#include "net/if.h"
//#include "singleton.h"
//
//using namespace std;
//
//#define SERVER_39_IP "114.215.188.39"
//#define SERVER_19_IP "112.124.120.19"
//#define SERVER_74_IP "218.244.138.174"
//#define SERVER_52_IP "47.99.100.52"
//
//class LocalInfo
//{
//public :
//    static LocalInfo * Instance(void);
//    bool Init();
//    bool JudInTestEnv();    //judge 是否是测试环境
//    int GetLocalIP();       //获得该server绑定的ip
//private:
//    vector<char*> m_vtTestIp;   //测试外网ip写死
//    char m_szIp[256];           //外网ip
//    char m_szInnerIp[256];      //内网ip
//};
//
///*
//struct ifconf{
//
//lint ifc_len;
//
//    union{
//
//        caddr_t  ifcu_buf
//
//        Struct   ifreq *ifcu_req;
//
//    }ifc_ifcu
//
//}
//*/
//
//
///*
//struct ifreq {
//    char ifr_name[IFNAMSIZ];
//    union {
//        struct sockaddr ifru_addr;
//        struct sockaddr ifru_dstaddr;
//        struct sockaddr ifru_broadaddr;
//        short ifru_flags;
//        int ifru_metric;
//        caddr_t ifru_data;
//    } ifr_ifru;
//};
//*/
//

#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <netdb.h>
#include <sys/socket.h>
#include <memory.h>
#include "global.h"
#include "singleton.h"

using namespace std;

/*
测试服需自行添加
在init中加入测试服vector
*/
const string TEST_19 = "arthur_test_19";
const string TEST_39 = "arthur_test_39";
const string TEST_52 = "arthur_test_52";

class LocalInfo
{
public:
	LocalInfo()
	{
	}

	~LocalInfo()
	{
	}

	/*
	获得单例
	*/
	static LocalInfo * Instance(void);

	/*
	初始化
	*/
	bool Init();

	/*
	查看是否是本地
	*/
	bool JudInTestEnv();    //judge 是否是测试环境
private:

	/*
	获得hostname
	*/
	bool GetHostName(char * a_pszHostName, int a_iHostNameSize);

	/*
	本地的测试地址
	*/
	char m_szHostName[256];

	/*
	本地的测试地址
	*/
	vector<string> m_MyTestHostNameVect;
};