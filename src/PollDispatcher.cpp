#include "../include/PollDispatcher.h"
#include <poll.h>
#include <stdlib.h>
#include <stdio.h>

PollDispatcher::PollDispatcher(EventLoop *evLoop) : Dispatcher(evLoop)
{
    m_maxfd = 0;
    // 初始化结构体数组
    m_fds = new struct pollfd[m_maxNode];
    for (int i = 0; i < m_maxNode; ++i)
    {
        m_fds[i].fd = -1;
        m_fds[i].events = 0;
        m_fds[i].revents = 0;
    }
    m_name = "Poll";
}

PollDispatcher::~PollDispatcher()
{
    delete[] m_fds;
}

int PollDispatcher::add()
{
    int events = 0;
    if (m_channel->getEvent() & (int)FDEvent::ReadEvent)
    {
        events |= POLLIN;
    }

    if (m_channel->getEvent() & (int)FDEvent::WriteEvent)
    {
        events |= POLLOUT;
    }
    int i = 0;
    for (; i < m_maxNode; ++i)
    {
        if (m_fds[i].fd == -1)
        {
            m_fds[i].events = events;
            m_fds[i].fd = m_channel->getSocket();
            m_maxfd = i > m_maxfd ? i : m_maxfd;
            break;
        }
    }
    if (i >= m_maxfd)
    {
        return -1;
    }
    return 0;
}

int PollDispatcher::remove()
{
    int i = 0;
    for (; i < m_maxNode; ++i)
    {
        if (m_fds[i].fd == m_channel->getSocket())
        {
            m_fds[i].events = 0;
            m_fds[i].revents = 0;
            m_fds[i].fd = -1; // 恢复初始化
            break;
        }
    }
    // 通过channel释放对应的TcpConn资源
    m_channel->destroyCallback(const_cast<void *>(m_channel->getArg()));
    if (i >= m_maxNode)
    {
        return -1;
    }
    return 0;
}

int PollDispatcher::modify()
{
    int events = 0;
    if (m_channel->getEvent() & (int)FDEvent::ReadEvent)
    {
        events |= POLLIN;
    }

    if (m_channel->getEvent() & (int)FDEvent::WriteEvent)
    {
        events |= POLLOUT;
    }
    int i = 0;
    for (; i < m_maxNode; ++i)
    {
        if (m_fds[i].fd == -1)
        {
            m_fds[i].events = events;
            break;
        }
    }
    if (i >= m_maxNode)
    {
        return -1;
    }
    return 0;
}

int PollDispatcher::dispatch(int timeout)
{
    int count = poll(m_fds, m_maxfd + 1, timeout * 1000); // 注意这里是maxfd+1，好像是因为底层判断的时候是>=
    // 返回的count是指有多少个文件描述符处于激活的状态
    if (count == -1)
    {
        perror("poll");
        exit(0);
    }
    for (int i = 0; i < m_maxfd; i++)
    {
        if (m_fds[i].fd == -1)
        {
            continue;
        }
        if (m_fds[i].revents & POLLIN)
        {
            // 触发读事件，调用fd对应的读回调
            // 这个读回调是在channel中注册的
            // 因此要找到对应的channel，从channel中进行调用
            m_evLoop->activate(m_fds[i].fd, (int)FDEvent::ReadEvent);
            // eventActivate(evLoop, m_fds[i].fd, ReadEvent);
        }
        if (m_fds[i].revents & POLLOUT)
        {
            // 触发写事件
            // eventActivate(evLoop, m_fds[i].fd, WriteEvent);
            m_evLoop->activate(m_fds[i].fd, (int)FDEvent::WriteEvent);
        }
    }
}
