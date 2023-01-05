//
//  main.cpp
//  Final_project
//
//  Created by 神奇bug在哪里 on 2022/12/14.
//
#include "master.h"
#include "settings.h"
#include "listener.h"
#include <wait.h>
master * master;
int main() {
    database datas;
    datas.init();
    listener targetServer;
    targetServer.init(SERVER_PORT, datas);
    signal(SIGTERM, hander); //注册信号处理函数
    targetServer.listen();

}

void hander(int singleNum)
{
    log(info,"主进程已收到信号",singleNum);
    master->exit();

}
void master::exit()
{
    for (int i = 0; i < runningProcess.size(); ++i) {
        kill(runningProcess.at(i),SIGTERM);
        waitpid(runningProcess.at(i),nullptr,1);
        log(info,"进程已回收",i);
    }
    /* 用以停止所有正在运行的进程 */
    log(info,"所有进程均已完成回收！");
    ::exit(0);
}


