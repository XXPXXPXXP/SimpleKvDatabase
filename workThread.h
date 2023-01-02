//
// Created by 神奇bug在哪里 on 2022/12/29.
//

#ifndef FINAL_PROJECT_WORKTHREAD_H
#define FINAL_PROJECT_WORKTHREAD_H
#include <vector>
#include "serverCore.h"
class threadsPool {
    pthread_mutex_t taskLocker;
    std::vector<pthread_t> runningThreads;
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
