//
// Created by 徐鑫平 on 2022/12/23.
//

#ifndef FINAL_PROJECT_SERVERLOG_H
#define FINAL_PROJECT_SERVERLOG_H
#include <iostream>
enum log_level {info,warning,error};
void log(enum log_level level,const std::string& context);
void log(enum log_level level,const std::string& context,int);
void logh(enum log_level level);
#endif //FINAL_PROJECT_SERVERLOG_H
