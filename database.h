//
// Created by 徐鑫平 on 2022/12/24.
//

#ifndef FINAL_PROJECT_DATABASE_H
#define FINAL_PROJECT_DATABASE_H
#include <iostream>
#include <cctype>
#include <cstdlib>
#include <deque>
#include <fstream>
enum log_level {info,warning,error};
void log(enum log_level level,const std::string& context);
void logh(enum log_level);
class database{
private:
    size_t obj_count=0;
    std::deque<std::string> keys;
    std::deque<std::string> values;
public:
    bool init();
    bool add_value(std::string t_key,std::string t_value);
    std::string get_value(std::string t_key);
    bool delete_value(std::string t_key);
    bool save_to_file();
    bool read_from_file();
    int search(std::string target);
    bool isHave(std::string target);
    ~database(){
        log(warning,"Force exit tiggered!\nSaving datas now....");
        save_to_file();
    }

};

#endif //FINAL_PROJECT_DATABASE_H
