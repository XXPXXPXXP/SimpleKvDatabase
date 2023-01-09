//
// Created by 神奇bug在哪里 on 2022/12/24.
//

#ifndef FINAL_PROJECT_DATABASE_H
#define FINAL_PROJECT_DATABASE_H
#include <deque>
#include <map>
#include <mutex>
#include "serverLog.h"
#include "threadPool.h"
class database : public threadPool{
private:
    std::map<std::string,std::string> datas;
    std::mutex databaseLocker;
    std::mutex saveLocker;
    std::mutex pipeLocker;
    threadPool databaseThreadPool;

public:
    void start(int readerFd[2],int senderFd[2]);
    bool putValue(const std::string& targetKey, const std::string& targetValue);
    std::string getValue(const std::string& targetKey);
    bool deleteValue(const std::string& t_key);
    bool saveToFile();
    bool readFromFile();
    [[noreturn]] void taskSync(int readerFd[2],int senderFd[2]);
    void putResponse(string &targetKey, string &targetValue, int sockID, int senderFd[2]);
    void deleteResponse(string &targetKey, int sockID, int senderFd[2]);
    void getResponse(string & targetKey, int sockID, int senderFd[2]);
    ~database(){
        log(warning,"Force exit!\nSaving datas now....");
        saveToFile();
    }
    static void sigHelder(int);
};
#endif //FINAL_PROJECT_DATABASE_H
