//
// Created by 神奇bug在哪里 on 2023/1/5.
//

#ifndef FINAL_PROJECT_MASTER_H
#define FINAL_PROJECT_MASTER_H
#include <vector>
#include <thread>
class master {
private:
    std::vector<pid_t> runningProcess;
public:
    void exit();
};
void hander(int);
#endif //FINAL_PROJECT_MASTER_H
