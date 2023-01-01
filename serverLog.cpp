//
// Created by 徐鑫平 on 2022/12/23.
//
#include "serverLog.h"
enum log_level global_log_level;

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

void log(enum log_level level, int i, const std::string &context, int id) {
    if (level >= global_log_level) {
        std::cout << (level == info ? "[I] " : (level == warning ? "[W] " : "[E] "));
        std::cout << "[线程" << i << "]";
        std::cout << context << "\t[sockID]: " << id << std::endl;
    }
}

