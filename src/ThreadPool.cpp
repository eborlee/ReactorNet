#include "ThreadPool.h"
#include <cassert>

ThreadPool::ThreadPool(EventLoop *mainLoop, int count)
{
    m_index = 0;
    m_isStart = false;
    m_mainLoop = mainLoop;
    m_threadNum  = count;
    m_workerThreads.clear();
}

ThreadPool::~ThreadPool()
{
    for(auto &workerThread : m_workerThreads){
        delete workerThread;
    }
}



void ThreadPool::run()
{
    assert(m_isStart);
    if(m_mainLoop->getThreadID() == std::this_thread::get_id()){
        exit(0);
    }
    m_isStart = true;
    if (m_threadNum>0){
        for(int i=0;i<m_threadNum;++i){
            WorkerThread* subThread = new WorkerThread(i);
            subThread->run();
            m_workerThreads.push_back(subThread);
        }
    }
}

EventLoop *ThreadPool::takeWorkerEventLoop()
{
    assert(m_isStart);
    if(m_mainLoop->getThreadID() == std::this_thread::get_id()){
        exit(0);
    }
    // 从线程池中找一个子线程，然后取出一个反应堆实例
    struct EventLoop* evLoop = m_mainLoop;
    if(m_threadNum>0){
        evLoop = m_workerThreads[m_index]->getEvLoop();
    	m_index = ++m_index % m_threadNum;
    }
    return evLoop;
}
