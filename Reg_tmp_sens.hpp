#pragma once

#include <cstddef>
#include <cstdint>

// ============================================================
//  Reg_TempSensor.h
//
//  Offset  Name      Access  Description
//  0x00    DEV_ID    RO      Device identifier (0x55)
//  0x01    STATUS    RO      bit0 = DATA_READY
//  0x02    TEMP_OUT  RO      Current temperature (signed)
// ============================================================

struct TempSensor_RegMap {
    uint8_t dev_id;    // 0x00  RO
    uint8_t status;    // 0x01  RO  bit0 = DATA_READY
    int8_t  temp_out;  // 0x02  RO  signed temperature °C
    uint8_t data;     // 0x03  RW  general-purpose data register for testing writes
   
};

namespace TempReg {
    constexpr uint8_t DEV_ID   = offsetof(TempSensor_RegMap, dev_id);   // 0x00
    constexpr uint8_t STATUS   = offsetof(TempSensor_RegMap, status);   // 0x01
    constexpr uint8_t CURRENT_TEMP = offsetof(TempSensor_RegMap, temp_out); // 0x02
    constexpr uint8_t DATA     = offsetof(TempSensor_RegMap, data);     // 0x03
}

namespace TempDefault {
    constexpr uint8_t DEV_ID_VAL = 0x55;
    constexpr int8_t  INIT_TEMP  = 25;
    constexpr uint8_t INIT_DATA  = 0x00;
}

