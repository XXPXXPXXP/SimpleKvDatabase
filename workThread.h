//
// Created by 神奇bug在哪里 on 2022/12/29.
//

#ifndef FINAL_PROJECT_WORKTHREAD_H
#define FINAL_PROJECT_WORKTHREAD_H
#include <vector>
#include "settings.h"
#include "serverCore.h"
class threadsPool {
    pthread_mutex_t taskLocker;
    std::vector<pthread_t> runningThreads;
    const uint32_t maxThread = MAX_WORK_THREAD;
    const uint32_t minThread = MIN_WORK_THREAD;
    /* 将会采用线程池来减少线程之间的重复销毁和创建 */
public:

    static void * worker(void * args);
    void start(int targetSockId, serverSocket *server);

    ~threadsPool(){
        pthread_mutex_destroy(&taskLocker);
        log(warning,"线程池正在被回收！");
    }
};
struct args
{
    int sockID;
    serverSocket * server;
};


#endif //FINAL_PROJECT_WORKTHREAD_H
