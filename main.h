//
// Created by 徐鑫平 on 2022/12/16.
//

#ifndef FINAL_PROJECT_MAIN_H
#define FINAL_PROJECT_MAIN_H
#include <iostream>
#include <vector>
#include "serverCore.h"
serverSocket * p_target_server;
void cleanup( int signum)
{
    std::cout << "[W] 收到外部信号 ("<<signum<<") !"<<std::endl;
    p_target_server->stop();
    /* 用以停止所有正在运行的进程 */
    std::cout<<"[I] 所有实例已完成回收"<<std::endl;
    exit(signum);

}
#endif //FINAL_PROJECT_MAIN_H
