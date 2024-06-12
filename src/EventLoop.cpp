#include "../include/EventLoop.h"
#include <assert.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <thread>
#include "../include/SelectDispatcher.h"
#include "../include/PollDispatcher.h"
#include "../include/EpollDispatcher.h"
#include "../include/Channel.h"
#include "EventLoop.h"

EventLoop::EventLoop(const std::string threadName)
{
    // 分配内存
    m_isQuit = true; // 默认没有启动
    m_threadID = std::this_thread::get_id();

    // 数组不能通过等号赋值
    m_threadName = threadName == std::string() ? "MainThread" : threadName;

    m_dispatcher = new EpollDispatcher(this);

    // map
    m_channels.clear();
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, m_socketPair);
    if (ret == -1)
    {
        perror("socketpair");
        exit(0);
    }
#if 0
    // 指定规则： evLoop->socketPair[0] 发送， evLoop->socketPair[1] 接收数据
    // 好像是什么往sp0里写，然后sp1就能接收到，然后激活了事件
    Channel *channel = new Channel(m_socketPair[1],
                                   FDEvent::ReadEvent,
                                   readLocalMessage, nullptr, nullptr, this);
    // 这两个本地通信用的文件描述符是不需要关闭的。因此destory回调为NULL。别忘了这两个文件描述符是干吗用的，为了打断阻塞状态用来处理添加任务
    // channel 添加到任务队列
#else
    // 可调用对象的绑定
    auto obj = std::bind(&EventLoop::readMessage, this);
    Channel *channel = new Channel(m_socketPair[1],
                                   FDEvent::ReadEvent,
                                   obj, nullptr, nullptr, this);
#endif
    addTask(channel, ElemType::ADD);
}

EventLoop::~EventLoop()
{
    // 这个服务器运行过程中并不会结束主和子线程，也就不会销毁eventloop
}

int EventLoop::run()
{
    m_isQuit = false;
    // 比较线程id是否正常
    if (m_threadID != std::this_thread::get_id())
    {
        return -1;
    }
    // 循环进行事件处理
    while (!m_isQuit)
    {
        m_dispatcher->dispatch(2); // 超时2秒
        // 此处每调用一次dispatch就是调用不同分发模型里的比如EPOLL_WAIT
        processTask(); // 处理任务队列
    }
}

int EventLoop::activate(int fd, int event)
{
    if (fd < 0)
    {
        return -1;
    }

    // 取出channel
    // 说此处对应channelMap的二级指针，指向一个指针数组，fd的值就是数组下标
    Channel *channel = m_channels[fd];
    assert(channel->getSocket() == fd);
    if (event & (int)FDEvent::ReadEvent && channel->readCallback)
    {
        channel->readCallback(const_cast<void*>(channel->getArg()));
    }
    if (event & (int)FDEvent::WriteEvent && channel->writeCallback)
    {
        channel->writeCallback(const_cast<void*>(channel->getArg()));
    }
}

int EventLoop::addTask(Channel *channel, ElemType type)
{
    // 加锁， 保护共享资源
    m_mutex.lock();
    struct ChannelElement* node = new ChannelElement;
    node->channel = channel;
    node->type = type;
    m_taskQueue.push(node);
    m_mutex.unlock();
    // 处理节点（假设当前的evl反应堆属于子线程
    // 细节1：对于链表节点的添加：可能是当前线程也可能是主线程。
    //		比如说修改fd的事件，当前子线程发起，当前子线程处理
    // 		添加新的fd，主线程发起，也就是主线程添加任务节点
    // 细节2：如果当前线程为主线程，它只负责客户端连接，剩下的任务都应该由子线程完成。因此它不管任务队列里节点的处理。交给子线程的dispatcher处理。不能让给主线程处理任务队列
    if(m_threadID == std::this_thread::get_id()){
        // 当前子线程(运行状态)，直接处理任务队列里的任务
        processTask();
        
    }else{
        // 主线程：通知子线程处理任务队列里的任务
        // 1. 子线程正在工作
        // 2. 子线程被阻塞了（被底层的dispatcher阻塞了，select，poll，epoll）三者都是没有激活的事件时，xxWait会阻塞。我们之前设置了阻塞时长为2秒。需要唤醒子线程：基于socketPair
        taskWakeup(); // 此时向sp0里写了随便的数据，sp1被激活，其读回调被调用;如果此时子线程正在工作，那么这个函数的执行没有影响，所以没关系
    }
    return 0;
}

int EventLoop::processTask()
{
    
    // 取出头节点

    while(!m_taskQueue.empty()){
        m_mutex.lock();
        ChannelElement* node = m_taskQueue.front();
        m_taskQueue.pop();
        m_mutex.unlock();

        Channel* channel = node->channel;
        if(node->type == ElemType::ADD){
            // 添加
            add(channel);
        }
        else if(node->type == ElemType::DELETE){
            // 删除
             remove(channel);
            // 还需要回收channel对象
            destroyChannel(channel);
            
        }
        else if(node->type == ElemType::MODIFY){
            // 修改
             modify(channel);
        }
        delete node;
    }
    
    return 0;
}

int EventLoop::add(Channel *channel)
{
     // 将任务节点里的任务添加到dispatcher检测的检测集合里
    int fd = channel->getSocket();

    // 找到fd对应的数组元素位置，并存储
    if (m_channels.find(fd) == m_channels.end()){
        m_channels.insert(std::pair<int, Channel*>(fd, channel));
        m_dispatcher->setChannel(channel);
        int ret = m_dispatcher->add();
        return ret;
    }
    return -1;
}

int EventLoop::remove(Channel *channel)
{
    int fd = channel->getSocket();
    if (m_channels.find(fd) == m_channels.end())
    {
        return -1;
    }
    m_dispatcher->setChannel(channel);
    int ret = m_dispatcher->remove();
	return ret;
}

int EventLoop::modify(Channel *channel)
{
    int fd = channel->getSocket();
    if (m_channels.find(fd) == m_channels.end())
    {
        return -1;
    }
    m_dispatcher->setChannel(channel);
    int ret = m_dispatcher->modify();
	return ret;
}

EventLoop::EventLoop() : EventLoop(std::string())
{
}

int EventLoop::destroyChannel(Channel *channel)
{
    // 删除channel和fd的对应关系
   	auto it = m_channels.find(channel->getSocket());
    if (it != m_channels.end())
    {
        m_channels.erase(it);
        close(channel->getSocket());
        delete channel;
    }
    return 0;
}

int EventLoop::readLocalMessage(void *arg)
{
    // 由于这个函数是static的，因此无法使用eventlopp自身的this指针。所以需要接收传参
    EventLoop *evLoop = static_cast<EventLoop *>(arg);
    char buf[256];
    // 注意此时访问类的内部成员是没问题的，因为这个函数依然是类的成员函数
    read(evLoop->m_socketPair[1], buf, sizeof(buf)); // 此处接收到的buf其实并不重要，这块真正目的是让对用的dispatcher解除阻塞
    return 0;
}

int EventLoop::readMessage()
{
    char buf[256];
    // 注意此时访问类的内部成员是没问题的，因为这个函数依然是类的成员函数
    read(m_socketPair[1], buf, sizeof(buf)); // 此处接收到的buf其实并不重要，这块真正目的是让对用的dispatcher解除阻塞
    return 0;
}

// 写数据
void EventLoop::taskWakeup(){
    const char* msg = "一段文字";
    write(m_socketPair[0], msg, strlen(msg));
}
