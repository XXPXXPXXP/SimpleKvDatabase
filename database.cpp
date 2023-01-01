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
    return true;
}

bool database::addValue(std::string t_key, std::string t_value) {
    int index = search(t_key);
    if (index==-1) {
        keys.emplace_back(t_key.data());
        values.emplace_back(t_value.data());
    } else
    {
        if (compare(values.at(index).data(),t_value.data())) {
            logh(warning);
            printf("待写入的数据: %s 已经存在!\n",t_value.data());
        }
        else {
            logh(warning);
            printf("原数据: %s ,将替换为: %s !",values.at(index).data(),t_value.data());
            values.at(index) = t_value;
        }
    }
    return true;
}

std::string database::getValue(std::string t_key) {
    int i = search(std::move(t_key));
    std::string result;
    if (i == -1)
        return result;
    result = values.at(i);
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
            keys_on_file.read(key.data(),size);
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
            keys_on_file.read(value.data(), size);
        }
        values.emplace_back(value);
        logh(info);
        std::cout<<"已从文件中读取: key: " << key <<" value: "<< value << std::endl;
        key.clear();
        value.clear();
    }
    keys_on_file.close();
    return true;
}

bool database::deleteValue(std::string t_key) {
    int index = search(t_key);
    if (index==-1)
    {
        log(error,"待删除的数据: "+t_key+"无法找到!");
        return false;
    }
    keys.erase(keys.begin()+index);
    values.erase(values.begin()+index);
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
        logh(info);
        std::cout<<"已写入到文件: key: "<<keys.at(i)<<" ;value: "<<values.at(i)<<std::endl;
    }
    size = 0;
    file.write(reinterpret_cast<char *>(&size),4);
    file.close();
    return true;
}


int database::search(std::string target) {
    uint32_t size = keys.size();
    for (int i = 0; i < size; ++i) {
//        char * a = keys.at(i).data();
//        char * b = target.data();
//        bool flag = true;
//        for (;*a!='\0'&&*b!='\0';a++,b++) {
//            if (*a!=*b)
//            {
//                flag = false;
//                break;
//            }
//        }
//        if (!(*a=='\0'&&*b=='\0'))
//            flag = false;
//        if (flag)
//            return i;
        if (keys.at(i)==target)
        {
            return i;
        }
    }
    return -1;
}
