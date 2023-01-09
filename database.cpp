#pragma clang diagnostic push
#pragma ide diagnostic ignored "modernize-avoid-bind"
#pragma ide diagnostic ignored "performance-inefficient-string-concatenation"
//
// Created by 神奇bug在哪里 on 2022/12/24.
//
#include "database.h"
#include <fstream>
#include <csignal>
#include <unistd.h>
#include <functional>

database *globalSignalPointer;

void database::start(int readerFd[2], int senderFd[2]) {
    close(readerFd[1]);//reader仅读取
    close(senderFd[0]);//sender仅写入
    if (!readFromFile()) {
        log(warning, "数据文件读取失败！");
    }
    signal(SIGTERM, sigHelder);
    log(info, "database:start success！");
    putValue("ServerVersion","5.0");
    taskSync(readerFd, senderFd);//主线程负责从管道读取数据并且
    exit(-1);
}

bool database::putValue(const std::string &targetKey, const std::string &targetValue) noexcept(false) {
    try {
        std::string value = datas.at(targetKey); //判断要写入的数据是否已经存在
        if (value == targetValue) {
            log(warning, "待写入的数据: " + targetValue + " 已经存在!");
        } else {
            log(warning, "原数据: " + value + " 将会替换成: " + targetValue);
            databaseLocker.lock();
            datas.at(targetKey) = targetValue;
            databaseLocker.unlock();
        }
    } catch (std::exception &e) {//若不存在则会出现out_of_range错误并被捕获，然后再向容器中写入数据
        databaseLocker.lock();
        datas.emplace(targetKey, targetValue);
        databaseLocker.unlock();
    }
    return true;
}

std::string database::getValue(const std::string &targetKey)
/*
 * description: 用于获取Key对应的Value
 * return: 出错时会返回空的std::string，成功时会返回Value
 */
{
    std::string result;
    try {
        databaseLocker.lock();
        result = datas.at(targetKey);
        databaseLocker.unlock();
        log(info, "database:数据库中找到已经存在的数据:value=" + result);
    } catch (std::exception &e) {
        return result;
    }
    return result;
}

bool database::readFromFile() {
    std::ifstream file;
    file.open("datas.dat", std::ios_base::in | std::ios_base::binary);
    if (!file.is_open())
        return false;
    while (true) {
        uint32_t size;
        std::string value, key;
        file.read(reinterpret_cast<char *>(&size), 4);
        key.resize(size);
        if (size <= 0)
            break;
        else {
            file.read(const_cast<char *>(key.data()), size);
        }

        file.read(reinterpret_cast<char *>(&size), 4);
        if (size <= 0) {
            log(error, "文件不完整！");
            datas.emplace(key, "null");
            return false;
        } else {
            value.resize(size);
            file.read(const_cast<char *>(value.data()), size);
        }
        datas.emplace(key, value);
        log(info, "已从文件中读取: key=" + key + "value=" + value);
        key.clear();
        value.clear();
    }
    file.close();
    return true;
}

bool database::deleteValue(const std::string &t_key) {
    try {
        databaseLocker.lock();
        datas.erase(t_key);
        databaseLocker.unlock();
    } catch (std::exception &e) {
        log(warning, "待删除的数据不存在！");
        return false;
    }
    return true;
}

bool database::saveToFile() {
    //这个函数理论上只会在退出的时候被调用
    log(info, "database:开始保存数据...");
    uint32_t size;
    std::ofstream file;
    file.open("datas.dat", std::ios_base::out | std::ios_base::binary);
    if (!file.is_open()) {
        log(error, "文件保存失败！");
        return false;
    }
    log(info,"文件已打开！");
    //log(info, "待保存的数据大小:", (int) datas.size());
    for (auto &data: datas) {
        std::string targetKey = data.first, targetValue = data.second;
        size = targetKey.size();
        file.write(reinterpret_cast<char *>(&size), 4);
        file.write(targetKey.c_str(), size);
        size = targetValue.size();
        file.write(reinterpret_cast<char *>(&size), 4);
        file.write(targetValue.c_str(), size);
        log(info, "已写入到文件,key=" + targetKey + "value=" + targetValue);
    }
    size = 0;
    file.write(reinterpret_cast<char *>(&size), 4);
    file.close();
    log(info,"数据保存完成!");
    return true;
}

void database::sigHelder(int signum) {
    log(warning, "数据进程收到信号！", signum);
    globalSignalPointer->databaseThreadPool.stop();
    globalSignalPointer->saveToFile();
    exit(signum);
}

void database::taskSync(int *readerFd, int *senderFd) {
    uint32_t type = 0;
    int sockID = -1;
    while (true) {
        if (read(readerFd[0], &type, sizeof(int)) != sizeof(int)) {
            log(warning, "database:从管道读取数据异常！");
            continue;
        }
        switch (type) {
            case 0://put
            {
                bool readResult = true;
                uint32_t keySize, valueSize = 0;
                std::string targetKey, targetValue;
                readResult = read(readerFd[0], &keySize, 4) == 4;
                targetKey.resize(keySize);
                readResult = readResult && read(readerFd[0], const_cast<char *>(targetKey.data()), keySize) == keySize;
                readResult = readResult && read(readerFd[0], &valueSize, 4) == 4;
                targetValue.resize(valueSize);
                readResult = readResult && read(readerFd[0], const_cast<char *>(targetValue.data()), valueSize) == valueSize;
                readResult = readResult && read(readerFd[0], &sockID, sizeof(int)) == sizeof(int);
                if (!readResult) {
                    log(error, "database：管道读取异常！");
                }
                log(info, "database:收到put请求！");
                databaseThreadPool.addTasks(
                        std::bind(&database::putResponse, this, targetKey, targetValue, sockID, senderFd));
                break;
            }
            case 1://delete
            {
                uint32_t keySize;
                std::string targetKey;
                bool pipeResult = read(readerFd[0], &keySize, 4) == 4;
                targetKey.resize(keySize);
                pipeResult = pipeResult && read(readerFd[0], const_cast<char *>(targetKey.data()), keySize) == keySize;
                pipeResult = pipeResult && read(readerFd[0], &sockID, sizeof(int)) == sizeof(int);
                if (!pipeResult) {
                    log(error, "database: 管道read出现错误！");
                }
                log(info, "database:收到delete请求!key=" + targetKey);
                databaseThreadPool.addTasks(std::bind(&database::deleteResponse, this, targetKey, sockID, senderFd));
                break;
            }
            case 2: {
                uint32_t keySize;
                std::string targetKey;
                bool pipeResult = read(readerFd[0], &keySize, 4) == 4;
                targetKey.resize(keySize);
                pipeResult = pipeResult && read(readerFd[0], const_cast<char *>(targetKey.data()), keySize) == keySize;
                pipeResult = pipeResult && read(readerFd[0], &sockID, sizeof(int)) == sizeof(int);
                if (!pipeResult) {
                    log(error, "database: reader管道出现异常！");
                }
                log(info, "database:处理Get请求:key=" + targetKey);
                databaseThreadPool.addTasks(std::bind(&database::getResponse, this, targetKey, sockID, senderFd));
                break;
            }
            default: {
                log(error, "database:错误的type！");
                continue;
            }
        }//从管道中解析任务并且添加对应的任务到线程池
    }
}

void database::putResponse(std::string targetKey, std::string targetValue, int sockID, int senderFd[2]) {
    log(info, "database:子线程开始处理");
    uint32_t type = 3;
    bool result = putValue(targetKey, targetValue);
    pipeLocker.lock();
    bool pipeWrite = write(senderFd[1], &type, 4);
    pipeWrite = pipeWrite && write(senderFd[1], &result, sizeof(bool)) == sizeof(bool);
    pipeWrite = pipeWrite && write(senderFd[1], &sockID, sizeof(int)) == sizeof(int);
    if (!pipeWrite) {
        log(error, "管道发送错误！");
    }
    pipeLocker.unlock();
}

void database::deleteResponse(std::string &targetKey, int sockID, int senderFd[2]) {
    uint32_t type = 4;
    bool result = deleteValue(targetKey);
    pipeLocker.lock();
    bool pipeResult = write(senderFd[1], &type, 4) == 4;
    pipeResult = pipeResult && write(senderFd[1], &result, sizeof(bool)) == sizeof(bool);
    pipeResult = pipeResult && write(senderFd[1], &sockID, sizeof(int)) == sizeof(int);
    pipeLocker.unlock();
    if (!pipeResult) {
        log(error, "database: 管道read出现错误！");
    }
}

void database::getResponse(std::string &targetKey, int sockID, int senderFd[2]) {
    uint32_t type = 5;
    std::string value = getValue(targetKey);
    bool pipeResult;
    if (value.empty()) {
        log(error, "查找键值对失败");
        uint32_t valueSize = 4;
        value = "null";
        pipeLocker.lock();
        pipeResult = write(senderFd[1], &type, 4) == 4;
        pipeResult = pipeResult && write(senderFd[1], &valueSize, 4) == 4;
        pipeResult =
                pipeResult && write(senderFd[1], const_cast<char *>(value.data()), valueSize) == valueSize;
        pipeResult = pipeResult && write(senderFd[1], &sockID, sizeof(int)) == sizeof(int);
        pipeLocker.unlock();
        if (!pipeResult) {
            log(error, "database: sender管道出现异常！");
        }
        log(info, "database:Get请求完成处理，已经放回管道");
    } else {
        uint32_t valueSize = value.size();
        log(info, "database:查找键值对成功！value=" + value);
        pipeResult = write(senderFd[1], &type, 4) == 4;
        pipeResult = pipeResult && write(senderFd[1], &valueSize, 4) == 4;
        pipeResult =
                pipeResult && write(senderFd[1], const_cast<char *>(value.data()), valueSize) == valueSize;
        pipeResult = pipeResult && write(senderFd[1], &sockID, sizeof(int)) == sizeof(int);
        log(info, "database:数据写入管道成功");
        if (!pipeResult) {
            log(error, "database: sender管道出现异常！");
        }
    }
}


#pragma clang diagnostic pop