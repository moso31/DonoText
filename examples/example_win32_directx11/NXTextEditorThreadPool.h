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
                    // 同时只能有一个线程访问、修改任务队列
                    m_mutex.lock();
                    if (!m_tasks.empty())
                    {
                        // 如果任务队列不为空，从中取出一个任务并执行
                        FuncType task = std::move(m_tasks.front());
                        m_tasks.pop();
                        m_mutex.unlock();
                        task();
                    }
                    else
                    {
                        // 如果任务队列为空，睡眠100ms，再重新检查
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        m_mutex.unlock();
                    }
                }
            });
        }
    }

    void Shutdown()
    {
        m_bShutdown = true;
        for (auto& thread : m_threads)
        {
            if (thread.joinable())
                thread.join();
        }
    }

    void AddTaskFunc(FuncType func)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_tasks.push(func);
    }

    ~NXTextEditorThreadPool() { Shutdown(); }

private:
    std::vector<std::thread> m_threads;
    std::queue<FuncType> m_tasks;

    std::mutex m_mutex;

    std::atomic_bool m_bShutdown = false;
};
