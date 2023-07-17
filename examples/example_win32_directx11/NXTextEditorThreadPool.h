// ������д��һ�������̳߳أ��������δ�����ʲô������
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
                        // ͬʱֻ����һ���̷߳��ʡ��޸��������
                        std::unique_lock<std::mutex> lock(m_mutex);

                        // ���� wait ����ܻ�����������
                        // lambda = true: ��task���в�Ϊ�ա���ʱ�ᱣ������״̬��lambda����return��ִ�к������롣
                        // lambda = false: ����ǰtask����Ϊ�ա���ʱ�����mutex�������߳̾Ϳ��Խ�������صĲ����ˣ������Զ�������ǰ�̣߳�ֱ�������ط�����notify_one��notify_all�������»�������
                        //      note���ڵ�ǰ�ﾳ�£�Ҫ����notify_one��notify_all���Ѵ��߳�֮ǰ�������ȷ��task���в�Ϊ�գ��������ָ�ɡ���Ȼwait���������������AddTaskFunc����
                        m_condition.wait(lock, [this]() { return m_bShutdown || !m_tasks.empty(); });

                        // �����������ȡ��һ������ִ�С�
                        FuncType task = std::move(m_tasks.front());
                        m_tasks.pop();
                    } // �������� ���Զ������������߳̾Ϳ��Խ�������ز����ˡ�

                    // Ȼ��Ϳ���ִ��ʵ��������
                    task();
                } 
            });
        }
    }

    void Shutdown()
    {
        m_bShutdown = true;
        // ���������̣߳��������˳�ѭ��
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

        m_mutex.lock();     // ������
        m_tasks.push(func); // ���޸��������
        m_mutex.unlock();   // �޸���ɺ����

        // ȷ������������������Ժ���ʹ��notify_one����ĳ���̣߳�ִ��wait֮��Ĵ���
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
