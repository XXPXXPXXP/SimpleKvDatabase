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
    void start(int senderFd[2]) override;
    [[noreturn]] static void *worker(int *senderFd, sender *_this);
};
bool sendField(int target_sock_id, void *data_to_send, uint32_t size, int extra);
bool sendHeader(int target_sock_id, uint32_t full_size, uint32_t type);
#endif//FINAL_PROJECT_SENDER_H
