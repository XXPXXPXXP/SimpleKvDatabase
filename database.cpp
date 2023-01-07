#pragma ide diagnostic ignored "performance-inefficient-string-concatenation"
//
// Created by 神奇bug在哪里 on 2022/12/24.
//
#include "database.h"
#include <fstream>

void database::start(int readerFd[2],int senderFd[2]) {
    if(!readFromFile())
    {
        log(warning,"读取数据文件失败！");
    }
    managerID = std::thread(manager, this);
    managerID.join();
    log(error,"database: 管理线程退出，数据进程即将崩溃！");
    exit(-1);
}

bool database::putValue(const std::string& targetKey, const std::string& targetValue) noexcept(false){
    try {
        std::string value = datas.at(targetKey);
        if (value == targetValue) {
            log(warning, "待写入的数据: " + targetValue + " 已经存在!");
        }
        else {
            log(warning, "原数据: " + value + " 将会替换成: " + targetValue);
            databaseLocker.lock();
            datas.at(targetKey) = targetValue;
            databaseLocker.unlock();
        }
    }
    catch(std::exception & e) {
        databaseLocker.lock();
        datas.emplace(targetKey,targetValue);
        databaseLocker.unlock();
    }
    return true;
}

std::string database::getValue(const std::string& targetKey)
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
    }
    catch (std::out_of_range) {
        return result;
    }
    return result;
}
bool database::readFromFile() {
    std::ifstream file;
    file.open("datas.dat", std::ios_base::in | std::ios_base::binary);
    if(!file.is_open())
        return false;
    while (true)
    {
        uint32_t size;
        std::string value,key;
        file.read(reinterpret_cast<char *>(&size), 4);
        key.resize(size);
        if (size<=0)
            break;
        else
        {
            file.read(const_cast<char *>(key.data()), size);
        }

        file.read(reinterpret_cast<char *>(&size), 4);
        if (size<=0) {
            log(error,"文件不完整！");
            datas.emplace(key,"null");
            return false;
        }
        else
        {
            value.resize(size);
            file.read(const_cast<char *>(value.data()), size);
        }
        datas.emplace(key,value);
        log(info,"已从文件中读取: key="+key+"value="+value);
        key.clear();
        value.clear();
    }
    file.close();
    return true;
}
bool database::deleteValue(const std::string& t_key) {
    try {
        databaseLocker.lock();
        datas.erase(t_key);
        databaseLocker.unlock();
    }
    catch (std::out_of_range) {
        return false;
    }
    return true;
}

bool database::saveToFile() {
    uint32_t size;
    std::ofstream file;
    file.open("datas.dat",std::ios_base::out|std::ios_base::binary);
    if (!file.is_open())
    {
        log(error,"文件保存失败！");
        return false;
    }
    for (auto & data : datas) {
        std::string targetKey = data.first,targetValue= data.second;
        file.write(reinterpret_cast<char *>(&size),4);
        file.write(targetKey.c_str(),size);
        size = targetValue.size();
        file.write(reinterpret_cast<char *>(&size),4);
        file.write(targetValue.c_str(),size);
        log(info,"已写入到文件,key="+targetKey+"value="+targetValue);
    }
    size = 0;
    file.write(reinterpret_cast<char *>(&size),4);
    file.close();
    return true;
}

