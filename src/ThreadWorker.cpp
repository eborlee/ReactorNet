#include "ThreadWorker.h"

WorkerThread::WorkerThread(int index)
{
    m_evLoop = nullptr;
    m_thread = nullptr;
    m_threadID = std::thread::id();
    m_name = "SubThread-" + std::to_string(index);
}

WorkerThread::~WorkerThread()
{
    if(m_thread != nullptr){
        delete m_thread;
    }
}

void* WorkerThread::running(){
    // 需要确保反应堆实例存在
    m_mutex.lock();
    m_evLoop = new EventLoop(m_name);
    m_mutex.unlock();
    m_cond.notify_one();
    m_evLoop->run();

}

void WorkerThread::run()
{   
    m_thread = new std::thread(&WorkerThread::running, this);
    // 主线程创建子线程
    std::unique_lock<std::mutex> locker(m_mutex);
    // 理想状态下，逻辑链是：主线程创建了这个子线程，认为该线程已经初始化完毕，于是从线程池中取出，并添加了一个任务给它。但问题是，有可能此时evLoop还没初始化完毕
    // 主线程阻塞一会儿
    while (m_evLoop == nullptr)
    {
        m_cond.wait(locker);
    }
}
