//
// Created by 徐鑫平 on 2022/12/16.
//

#ifndef FINAL_PROJECT_MAIN_H
#define FINAL_PROJECT_MAIN_H
#include <iostream>
#include <vector>
#include "serverCore.h"
serverSocket * pTargetServer;
void cleanup( int signum)
{
    std::cout << "[W] 收到外部信号 ("<<signum<<") !"<<std::endl;
    pTargetServer->stop();
    /* 用以停止所有正在运行的进程 */
    log(info,"所有进程均已完成回收！");
    exit(signum);

}
#endif //FINAL_PROJECT_MAIN_H
