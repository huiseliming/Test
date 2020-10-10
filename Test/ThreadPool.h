#pragma once
#include<condition_variable>
#include<thread>
#include<queue>
#include<future>
#include<iostream>
#include<atomic>

class ThreadPool
{
public:
    ThreadPool()
    {
        Start();
    }

    explicit ThreadPool(uint32_t num_thread)
    {
        Start(num_thread);
    }

    ThreadPool(const ThreadPool& thread_pool) = delete;
    ThreadPool(ThreadPool&& thread_pool) = delete;
    ThreadPool& operator=(const ThreadPool& thread_pool) = delete;
    ThreadPool& operator=(ThreadPool&& thread_pool) = delete;

    ~ThreadPool()
    {
        Stop();
    }

    template<typename Task, typename ...Args>
    void ExecuteTask(Task&& task, Args&& ...args)
    {
        std::shared_ptr<std::function<void()>> executeTask =
            std::make_shared<std::function<void()>>(
                [task = std::move(task), args = std::make_tuple(std::forward<Args>(args)...)]() mutable
        {
            return std::apply(std::move(task), std::move(args));
        }
        );
        {
            std::unique_lock<std::mutex> UniqueLock(mutex_);
            tasks_.emplace([=] {
                (*executeTask)();
                });
        }
        condition_variable_.notify_one();
    }

    template<typename Task>
    void ExecuteTask(Task&& task)
    {
        std::shared_ptr<std::function<void()>> executeTask = std::make_shared<std::function<void()>>(std::forward<Task>(task));
        {
            std::unique_lock<std::mutex> UniqueLock(mutex_);
            tasks_.emplace([=] {
                (*executeTask)();
                });
        }
        condition_variable_.notify_one();
    }

    template<typename Task, typename ...Args>
    auto PackagedTask(Task&& task, Args&& ...args)
    {

        std::shared_ptr< std::packaged_task<std::invoke_result_t<Task, Args...>()> > packaged_task =
            std::make_shared< std::packaged_task<std::invoke_result_t<Task, Args...>()> >(
                [task = std::move(task), args = std::make_tuple(std::forward<Args>(args)...)]() mutable
        {
            return std::apply(std::move(task), std::move(args));
        }
        );

        {
            std::unique_lock<std::mutex> UniqueLock(mutex_);
            tasks_.emplace([=] {
                (*packaged_task)();
                });
        }
        condition_variable_.notify_one();
        return packaged_task->get_future();
    }

    template<typename Task>
    auto PackagedTask(Task&& task)->std::future<decltype(std::forward<Task>(task)())>
    {
        std::shared_ptr< std::packaged_task<decltype(std::forward<Task>(task)())()> > packaged_task =
            std::make_shared< std::packaged_task<decltype(std::forward<Task>(task)())()> >(task);
        {
            std::unique_lock<std::mutex> UniqueLock(mutex_);
            tasks_.emplace([=] {
                (*packaged_task)();
                });
        }
        condition_variable_.notify_one();
        return packaged_task->get_future();
    }

    void WaitTaskEmpty()
    {
        while (!TaskEmpty())
            std::this_thread::yield();
    }

    bool TaskEmpty()
    {
        return tasks_.size() == 0;
    }

    int IdleNumber()
    {
        return thread_count_ - current_working_;
    }

private:
    int thread_count_ = 0;
    std::atomic_int32_t current_working_;
    bool stop_ = false;
    std::mutex mutex_;
    std::vector<std::thread> threads_;
    std::queue<std::function<void()>> tasks_;
    std::condition_variable condition_variable_;

    void Start(uint32_t NumThread = 0)
    {
        uint32_t thread_count_ = NumThread ? NumThread : (std::thread::hardware_concurrency() * 2);
        current_working_ = 0;
        for (size_t i = 0; i < thread_count_; i++)
        {
            threads_.emplace_back([=, id = i] {
                std::function<void()> task;
                while (true)
                {
                    {
                        std::unique_lock<std::mutex> lock(this->mutex_);
                        current_working_++;
                        condition_variable_.wait(lock, [=] {return this->stop_ || !tasks_.empty(); });
                        current_working_--;
                        if (this->stop_ && tasks_.empty())
                            break;
                        task = std::move(this->tasks_.front());
                        this->tasks_.pop();
                    }
                    try
                    {
                        task();
                    }
                    catch (const std::exception& e) {
                        std::cout << "[std::exception>{}]" <<  e.what() << std::endl;
                    }
                    catch (...) {
                        std::cout << "Unknow Exception" << std::endl;
                    }
                }
                });
        }
    }

    void Stop()noexcept
    {
        {
            std::unique_lock<std::mutex> lock{ mutex_ };
            stop_ = true;
        }
        condition_variable_.notify_all();
        for (auto& thread : threads_)
        {
            thread.join();
        }
    }
};