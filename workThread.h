//
// Created by 徐鑫平 on 2022/12/29.
//

#ifndef FINAL_PROJECT_WORKTHREAD_H
#define FINAL_PROJECT_WORKTHREAD_H
#include <vector>
#include "serverCore.h"
class threads_pool {
    std::vector<pid_t> runningThreads;
    std::vector<short> workersTasksCount;
public:
    void createThreadPool(int max_thread, server_socket *server);
    static void * threadWorker(void *args);
    void addTasks(int targetId);
    int foundMin();
};
#endif //FINAL_PROJECT_WORKTHREAD_H
