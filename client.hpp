//
// Created by 神奇bug在哪里 on 2022/12/26.
//

#ifndef FINAL_PROJECT_CLIENT_HPP
#define FINAL_PROJECT_CLIENT_HPP
#include <iostream>
#include <ctime>
#include <sys/socket.h>
#include <arpa/inet.h>
/* inet.h 使用的是MacOSX13.0.sdk内置的版本 */
#include <netinet/in.h>
//#include <sys/event.h>
#include <sys/types.h>
#include <unistd.h>
/* macOS搞了另一套东西来实现epoll */
#include <thread>
enum log_level {info,warning,error};
void log(enum log_level level,const std::string& context);
void log(enum log_level level,const std::string& context,int);
void logf(enum log_level level,const char * format,const std::string& context);

bool send_header(int,uint32_t,uint32_t);
bool send_body_put(int target_sock_id,std::string & key,std::string & value)
{
    uint32_t key_size = key.size(),value_size = value.size();
    send(target_sock_id,&key_size,4,0);
    send(target_sock_id,&key,key_size,0);
    send(target_sock_id,&value_size,4,0);
    send(target_sock_id,&value,value_size,0);
    return true;
}
void send_body_get(int target_sock_id,std::string & key)
{
    uint32_t key_size = key.size();
    send(target_sock_id,&key_size,4,0);
    send(target_sock_id,&key,key_size,0);
    uint32_t return_value_size;
    std::string header,return_data;
    header.resize(16);
    read(target_sock_id,&header,16);
    read(target_sock_id,&return_value_size,4);
    return_data.resize(return_value_size);
    read(target_sock_id,&return_data,return_value_size);
    std::cout<<return_data<<std::endl;
}
log_level global_log_level = info;
#endif //FINAL_PROJECT_CLIENT_HPP
