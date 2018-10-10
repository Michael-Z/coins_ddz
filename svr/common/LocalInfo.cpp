//#include "LocalInfo.h"
//
//
////具体过程是先通过ictol获取本地的所有接口信息，存放到ifconf结构中，再从其中取出每个ifreq表示的ip信息
//
///*
// * 获得单例
//*/
//LocalInfo * LocalInfo::Instance()
//{
//    return CSingleton<LocalInfo>::Instance();
//}
//
///*
// * 初始化
// *
// * 初始化测试服ip_vect
// * 如需添加测试服，则向m_vtTestIp中添加即可
//*/
//bool LocalInfo::Init()
//{
//    m_vtTestIp.push_back(SERVER_39_IP);
//    m_vtTestIp.push_back(SERVER_19_IP);
//    m_vtTestIp.push_back(SERVER_74_IP);
//    m_vtTestIp.push_back(SERVER_52_IP);
//
//    if(GetLocalIP() == -1)
//    {
//        ErrMsg("Get local ip failed");
//        return false;
//    }
//
//    return true;
//}
//
///*
// * 获得主机的ip
// */
//int LocalInfo::GetLocalIP()
//{
//    int iSocketFd;
//    struct ifconf ifconf;
//    char szBuf[512];
//    struct ifreq *ifreq;
//    char * pszIp;
//    char * pszName;
//
//    //初始化ifconf
//    ifconf.ifc_len = 512;
//    ifconf.ifc_buf = szBuf;
//
//    if((iSocketFd = ::socket(AF_INET, SOCK_DGRAM, 0)) < 0)
//    {
//        return -1;
//    }
//
//    ioctl(iSocketFd, SIOCGIFCONF, &ifconf); //获得接口信息
//    ::close(iSocketFd);
//
//    ifreq = (struct ifreq*)szBuf;
//    for (int i = (ifconf.ifc_len / sizeof(struct ifreq)); i > 0; i--)
//    {
//        pszName = ifreq->ifr_name;
//        pszIp = inet_ntoa(((struct sockaddr_in*)&(ifreq->ifr_addr))->sin_addr);
//
//        if(ifreq->ifr_flags == AF_INET)
//        {
//            if(strcmp(pszName,"eth1") == 0) //外网端口
//            {
//                strcpy(m_szIp,pszIp);
//            }
//            else if(strcmp(pszName,"eth0") == 0)    //内网端口
//            {
//                strcpy(m_szInnerIp,pszIp);
//            }
//
//            ifreq++;
//        }
//    }
//
//    VLogMsg(CLIB_LOG_LEV_DEBUG,"ip : %s", m_szIp);
//    VLogMsg(CLIB_LOG_LEV_DEBUG,"innerip : %s", m_szInnerIp);
//
//    return 0;
//}
//
///*
// * 判定是否是测试环境
// */
//bool LocalInfo::JudInTestEnv()
//{
//    for(unsigned int i = 0 ; i < m_vtTestIp.size() ; i ++)
//    {
//        char * pszTestIp = m_vtTestIp[i];
//        if(strcmp(pszTestIp,m_szIp) == 0)
//        {
//            VLogMsg(CLIB_LOG_LEV_DEBUG,"is_test_local");
//            return true;
//        }
//    }
//
//    return false;
//}

#include "LocalInfo.h"

/*
 * 获得单例
*/
LocalInfo * LocalInfo::Instance()
{
    return CSingleton<LocalInfo>::Instance();
}

/*
获得hostname
*/
bool LocalInfo::GetHostName(char * a_pszHostName, int a_iHostNameSize)
{
	return gethostname(a_pszHostName, a_iHostNameSize) == 0;
}

/*
初始化
*/
bool LocalInfo::Init()
{
	/*
	添加本地测试服
	*/
	m_MyTestHostNameVect.push_back(TEST_19);
	m_MyTestHostNameVect.push_back(TEST_39);
	m_MyTestHostNameVect.push_back(TEST_52);

	//获得本地hostname
	memset(m_szHostName,0,sizeof(m_szHostName));
	if (GetHostName(m_szHostName, sizeof(m_szHostName)))
	{
		return false;
	}

	return true;
}

/*
查看是否是本地
*/
bool LocalInfo::JudInTestEnv()
{
	bool bInclude = false;
	for (unsigned int i = 0 ; i < m_MyTestHostNameVect.size() ; i ++)
	{
		string szTestName = m_MyTestHostNameVect.at(i);
		if (memcmp(szTestName.c_str(),m_szHostName,szTestName.size()) == 0)
		{
			bInclude = true;
		}
	}

	return bInclude;
}
