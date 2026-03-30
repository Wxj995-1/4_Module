#include "EventLoopThread.h"
#include "EventLoop.h"

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb,
                                 const std::string &name)
    : loop_(nullptr), exiting_(false), thread_(std::bind(&EventLoopThread::threadFunc, this), name), mutex_(), cond_(), callback_(cb)
{
}

EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    if (loop_ != nullptr)
    {
        loop_->quit();
        thread_.join();
    }
}

// 由主线程调用，负责启动子线程 + 等待子线程创建好 EventLoop + 返回 loop 指针 （给主线程用）
EventLoop *EventLoopThread::startLoop()
{
    thread_.start(); // 启动底层的新线程

    EventLoop *loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (loop_ == nullptr)
        {
            cond_.wait(lock);
        }
        loop = loop_;
    }
    return loop;
}

// 下面这个方法，实在单独的新线程里面运行的
// 由子线程执行，是子线程的入口函数，负责创建 EventLoop + 通知主线程 + 启动死循环
void EventLoopThread::threadFunc()
{
    // 核心：one loop per thread → 这个 loop 只属于当前子线程
    EventLoop loop; // 创建一个独立的eventloop，和上面的线程是一一对应的，one loop per thread

    if (callback_)
    {
        callback_(&loop);
    }

    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_one();
    }

    loop.loop(); // EventLoop loop  => Poller.poll   EventLoop 的核心循环 => 底层调用 Poller::poll()
    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = nullptr;
}