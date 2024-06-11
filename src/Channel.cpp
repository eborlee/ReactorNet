#include "../include/Channel.h"
#include <stdlib.h>

Channel::Channel(int fd, FDEvent event, handleFunc readFunc,
                 handleFunc writeFunc, handleFunc destroyFunc, void *arg)
{
    m_arg = arg;
    m_fd = fd;
    m_event = (int)event;
    readCallback = readFunc;
    writeCallback = writeFunc;
    destroyCallback = destroyFunc;
}

void Channel::writeEventEnable(bool flag)
{
    // 处理第三个标志位
    if (flag)
    {
        // events |= (int)FDEvent::WriteEvent;
        m_event |= static_cast<int>(FDEvent::WriteEvent);
    }
    else
    {
        m_event |= m_event & ~(int)FDEvent::WriteEvent;
    }
}

// 按位与，WE是0x04， 00000100，那么如果channel->events是0x00,函数将返回false。假如其第三位是1，则返回True。
bool Channel::isWriteEventEnable()
{
    return m_event & (int)FDEvent::WriteEvent; // 按位与
}