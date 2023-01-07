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
public:
    void start(int readerFd[2],int senderFd[2]);
    bool putValue(const std::string& targetKey, const std::string& targetValue);
    std::string getValue(const std::string& targetKey);
    bool deleteValue(const std::string& t_key);
    bool saveToFile();
    bool readFromFile();
    static void worker(database *_this);
    static void manager(database *_this);
    ~database(){
        log(warning,"Force exit tiggered!\nSaving datas now....");
        saveToFile();
    }
};
#endif //FINAL_PROJECT_DATABASE_H
