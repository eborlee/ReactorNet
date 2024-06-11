#include "../include/EpollDispatcher.h"
#include <sys/epoll.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

// epfd和channel里fd的区别
// epoll_create创建一个epoll实例，返回一个文件描述符，
// 这个文件描述符是epoll实例的文件描述符，用来操作epoll实例
// channel里的fd是要监控的文件描述符，是epoll实例监控的对象, 即客户端的socket

int EpollDispatcher::epollCtl(int op)
{
    struct epoll_event ev;
    ev.data.fd = m_channel->getSocket();
    int events = 0;
    if (m_channel->getEvent() & (int)FDEvent::ReadEvent){
        events |= EPOLLIN;
    }
        
    if(m_channel->getEvent() & (int)FDEvent::WriteEvent){
        events |= EPOLLOUT;
    }
    ev.events = events;
    // 调用 epoll_ctl 函数,将 Channel 的文件描述符和事件添加到 epoll 实例中.你将一个新的区域(channel->fd)添加到你的警察局(data->epfd)的管辖范围内。你同时指定了负责监视这个区域的警察(I/O事件)的职责描述(&ev)。
    int ret = epoll_ctl(m_epfd, op, m_channel->getSocket(), &ev);
    return ret;
}


// 通过初始化列表初始化父类的成员变量
EpollDispatcher::EpollDispatcher(EventLoop *evLoop) : Dispatcher(evLoop)
{
    m_epfd = epoll_create(10); // 传入一个大于0的随便什么数字
    if (m_epfd == -1)
    {
        perror("epoll_create");
        exit(0);
    }
    // calloc 不仅可以分配内存还可以初始化为0.不像malloc要先分配再mem。第一个参数是数组元素个数，第二个是每个元素的大小
    m_events = new struct epoll_event[m_MaxNode];
    m_name = "Poll";
}

EpollDispatcher::~EpollDispatcher()
{
    close(m_epfd);
    delete[] m_events;
}

int EpollDispatcher::add()
{
    int ret = epollCtl(EPOLL_CTL_ADD);
    if (ret == -1)
    {
        perror("epoll_ctl add");
        exit(0);
    }
    return ret;
}

// delete
int EpollDispatcher::remove()
{
    int ret = epollCtl(EPOLL_CTL_DEL);
    if (ret == -1)
    {
        perror("epoll_ctl delete");
        exit(0);
    }
    // 通过channel释放对应的TcpConnection资源
    // 并通过const_cast去掉const属性
    m_channel->destroyCallback(const_cast<void*>(m_channel->getArg()));
    return ret;
}

// edit
int EpollDispatcher::modify(){
    int ret = epollCtl(EPOLL_CTL_MOD);
    if (ret == -1){
        perror("epoll_ctl modify");
        exit(0);
    }
    return ret;
}

// 事件监测 超时 s
int EpollDispatcher::dispatch(int timeout = 2){
    int count = epoll_wait(m_epfd, m_events, m_MaxNode, timeout*1000); // 转毫秒
	for(int i=0;i<count;i++)
    {
        int events = m_events[i].events;
        int fd = m_events[i].data.fd;
        // 断开连接产生的两个错误事件
        if(events & EPOLLERR || events & EPOLLHUP)
        {
            // 对方断开连接，删除fd
            //epollRemove(Channel, evLoop);
            continue;
        }
        // 触发文件描述符的读事件
        if (events& EPOLLIN)
        {
            eventActivate(evLoop,fd, ReadEvent);
        }
        // 触发文件描述符的读事件
        if (events & EPOLLOUT)
        {
            eventActivate(evLoop, fd, WriteEvent);
        }
    }
} // 2s