//
// Created by 徐鑫平 on 2022/12/23.
//
#include "server_log.h"



void log(enum log_level level, const std::string &context) {
    if (level >= global_log_level) {
        std::cout << (level == info ? "[I] " : (level == warning ? "[W] " : "[E] "));
        std::cout << context << std::endl;
    }
}
void log(enum log_level level, const std::string &context,int id) {
    if (level >= global_log_level) {
        std::cout << (level == info ? "[I] " : (level == warning ? "[W] " : "[E] "));
        std::cout << context << "\t[ID]: " << id << std::endl;
    }
}
void logh(enum log_level level)
{
    if (level >= global_log_level)
        std::cout << (level == info ? "[I] " : (level == warning ? "[W] " : "[E] "));
}

