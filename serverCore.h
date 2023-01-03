//
// Created by 神奇bug在哪里 on 2022/12/16.
//

#ifndef FINAL_PROJECT_SERVERCORE_H
#define FINAL_PROJECT_SERVERCORE_H
#include <ctime>
#include <sys/socket.h>
#include <arpa/inet.h>
/* inet.h 使用的是MacOSX13.0.sdk内置的版本 */
#include <netinet/in.h>
#include <kqueue/sys/event.h>
#include <cstring>
#include <string>
//#include <MacTypes.h>
#include <sys/types.h>
#include <unistd.h>
#include <deque>
#include <csignal>
/* macOS搞了另一套东西来实现epoll */
#include <thread>
#include "serverLog.h"
#include "database.h"

class serverSocket{
private:
    int listenSockId = -1;
    bool isInit = false;
    struct sockaddr_in ipConfig;
    database temp;
    struct kevent *listenWatchList,*listenTiggerList;
    database & data = temp;
public:
    bool init(short port, database &datas);
    /* 默认端口采用1433，也就是SQL的默认端口 */
    bool startListen();
    bool process(int target_sock_id, uint32_t type);
    bool processGet(int target_sock_id);
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
