// 这是我写的一个简易线程池，你觉得这段代码有什么问题吗？
#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

// for example: FuncType = std::function<void()>
template <typename FuncType>
class NXTextEditorThreadPool
{
public:
    NXTextEditorThreadPool(const NXTextEditorThreadPool&) = delete;
    NXTextEditorThreadPool& operator=(const NXTextEditorThreadPool&) = delete;

    NXTextEditorThreadPool(int threadCount = 4)
    {
        m_threads.resize(threadCount);
        for (auto& thread : m_threads)
        {
            thread = std::thread([this]() {
                while (!m_bShutdown)
                {
                    FuncType task;
                    {
                        // 同时只能有一个线程访问、修改任务队列
                        std::unique_lock<std::mutex> lock(m_mutex);

                        // 调用 wait 后可能会出现两种情况
                        // lambda = true: 即task队列不为空。此时会保持上锁状态，lambda立即return，执行后续代码。
                        // lambda = false: 即当前task队列为空。此时会解锁mutex（其它线程就可以进行锁相关的操作了），并自动阻塞当前线程，直到其它地方调用notify_one或notify_all才能重新唤醒它。
                        //      note：在当前语境下，要调用notify_one或notify_all唤醒此线程之前，最好先确保task队列不为空，有任务可指派。不然wait会重新阻塞（详见AddTaskFunc）。
                        m_condition.wait(lock, [this]() { return m_bShutdown || !m_tasks.empty(); });

                        // 从任务队列中取出一个任务并执行。
                        FuncType task = std::move(m_tasks.front());
                        m_tasks.pop();
                    } // 出作用域 会自动解锁。其它线程就可以进行锁相关操作了。

                    // 然后就可以执行实际任务了
                    task();
                } 
            });
        }
    }

    void Shutdown()
    {
        m_bShutdown = true;
        // 唤醒所有线程，让它们退出循环
        m_condition.notify_all();
        for (auto& thread : m_threads)
        {
            if (thread.joinable())
                thread.join();
        }
    }

    void AddTaskFunc(FuncType func)
    {
        if (m_bShutdown) return;

        m_mutex.lock();     // 先上锁
        m_tasks.push(func); // 再修改任务队列
        m_mutex.unlock();   // 修改完成后解锁

        // 确保任务队列中有任务以后，再使用notify_one唤醒某个线程，执行wait之后的代码
        m_condition.notify_one();
    }

    ~NXTextEditorThreadPool() { Shutdown(); }

private:
    std::vector<std::thread> m_threads;
    std::queue<FuncType> m_tasks;

    std::condition_variable m_condition;
    std::mutex m_mutex;

    std::atomic_bool m_bShutdown = false;
};
