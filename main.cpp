//
//  main.cpp
//  Final_project
//
//  Created by 神奇bug在哪里 on 2022/12/14.
//
#include "main.h"


int main(int argc, const char * argv[]) {
    database datas;
    datas.init();
    serverSocket target_server;
    pTargetServer = &target_server;
    target_server.init(1434, datas);
    signal(SIGTERM, cleanup); //注册信号处理函数
    target_server.startListen();
    std::cout << "[W] server stoped!\n";
    return 0;
}
