//
// Created by 神奇bug在哪里 on 2022/12/24.
//

#ifndef FINAL_PROJECT_DATABASE_H
#define FINAL_PROJECT_DATABASE_H
#include <deque>
#include <map>
#include "serverLog.h"
class database{
private:
    std::map<std::string,std::string> datas;
    pthread_mutex_t Locker;
public:
    bool init();
    bool putValue(const std::string& targetKey, const std::string& targetValue);
    std::string getValue(const std::string& targetKey);
    bool deleteValue(const std::string& t_key);
    bool saveToFile();
    bool readFromFile();
    void dataProvider();
    ~database(){
        log(warning,"Force exit tiggered!\nSaving datas now....");
        saveToFile();
    }
};
#endif //FINAL_PROJECT_DATABASE_H
