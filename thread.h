//
// Created by 徐鑫平 on 2022/12/29.
//

#ifndef FINAL_PROJECT_THREAD_H
#define FINAL_PROJECT_THREAD_H
#include <vector>
#include <pthread.h>
#include "server_core.h"
class threads_pool {
    std::vector<int> wait_list;
    std::vector<std::thread *> thread_list;
public:
    void create_thread_pool(int max_thread, server_socket *server);
    int thread_worker(server_socket *, std::vector<int> &wait_list);
    static void * thread_accept(void *args);
};
#endif //FINAL_PROJECT_THREAD_H
