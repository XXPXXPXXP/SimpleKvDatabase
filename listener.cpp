//
// Created by 神奇bug在哪里 on 2022/12/16.
//


#include "listener.h"
#include "settings.h"

void pipeHandle(int) {
    log(error, "socket发生了异常关闭！");
}
/* macOS/Linux 在初始化的时候比Windows少调用两个函数，不得不说，为什么这俩玩意总是在奇怪的地方搞差异化啊！！！ */
int listener::init(short port, database &datas) {
    data = &datas;
    listenSockId = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSockId == -1) {
        log(error, "socket init error!");
        return -1;
    }
    /* 创建基本的socket */
    int opt_code = 1;
    if (setsockopt(listenSockId, SOL_SOCKET, SO_REUSEADDR, (const void *) &opt_code, sizeof(opt_code)))
    {
        log(error,"端口复用设置失败！");
        close(listenSockId);
        exit(-1);
    }
    /* 设置端口复用 */
    memset(&ipConfig, 0, sizeof(ipConfig)); //对地址结构体填充0,optional on macOS
    ipConfig.sin_family = AF_INET;//IPv4
    ipConfig.sin_port = htons(port);
    ipConfig.sin_addr.s_addr = inet_addr("0.0.0.0");//绑定IP为0.0.0.0，接受来自任何IP的访问
    /* 创建地址结构体 */
    if (bind(listenSockId, (struct sockaddr *) &ipConfig, sizeof(ipConfig)) < 0) {
        log(error, "bind error!");
        return -1;
    }
    log(info, "Trying startup listening....");
    if (::listen(listenSockId, MAX_SOCKET) < 0) {
        log(error, "listen startup error!");
        exit(errno);
    }
    /* 准备创建epoll队列 */
    listeningEpoll = epoll_create(1);
    if (listeningEpoll == -1) {
        log(error, "Listen: epoll队列无法创建！");
        exit(errno);
    }
    /* 注册信号处理函数SIGPIPE(用于解决一些socket意外断开的问题) */
    signal(SIGPIPE, pipeHandle);
    listenEV.data.fd = listenSockId;
    listenEV.events = EPOLLIN;  // 类似于 kqueue的 EV_READ
    int ret = epoll_ctl(listeningEpoll, EPOLL_CTL_ADD, listenSockId, &listenEV);
    if (ret == -1) {
        log(error, "epoll_ctl error");
        exit(errno);
    }
    /* 创建epoll结构体并注册事件，准备用来阻塞 */
    return listenSockId;

}

[[noreturn]] void listener::listen() {
    int size = sizeof(listenEpollEvent) / sizeof(struct epoll_event);
    while (true) {
        log(info, "开始阻塞");
        int num = epoll_wait(listeningEpoll, listenEpollEvent, size, -1);
        if (num < 1) {
            log(error, "epoll队列异常！");
            continue;
        }
        int targetSockId = accept(listenSockId, nullptr, nullptr);    // 传入的socket
        if (targetSockId == -1) {
            log(error, "accept error!");
            continue;
        }
        log(info, "accept成功！", targetSockId);
        struct timeval timeOut{};
        timeOut.tv_sec = 3;
        timeOut.tv_usec = 0;
        /* 设置连接超时,防止连接卡服 */
        setsockopt(targetSockId, SOL_SOCKET, SO_RCVTIMEO, &timeOut, sizeof(timeOut));
        pool.start(targetSockId, data);
        /* 采用多进程来进行accept,线程进行处理  */
    }
}

void listener::stop() const {
    close(listenSockId);
}

