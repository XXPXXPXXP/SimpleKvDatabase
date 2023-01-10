//
//  main.cpp
//  Final_project
//
//  Created by 神奇bug在哪里 on 2022/12/14.
//

#include "init.h"
#include "networkIO.h"
#include <wait.h>
#include <unistd.h>

int main() {
    log(info, "starting...");
    pInit = &init;
    init.start();
    //理论上不会再往下执行
    log(error, "Unknown Reason for main exiting!");
    return 0;
}

void sigHandler(int singleNum) {
    log(info, "主进程已收到信号", singleNum);
    pInit->exit();
}

void init::exit() {
    for (int i = 0; i < pid.size(); ++i) {
        kill(pid.at(i), SIGTERM);
        waitpid(pid.at(i), nullptr, 1);
        log(info, "进程已回收", i);
    }
    /* 用以停止所有正在运行的进程 */
    log(info, "所有进程均已完成回收！");
    ::exit(0);
}

[[noreturn]] void init::start() {
    int readerFd[2], senderFd[2];
    /* 下面开始创建管道 */
    if (pipe(readerFd) == -1 || pipe(senderFd) == -1) {
        log(error, "init: 管道出现错误！");
        ::exit(errno);
    }
    /* 下面开始创建进程 */
    pid_t processorID;
    processorID = fork();
    if (processorID == 0) {
        /*
         * 进程1: 数据进程
         * description: 用于处理数据IO和文件IO
         */
        database database;
        database.start(readerFd, senderFd);//正常情况下该函数将不会返回
        log(error, "databaseMain:数据进程异常退出！");
        ::exit(errno);
    }
    pid.emplace_back(processorID);
    processorID = fork();
    if (processorID == 0) {
        /*
         * 进程2: 网络进程
         * description: 用于处理网络IO
         */
        networkIO networkIO;
        networkIO.start(readerFd, senderFd); //正常情况下该函数不会返回
        log(error, "networkIO:网络IO进程异常退出！");
        ::exit(errno);
    }
    pid.emplace_back(processorID);
    /* 下面父进程关闭管道 */
    close(readerFd[0]);
    close(readerFd[1]);
    close(senderFd[0]);
    close(senderFd[1]);
    log(info, "父进程已关闭全部管道!");
    signal(SIGTERM, sigHandler); //注册信号处理函数
    signal(SIGSEGV, sigHandler);
    signal(SIGINT, sigHandler);
    signal(SIGSTOP, sigHandler);
    signal(SIGKILL, sigHandler);
    signal(SIGQUIT, sigHandler);
    log(info, "主进程信号处理函数注册完成");
    management();//父进程负责监听各子进程的异常退出情况
}

[[noreturn]] void init::management() {
    auto * status = new int;
    while (true){
        waitpid(-1,status,0);
        if (WIFEXITED(* status))//进程异常退出
        {
            log(error,"子进程异常退出！");
            restart();
        }
        else
            break;
    }
    exit();
    ::exit(-1);
}

void init::restart() {
    log(warning,"init: 进程自动重启开始...");
    for (int i = 0; i < pid.size(); ++i) {
        kill(pid.at(i), SIGTERM);
        waitpid(pid.at(i), nullptr, 1);
        log(info, "进程已停止", i);
    }
    log(warning,"init:开始重启!");
    pInit->start();
}


