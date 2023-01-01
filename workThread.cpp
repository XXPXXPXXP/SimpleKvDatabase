//
// Created by 徐鑫平 on 2022/12/29.
//
#include "workThread.h"
#include <thread>
#include <climits>
void threads_pool::createThreadPool(int max_thread, server_socket *server) {
    pthread_t p_thread_accept;
    std::vector<void *> args(3);
    args.at(0) = this;
    args.at(1) = server;
    /* 构造将要传入的参数列表 */
    /* 创建实际的线程 */
}


void * threads_pool::threadWorker(void *args) {
    auto * _this = reinterpret_cast<threads_pool *>(reinterpret_cast<std::vector<void *> *>(args)->at(0));
    auto * server = reinterpret_cast<server_socket *>(reinterpret_cast<std::vector<void *> *>(args)->at(1));
    auto * kId = reinterpret_cast<int *>(reinterpret_cast<std::vector<void *> *>(args)->at(2));
    /* 接收传入的参数列表 */
    uint32_t magic_number;
    while (read(targetSockId, (char *) &magic_number, 4) > 0)//判断socket是否结束
    {
        //准备读取header数据
        uint32_t  size, type, rubbish;
        if (magic_number != 1234) {
            log(warning, "MagicNumber不匹配！");
            continue;
        }
        log(info, "magicNumber校验通过");
        if (read(targetSockId, (char *) &size, 4) <= 0) {
            logh(error);
            printf("无法读取Size!\t[id]: %d\n", targetSockId);
            continue;
        }
        if (read(targetSockId, (char *) &type, 4) <= 0) {
            logh(error);
            printf("无法读取Type!\t[id]: %d\n", targetSockId);
            continue;
        }
        if (read(targetSockId, (char *) &rubbish, 4) <= 0) {
            logh(error);
            printf("无法读取Padding!\t[id]: %d\n", targetSockId);
            continue;
        }
        log(info, "header信息成功接收", targetSockId);
        process(targetSockId, type);
        log(info, "数据已完成处理!");
        close(targetSockId);
        log(info, "当前sock连接已断开!", targetSockId);
        return nullptr;
    }
}

void threads_pool::addTasks(int targetId) {

}

int threads_pool::foundMin() {
    int min = INT_MAX;
    for (int i = 0; i < workersTasksCount.size(); ++i) {
        if (min > workersTasksCount.at(i))
            min = workersTasksCount.at(i);
    }
    return min;
}
