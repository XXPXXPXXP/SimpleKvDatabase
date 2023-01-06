//
//  main.cpp
//  Final_project
//
//  Created by 神奇bug在哪里 on 2022/12/14.
//
#include "master.h"
#include "listener.h"
#include <wait.h>
master * p_master;
class master Master;
int main() {
    log(info,"starting...");
    p_master = &Master;
    Master.start();
    //理论上不会再往下执行
    log(error,"Unknown Reason for main exiting!");
    return 0;
}

void handler(int singleNum)
{
    log(info,"主进程已收到信号",singleNum);
    p_master->exit();
}

void stopProcessor(int singleNum) {
    log(info,"子进程收到信号！",singleNum);
    exit(singleNum);
}

void master::exit()
{
    for (int i = 0; i < pid.size(); ++i) {
        kill(pid.at(i),SIGTERM);
        waitpid(pid.at(i),nullptr,1);
        log(info,"进程已回收",i);
    }
    /* 用以停止所有正在运行的进程 */
    log(info,"所有进程均已完成回收！");
    ::exit(0);
}

[[noreturn]] void master::start() {
    signal(SIGTERM, handler); //注册信号处理函数
    signal(SIGSEGV, sigsegvHandler);
    datas.init();
    log(info,"database初始化完成");
    auto args = new struct args;
    args->sockID = targetServer.init(SERVER_PORT, datas);
    args->server = &targetServer;
    args->datas = &datas;
//    pid.resize(PROCESS_SIZE);
//    for (int i = 0; i < PROCESS_SIZE; ++i) {
//        pid.at(i) = clone(listen,args+sizeof(struct args),CLONE_VM|CLONE_SETTLS, args);
//        log(info,"clone status",(int)pid.at(i));
//    }

    targetServer.listen();

}

int master::listen(void * args) {
    log(info,"clone success");
    int sockID = reinterpret_cast<struct args *>(args)->sockID;
    auto Server = reinterpret_cast<struct args *>(args)->server;
    auto datas = reinterpret_cast<struct args *>(args)->datas;
    signal(SIGTERM, stopProcessor);//注册子进程退出时的函数
    Server->listen();
}

void sigsegvHandler(int num)
{
    log(info,"接收到信号",num);
    exit(0);
}

