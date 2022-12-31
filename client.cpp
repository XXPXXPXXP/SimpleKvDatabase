//
// Created by 徐鑫平 on 2022/12/26.
//
#include "client.hpp"
int main(){
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in client_addr{};
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(1434);
    client_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    int i = connect(sock,(sockaddr *)&client_addr,sizeof(client_addr));
    std::cout << "connection:"<<i<<std::endl;
    std::string key = "test",value="test_1";
    send_header(sock,key.size()+value.size()+8,MSG_DONTWAIT);
    send_body_put(sock,key,value);
    close(sock);
    sock = socket(AF_INET, SOCK_STREAM, 0);
    i = connect(sock,(sockaddr *)&client_addr,sizeof(client_addr));
    std::cout << "connection:"<<i<<std::endl;
    key="fuck",value="true";
    send_header(sock,key.size()+value.size()+8,MSG_DONTWAIT);
    send_body_put(sock,key,value);
    close(sock);
    sock = socket(AF_INET, SOCK_STREAM, 0);
    i = connect(sock,(sockaddr *)&client_addr,sizeof(client_addr));
    std::cout << "connection:"<<i<<std::endl;
    key = "test";
    send_header(sock,key.size()+4,2);
    send_body_get(sock,key);
    std::cout<<"end"<<std::endl;
}
bool send_header(int target_sock_id, uint32_t full_size, uint32_t type) {
    /*
     * 描述：用于发送response的头部数据
     * 返回值: 是否发送成功
     * 参数列表:
     *  target_sock_id: 发送的目标sock_id
     *  full_size: 需要发送的body数据的总大小
     *  type: 该数据包的类型
     */
    uint32_t magic_number = 1234,padding = {0};
    /*
     * field: magic_number
     * size: 4 bytes
     * type: uint32_t
     * description: 固定为1234
     */
    if(send(target_sock_id,&magic_number,4,0)<0)
    {
        log(error,"magic_number无法发送",target_sock_id);
        return false;
    }
    /*
     * field: Size(head)
     * size: 4 bytes
     * type: uint32_t
     * description: body的大小
     */
    send(target_sock_id,&full_size,4,0);
    send(target_sock_id,&type,4,0);
    send(target_sock_id,&padding,4,0);
    return true;
}
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