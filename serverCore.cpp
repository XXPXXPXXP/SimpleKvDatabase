//
// Created by 徐鑫平 on 2022/12/16.
//


#include "serverCore.h"
#include "workThread.h"
#include "settings.h"



/* macOS/Linux 在初始化的时候比Windows少调用两个函数，不得不说，为什么这俩玩意总是在奇怪的地方搞差异化啊！！！ */
void pipeHandle(int) {
    log(error, "socket发生了异常关闭！");
}

bool serverSocket::init(short port, database &datas) {
    data = datas;
    //delete &temp;  //释放掉为了规避c++的限制而使用的临时对象
    listenSockId = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSockId == -1) {
        log(error, "socket init error!");
        return false;
    }
    /* 创建基本的socket */
    int opt_code = 1;
    setsockopt(listenSockId, SOL_SOCKET, SO_REUSEADDR, (const void *) &opt_code, sizeof(opt_code));
    /* 设置端口复用 */
    memset(&ipConfig, 0, sizeof(ipConfig)); //对地址结构体填充0,optional on macOS
    ipConfig.sin_family = AF_INET;//IPv4
    ipConfig.sin_port = htons(port);
    ipConfig.sin_addr.s_addr = inet_addr("0.0.0.0");//绑定IP为0.0.0.0，接受来自任何IP的访问
    if (bind(listenSockId, (struct sockaddr *) &ipConfig, sizeof(ipConfig)) < 0) {
        log(error, "bind error!");
        return false;
    }
    return true;
}
/* 本来想使用epoll进行阻塞处理的，可是macOS内核不支持epoll */

/* 所以采用kqueue进行阻塞 */
bool serverSocket::startListen() {
    log(info, "Trying startup listening....");
    if (listen(listenSockId, MAX_SOCKET) < 0) {
        log(error, "listen startup error!");
        return false;
    }
    /* 启动线程池 */
    threadsPool pool;
    //pool.createThreadPool(8, this);
    /* 准备创建epoll队列 */
    int listeningEpoll = epoll_create(MAX_EPOLL);
    if (listeningEpoll == -1) {
        log(error, "Listen: epoll队列无法创建！");
        return false;
    }
    /* 注册信号处理函数SIGPIPE(用于解决一些奇怪的socket意外断开的问题) */
    signal(SIGPIPE, pipeHandle);
    while (true) {
        log(info, "开始阻塞");
        int targetSockId = accept(listenSockId, nullptr, nullptr);    // 传入的socket,阻塞模式
        if (targetSockId == -1) {
            log(error, "accept error!");
            return false;
        }
        log(info, "accept成功！", targetSockId);
        struct timeval timeOut{};
        timeOut.tv_sec = 3;
        timeOut.tv_usec = 0;
        /* 设置连接超时,防止连接卡服 */
        setsockopt(targetSockId, SOL_SOCKET, SO_RCVTIMEO, &timeOut, sizeof(timeOut));
        pool.producerConsumerMode(targetSockId, this);
        /* 采用单线程来进行accept  */

    }
}


bool serverSocket::process(int target_sock_id, uint32_t type) {
    if (type == 0) {
        log(info, "收到put请求!", target_sock_id);
        if (!process_add(target_sock_id)) {
            log(error, "添加键值对失败！");
            return false;
        }
    } else if (type == 1) {
        log(info, "收到delete请求!", target_sock_id);
        if (!process_delete(target_sock_id)) {
            log(error, "删除键值对失败！");
            return false;
        }
    } else if (type == 2) {
        log(info, "收到get请求！", target_sock_id);
        if (!process_get(target_sock_id)) {
            log(error, "查找键值对失败！");
            return false;
        }
    }
    return true;
}

void serverSocket::stop() const {
    close(listenSockId);
    data.saveToFile();
}

bool serverSocket::process_get(int target_sock_id) {
    uint32_t size;
    read(target_sock_id, &size, 4);
    std::string key;
    key.resize(size);
    long real_size = read(target_sock_id, const_cast<char *>(key.data()), size);
    /* 这里不知道为什么要用常转换 */
    if (real_size < 0) {
        log(error, "读取Key出现错误!", target_sock_id);
        key.clear();
        return false;
    } else if (real_size != size) {
        log(error, "读取到的Key大小异常!", target_sock_id);
        key.clear();
        return false;
    }
    std::string value = data.getValue(key);
    if (value.empty()) {
        send(target_sock_id, "null", 5, MSG_NOSIGNAL);
        return false;
    }
    if (!send_header(target_sock_id, sizeof(uint32_t) + value.size(), 5)) {
        log(error, "head数据发送失败！", target_sock_id);
        return false;
    }
    log(info, "head数据发送成功！", target_sock_id);
    uint32_t value_size = value.size();
    //usleep(300000);
    if (send(target_sock_id, &value_size, 4, MSG_NOSIGNAL) != 4) {
        log(error, "body:size数据发送失败!");
        return false;
    }
    if (send(target_sock_id, const_cast<char *>(value.data()), value_size, MSG_NOSIGNAL) != value_size) {
        log(error, "body:value数据发送失败!");
        return false;
    }
    log(info, "返回的数据: value = " + value);
    return true;
}

bool serverSocket::process_delete(int target_sock_id) {
    uint32_t size;
    if (read(target_sock_id, &size, 4) < 0) {
        log(error, "读取数据大小失败!");
        return false;
    }
    std::string target_key;
    target_key.resize(size);
    if (read(target_sock_id, const_cast<char *>(target_key.data()), size) != size) {
        log(error, "读取key内容失败!");
        return false;
    }
    /*
     * 完成数据读取
     */
    bool result;
    result = data.deleteValue(target_key);
    if (!send_header(target_sock_id, 1, 4)) {
        log(error, "发送head失败!");
        return false;
    }
    //usleep(300000);
    send_safe(target_sock_id, &result, 1, MSG_NOSIGNAL);
    return true;
}

bool serverSocket::send_safe(int target_sock_id, void *data_to_send, uint32_t size, int extra) {
    if (send(target_sock_id, data_to_send, size, extra) != size) {
        log(error, "发送body失败！");
        return false;
    } else {
        return true;
    }

}

bool serverSocket::process_add(int target_sock_id) {
    /*
     * description: 用于添加键值对数据
     */
    uint32_t key_size, value_size;
    std::string target_key, target_value;
    if (read(target_sock_id, &key_size, 4) < 0) {
        log(error, "body:key_size读取失败！", target_sock_id);
        return false;
    }
    target_key.resize(key_size);
    if (read(target_sock_id, const_cast<char *>(target_key.data()), key_size) < 0) {
        log(error, "body:key读取失败！", target_sock_id);
        //这里函数会退出，所以这里资源会被自动回收。固不再手动回收资源
        return false;
    }
    if (read(target_sock_id, &value_size, 4) < 0) {
        log(error, "body:value_size读取失败!", target_sock_id);
        return false;
    }
    target_value.resize(value_size);
    if (read(target_sock_id, const_cast<char *>(target_value.data()), value_size) < 0) {
        log(error, "body:value读取失败!", target_sock_id);
        return false;
    }

    if (data.addValue(target_key, target_value)) {
        log(info, "键值对成功保存!", target_sock_id);
        send_header(target_sock_id, 1, 3);
        bool status = true;
        //usleep(300000);
        send_safe(target_sock_id, &status, 1, MSG_NOSIGNAL);
        log(info, "Key: " + target_key + "Value:" + target_value);
        return true;
    } else {
        log(error, "键值对保存失败!", target_sock_id);
        send_header(target_sock_id, 1, 3);
        bool status = false;
        send_safe(target_sock_id, &status, 1, MSG_NOSIGNAL);
        return false;
    }
}

bool serverSocket::send_header(int target_sock_id, uint32_t full_size, uint32_t type) {
    /*
     * 描述：用于发送response的头部数据
     * 返回值: 是否发送成功
     * 参数列表:
     *  target_sock_id: 发送的目标sock_id
     *  full_size: 需要发送的body数据的总大小
     *  type: 该数据包的类型
     */
    uint32_t magic_number = 1234, padding = {0};
    /*
     * field: magic_number
     * size: 4 bytes
     * type: uint32_t
     * description: 固定为1234
     */
    bool result;
    result = send_safe(target_sock_id, &magic_number, 4, MSG_NOSIGNAL);
    /*
     * field: Size(head)
     * size: 4 bytes
     * type: uint32_t
     * description: body的大小
     */
    result = result && send_safe(target_sock_id, &full_size, 4, MSG_NOSIGNAL);
    result = result && send_safe(target_sock_id, &type, 4, MSG_NOSIGNAL);
    result = result && send_safe(target_sock_id, &padding, 4, MSG_NOSIGNAL);
    return result;
}


