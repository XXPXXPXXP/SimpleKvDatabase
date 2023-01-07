//
// Created by 神奇bug在哪里 on 2023/1/6.
//

#ifndef FINAL_PROJECT_READER_H
#define FINAL_PROJECT_READER_H
#include "workThread.h"

class reader : public threadsPool{

public:
    [[noreturn]] static void manager(void * _this);
    void start(int readerFd[2],int listenFd[2]);
    [[noreturn]] static void *worker(int readerFd[2], int listenFd[2], reader *_this);
};


#endif//FINAL_PROJECT_READER_H
