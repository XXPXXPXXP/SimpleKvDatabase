//
// Created by 徐鑫平 on 2022/12/29.
//

#ifndef FINAL_PROJECT_WORKTHREAD_H
#define FINAL_PROJECT_WORKTHREAD_H
#include <vector>
#include "serverCore.h"
class threadsPool {
    pthread_mutex_t taskLocker;
    std::vector<pthread_t> runningThreads;
    int kId;
public:

    static void * worker(void * args);
    void start(int targetSockId, serverSocket *server);

    ~threadsPool(){
        pthread_mutex_destroy(&taskLocker);
        log(warning,"线程池正在被回收！");
    }
};
struct argsNtMode
{
    int sockID;
    serverSocket * server;
};
struct args
{
    threadsPool * _this;
    serverSocket * server;
    int count;
};

#endif //FINAL_PROJECT_WORKTHREAD_H
