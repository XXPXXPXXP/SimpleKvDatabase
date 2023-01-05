//
// Created by 神奇bug在哪里 on 2023/1/5.
//

#ifndef FINAL_PROJECT_SOCKET_H
#define FINAL_PROJECT_SOCKET_H

#include <cstdint>

bool get(int targetSockId);
bool process(int target_sock_id, uint32_t type);
bool deleteData(int targetSockId);
bool sendField(int target_sock_id, void *data_to_send, uint32_t size, int extra);
bool add(int target_sock_id);
bool sendHeader(int target_sock_id, uint32_t full_size, uint32_t type);
#endif //FINAL_PROJECT_SOCKET_H
