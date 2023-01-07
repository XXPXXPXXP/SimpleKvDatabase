#pragma ide diagnostic ignored "performance-inefficient-string-concatenation"
//
// Created by 神奇bug在哪里 on 2022/12/24.
//
#include "database.h"
#include <fstream>
#include <csignal>
#include <unistd.h>

database * globalSignalPointer;
void database::start(int readerFd[2], int senderFd[2]) {
    close(readerFd[1]);//reader仅读取
    close(senderFd[0]);//sender仅写入
    if (!readFromFile()) {
        log(warning, "读取数据文件失败！");
    }
    globalSignalPointer = this;
    signal(SIGTERM,sigHelder);
    for (int i = 0; i < minThread; ++i) {
        workerIDs.emplace_back(worker, this, readerFd, senderFd);//创建工作线程
    }
    log(info,"database:工作线程创建！");
    workerIDs.at(0).join();
    exit(-1);
}

bool database::putValue(const std::string &targetKey, const std::string &targetValue) noexcept(false) {
    try {
        std::string value = datas.at(targetKey);
        if (value == targetValue) {
            log(warning, "待写入的数据: " + targetValue + " 已经存在!");
        } else {
            log(warning, "原数据: " + value + " 将会替换成: " + targetValue);
            databaseLocker.lock();
            datas.at(targetKey) = targetValue;
            databaseLocker.unlock();
        }
    } catch (std::exception &e) {
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
        return false;
    }
    return true;
}

bool database::saveToFile() {
    uint32_t size;
    std::ofstream file;
    file.open("datas.dat", std::ios_base::out | std::ios_base::binary);
    if (!file.is_open()) {
        log(error, "文件保存失败！");
        return false;
    }
    for (auto &data: datas) {
        std::string targetKey = data.first, targetValue = data.second;
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
    return true;
}
[[noreturn]] void database::manager(database *_this) {
    while (true);
}
[[noreturn]] void database::worker(database *_this, int readerFd[2], int senderFd[2]) {

    while (true) {
        uint32_t type;
        _this->pipeReadLocker.lock();
        if (read(readerFd[0], &type, sizeof(int)) != sizeof(int)) {
            log(error, "database:从管道读取数据异常！");
            continue;
        }
        int sockID;
        switch (type) {
            case 0://put
            {
                bool readResult;
                uint32_t keySize, valueSize;
                std::string targetKey, targetValue;
                readResult = read(readerFd[0], &keySize, 4) == 4;
                readResult = readResult && read(readerFd[0], const_cast<char *>(targetKey.data()), keySize) == keySize;
                readResult = readResult && read(readerFd[0], &valueSize, 4) == 4;
                readResult = readResult && read(readerFd[0], const_cast<char *>(targetValue.data()), valueSize) == valueSize;
                readResult = readResult && read(readerFd[0], &sockID, sizeof(int)) == sizeof(int);
                _this->pipeReadLocker.unlock();
                type += 3;//构造发送的type值
                if (!readResult) {
                    log(error, "database：管道读取异常！");
                }
                readResult = _this->putValue(targetKey, targetValue);//这里重用了这个readResult
                _this->pipeWriteLocker.lock();
                bool pipeWrite = write(senderFd[1], &type, 4);
                pipeWrite = pipeWrite && write(senderFd[1], &readResult, sizeof(bool)) == sizeof(bool);
                pipeWrite = pipeWrite && write(senderFd[1], &sockID, sizeof(int)) == sizeof(int);
                _this->pipeLocker.unlock();
                if (!pipeWrite) {
                    log(error, "管道发送错误！");
                    continue;
                }
                break;
            }
            case 1://delete
            {
                uint32_t keySize;
                std::string targetKey;
                bool pipeResult = read(readerFd[0], &keySize, 4) == 4;
                pipeResult = pipeResult && read(readerFd[0], const_cast<char *>(targetKey.data()), keySize) == keySize;
                pipeResult = pipeResult && read(readerFd[0], &sockID, sizeof(int)) == sizeof(int);
                if (!pipeResult) {
                    log(error, "database: 管道read出现错误！");
                    continue;
                }
                _this->pipeReadLocker.unlock();
                type += 3;//构造发送的type值
                bool result = _this->deleteValue(targetKey);
                _this->pipeWriteLocker.lock();
                pipeResult = write(senderFd[1], &type, 4) == 4;
                pipeResult = pipeResult && write(senderFd[1], &result, sizeof(bool)) == sizeof(bool);
                pipeResult = pipeResult && write(senderFd[1], &sockID, sizeof(int)) == sizeof(int);
                _this->pipeWriteLocker.unlock();
                if (!pipeResult) {
                    log(error, "database: 管道read出现错误！");
                    continue;
                }
            }
            case 2: {//get
                uint32_t keySize;
                std::string targetKey;
                bool pipeResult = read(readerFd[0],&keySize,4) == 4;
                pipeResult = pipeResult && read(readerFd[0],const_cast<char *>(targetKey.data()),keySize) == keySize;
                pipeResult = pipeResult && read(readerFd[0],&sockID, sizeof(int)) == sizeof(int);
                _this->pipeReadLocker.unlock();
                if (!pipeResult)
                {
                    log(error,"database: reader管道出现异常！");
                    continue ;
                }
                std::string  value = _this->getValue(targetKey);
                if (value.empty())
                {
                    log(error,"查找键值对失败");
                    uint32_t valueSize = 4;
                    value = "null";
                    type += 3;//构造发送的type值
                    _this->pipeWriteLocker.lock();
                    pipeResult = write(senderFd[1],&type,4)==4;
                    pipeResult = pipeResult && write(senderFd[1],&valueSize,4) == 4;
                    pipeResult = pipeResult && write(senderFd[1],const_cast<char *>(value.data()),valueSize) == valueSize;
                    pipeResult = pipeResult && write(senderFd[1],&sockID, sizeof(int))== sizeof(int );
                    _this->pipeWriteLocker.unlock();
                    if (!pipeResult)
                    {
                        log(error,"database: sender管道出现异常！");
                        continue ;
                    }
                    continue ;
                }
                else
                {
                    uint32_t valueSize = value.size();
                    type += 3;//构造发送的type值
                    _this->pipeWriteLocker.lock();
                    pipeResult = write(senderFd[1],&type,4)==4;
                    pipeResult = pipeResult && write(senderFd[1],&valueSize,4) == 4;
                    pipeResult = pipeResult && write(senderFd[1],const_cast<char *>(value.data()),valueSize) == valueSize;
                    pipeResult = pipeResult && write(senderFd[1],&sockID, sizeof(int))== sizeof(int );
                    _this->pipeWriteLocker.unlock();
                    if (!pipeResult)
                    {
                        log(error,"database: sender管道出现异常！");
                        continue ;
                    }

                }
                break ;
            }
            default:
                log(error,"database:type错误");
                break ;
        }
    }
}
void database::sigHelder(int signum) {
    log(warning,"数据进程收到信号！",signum);
    globalSignalPointer ->saveToFile();
    exit(signum);
}
