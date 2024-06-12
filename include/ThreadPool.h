#ifndef THREADPOOL_HPP
#define THREADPOOL_HPP

#endif // THREADPOOL_HPP

#include "EventLoop.h"
#include <stdbool.h>
#include "ThreadWorker.h"
#include <vector>

// 定义线程池
class ThreadPool
{
public:
    // 初始化线程池
    ThreadPool(struct EventLoop *mainLoop, int count);
    ~ThreadPool();

    // 启动线程池
    void run();
    // 取出线程池中的某个子线程的反应堆实例
    struct EventLoop *takeWorkerEventLoop();

private:
    // 主线程的反应堆模型
    struct EventLoop *m_mainLoop; // 只负责和客户端的创建连接，除非线程池设置的子线程数为0
    bool m_isStart;
    int m_threadNum;
    std::vector<WorkerThread*> m_workerThreads; // 这里是一个指向结构体的指针数组，语法没错。只要使用前分配了足够的空间 见下文
    int m_index;
};
