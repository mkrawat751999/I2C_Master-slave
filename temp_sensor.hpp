#pragma once

#include "Ii2c_slavedevice.hpp"
#include "Reg_tmp_sens.hpp"
#include <cstdint>
#include <algorithm>
#include <iostream>

class TempSensor : public I2CSlaveDevice {
private:
    inline void update_status()
    {
        uint8_t status = temp_sens_regs.status;
        // New Temprature is ready — set Status bit
        status |= 0x01;
        temp_sens_regs.status = status;  
    }

    int8_t simulated_temp;
    uint8_t data_reg;   // general-purpose register for testing writes
    TempSensor_RegMap temp_sens_regs{};   // internal register file
public:
    // 'address'      : 7-bit I2C address
    // 'init_temp_c'  : starting simulated temperature in °C
    explicit TempSensor(uint8_t address, int8_t init_temp_c)
        : I2CSlaveDevice(address, TempDefault::DEV_ID_VAL, "TempSensor")
        , simulated_temp(init_temp_c)
    {
        // Default thresholds
        temp_sens_regs.dev_id = TempDefault::DEV_ID_VAL;
        temp_sens_regs.temp_out = init_temp_c;
        temp_sens_regs.data = TempDefault::INIT_DATA;
        update_status();
    }

    // ----------------------------------------------------------
    //  Test helpers — let the test harness inject a temperature
    // ----------------------------------------------------------
    inline void set_temperature(int8_t temp_c)
    {
        simulated_temp = temp_c;
        temp_sens_regs.temp_out = temp_c;
        update_status();
    }

    inline int8_t get_temperature() const { return simulated_temp; }
    inline uint8_t get_data_reg() const { return data_reg; }

protected:
    
    inline bool on_read(uint8_t reg, uint8_t& value) const override
    {
        switch (reg) {
            case TempReg::DEV_ID:
                value = temp_sens_regs.dev_id;
                return true;
            case TempReg::CURRENT_TEMP:
                value = static_cast<uint8_t>(temp_sens_regs.temp_out);
                return true;
            case TempReg::DATA:
                value = temp_sens_regs.data;
                return true;
            default:
                std::cerr << "read not allowed from reg 0x" << std::hex << static_cast<int>(reg) << std::endl;
                return false;   // NACK for unsupported registers
        }       
    }

    inline void on_write(uint8_t reg, uint8_t value) override
    {
        if (reg == TempReg::DATA) {
            data_reg = value;
            temp_sens_regs.data = value;
        } else {
            std::cerr << "write not allowed to reg 0x" << std::hex << static_cast<int>(reg)
                      << " val=0x" << static_cast<int>(value) << std::endl;
        }
    }

};


