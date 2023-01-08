//
// Created by 神奇bug在哪里 on 2022/12/24.
//

#ifndef FINAL_PROJECT_DATABASE_H
#define FINAL_PROJECT_DATABASE_H
#include "workThread.h"
#include <deque>
#include <map>
#include <mutex>
#include "serverLog.h"
class database : public threadsPool{
private:
    std::map<std::string,std::string> datas;
    std::mutex databaseLocker;
    std::mutex pipeReadLocker;
    std::mutex pipeWriteLocker;
    std::mutex saveLocker;
    struct epoll_event pipeEpollEvent[128];
    struct epoll_event pipeEV{};
    int pipeEpoll;
public:
    void start(int readerFd[2],int senderFd[2]);
    bool putValue(const std::string& targetKey, const std::string& targetValue);
    std::string getValue(const std::string& targetKey);
    bool deleteValue(const std::string& t_key);
    bool saveToFile();
    bool readFromFile();
    [[noreturn]] static void worker(database *_this, int readerFd[2], int senderFd[2]);
    [[noreturn]] static void manager(database *_this);
    ~database(){
        log(warning,"Force exit!\nSaving datas now....");
        saveToFile();
    }
    static void sigHelder(int);
};
#endif //FINAL_PROJECT_DATABASE_H
