//
// Created by 神奇bug在哪里 on 2022/12/29.
//

#ifndef FINAL_PROJECT_WORKTHREAD_H
#define FINAL_PROJECT_WORKTHREAD_H
#include <vector>
#include "settings.h"
#include <thread>
#include <mutex>
#include "serverLog.h"
#include "sys/epoll.h"
class threadsPool {
protected:
    const uint32_t maxThread = MAX_WORK_THREAD;
    const uint32_t minThread = MIN_WORK_THREAD;
    std::vector<std::thread> workerIDs;
    std::thread managerID;
    std::mutex pipeLocker;
    struct epoll_event pipeEvent[10];
    struct epoll_event pipeEV{};
    int pipeEpoll;
    /* 将会采用线程池来减少线程之间的重复销毁和创建 */
public:
    ~threadsPool(){
        log(warning,"线程池被回收！");
    }
};


#endif //FINAL_PROJECT_WORKTHREAD_H
