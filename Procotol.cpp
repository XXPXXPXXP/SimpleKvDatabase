//
// Created by 神奇bug在哪里 on 2023/1/5.
//
#include <iostream>
#include "serverLog.h"
#include <sys/socket.h>
#include <csignal>
#include "Procotol.h"

bool get(int targetSockId, database &datas) {
    uint32_t size;
    std::string key;
    if (read(targetSockId, &size, 4) < 0) {
        log(error, "读取body: size失败！", targetSockId);
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
    std::string value = datas.getValue(key);
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
    bool sendResult;
    sendResult = sendField(targetSockId, &valueSize, 4, MSG_NOSIGNAL);
    sendResult = sendResult && sendField(targetSockId, const_cast<char *>(value.data()), valueSize, MSG_NOSIGNAL);
    log(info, "返回的数据: value = " + value);
    return sendResult;
}
bool process(int target_sock_id, uint32_t type, database *database) {
    if (type == 0) {
        log(info, "收到put请求!", target_sock_id);
        if (!add(target_sock_id, *database)) {
            log(error, "添加键值对失败！");
            return false;
        }
    } else if (type == 1) {
        log(info, "收到delete请求!", target_sock_id);
        if (!deleteData(target_sock_id, *database)) {
            log(error, "删除键值对失败！");
            return false;
        }
    } else if (type == 2) {
        log(info, "收到get请求！", target_sock_id);
        if (!get(target_sock_id, *database)) {
            log(error, "查找键值对失败！");
            return false;
        }
    }
    return true;
}
bool deleteData(int targetSockId, database &datas) {
    /* 数据读取部分结束 */
    bool result;
    result = datas.deleteValue(targetKey);
    if (!sendHeader(targetSockId, 1, 4)) {
        log(error, "发送head失败!");
        return false;
    }

    return sendField(targetSockId, &result, 1, MSG_NOSIGNAL);
}

bool sendField(int target_sock_id, void *data_to_send, uint32_t size, int extra)
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

bool add(int target_sock_id, database &datas) {
    /*
     * description: 用于添加键值对数据
     */

    if (datas.putValue(target_key, target_value)) {
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

bool sendHeader(int target_sock_id, uint32_t full_size, uint32_t type) {
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
