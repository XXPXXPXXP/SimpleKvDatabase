//
// Created by 徐鑫平 on 2022/12/29.
//
#include "workThread.h"
#include <thread>

void threadsPool::createThreadPool(int maxThread, serverSocket *server) {
    pthread_t pthread;
    struct argsTpMode *args = new struct argsTpMode;
    runningThreads.resize(maxThread);
    args->_this = this;
    args->server = server;
    /* 构造将要传入的参数列表 */
    pthread_mutex_init(&taskLocker, nullptr);
    /* 初始化互斥锁，用于不同的线程进行竞争 */
    for (int i = 0; i < maxThread; ++i) {
        args->count = i;
        if (pthread_create(&(runningThreads.at(0)), nullptr, threadWorker, args)) {
            log(error, "线程创建错误！", i);
        }
    }
    /* 创建实际的线程 */
}


[[noreturn]] void *threadsPool::threadWorker(void *args) {
    auto *_this = reinterpret_cast<struct argsTpMode *>(args)->_this;
    auto *server = reinterpret_cast<struct argsTpMode *>(args)->server;
    int i = reinterpret_cast<struct argsTpMode *>(args)->count;
    int kId = _this->kId;
    /* 接收传入的参数列表 */
    delete reinterpret_cast<struct argsTpMode *>(args);
    /* 回收堆内存中的资源 */
    while (true);
}


void threadsPool::addTasks(int targetId) {
    return;
}

void threadsPool::producerConsumerMode(int targetSockId, serverSocket *server) {
    struct argsPcMode *args = new struct argsPcMode;
    args->server = server;
    args->sockID = targetSockId;
    runningThreads.resize(1);
    if (pthread_create(&(runningThreads.at(0)), nullptr, process_theard, args)) {
        log(error, "线程创建错误！", targetSockId);
    }
    // usleep(20);
}

void *threadsPool::process_theard(void *args) {
    int targetSockId = reinterpret_cast<struct argsPcMode *>(args)->sockID;
    auto server = reinterpret_cast<struct argsPcMode *>(args)->server;
    delete reinterpret_cast<struct argsPcMode *>(args);
    /* 接收参数 */
    uint32_t magic_number;
    while (read(targetSockId, (char *) &magic_number, 4) > 0)//判断socket是否结束
    {
        log(info, "线程读取sock成功", targetSockId);
        //准备读取header数据
        uint32_t size, type, rubbish;
        if (magic_number != 1234) {
            log(warning, "MagicNumber不匹配！", targetSockId);
            continue;
        }
        log(info, "magicNumber校验通过", targetSockId);
        if (read(targetSockId, (char *) &size, 4) <= 0) {
            log(error, "无法读取Size!", targetSockId);
            continue;
        }
        if (read(targetSockId, (char *) &type, 4) <= 0) {
            log(error, "无法读取Type!", targetSockId);
            continue;
        }
        if (read(targetSockId, (char *) &rubbish, 4) <= 0) {
            log(error, "无法读取Padding!", targetSockId);
            continue;
        }
        log(info, "header信息成功接收", targetSockId);
        server->process(targetSockId, type);
        log(info, "数据已完成处理!");
    }
    /* 开始读取head */
    close(targetSockId);
    log(info, "当前sock连接已断开!", targetSockId);
    return nullptr;
    /* 断开连接并且释放资源 */
}






