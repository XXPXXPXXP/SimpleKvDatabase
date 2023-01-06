//
// Created by 神奇bug在哪里 on 2023/1/5.
//

#ifndef FINAL_PROJECT_MASTER_H
#define FINAL_PROJECT_MASTER_H
#include <vector>
#include <thread>
#include "database.h"
#include "settings.h"
#include "listener.h"
class master {
private:
    std::vector<pid_t> pid;
public:
    void exit();
    [[noreturn]] void start();
    static int reader(void *);
    static int dataProvider(void *);
    static int sender(void *);
};
struct args{
    int sockID;
    listener * server;
    database * datas;
};
void handler(int);
void stopProcessor(int singleNum);
void sigsegvHandler(int);
#endif //FINAL_PROJECT_MASTER_H
