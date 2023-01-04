#pragma ide diagnostic ignored "performance-inefficient-string-concatenation"
//
// Created by 神奇bug在哪里 on 2022/12/24.
//
#include "database.h"
#include <fstream>


bool database::init() {
    if(!readFromFile())
    {
        log(warning,"读取数据文件失败！");
    }
    pthread_mutex_init(&Locker, nullptr);
    return true;
}

bool database::addValue(const std::string& targetKey, const std::string& targetValue) {
    pthread_mutex_lock(&Locker);
    int index = search(targetKey);
    pthread_mutex_unlock(&Locker);
    if (index==-1) {
        pthread_mutex_lock(&Locker);
        keys.emplace_back(targetKey);
        values.emplace_back(targetValue);
        pthread_mutex_unlock(&Locker);
    } else
    {
        if (values.at(index) == targetValue) {
            log(warning, "待写入的数据: " + targetValue + " 已经存在!");
        }
        else {
            log(warning, "原数据: " + values.at(index) + " 将会替换成: " + targetValue);
            pthread_mutex_lock(&Locker);
            values.at(index) = targetValue;
            pthread_mutex_unlock(&Locker);
        }
    }
    return true;
}

std::string database::getValue(const std::string& targetKey)
/*
 * description: 用于获取Key对应的Value
 * return: 出错时会返回空的std::string，成功时会返回Value
 */
{
    pthread_mutex_lock(&Locker);
    int i = search(targetKey);
    std::string result;
    if (i == -1)
        return result;
    result = values.at(i);
    pthread_mutex_unlock(&Locker);
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

        keys.emplace_back(key);
        file.read(reinterpret_cast<char *>(&size), 4);
        if (size<=0) {
            log(error,"文件不完整！");
            values.emplace_back("null");
            return false;
        }
        else
        {
            value.resize(size);
            file.read(const_cast<char *>(value.data()), size);
        }
        values.emplace_back(value);
        log(info,"已从文件中读取: key="+key+"value="+value);
        key.clear();
        value.clear();
    }
    file.close();
    return true;
}

bool database::deleteValue(const std::string& t_key) {
    pthread_mutex_lock(&Locker);
    int index = search(t_key);
    if (index==-1)
    {
        log(error,"待删除的数据: "+t_key+"无法找到!");
        pthread_mutex_unlock(&Locker);
        return false;
    }
    keys.erase(keys.begin()+index);
    values.erase(values.begin()+index);
    pthread_mutex_unlock(&Locker);
    return true;
}

bool database::saveToFile() {
    uint32_t size;
    std::string datas;
    std::ofstream file;
    file.open("datas.dat",std::ios_base::out|std::ios_base::binary);
    if (!file.is_open())
    {
        log(error,"文件保存失败！");
        return false;
    }
    for (int i = 0; i < keys.size(); ++i) {
        size = keys.at(i).size();
        file.write(reinterpret_cast<char *>(&size),4);
        file.write(keys.at(i).c_str(),size);
        size = values.at(i).size();
        file.write(reinterpret_cast<char *>(&size),4);
        file.write(values.at(i).c_str(),size);
        log(info,"已写入到文件,key="+keys.at(i)+"value="+values.at(i));
    }
    size = 0;
    file.write(reinterpret_cast<char *>(&size),4);
    file.close();
    return true;
}


int database::search(const std::string& target) {
    uint32_t size = keys.size();
    for (int i = 0; i < size; ++i) {
        if (keys.at(i)==target)
        {
            return i;
        }
    }
    return -1;
}
