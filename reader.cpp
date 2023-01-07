//
// Created by 神奇bug在哪里 on 2023/1/6.
//

#include "reader.h"
#include <unistd.h>
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-result"
[[noreturn]] void *reader::worker(int readerFd[2], int listenFd[2], reader *_this) {
    log(info,"Reader:工作线程创建成功！");
    while (true) {
        int num = epoll_wait(_this->pipeEpoll, _this->pipeEvent, sizeof(pipeEvent) / sizeof(struct epoll_event), -1);
        if (num < 1) {
            log(error, "reader:epoll队列异常！");
            sleep(1);
            continue;
        }
        log(info,"reader:epoll事件触发！");
        int targetSockId = -1;
        if (read(listenFd[0],&targetSockId,sizeof (int))!= sizeof(int))
        {
            log(error,"reader: 发生了管道读取错误!");
            continue;
        }
        log(info,"reader:pipe收到sockID",targetSockId);
        /* listen管道没有顺序要求。因此不用加锁 */
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
                    _this->pipeLocker.lock();
                    write(readerFd[1], &type, 4);//将请求发往数据进程
                    write(readerFd[1], &keySize, 4);
                    write(readerFd[1], const_cast<char *>(target_key.data()), keySize);
                    write(readerFd[1], &valueSize, 4);
                    write(readerFd[1], const_cast<char *>(target_value.data()), valueSize);
                    write(readerFd[1],&targetSockId, sizeof(int));
                    _this->pipeLocker.unlock();
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
                    _this->pipeLocker.lock();
                    write(readerFd[1],&type,4);
                    write(readerFd[1],&keySize,4);
                    write(readerFd[1],const_cast<char *>(targetKey.data()), keySize);
                    write(readerFd[1],&targetSockId, sizeof(int));
                    _this->pipeLocker.unlock();
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
                    _this->pipeLocker.lock();
                    write(readerFd[1],&type,4);
                    write(readerFd[1],&keySize,4);
                    write(readerFd[1],const_cast<char *>(key.data()), keySize);
                    write(readerFd[1],&targetSockId, sizeof(int));
                    _this->pipeLocker.unlock();
                }
                default:
                    log(error,"接收到错误的类型！");
            }
            log(info, "reader: 数据已完成传递!");
        }
    }
}
#pragma clang diagnostic pop
[[noreturn]] void reader::manager(void *_this) {
    while (true);

}
void reader::start(int readerFd[2],int listenFd[2]) {
    //managerID = std::thread(manager, this);
    pipeEpoll = epoll_create(1);
    if (pipeEpoll == -1)
    {
        log(error,"reader:epoll队列异常！");
        exit(errno);
    }
    pipeEV.data.fd = listenFd[0];
    pipeEV.events = EPOLLIN;
    int ret = epoll_ctl(pipeEpoll, EPOLL_CTL_ADD, listenFd[0], &pipeEV);
    if (ret == -1) {
        log(error, "reader: epoll_ctl error");
        exit(errno);
    }
    /*epoll队列完成初始化*/
    close(readerFd[0]);
    /* 关闭reader管道的读端，该管道用于向数据库发送请求 */
    close(listenFd[1]);
    /* 关闭listen管道的写端，该管道用于读取targetSockId */
    for (int i = 0; i < minThread; ++i) {
        workerIDs.emplace_back(worker, readerFd, listenFd, this);
    }
    //managerID.join();
    workerIDs.at(0).join();
    log(error,"reader:管理线程异常退出！");
    exit(-1);
}
