#pragma once

template<typename ObjectType>
class CSingleton
{
public:
    static ObjectType * Instance() {
        return &Reference();
    }

    static ObjectType & Reference() {
        static ObjectType _Instance;     //每次调用都想通过赋值初始化，其实只初始化了第一次而已
        return _Instance;
    }
 
protected:
    CSingleton() { }                     // construtor is hidden
    CSingleton(CSingleton const &) { }    // copy constructor is hidden
};

