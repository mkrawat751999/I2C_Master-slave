#pragma once

#include <cstdint>
#include "Reg_i2c.hpp"

// Forward declaration
class I2CBus;

// Register offsets — match Reg_i2c.h layout
enum class RegOffset : uint32_t {
    CONTROL = 0x00,
    STATUS  = 0x04,
    TXDATA  = 0x08,
    RXDATA  = 0x0C,
    CMD     = 0x10
};

// ============================================================
//  I2CController  — Memory-Mapped I2C Master
//
//  All interaction is through mmio_read() / mmio_write() using the
//  offsets defined in Reg_i2c.h.
//
//  Typical write transaction sequence:
//    mmio_write(RegOffset::CONTROL, 0x1)        → CONTROL.EN = 1
//    mmio_write(RegOffset::TXDATA, 0x4800)     → TXDATA: addr=0x48, data=ignored
//    mmio_write(RegOffset::CMD, CMD_START)  → START condition; STATUS.BUSY=1
//    mmio_write(RegOffset::TXDATA, 0x0242)     → TXDATA: reg=0x02, data=0x42
//    mmio_write(RegOffset::CMD, CMD_WRITE)  → send byte; STATUS.DONE=1
//    mmio_write(RegOffset::CMD, CMD_STOP)   → STOP; STATUS.BUSY=0
//
//  Typical read transaction sequence:
//    mmio_write(RegOffset::TXDATA, 0x8148)     → TXDATA: addr=0x48, RW=1 (read), reg=0x01
//    mmio_write(RegOffset::CMD, CMD_START)  → START condition
//    mmio_write(RegOffset::CMD, CMD_READ)   → receive byte into RXDATA
//    uint8_t v = mmio_read(RegOffset::RXDATA)  → RXDATA.DATA
//    mmio_write(RegOffset::CMD, CMD_STOP)   → STOP
// ============================================================

class I2CController {
private:
    // ----------------------------------------------------------
    //  Internal state machine
    // ----------------------------------------------------------
    enum class State {
        IDLE,
        START,
        SEND_ADDR,
        TRANSFER,
        STOP
    };

    static const char* state_name(State s);

    // Executes the command written to CMD register
    void execute_cmd(uint32_t cmd);

    // STATUS helpers
    void set_busy (bool v);
    void set_done (bool v);
    void set_error(bool v);
    void set_nack (bool v);
    void clear_status();

   /*
    *Register file  
   */
    I2C_REGISTER_BLOCK i2c_reg;

    // ----------------------------------------------------------
    //  Internal state
    // ----------------------------------------------------------
    State   state     = State::IDLE;
    uint8_t cur_addr  = 0x00;   // latched slave address from TXDATA
    bool    read_mode = false;  // latched RW bit from TXDATA

    I2CBus& bus;

public:
    explicit I2CController(I2CBus& bus);

    void     mmio_write(uint32_t offset, uint32_t value);
    uint32_t mmio_read(uint32_t offset) const;

    // ----------------------------------------------------------
    //  Diagnostics
    // ----------------------------------------------------------
    void dump_regs() const;


};

