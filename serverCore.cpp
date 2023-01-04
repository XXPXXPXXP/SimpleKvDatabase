//
// Created by 神奇bug在哪里 on 2022/12/16.
//


#include "serverCore.h"
#include "workThread.h"
#include "settings.h"

extern std::vector<pid_t> runningProcess;
extern serverSocket *p_target_server;

void pipeHandle(int) {
    log(error, "socket发生了异常关闭！");
}
/* macOS/Linux 在初始化的时候比Windows少调用两个函数，不得不说，为什么这俩玩意总是在奇怪的地方搞差异化啊！！！ */
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
    /* 创建地址结构体 */
    if (bind(listenSockId, (struct sockaddr *) &ipConfig, sizeof(ipConfig)) < 0) {
        log(error, "bind error!");
        return false;
    }
    return true;
}
/* 本来想使用epoll进行阻塞处理的，可是macOS内核不支持epoll */

/* 所以采用kqueue进行阻塞 */
[[noreturn]] void serverSocket::startServer() {
    log(info, "Trying startup listening....");
    if (listen(listenSockId, MAX_SOCKET) < 0) {
        log(error, "listen startup error!");
        exit(errno);
    }
    /* 启动线程池 */
    threadsPool pool{};
    /* 准备创建epoll队列 */
    int listeningEpoll = epoll_create(1);
    if (listeningEpoll == -1) {
        log(error, "Listen: epoll队列无法创建！");
        exit(errno);
    }
    /* 注册信号处理函数SIGPIPE(用于解决一些socket意外断开的问题) */
    signal(SIGPIPE, pipeHandle);
    struct epoll_event listenEV{};
    listenEV.data.fd = listenSockId;
    listenEV.events = EPOLLIN;  // 类似于 kqueue的 EV_READ
    int ret = epoll_ctl(listeningEpoll, EPOLL_CTL_ADD, listenSockId, &listenEV);
    if (ret == -1) {
        log(error, "epoll_ctl error");
        exit(errno);
    }
    struct epoll_event listenEpollEvent[128];
    int size = sizeof(listenEpollEvent) / sizeof(struct epoll_event);
    /* 创建epoll结构体并注册事件，准备用来阻塞 */
    runningProcess.resize(PROCESS_SIZE);
    for (int i = 0; i < PROCESS_SIZE; ++i) {
        pid_t pid = vfork();
        if (pid != 0) {
            runningProcess.at(i) = pid;
        } else {
            child = true;
            break;
        }
    }
    /* 准备创建进程 */
    while (true) {
        log(info, "开始阻塞");
        int num = epoll_wait(listeningEpoll, listenEpollEvent, size, -1);
        if (num < 1) {
            log(error, "epoll队列异常！");
            continue;
        }
        int targetSockId = accept(listenSockId, nullptr, nullptr);    // 传入的socket
        if (targetSockId == -1) {
            log(error, "accept error!");
            continue;
        }
        log(info, "accept成功！", targetSockId);
        struct timeval timeOut{};
        timeOut.tv_sec = 3;
        timeOut.tv_usec = 0;
        /* 设置连接超时,防止连接卡服 */
        setsockopt(targetSockId, SOL_SOCKET, SO_RCVTIMEO, &timeOut, sizeof(timeOut));
        pool.start(targetSockId, this);
        /* 采用多进程来进行accept,线程进行处理  */
    }
}


bool serverSocket::process(int target_sock_id, uint32_t type) {
    if (type == 0) {
        log(info, "收到put请求!", target_sock_id);
        if (!add(target_sock_id)) {
            log(error, "添加键值对失败！");
            return false;
        }
    } else if (type == 1) {
        log(info, "收到delete请求!", target_sock_id);
        if (!deleteData(target_sock_id)) {
            log(error, "删除键值对失败！");
            return false;
        }
    } else if (type == 2) {
        log(info, "收到get请求！", target_sock_id);
        if (!get(target_sock_id)) {
            log(error, "查找键值对失败！");
            return false;
        }
    }
    return true;
}

void serverSocket::stop() const {
    close(listenSockId);
    if (!child) {
        data.saveToFile();
    }
}

bool serverSocket::get(int targetSockId) {
    uint32_t size;
    std::string key;
    bool sendResult;

    if (read(targetSockId, &size, 4) < 0) {
        log(error, "读取boay: size失败！", targetSockId);
        return false;
    }
    key.resize(size);
    long real_size = read(targetSockId, const_cast<char *>(key.data()), size);
    /* 这里不知道为什么要用常转换 */
    if (real_size < 0) {
        log(error, "读取Key出现错误!", targetSockId);
        key.clear();
        return false;
    } else if (real_size != size) {
        log(error, "读取到的Key大小异常!", targetSockId);
        key.clear();
        return false;
    }
    std::string value = data.getValue(key);
    if (value.empty()) {
        sendField(targetSockId, const_cast<char *>("null"), 5, MSG_NOSIGNAL);
        return false;
    }
    if (!sendHeader(targetSockId, sizeof(uint32_t) + value.size(), 5)) {
        log(error, "head数据发送失败！", targetSockId);
        return false;
    }
    log(info, "head数据发送成功！", targetSockId);
    uint32_t valueSize = value.size();
    sendResult = sendField(targetSockId, &valueSize, 4, MSG_NOSIGNAL);
    sendResult = sendResult && sendField(targetSockId, const_cast<char *>(value.data()), valueSize, MSG_NOSIGNAL);
    log(info, "返回的数据: value = " + value);
    return sendResult;
}

bool serverSocket::deleteData(int targetSockId) {
    uint32_t size;
    if (read(targetSockId, &size, 4) < 0) {
        log(error, "读取数据大小失败!");
        return false;
    }
    std::string targetKey;
    targetKey.resize(size);
    if (read(targetSockId, const_cast<char *>(targetKey.data()), size) != size) {
        log(error, "读取key内容失败!");
        return false;
    }
    /* 数据读取部分结束 */
    bool result;
    result = data.deleteValue(targetKey);
    if (!sendHeader(targetSockId, 1, 4)) {
        log(error, "发送head失败!");
        return false;
    }

    return sendField(targetSockId, &result, 1, MSG_NOSIGNAL);
}

bool serverSocket::sendField(int target_sock_id, void *data_to_send, uint32_t size, int extra)
/*
 * description: 用来发送每一个Field的数据,会检查read函数的返回值，这样做是为了减少重复的代码数量
 * return: 是否成功
 */
{
    if (send(target_sock_id, data_to_send, size, extra) != size) {
        log(error, "发送body失败！");
        return false;
    } else
        return true;
}

bool serverSocket::add(int target_sock_id) {
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
        sendHeader(target_sock_id, 1, 3);
        bool status = true;
        //usleep(300000);
        sendField(target_sock_id, &status, 1, MSG_NOSIGNAL);
        log(info, "Key: " + target_key + "Value:" + target_value);
        return true;
    } else {
        log(error, "键值对保存失败!", target_sock_id);
        sendHeader(target_sock_id, 1, 3);
        bool status = false;
        sendField(target_sock_id, &status, 1, MSG_NOSIGNAL);
        return false;
    }
}

bool serverSocket::sendHeader(int target_sock_id, uint32_t full_size, uint32_t type) {
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
    result = sendField(target_sock_id, &magic_number, 4, MSG_NOSIGNAL);
    /*
     * field: Size(head)
     * size: 4 bytes
     * type: uint32_t
     * description: body的大小
     */
    result = result && sendField(target_sock_id, &full_size, 4, MSG_NOSIGNAL);
    result = result && sendField(target_sock_id, &type, 4, MSG_NOSIGNAL);
    result = result && sendField(target_sock_id, &padding, 4, MSG_NOSIGNAL);
    return result;
}

