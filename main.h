//
// Created by 徐鑫平 on 2022/12/16.
//

#ifndef FINAL_PROJECT_MAIN_H
#define FINAL_PROJECT_MAIN_H
#include <iostream>
#include <vector>
#include "server_core.h"
std::vector<server_socket *> runnings;
void cleanup( int signum)
{
    std::cout << "[W] 收到外部信号 ("<<signum<<") !"<<std::endl;
    for (int i = 0; i < runnings.size(); ++i) {
        std::cout<<"[W] 开始回收第"<<i+1<<"个实例"<<std::endl;
        runnings.at(i)->stop();
    }
    /* 用以停止所有正在运行的进程 */
    std::cout<<"[I] 所有实例已完成回收"<<std::endl;
    exit(signum);

}
#endif //FINAL_PROJECT_MAIN_H
