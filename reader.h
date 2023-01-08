//
// Created by 神奇bug在哪里 on 2023/1/6.
//

#ifndef FINAL_PROJECT_READER_H
#define FINAL_PROJECT_READER_H
#include "workThread.h"
#include <queue>
class reader : public threadsPool{
    std::queue<int> targetSockIds;
    std::mutex taskLocker;
    std::vector<std::thread> runningSingleModeThreads;
public:
    [[noreturn]] static void manager(void *_this);
    void start(int readerFd[2]);
    [[noreturn]] static void *worker(int readerFd[2],reader *_this);
    static void *singleWorker(reader *_this, int targetSockId, int readerFd[2]);
    void addTask(int targetSockID);
    void addSingleTask(int targetSockId, int readerFd[2]);
};


#endif//FINAL_PROJECT_READER_H
