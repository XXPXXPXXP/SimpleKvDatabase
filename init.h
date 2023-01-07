//
// Created by 神奇bug在哪里 on 2023/1/5.
//

#ifndef FINAL_PROJECT_INIT_H
#define FINAL_PROJECT_INIT_H
#include <vector>
#include <thread>
#include "database.h"
#include "settings.h"
#include "networkIO.h"
class init {
private:
    std::vector<pid_t> pid;
public:
    void exit();
    [[noreturn]] void start();

    [[noreturn]] void management();
};
init * p_master;
class init Master;
void sigHandler(int);
void sigsegvHandler(int);
#endif//FINAL_PROJECT_INIT_H
