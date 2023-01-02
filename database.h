//
// Created by 神奇bug在哪里 on 2022/12/24.
//

#ifndef FINAL_PROJECT_DATABASE_H
#define FINAL_PROJECT_DATABASE_H
#include <deque>
#include "serverLog.h"

class database{
private:
    size_t obj_count=0;
    std::deque<std::string> keys;
    std::deque<std::string> values;
    pthread_mutex_t Locker;
public:
    bool init();
    bool addValue(const std::string& targetKey, const std::string& targetValue);
    std::string getValue(const std::string& targetKey);
    bool deleteValue(const std::string& t_key);
    bool saveToFile();
    bool readFromFile();
    int search(const std::string& target);
    ~database(){
        log(warning,"Force exit tiggered!\nSaving datas now....");
        saveToFile();
    }

};

#endif //FINAL_PROJECT_DATABASE_H
