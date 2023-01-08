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
    uint32_t maxThread;
    uint32_t minThread;
    std::vector<std::thread> workerIDs;
    std::thread managerID;
    std::mutex pipeLocker;
    int workingNum;   //忙的线程数
    int onlineNum;    //存活的线程数
    int destroyNum;    //要销毁的线程数
    /* 将会采用线程池来减少线程之间的重复销毁和创建 */
public:
    ~threadsPool(){
        log(warning,"线程池被回收！");
    }
    threadsPool(){
        workingNum = 0;
        onlineNum = 0;
        destroyNum = 0;
        maxThread = MAX_WORK_THREAD;
        minThread = MIN_WORK_THREAD;
        log(info,"线程池构造函数完成处理！");
    }
};


#endif //FINAL_PROJECT_WORKTHREAD_H
