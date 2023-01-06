//
// Created by 神奇bug在哪里 on 2023/1/5.
//

#ifndef FINAL_PROJECT_PROCOTOL_H
#define FINAL_PROJECT_PROCOTOL_H

#include <cstdint>
#include "database.h"

bool get(int targetSockId, database &datas);
bool process(int target_sock_id, uint32_t type, database *database);
bool deleteData(int targetSockId, database &datas);
bool sendField(int target_sock_id, void *data_to_send, uint32_t size, int extra);
bool add(int target_sock_id, database &datas);
bool sendHeader(int target_sock_id, uint32_t full_size, uint32_t type);
#endif //FINAL_PROJECT_PROCOTOL_H
