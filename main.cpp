//
//  main.cpp
//  Final_project
//
//  Created by 徐鑫平 on 2022/12/14.
//
#include "main.h"
#include "settings.h"
#include <csignal>

int main(int argc, const char * argv[]) {
    database datas;
    datas.init();
    serverSocket target_server;
    p_target_server = &target_server;
    target_server.init(SERVER_PORT, datas);
    signal(SIGTERM, cleanup); //注册信号处理函数
    target_server.startListen();
    std::cout << "[W] server stoped!\n";
    return 0;
}

void cleanup( int signum)
{
    std::cout << "[W] 收到外部信号 ("<<signum<<") !"<<std::endl;
    p_target_server->stop();
    /* 用以停止所有正在运行的进程 */
    log(info,"所有进程均已完成回收！");
    exit(signum);

}