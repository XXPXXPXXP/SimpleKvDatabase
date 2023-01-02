﻿//
// Created by 徐鑫平 on 2022/12/16.
//

#ifndef FINAL_PROJECT_SERVERCORE_H
#define FINAL_PROJECT_SERVERCORE_H
#include <ctime>
/* 该分支不再能在mac上可用 */
#include <netinet/in.h>
#include <sys/types.h>
#include <deque>
#include "serverLog.h"
#include "database.h"
#include <sys/socket.h>
#include <arpa/inet.h>
/* 这是epoll的版本 */
#include <sys/epoll.h>
#include <csignal>
#include <unistd.h>
#include <cstring>
#include <string>

class serverSocket{
private:
    int listenSockId = -1;
    bool isInit = false;
    struct sockaddr_in ipConfig;
    database temp;
    database & data = temp;
public:
    bool init(short port, database &datas);
    /* 默认端口采用1433，也就是SQL的默认端口 */
    bool startListen();
    bool process(int target_sock_id, uint32_t type);
    bool process_get(int target_sock_id);
    bool process_delete(int target_sock_id);
    bool process_add(int target_sock_id);
    static bool send_header(int target_sock_id, uint32_t full_size, uint32_t type) ;
    ~serverSocket()
    {
        log(warning,"server触发了回收!");
        stop();
    }

    void stop() const;

    static bool send_safe(int target_sock_id, void *data_to_send, uint32_t size, int extra);
};
void cleanup( int signum);
#endif //FINAL_PROJECT_SERVERCORE_H
