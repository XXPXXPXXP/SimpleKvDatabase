//
// Created by 神奇bug在哪里 on 2022/12/16.
//

#ifndef FINAL_PROJECT_SERVER_CORE_H
#define FINAL_PROJECT_SERVER_CORE_H
#include <ctime>
#include <sys/socket.h>
#include <arpa/inet.h>
/* inet.h 使用的是MacOSX13.0.sdk内置的版本 */
#include <netinet/in.h>
#include <kqueue/sys/event.h>
#include <sys/types.h>
#include <unistd.h>
#include <string>
#include <cstring>
//#include <MacTypes.h>
#include <deque>
/* macOS搞了另一套东西来实现epoll */
#include <thread>
#include <signal.h>
enum log_level {info,warning,error};
void log(enum log_level level,const std::string& context);
void log(enum log_level level, const std::string &context,int id);
void logh(enum log_level level);
class database {
private:
    size_t obj_count = 0;
    std::deque<std::string> keys;
    std::deque<std::string> values;
public:
    bool init();

    bool add_value(std::string t_key, std::string t_value);

    std::string get_value(std::string t_key);

    bool delete_value(std::string t_key);

    bool save_to_file();

    bool read_from_file();

    ~database() {
        log(warning, "Force exit tiggered!\nSaving datas now....");
        save_to_file();
    }
};
class server_socket{
private:
    int sock_id = -1;
    bool isInit = false;
    struct sockaddr_in socket_ip_config;
    database temp;
    database & data = temp;
public:
    bool init(short port, database &datas);
    /* 默认端口采用1433，也就是SQL的默认端口 */
    bool start_listen();
    bool process(int target_sock_id, uint32_t type);
    bool process_get(int target_sock_id);
    bool process_delete(int target_sock_id);
    bool process_add(int target_sock_id);
    static bool send_header(int target_sock_id, uint32_t full_size, uint32_t type) ;
    ~server_socket()
    {
        log(warning,"server触发了回收!");
        stop();
    }

    void stop() const;

    static bool send_safe(int target_sock_id, void *data_to_send, uint32_t size, int extra);
};
void cleanup( int signum);
#endif //FINAL_PROJECT_SERVER_CORE_H
