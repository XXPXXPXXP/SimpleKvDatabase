﻿//
// Created by 神奇bug在哪里 on 2022/12/16.
//

#ifndef FINAL_PROJECT_SERVERCORE_H
#define FINAL_PROJECT_SERVERCORE_H

/* 该分支不再能在mac上可用 */
#include <netinet/in.h>
#include <deque>
#include "serverLog.h"
#include "database.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
/* 这是epoll的分支，所以自然就使用epoll啦 */
#include <csignal>
#include <cstring>
/* 为了能够使用memset */
#include <string>

class serverSocket {
private:
    int listenSockId = -1;
    struct sockaddr_in ipConfig;
    database temp;
    database &data = temp;
    bool child = false;
    struct epoll_event listenEpollEvent[128];
    struct epoll_event listenEV{};
    int listeningEpoll;
public:
    bool init(short port, database &datas);

    /* 默认端口采用1433，也就是SQL的默认端口 */
    [[noreturn]]   void listen();

    bool process(int target_sock_id, uint32_t type);

    bool get(int targetSockId);
    void setChild(){child = true;}
    bool deleteData(int targetSockId);
    bool isChild() const{return child;}
    bool add(int target_sock_id);;

    static bool sendHeader(int target_sock_id, uint32_t full_size, uint32_t type);

    ~serverSocket() {
        log(warning, "server触发了回收!");
        stop();
    }

    void stop() const;

    static bool sendField(int target_sock_id, void *data_to_send, uint32_t size, int extra);
};

void cleanUpAll(int singleNumber);
#endif //FINAL_PROJECT_SERVERCORE_H
