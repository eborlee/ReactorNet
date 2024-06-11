#ifndef POLLDISPATCHER_HPP
#define POLLDISPATCHER_HPP

#endif // POLLDISPATCHER_HPP

// Path: src/PollDispatcher.cpp

// #pragma once
#include "Channel.h"
#include "EventLoop.h"
#include "Dispatcher.h"
#include <string>
#include <poll.h>


class PollDispatcher: public Dispatcher
{
private:
    /* data */
public:
    PollDispatcher(Eventloop *evLoop);
    // 析构函数
    // 为什么析构函数是虚函数？
    // 为了能够在子类中释放子类的资源 当父类指针指向子类对象时，
    // delete父类指针时，只会调用父类的析构函数，而不会调用子类的析构函数，所以析构函数需要是虚函数
    ~PollDispatcher();

    // add  通过evLoop取出当前dispatcher工作所需的数据。比如epoll的epollevent
    int add() override; // override表示覆盖父类的虚函数，可以使得编译器检查是否真的覆盖了父类的虚函数

    // delete
    int remove() override;
    // edit
    int modify() override;

    // 事件监测 超时 s
    int dispatch(int timeout = 2) override; // 2s

private:
    int m_maxfd;
    struct pollfd* m_fds;
    const int m_maxNode = 1024;
};
