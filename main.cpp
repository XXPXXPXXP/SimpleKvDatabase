//
//  main.cpp
//  Final_project
//
//  Created by 神奇bug在哪里 on 2022/12/14.
//
#include "settings.h"
#include "serverCore.h"
#include <wait.h>
#include <vector>

serverSocket * p_target_server;
std::vector<pid_t> runningProcess;
int main() {
    database datas;
    datas.init();
    serverSocket targetServer;
    p_target_server = &targetServer;
    targetServer.init(SERVER_PORT, datas);
    signal(SIGTERM, cleanUpAll); //注册信号处理函数
    targetServer.startServer();
}

void cleanUpAll(int singleNumber)
{
    if (!p_target_server->isChild()){
    log(warning, "收到外部信号！", singleNumber);
    p_target_server->stop();
    for (int i = 0; i < runningProcess.size(); ++i) {
        kill(runningProcess.at(i),SIGTERM);
        waitpid(runningProcess.at(i),nullptr,1);
        log(info,"进程已回收",i);
    }
    /* 用以停止所有正在运行的进程 */
    log(info,"所有进程均已完成回收！");
    exit(singleNumber);
    }
    else
    {
        log(info,"子进程退出!");
        exit(0);
    }
}


