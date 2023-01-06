//
// Created by 神奇bug在哪里 on 2022/12/29.
//
#include "Procotol.h"
#include "workThread.h"
#include <thread>

void threadsPool::startSingle(int targetSockId, database * database) {
    std::thread workers(worker,targetSockId,database,&threadsCount);
    log(info,"线程创建!");
    threadsCount++;
}

void *threadsPool::worker(int targetSockId, database *datas, std::atomic<int> *threadsCount) {
    /* 接收参数 */
    uint32_t magic_number;
    while (read(targetSockId, (char *) &magic_number, 4) > 0)//判断socket是否结束
    {
        log(info, "线程读取sock成功", targetSockId);
        //准备读取header数据
        uint32_t size, type, rubbish;
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
        process(targetSockId, type, nullptr);
        log(info, "数据已完成处理!");
    }

    close(targetSockId);
    log(info, "当前sock连接已断开!", targetSockId);
    log(info,"线程退出！");
    (*threadsCount)--;
    exit(0);
    /* 断开连接并且释放资源 */
}






