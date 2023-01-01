//
// Created by 徐鑫平 on 2022/12/24.
//

#ifndef FINAL_PROJECT_DATABASE_H
#define FINAL_PROJECT_DATABASE_H
#include <iostream>
#include <deque>
#include "serverLog.h"

class database{
private:
    size_t obj_count=0;
    std::deque<std::string> keys;
    std::deque<std::string> values;
public:
    bool init();
    bool addValue(std::string t_key, std::string t_value);
    std::string getValue(std::string t_key);
    bool deleteValue(std::string t_key);
    bool saveToFile();
    bool readFromFile();
    int search(std::string target);
    ~database(){
        log(warning,"Force exit tiggered!\nSaving datas now....");
        saveToFile();
    }

};

#endif //FINAL_PROJECT_DATABASE_H
