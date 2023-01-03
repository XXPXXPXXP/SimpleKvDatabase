//
// Created by 神奇bug在哪里 on 2022/12/29.
//

#ifndef FINAL_PROJECT_WORKTHREAD_H
#define FINAL_PROJECT_WORKTHREAD_H
#include <vector>
#include "serverCore.h"
class threadsPool {
    std::vector<pthread_t> runningThreads;
    pthread_mutex_t taskLocker;
public:
    static void * process_theard(void * args);
    void producerConsumerMode(int targetSockId, serverSocket *server);
    ~threadsPool(){
        pthread_mutex_destroy(&taskLocker);
        log(warning,"线程池正在被回收！");
    }
};
struct argsPcMode
{
    int sockID;
    serverSocket * server;
};

#endif //FINAL_PROJECT_WORKTHREAD_H
