//
// Created by 徐鑫平 on 2022/12/29.
//
#include "thread.h"
void * threads_pool::thread_accept(std::vector<void *> args) {
    auto * _this = reinterpret_cast<threads_pool *>(args.at(0));
    auto server = reinterpret_cast<server_socket *>(args.at(1));
    auto wait_list = reinterpret_cast<std::vector<int> *>(args.at(2));

    return nullptr;
}

void threads_pool::create_thread_pool(int max_thread, server_socket *server) {
    for (int i = 0; i < max_thread/3+1; ++i) {
        pthread_t p_thread_accept;
        std::vector<void *> args(3);
        args.at(0) = this;
        args.at(1) = server;
        args.at(2) = &wait_list;
        if (pthread_create(&p_thread_accept, nullptr,thread_accept,&args))
        {
            log(error,"accept线程无法创建!");
        }
    }
    for (int i = 0; i < max_thread/3*2; ++i) {

    }
}



