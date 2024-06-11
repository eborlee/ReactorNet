#pragma once
#include "Dispatcher.h"
#include "Channel.h"
#include <thread>
#include <queue>
#include <map>
#include <mutex>
#include <string>

// 如何处理该节点中的channel
// 使用强类型枚举并指定底层存储为char，以节省内存，每个枚举值1字节。默认是int，4字节
enum class ElemType : char
{
    ADD,
    DELETE,
    MODIFY
};

// 定义任务队列的节点
struct ChannelElement
{
    ElemType type; // 如何处理该节点中的channel
    Channel *channel;
};

class Dispatcher; // 告诉编译器我有这个类型，以解决互相包含的问题

class EventLoop
{
public:
    // 初始化, 主线程和子线程们都有evl，但是子线程们需要通过threadName区分，因此需要不同的初始化函数，因为c语言不支持函数重载
    EventLoop();
    EventLoop(const std::string threadName);

    ~EventLoop();

    // 启动反应堆模型
    int run();

    // 处理被激活的文件fd
    int activate(int fd, int event);

    // 添加任务到任务队列
    int addTask(Channel *channel, ElemType type);

    // 处理任务队列中的任务
    int processTask();

    // 处理dispatcher中的节点
    int add(Channel *channel);
    int remove(Channel *channel);
    int modify(Channel *channel);

    // 释放channel, 当触发remove，从待检测集合里移除fd时
    int destroyChannel(Channel *channel);

    static int readLocalMessage(void* arg);
    int readMessage();

private:
    void EventLoop::taskWakeup();

private:
    bool m_isQuit;
    // 父类指针指向子类实例
    Dispatcher *m_dispatcher;

    // 任务队列
    std::queue<ChannelElement*> m_taskQueue;

    // map
    std::map<int, Channel*> m_channels;

    // 线程信息
    std::thread::id m_threadID;
    std::string m_threadName;
    std::mutex m_mutex;
    int m_socketPair[2]; // 存储本地通信的fd，通过socketpair初始化
};


