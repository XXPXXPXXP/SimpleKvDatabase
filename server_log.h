//
// Created by 徐鑫平 on 2022/12/23.
//

#ifndef FINAL_PROJECT_SERVER_LOG_H
#define FINAL_PROJECT_SERVER_LOG_H
#include <iostream>
enum log_level {info,warning,error};
enum log_level global_log_level;
void log(enum log_level level,const std::string& context);
void logf(enum log_level level,const char * format,const std::string& context);
void logh(enum log_level level);
#endif //FINAL_PROJECT_SERVER_LOG_H
