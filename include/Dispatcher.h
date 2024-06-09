#pragma once
#include "Channel.h"
#include "EventLoop.h"
#include <string>

struct Eventloop;
// eventloop包含dispatcher和本结构体的init出来的数据块
class Dispatcher
{
private:
    /* data */
public:
    Dispatcher(Eventloop *evLoop);
    // 析构函数
    // 为什么析构函数是虚函数？
    // 为了能够在子类中释放子类的资源 当父类指针指向子类对象时，
    // delete父类指针时，只会调用父类的析构函数，而不会调用子类的析构函数，所以析构函数需要是虚函数
    virtual ~Dispatcher();

    // add  通过evLoop取出当前dispatcher工作所需的数据。比如epoll的epollevent
    virtual int add();

    // delete
    virtual int remove();
    // edit
    virtual int modify();

    // 事件监测 超时 s
    virtual int dispatch(int timeout = 2); // 2s

    inline void setChannel(Channel *channel)
    {
        m_channel = channel;
    }

private:
    /* data */
    Channel *m_channel;
    Eventloop *m_evLoop;
    std::string m_name = std::string(); // 用于标识dispatcher的名字，初始化为空
};
