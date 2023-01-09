#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <vector>
#include <queue>
#include <atomic>
#include <future>
#include <stdexcept>
#include "settings.h"


class threadPool {
    unsigned short minSize;       //初始化线程数量
    using Task = std::function<void()>; //定义函数类型，便于
    std::vector<std::thread> threadIDs;          //线程池
    std::queue<Task> tasks;            //任务队列
    std::mutex taskLocker;                   //任务队列同步锁
    std::mutex addThreadLocker;               //线程池添加线程同步锁
    std::condition_variable taskCv;   //条件阻塞，用来控制线程的运行
    std::atomic<bool> isRuning{true};     //线程池是否正在运行
    std::atomic<int> idleThreadCount{0};  //空闲线程数量


public:
    /* 提交一个任务
     * 调用.get()获取返回值会等待任务执行完,获取返回值
     * 一种是使用   bind： .addTasks(std::bind(&Dog::sayHello, &dog));
     * 一种是用   mem_fn： .addTasks(std::mem_fn(&Dog::sayHello), this)
     */
    template<class Func, class... Args>
    /* 使用可变参数模版类 */
    auto addTasks(Func &&f, Args &&... args) -> std::future<decltype(f(args...))> {
        if (!isRuning)    // stopped
            throw std::runtime_error("addTasks on ThreadPool is stopped.");

        using RetType = decltype(f(args...)); // typename std::result_of<F(Args...)>::type, 函数 f 的返回值类型
        auto task = make_shared<std::packaged_task<RetType()>>(
                bind(forward<Func>(f), forward<Args>(args)...)
        ); // 把函数入口及参数绑定
        std::future<RetType> future = task->get_future();
        {    // 添加任务到队列
            std::lock_guard<std::mutex> lock{
                    taskLocker};//对当前块的语句加锁  lock_guard 是 mutex 的 stack 封装类，构造的时候 lock()，析构的时候 unlock()
            tasks.emplace([task]() { // push(Task{...}) 放到队列后面
                (*task)();
            });
        }
        if (idleThreadCount < 1 && threadIDs.size() < MAX_WORK_THREAD)
            addThread(1);
        taskCv.notify_one(); // 唤醒一个线程执行
        return future;
    }

    //空闲线程数量
    int idleCount() { return idleThreadCount; }

    //线程数量
    int runningCount() { return (int) threadIDs.size(); }

    inline ~threadPool() {
        isRuning = false;
        taskCv.notify_all(); // 唤醒所有线程，并且执行清空任务队列
        for (std::thread &thread: threadIDs) {
            if (thread.joinable())
                thread.join(); // 等待全部任务结束
        }
    }

    inline threadPool() {
        minSize = MIN_WORK_THREAD;
        addThread(MIN_WORK_THREAD);
    }

    inline void stop() {
        isRuning = false;
        taskCv.notify_all(); // 唤醒所有线程，并且执行清空任务队列
        for (std::thread &thread: threadIDs) {
            if (thread.joinable())
                thread.join(); // 等待全部任务结束
        }
        log(warning,"线程池已手动停止！");
    }

private:
    //添加指定数量的线程
    void addThread(unsigned short size) {
        if (!isRuning)    // stopped
            throw std::runtime_error("Grow on ThreadPool is stopped.");
        std::unique_lock<std::mutex> lockGrow{addThreadLocker}; //自动增长锁
        for (; threadIDs.size() < MAX_WORK_THREAD && size > 0; --size) {   //增加线程数量,但不超过最大线程数
            threadIDs.emplace_back([this] { //工作线程函数
                while (true) //防止销毁线程池的时候工作线程直接停止,此时任务队列可能不为空
                {
                    Task task; // 获取一个待执行的任务
                    {
                        std::unique_lock<std::mutex> lock{taskLocker};
                        taskCv.wait(lock, [this] { // 等待任务传入, 或需要停止
                            return !isRuning || !tasks.empty();
                        });
                        if (!isRuning && tasks.empty())
                            return; //任务队列空了，直接退出
                        idleThreadCount--;
                        task = std::move(tasks.front()); // 按先进先出从队列取一个 task,使用std::move避免多余的拷贝构造
                        tasks.pop();
                    }
                    task();//执行任务
                    if (idleThreadCount > 0 && threadIDs.size() > minSize) //自动释放空闲线程,避免峰值过后大量空闲线程
                        return;
                    {
                        std::unique_lock<std::mutex> lock{taskLocker};
                        idleThreadCount++;
                    }
                }
            });
            {
                std::unique_lock<std::mutex> lock{taskLocker};
                idleThreadCount++;
            }
        }
    }
};


#endif