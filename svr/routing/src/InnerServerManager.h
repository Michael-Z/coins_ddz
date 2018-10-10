#pragma once
#include <map>
#include "RouteHandlerToken.h"
#include "singleton.h"
using namespace std;

typedef map<int,RouteHandlerToken*> InnerServerMap;
typedef map<int,InnerServerMap> ServerGroupMap;
typedef map<int,ServerGroupMap> InnerServerCluster;
typedef map<int,int> WorkGroupMap;
class InnerServerManager : public CSingleton<InnerServerManager>
{
public :
	RouteHandlerToken * GetInnerServer(int groupid,int stype,int svid);
	RouteHandlerToken * GetInnerServerByHash(int groupid,int stype,long long key);
	void CheckNode(RouteHandlerToken * ptoken,const PBRoute & route);
	void RegistInnerServer(RouteHandlerToken * phandler,const PBRoute & route,int stype,int svid,int groupid);
	void OnSvrdClosed(RouteHandlerToken * phandler);
	int GetWorkingGroup(int stype);
public:
	InnerServerCluster _inner_cluster;
	WorkGroupMap _work_group;
};

