#pragma clang diagnostic push
#pragma ide diagnostic ignored "performance-inefficient-string-concatenation"
#pragma ide diagnostic ignored "performance-unnecessary-value-param"
//
// Created by 徐鑫平 on 2022/12/24.
//
#include "database.h"
#include <fstream>
#include <cstdlib>
#include <utility>
bool compare(const char * a,const char *b)
{
    bool result = true;
    for (; *a!='\0'&&*b!='\0';a++,b++) {
        if (*a != *b)
        {
            result = false;
            break;
        }
    }
    if (!(*a=='\0'&&*b=='\0'))
        result= false;
    return result;
}
bool database::init() {
    if(!readFromFile())
    {
        log(warning,"读取数据文件失败！");
    }
    pthread_mutex_init(&Locker, nullptr);
    pthread_mutex_init(&Locker, nullptr);
    return true;
}

bool database::addValue(std::string t_key, std::string t_value) {
    pthread_mutex_lock(&Locker);
    int index = search(t_key);
    pthread_mutex_unlock(&Locker);
    if (index==-1) {
        pthread_mutex_lock(&Locker);
        keys.emplace_back(t_key.data());
        values.emplace_back(t_value.data());
        pthread_mutex_unlock(&Locker);
    } else
    {
        if (compare(values.at(index).data(),t_value.data())) {
            log(warning,"待写入的数据: "+t_value+" 已经存在!");
        }
        else {
            log(warning,"原数据: "+values.at(index)+" 将会替换成: "+t_value);
            pthread_mutex_lock(&Locker);
            values.at(index) = t_value;
            pthread_mutex_unlock(&Locker);
        }
    }
    return true;
}

std::string database::getValue(std::string t_key) {
    pthread_mutex_lock(&Locker);
    int i = search(std::move(t_key));
    std::string result;
    if (i == -1)
        return result;
    result = values.at(i);
    pthread_mutex_unlock(&Locker);
    return result;
}

bool database::readFromFile() {
    std::ifstream keys_on_file;
    keys_on_file.open("datas.dat",std::ios_base::in|std::ios_base::binary);
    if(!keys_on_file.is_open())
        return false;
    while (true)
    {
        uint32_t size;
        std::string value,key;
        keys_on_file.read(reinterpret_cast<char *>(&size),4);
        key.resize(size);
        if (size<=0)
            break;
        else
        {
            keys_on_file.read(const_cast<char *>(key.data()),size);
        }

        keys.emplace_back(key);
        keys_on_file.read(reinterpret_cast<char *>(&size),4);
        if (size<=0) {
            log(error,"文件不完整！");
            values.emplace_back("null");
            return false;
        }
        else
        {
            value.resize(size);
            keys_on_file.read(const_cast<char *>(value.data()), size);
        }
        values.emplace_back(value);
        log(info,"已从文件中读取: key="+key+"value="+value);
        key.clear();
        value.clear();
    }
    keys_on_file.close();
    return true;
}

bool database::deleteValue(std::string t_key) {
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


int database::search(std::string target) {
    uint32_t size = keys.size();
    for (int i = 0; i < size; ++i) {
        if (keys.at(i)==target)
        {
            return i;
        }
    }
    return -1;
}

#pragma clang diagnostic pop