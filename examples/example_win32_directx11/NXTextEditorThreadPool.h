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
                        std::unique_lock<std::mutex> lock(m_mutex);
                        m_condition.wait(lock, [this]() { return m_bShutdown || !m_tasks.empty(); });
                        task = std::move(m_tasks.front());
                        m_tasks.pop();
                    }
                    if (task) task();
                } 
            });
        }
    }

    void Shutdown()
    {
        m_bShutdown = true;
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
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_tasks.push(func);
        }
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
