//
// Created by 徐鑫平 on 2022/12/29.
//

#ifndef FINAL_PROJECT_WORKTHREAD_H
#define FINAL_PROJECT_WORKTHREAD_H
#include <vector>
#include "serverCore.h"
class threadsPool {
    std::vector<pthread_t> runningThreads;
    int workersTasksCount;
    struct kevent * watchList;
    struct kevent * eventList;
    pthread_mutex_t taskLocker;
    int kId;
public:
    void createThreadPool(int maxThread, serverSocket *server);
    [[noreturn]] static void * threadWorker(void *args);
    static void * process_theard(void * args);
    void producerConsumerMode(int targetSockId, serverSocket *server);
    void addTasks(int targetId);
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
struct argsTpMode
{
    threadsPool * _this;
    serverSocket * server;
    int count;
};

#endif //FINAL_PROJECT_WORKTHREAD_H
