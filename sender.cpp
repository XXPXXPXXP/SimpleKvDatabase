//
// Created by 神奇bug在哪里 on 2023/1/6.
//
#include "sender.h"
#include "Procotol.h"
#include <bits/socket.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-result"
void sender::start() {
    managerID = std::thread(manager, this);
    for (int i = 0; i < minThread; ++i) {
        workerIDs.emplace_back(worker, nullptr, this);
    }
    managerID.join();
    log(error,"reader:管理线程异常退出！");
    exit(-1);
}

void sender::manager(void *_this) {
}

void *sender::worker(int *senderFd, sender *_this) {
    close(senderFd[1]);//关闭写端
    int sockID;
    uint32_t type;
    while (true)
    {
        _this->pipeLocker.lock();
        read(senderFd[0],&type,4);
        switch (type) {
            case 3:{
                bool status;
                read(senderFd[0],&status, sizeof(bool));
                read(senderFd[0],&sockID,4);
                _this->pipeLocker.unlock();
                sendHeader(sockID, 1, 3);
                sendField(sockID,&status, sizeof(bool),MSG_NOSIGNAL);
                break ;
                }
            case 4: {
                bool status;
                read(senderFd[0], &status, sizeof(bool));
                read(senderFd[0], &sockID, 4);
                _this->pipeLocker.unlock();
                sendHeader(sockID, 1, 4);
                sendField(sockID, &status, sizeof(bool), MSG_NOSIGNAL);
                break;
                }
                case 5:
                {
                    uint32_t valueSize;
                    std::string value;
                    read(senderFd[0],&valueSize,4);
                    read(senderFd[0],const_cast<char *>(value.data()),valueSize);
                    read(senderFd[0],&sockID, sizeof(int));
                    _this->pipeLocker.unlock();
                    break ;
                }
                default:
                    log(error,"sender:错误的type!");
                    _this->pipeLocker.unlock();
                    break ;
        }
    }

}

#pragma clang diagnostic pop