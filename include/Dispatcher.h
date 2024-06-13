#pragma once
#include "Channel.h"
#include "EventLoop.h"
#include <string>

class EventLoop;
// eventloop包含dispatcher和本结构体的init出来的数据块
class Dispatcher
{
private:
    /* data */
public:
    Dispatcher(EventLoop* evLoop);
    ~Dispatcher();

    // 事件监测 超时 s
    virtual int dispatch(int timeout = 2); // 2s

    inline void setChannel(Channel *channel)
    {
        m_channel = channel;
    }

    virtual int add();

    virtual int remove();

    virtual int modify();

protected:
    /* data */
    Channel *m_channel;
    EventLoop *m_evLoop;
    std::string m_name = std::string(); // 用于标识dispatcher的名字，初始化为空
};
