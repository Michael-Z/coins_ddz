#include "ProcesserFactoryBasic.h"
#include "poker_msg.pb.h"
#include "SessionManager.h"

int ProcesserFactoryBasic::Process(CHandlerTokenBasic * ptoken,CSession * psession)
{
	if (psession == NULL) return 0 ;
	if (ptoken == NULL) return 0 ;
	//根据请求命令字获取处理器
	ProcessorBase* pProcesser = NULL;
	if (psession->_message_logic_type == EN_Message_Request || psession->_message_logic_type == EN_Message_Push)
	{
		pProcesser = GetProcesser(psession->_head.cmd());
	}
	else
	{
		pProcesser = GetProcesser(psession->request_cmd());
	}
	if ( pProcesser == NULL )
	{
		//没有找到对应的处理函数.
		VLogMsg(CLIB_LOG_LEV_ERROR,"failed to find processer for command[0x%lx]",
			psession->_head.cmd());
		SessionManager::Instance()->ReleaseSession(psession->sessionid());
		return -1;
	}
	pProcesser->Process(ptoken,psession);
	return 0;
}

int ProcesserFactoryBasic::ProcessInnerMsg(CSession * psession)
{
	if (psession == NULL) return 0 ;
    // proto 打印
   	std::string strProto;
    google::protobuf::TextFormat::PrintToString(psession->_request_msg, &strProto);
    strProto = "\n---Inner Proto Msg---\n" + strProto;
    ProtoMsg("%s", strProto.c_str());

	//根据请求命令字获取处理器
	ProcessorBase * pProcesser = GetProcesser(psession->_request_msg.msg_union_case());
	if ( pProcesser == NULL )
	{
		//没有找到对应的处理函数.
		VLogMsg(CLIB_LOG_LEV_ERROR,"failed to find processer for command[0x%x]",
			psession->_request_msg.msg_union_case());
		SessionManager::Instance()->ReleaseSession(psession->sessionid());
		return -1;
	}
	pProcesser->Process(NULL,psession);
	return 0;
}

