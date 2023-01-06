//
// Created by 神奇bug在哪里 on 2023/1/6.
//

#ifndef FINAL_PROJECT_SENDER_H
#define FINAL_PROJECT_SENDER_H
#include <string>
#include "workThread.h"
class sender : public threadsPool{
private:
public:
    [[noreturn]] static void manager(void * _this);
    void start() override;
    [[noreturn]] static void *worker(int *senderFd, sender *_this);

};
#endif//FINAL_PROJECT_SENDER_H
