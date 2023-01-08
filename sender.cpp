//
// Created by 神奇bug在哪里 on 2023/1/6.
//
#include "sender.h"
#include <sys/socket.h>
#include <csignal>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-result"
void sender::start(int senderFd[2]) {
    pipeEpoll = epoll_create(1);
    if (pipeEpoll == -1)
    {
        log(error,"sender:epoll队列无法创建");
        exit(errno);
    }
    pipeEV.data.fd = senderFd[0];
    pipeEV.events = EPOLLIN;
    int ret = epoll_ctl(pipeEpoll, EPOLL_CTL_ADD, senderFd[0], &pipeEV);
    if (ret == -1) {
        log(error, "sender:epoll_ctl error");
        exit(errno);
    }
    for (int i = 0; i < minThread; ++i) {
        workerIDs.emplace_back(worker, senderFd, this);
    }
    workerIDs.at(0).join();
    log(error, "reader:管理线程异常退出！");
    exit(-1);
}

void sender::manager(void *_this) {
    while (true);
}

void *sender::worker(int *senderFd, sender *_this) {
    log(info,"sender:工作线程创建!");
    int sockID;
    uint32_t type;
    int size = sizeof(pipeEpollEvent) / sizeof(struct epoll_event);
    while (true) {
        int num = epoll_wait(_this->pipeEpoll, _this->pipeEpollEvent, size, -1);
        if (num < 1) {
            log(error, "epoll队列异常！");
            continue;
        }
        log(info,"sender:epoll触发!");
        _this->pipeLocker.lock();
        read(senderFd[0], &type, 4);
        switch (type) {
            case 3: {//put
                bool status;
                read(senderFd[0], &status, sizeof(bool));
                read(senderFd[0], &sockID, 4);
                _this->pipeLocker.unlock();
                sendHeader(sockID, 1, 3);
                sendField(sockID, &status, sizeof(bool), MSG_NOSIGNAL);
                break;
            }
            case 4: {//delete
                bool status;
                read(senderFd[0], &status, sizeof(bool));
                read(senderFd[0], &sockID, sizeof(int));
                _this->pipeLocker.unlock();
                sendHeader(sockID, 1, 4);
                sendField(sockID, &status, sizeof(bool), MSG_NOSIGNAL);
                break;
            }
            case 5: {//get
                uint32_t valueSize;
                std::string value;
                log(info,"sender:get开始准备发送");
                read(senderFd[0], &valueSize, 4);
                value.resize(valueSize);
                log(info,"sender:从管道读取valueSize成功！");
                read(senderFd[0], const_cast<char *>(value.data()), valueSize);
                log(info,"sender:从管道读取value成功!value="+value);
                read(senderFd[0], &sockID, sizeof(int));
                log(info,"sender:从管道读取sockID成功！");
                _this->pipeLocker.unlock();
                log(info,"sender:get从管道获取数据完成");
                sendHeader(sockID,sizeof(valueSize)+value.size(),type);
                sendField(sockID,&valueSize,4,MSG_NOSIGNAL);
                sendField(sockID,const_cast<char *>(value.data()),valueSize,MSG_NOSIGNAL);
                break;
            }
            default:
                log(error, "sender:错误的type!",(int)type);
                _this->pipeLocker.unlock();
                break;
        }
    }
}

bool sendField(int target_sock_id, void *data_to_send, uint32_t size, int extra)
/*
 * description: 用来发送每一个Field的数据,会检查read函数的返回值，这样做是为了减少重复的代码数量
 * return: 是否成功
 */
{
    if (send(target_sock_id, data_to_send, size, extra) != size) {
        log(error, "发送body失败！");
        return false;
    } else
        return true;
}


bool sendHeader(int target_sock_id, uint32_t full_size, uint32_t type) {
    /*
     * 描述：用于发送response的头部数据
     * 返回值: 是否发送成功
     * 参数列表:
     *  target_sock_id: 发送的目标sock_id
     *  full_size: 需要发送的body数据的总大小
     *  type: 该数据包的类型
     */
    uint32_t magic_number = 1234, padding = {0};
    /*
     * field: magic_number
     * size: 4 bytes
     * type: uint32_t
     * description: 固定为1234
     */
    bool result;
    result = sendField(target_sock_id, &magic_number, 4, MSG_NOSIGNAL);
    /*
     * field: Size(head)
     * size: 4 bytes
     * type: uint32_t
     * description: body的大小
     */
    result = result && sendField(target_sock_id, &full_size, 4, MSG_NOSIGNAL);
    result = result && sendField(target_sock_id, &type, 4, MSG_NOSIGNAL);
    result = result && sendField(target_sock_id, &padding, 4, MSG_NOSIGNAL);
    return result;
}
#pragma clang diagnostic pop