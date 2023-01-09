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
    if (setsockopt(listenSockId, SOL_SOCKET, SO_REUSEADDR, (const void *) &opt_code, sizeof(opt_code))) {
        log(error, "端口复用设置失败！");
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
    log(info, "accepts: 工作线程创建！");
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
    log(info, "listener:开始初始化");
    init(SERVER_PORT);
    for (int i = 0; i < ACCEPT_THREAD; ++i) {
        acceptThreads.emplace_back(accepts, this, readerFd);
    }
    /* 启动监听线程 */
    acceptThreads.at(0).join();
    /* 启动sender的任务同步方法 */
    senderTaskerGetter(senderFd);

}

void *networkIO::reader(int readerFd[2], int targetSockId) {
    log(info, "reader:任务执行！");
    /* 从任务列表中取出任务 */
    uint32_t magic_number;
    while (read(targetSockId, (char *) &magic_number, 4) > 0)//判断socket是否结束
    {
        log(info, "线程读取sock成功", targetSockId);
        //读取header数据
        uint32_t size = 0, type = 0, rubbish = 0;
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
                log(info, "收到put请求", targetSockId);
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
                write(readerFd[1], &targetSockId, sizeof(int));
                pipeReadLocker.unlock();
                break;
            }
            case 1://delete
            {
                uint32_t keySize;
                if (read(targetSockId, &keySize, 4) < 0) {
                    log(error, "读取数据大小失败!");
                    continue;
                }
                std::string targetKey;
                targetKey.resize(keySize);
                if (read(targetSockId, const_cast<char *>(targetKey.data()), keySize) != keySize) {
                    log(error, "读取key内容失败!");
                    continue;
                }
                pipeReadLocker.lock();
                write(readerFd[1], &type, 4);
                write(readerFd[1], &keySize, 4);
                write(readerFd[1], const_cast<char *>(targetKey.data()), keySize);
                write(readerFd[1], &targetSockId, sizeof(int));
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
                    continue;
                }
                key.resize(keySize);
                long real_size = read(targetSockId, const_cast<char *>(key.data()), keySize);
                if (real_size < 0) {
                    log(error, "读取Key出现错误!", targetSockId);
                    key.clear();
                    continue;
                } else if (real_size != keySize) {
                    log(error, "读取到的Key大小异常!", targetSockId);
                    key.clear();
                    continue;
                }
                /* 下面向数据线程发送数据 */
                pipeReadLocker.lock();
                write(readerFd[1], &type, 4);
                write(readerFd[1], &keySize, 4);
                write(readerFd[1], const_cast<char *>(key.data()), keySize);
                write(readerFd[1], &targetSockId, sizeof(int));
                pipeReadLocker.unlock();
                break;
            }
            default://错误处理
                log(error, "接收到错误的类型！");
                break;
        }
        log(info, "reader: 数据已完成传递!");
    }
    close(targetSockId);
    log(warning, "连接已经被回收！", targetSockId);
    return nullptr;
}

void networkIO::senderTaskerGetter(int *senderFd) {
    /*
     * description: 用于从管道中取回database的返回数据并且发送
     * return: 该函数不会返回，将在主线程中持续运行
     * 参数列表: senderFd:用于与database进行通信的管道，仅保留读端
     */
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
                    log(error, "sender:管道读取错误！");
                    continue;
                }
                networkIoThreads.addTasks(std::bind(&networkIO::putResponse, this, status, sockID));
                break;
            }
            case 4: {
                bool status, readResult;
                readResult = read(senderFd[0], &status, sizeof(bool)) == sizeof(bool);
                readResult = readResult && read(senderFd[0], &sockID, sizeof(int)) == sizeof(int);
                if (!readResult) {
                    log(error, "sender:管道读取错误！");
                    continue;
                }
                networkIoThreads.addTasks(std::bind(&networkIO::deleteResponse, this, status, sockID));
                break;
            }
            case 5: {
                bool readResult;
                uint32_t valueSize;
                std::string value;
                readResult = read(senderFd[0], &valueSize, 4) == 4;
                value.resize(valueSize);
                readResult = readResult && read(senderFd[0], const_cast<char *>(value.data()), valueSize) == valueSize;
                readResult = readResult && read(senderFd[0], &sockID, sizeof(int)) == sizeof(int);
                networkIoThreads.addTasks(std::bind(&networkIO::getResponse, this, valueSize, value, sockID));
                break;
            }
            default:
                log(error, "sender:未知的type！");
        }
    }
}

void networkIO::putResponse(bool status, int sockID) {
    /*
     * description: 用于发送putResponse返回给Client
     * return: 该函数没有返回值
     */
    bool result;
    result = sendHeader(sockID, 1, 3);
    result = result && sendField(sockID, &status, sizeof(bool), MSG_NOSIGNAL);
    if (!result)
        log(error, "sender:putResponse发送失败！");
}

void networkIO::deleteResponse(bool status, int sockID) {
    bool result;
    result = sendHeader(sockID,1,4);
    result = result && sendField(sockID,&status, sizeof(bool),MSG_NOSIGNAL);
    if (!result)
        log(error,"sender:deleteResponse发送失败！");
}

void networkIO::getResponse(uint32_t size, string &targetValue, int sockID) {
    bool result;
    result = sendHeader(sockID, sizeof(uint32_t)+targetValue.size(),5);
    result = result && sendField(sockID,&size, sizeof(size),MSG_NOSIGNAL);
    result = result && sendField(sockID,const_cast<char *>(targetValue.data()),size,MSG_NOSIGNAL);
    if (!result)
        log(error,"sender:getResponse发送失败！");
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