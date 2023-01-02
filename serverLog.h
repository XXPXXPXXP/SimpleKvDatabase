//
// Created by 徐鑫平 on 2022/12/23.
//

#ifndef FINAL_PROJECT_SERVERLOG_H
#define FINAL_PROJECT_SERVERLOG_H
#define COLOR_END "\033[m"
#include <iostream>
enum log_level {info,warning,error};
void log(enum log_level level,const std::string& context);
void log(enum log_level level,const std::string& context,int);
void log(enum log_level level,int i,const std::string & context,int id);

#endif //FINAL_PROJECT_SERVERLOG_H
