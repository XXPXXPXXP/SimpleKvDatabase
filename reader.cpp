//
// Created by 神奇bug在哪里 on 2023/1/6.
//

#include "reader.h"
#include <unistd.h>
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-result"
[[noreturn]] void *reader::worker(int readerFd[2], reader *_this) {
    log(info,"Reader:工作线程创建成功！");
    while (true) {
        if (_this->targetSockIds.empty())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));//1ms轮询是否有新任务
            continue;
        }
        _this->taskLocker.lock();
        int targetSockId = _this->targetSockIds.front();
        _this->targetSockIds.pop();
        _this->taskLocker.unlock();
        log(info,"reader:已获取到任务！");
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
                    break;
                }
                default:
                    log(error,"接收到错误的类型！");
                    break;
            }
            log(info, "reader: 数据已完成传递!");
        }
        close(targetSockId);
        log(warning,"连接已经被回收！",targetSockId);
    }
}
#pragma clang diagnostic pop
[[noreturn]] void reader::manager(void *_this) {
    while (true);
}
void reader::start(int readerFd[2]) {
    //managerID = std::thread(manager, this);
    for (int i = 0; i < minThread; ++i) {
        workerIDs.emplace_back(worker, readerFd, this);
    }
}

void reader::addTask(int targetSockID) {
    targetSockIds.push(targetSockID);
}
