#include "singleton.h"
#include "ProcesserFactoryBasic.h"

class ProcesserManager : public ProcesserFactoryBasic
{
public:
	static ProcesserManager * Instance();
public:
    void Init();
	ProcessorBase * GetProcesser(int cmd);
};

