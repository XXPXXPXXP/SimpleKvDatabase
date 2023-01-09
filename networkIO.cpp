#pragma clang diagnostic push
#pragma ide diagnostic ignored "modernize-avoid-bind"
//
// Created by 神奇bug在哪里 on 2022/12/16.
//


#include "networkIO.h"
#include "settings.h"
#include <functional>
void pipeHandle(int) {
    log(error, "socket发生了异常关闭！");
}
/* macOS/Linux 在初始化的时候比Windows少调用两个函数，不得不说，为什么这俩玩意总是在奇怪的地方搞差异化啊！！！ */
int networkIO::init(short port) {
    listenSockId = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSockId == -1) {
        log(error, "socket reader error!");
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
    if (::listen(listenSockId, ACCEPT_QUEUE) < 0) {
        log(error, "reader startup error!");
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

    /* 初始化reader线程池 */
    return listenSockId;

}

[[noreturn]] void networkIO::accepts(networkIO *_this, int readerFd[2]) {
    log(info,"accepts: 工作线程创建！");
    int size = sizeof(listenEpollEvent) / sizeof(struct epoll_event);
    while (true) {
        log(info, "accepts:开始阻塞");
        int num = epoll_wait(_this->listeningEpoll, _this->listenEpollEvent, size, -1);
        if (num < 1) {
            log(error, "epoll队列异常！");
            continue;
        }
        int targetSockId = accept(_this->listenSockId, nullptr, nullptr);    // 传入的socket
        if (targetSockId == -1) {
            log(error, "accepts error!");
            continue;
        }
        log(info, "accepts:accept成功！", targetSockId);
        struct timeval timeOut{};
        timeOut.tv_sec = 3;
        timeOut.tv_usec = 0;
        /* 设置连接超时,防止连接一直不释放 */
        setsockopt(targetSockId, SOL_SOCKET, SO_RCVTIMEO, &timeOut, sizeof(timeOut));
        /* 下面开始处理交由reader处理 */
        _this->networkIoThreads.addTasks(std::bind(&networkIO::reader, _this, readerFd, targetSockId));
    }
}

void networkIO::stop() const {
    close(listenSockId);
}
void networkIO::start(int readerFd[2], int senderFd[2]) {
    close(readerFd[0]);//关闭读端，仅用来写入数据IO任务
    close(senderFd[1]);//关闭写端，仅用来读取数据IO结果
    log(info,"listener:开始初始化");
    init(SERVER_PORT);
    for (int i = 0; i < ACCEPT_THREAD; ++i) {
        acceptThreads.emplace_back(accepts, this,readerFd);
    }
    /* 启动监听线程 */
    acceptThreads.at(0).join();
    /* 启动sender的任务同步方法 */

}
void * networkIO::reader(int readerFd[2], int targetSockId) {
    log(info,"reader:任务执行！");
    /* 从任务列表中取出任务 */
    uint32_t magic_number;
    while (read(targetSockId, (char *) &magic_number, 4) > 0)//判断socket是否结束
    {
        log(info, "线程读取sock成功", targetSockId);
        //读取header数据
        uint32_t size = 0, type=0, rubbish=0;
        if (magic_number != 1234) {
            log(warning, "MagicNumber不匹配！", targetSockId);
            continue;
        }
        log(info, "magicNumber校验通过", targetSockId);
        if (read(targetSockId, (char *) &size, 4) <= 0) {
            log(error, "无法读取Size!", targetSockId);
            continue;
        }
        if (read(targetSockId, (char *) &type, 4) <= 0) {
            log(error, "无法读取Type!", targetSockId);
            continue;
        }
        if (read(targetSockId, (char *) &rubbish, 4) <= 0) {
            log(error, "无法读取Padding!", targetSockId);
            continue;
        }
        log(info, "header信息成功接收", targetSockId);
        switch (type) {
            case 0://put
            {
                log(info,"收到put请求",targetSockId);
                uint32_t keySize, valueSize;
                std::string target_key, target_value;
                if (read(targetSockId, &keySize, 4) < 0) {
                    log(error, "body:key_size读取失败！", targetSockId);
                    continue;
                }
                target_key.resize(keySize);
                if (read(targetSockId, const_cast<char *>(target_key.data()), keySize) < 0) {
                    log(error, "body:key读取失败！", targetSockId);
                    continue;
                }
                if (read(targetSockId, &valueSize, 4) < 0) {
                    log(error, "body:value_size读取失败!", targetSockId);
                    continue;
                }
                target_value.resize(valueSize);
                if (read(targetSockId, const_cast<char *>(target_value.data()), valueSize) < 0) {
                    log(error, "body:value读取失败!", targetSockId);
                    continue;
                }
                //所有数据完成读取
                pipeReadLocker.lock();
                write(readerFd[1], &type, 4);//将请求发往数据进程
                write(readerFd[1], &keySize, 4);
                write(readerFd[1], const_cast<char *>(target_key.data()), keySize);
                write(readerFd[1], &valueSize, 4);
                write(readerFd[1], const_cast<char *>(target_value.data()), valueSize);
                write(readerFd[1],&targetSockId, sizeof(int));
                pipeReadLocker.unlock();
                break ;
            }
            case 1://delete
            {
                uint32_t keySize;
                if (read(targetSockId, &keySize, 4) < 0) {
                    log(error, "读取数据大小失败!");
                    continue ;
                }
                std::string targetKey;
                targetKey.resize(keySize);
                if (read(targetSockId, const_cast<char *>(targetKey.data()), keySize) != keySize) {
                    log(error, "读取key内容失败!");
                    continue ;
                }
                pipeReadLocker.lock();
                write(readerFd[1],&type,4);
                write(readerFd[1],&keySize,4);
                write(readerFd[1],const_cast<char *>(targetKey.data()), keySize);
                write(readerFd[1],&targetSockId, sizeof(int));
                pipeReadLocker.unlock();
                break;
            }
            case 2://get
            {
                uint32_t keySize;
                std::string key;
                /* 下面开始读取来自socket的数据*/
                if (read(targetSockId, &keySize, 4) < 0) {
                    log(error, "读取body: size失败！", targetSockId);
                    continue ;
                }
                key.resize(keySize);
                long real_size = read(targetSockId, const_cast<char *>(key.data()), keySize);
                if (real_size < 0) {
                    log(error, "读取Key出现错误!", targetSockId);
                    key.clear();
                    continue ;
                } else if (real_size != keySize) {
                    log(error, "读取到的Key大小异常!", targetSockId);
                    key.clear();
                    continue ;
                }
                /* 下面向数据线程发送数据 */
                pipeReadLocker.lock();
                write(readerFd[1],&type,4);
                write(readerFd[1],&keySize,4);
                write(readerFd[1],const_cast<char *>(key.data()), keySize);
                write(readerFd[1],&targetSockId, sizeof(int));
                pipeReadLocker.unlock();
                break;
            }
            default://错误处理
                log(error,"接收到错误的类型！");
                break;
        }
        log(info, "reader: 数据已完成传递!");
    }
    close(targetSockId);
    log(warning,"连接已经被回收！",targetSockId);
    return nullptr;
}

void networkIO::senderTaskerSync(int *senderFd) {
    while (true) {
        log(info, "sender:工作线程创建!");
        int sockID;
        uint32_t type;
        /* 从管道读取任务目前是单线程，因此可以不用加锁 */
        if (read(senderFd[0], &type, 4) != 4) {
            log(error, "sender:管道读取错误!");
            continue;
        }
        switch (type) {
            case 3://put
            {
                bool status, readResult;
                readResult = read(senderFd[0], &status, sizeof(bool)) == sizeof(bool);
                readResult = readResult && read(senderFd[0], &sockID, 4) == 4;
                if (!readResult) {
                    log(error,"sender:管道读取错误！");
                    continue;
                }

            }
        }
    }

}
#pragma clang diagnostic pop