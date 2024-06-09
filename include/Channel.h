#pragma once
// #include <stdbool.h> // c++中不需要引入这个头文件

// 定义函数指针
// typedef int(*handleFunc) (void* arg);
using handleFunc = int (*)(void *arg);

// 定义文件描述符的读写事件，二进制，通过判断位数是1还是0来判断读写事件
// c++中枚举类型添加了class后变成了强类型枚举，不会隐式转换为整型，以保证类型安全
// 使用时必须加上枚举类型的名字，如FDEvent::TimeOut
// 在赋值或计算时需要强制转换为整型
enum class FDEvent
{
    TimeOut = 0x01,   // 超时，一般用不到
    ReadEvent = 0x02, // 二进制10
    WriteEvent = 0x04 // 二进制100
};

class Channel
{

public:
    Channel(int fd, int events, handleFunc readFunc, handleFunc writeFunc,
            handleFunc destroyFunc, void *arg);

    // 回调函数
    handleFunc readCallback;
    handleFunc writeCallback;
    handleFunc destroyCallback;

    // 修改fd的写事件：检测or 不检测
    void writeEventEnable(bool flag);

    // 判断是否需要检测文件描述符的写事件
    bool isWriteEventEnable();

    // 获取文件描述符
    inline int getSocket()
    {
        return m_fd;
    }

    // 获取事件
    inline int getEvents()
    {
        return m_events;
    }

    // 获取回调函数的参数
    // 加const使得该返回的指针不可修改
    inline const void *getArg()
    {
        return m_arg;
    }

private:
    // 文件描述符
    int m_fd;
    // 事件
    int m_events;
    // 回调函数的参数
    void *m_arg;
};
