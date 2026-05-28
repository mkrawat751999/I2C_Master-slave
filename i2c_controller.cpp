#include "i2c_controller.hpp"
#include "i2c_bus.hpp"

#include <iostream>
#include <iomanip>
#include <stdexcept>



// ----------------------------------------------------------
//  Constructor
// ----------------------------------------------------------

I2CController::I2CController(I2CBus& bus)
    : bus(bus)
{
    i2c_reg.CONTROL.raw = 0;
    i2c_reg.STATUS.raw  = 0;
    i2c_reg.TXDATA.raw  = 0;
    i2c_reg.RXDATA.raw  = 0;
    i2c_reg.CMD.raw     = 0;
}

// ----------------------------------------------------------
//  MMIO write dispatch
// ----------------------------------------------------------

void I2CController::mmio_write(uint32_t offset, uint32_t value)
{
    switch (static_cast<RegOffset>(offset)) {

        case RegOffset::CONTROL:
            i2c_reg.CONTROL.raw = value;
            std::cout << "[CTRL] CONTROL ← 0x" << std::hex << value
                  << "  EN=" << i2c_reg.CONTROL.bits.EN << std::endl;
            break;

        case RegOffset::STATUS:
            // STATUS is read-only; writes are ignored
            std::cerr << "[CTRL] WARNING: write to read-only STATUS register ignored\n";
            break;

        case RegOffset::TXDATA:
            i2c_reg.TXDATA.raw = value;
            std::cout << "[CTRL] TXDATA  ← 0x" << std::hex << value
                  << "  addr=0x" << static_cast<int>(i2c_reg.TXDATA.bits.SLAVE_ADDR)
                  << "  data=0x" << static_cast<int>(i2c_reg.TXDATA.bits.DATA)
                  << "  RW="     << i2c_reg.TXDATA.bits.RW_BIT << std::endl;
            break;

        case RegOffset::RXDATA:
            // RXDATA is read-only; writes are ignored
            std::cerr << "[CTRL] WARNING: write to read-only RXDATA register ignored\n";
            break;

        case RegOffset::CMD:
            i2c_reg.CMD.raw = value;
            execute_cmd(i2c_reg.CMD.bits.OP);
            break;

        default:
            std::cerr << "[CTRL] WARNING: write to unknown offset 0x"
                  << std::hex << offset << std::endl;
            break;
    }
}

// ----------------------------------------------------------
//  MMIO read dispatch
// ----------------------------------------------------------

uint32_t I2CController::mmio_read(uint32_t offset) const
{
    switch (static_cast<RegOffset>(offset)) {
        case RegOffset::CONTROL: return i2c_reg.CONTROL.raw;
        case RegOffset::STATUS:  return i2c_reg.STATUS.raw;
        case RegOffset::TXDATA:  return i2c_reg.TXDATA.raw;
        case RegOffset::RXDATA:  return i2c_reg.RXDATA.raw;
        case RegOffset::CMD:     return i2c_reg.CMD.raw;
        default:
            std::cerr << "[CTRL] WARNING: read from unknown offset 0x"
                    << std::hex << offset << std::endl;
            return 0xdeadbeef;
    }
}

// ----------------------------------------------------------
//  Command execution  (state machine core)
// ----------------------------------------------------------

void I2CController::execute_cmd(uint32_t cmd)
{
    const I2C_CMD_OP op = static_cast<I2C_CMD_OP>(cmd);

    // Controller must be enabled
    if (!i2c_reg.CONTROL.bits.EN && op != I2C_CMD_OP::NOP) {
        std::cerr << "[CTRL] CMD ignored — controller not enabled (CONTROL.EN=0)\n";
        set_error(true);
        return;
    }

    std::cout << "[CTRL] CMD=" << cmd
              << "  state=" << state_name(state) << std::endl;

    switch (op) {

    case I2C_CMD_OP::START:
    {
        clear_status();
        set_busy(true);
        cur_addr  = static_cast<uint8_t>(i2c_reg.TXDATA.bits.SLAVE_ADDR);
        read_mode = (i2c_reg.TXDATA.bits.RW_BIT == 1);
        state     = State::START;
        bool all_set  = bus.start(cur_addr, read_mode);
        if (!all_set) {
            state = State::IDLE;
            set_nack(true);
            set_error(true);
            set_busy(false);
        } else {
            state = State::SEND_ADDR;
        }
        break;
    }

    case I2C_CMD_OP::WRITE:
    {
        if (state != State::SEND_ADDR && state != State::TRANSFER) {
            std::cerr << "[CTRL] WRITE invalid in state " << state_name(state) << std::endl;
            set_error(true);
            break;
        }

        uint8_t reg  = static_cast<uint8_t>(i2c_reg.TXDATA.bits.SLAVE_ADDR); // re-used as reg index after START
        uint8_t data = static_cast<uint8_t>(i2c_reg.TXDATA.bits.DATA);

        // After SEND_ADDR the upper byte of TXDATA holds the register index
        // and the lower byte holds the data value
        bool all_set = bus.write(reg, data);
        if (!all_set) {
            set_nack(true);
            set_error(true);
        } else {
            state = State::TRANSFER;
            set_done(true);
        }
        break;
    }

    case I2C_CMD_OP::READ:
    {
        if (state != State::SEND_ADDR && state != State::TRANSFER) {
            std::cerr << "[CTRL] READ invalid in state " << state_name(state) << std::endl;
            set_error(true);
            break;
        }

        uint8_t reg = static_cast<uint8_t>(i2c_reg.TXDATA.bits.SLAVE_ADDR);
        uint8_t data = 0;

        bool all_set = bus.read(reg, data);
        if (!all_set) {
            set_nack(true);
            set_error(true);
        } else {
            i2c_reg.RXDATA.bits.DATA  = data;
            i2c_reg.RXDATA.bits.VALID = 1;
            state = State::TRANSFER;
            set_done(true);
        }
        break;
    }

    case I2C_CMD_OP::STOP:
    {
        bus.stop();
        state = State::STOP;

        set_busy(false);
        set_done(true);

        state = State::IDLE;
        break;
    }

    default:
        std::cerr << "[CTRL] Unknown command opcode: " << cmd << std::endl;
        set_error(true);
        break;
    }
}

// ----------------------------------------------------------
//  STATUS helpers
// ----------------------------------------------------------

void I2CController::set_busy (bool v) { i2c_reg.STATUS.bits.BUSY = v ? 1u : 0u; }
void I2CController::set_done (bool v) { i2c_reg.STATUS.bits.DONE = v ? 1u : 0u; }
void I2CController::set_error(bool v) { i2c_reg.STATUS.bits.ERR  = v ? 1u : 0u; }
void I2CController::set_nack (bool v) { i2c_reg.STATUS.bits.NACK = v ? 1u : 0u; }

void I2CController::clear_status()
{
    i2c_reg.STATUS.raw        = 0;
    i2c_reg.RXDATA.bits.VALID = 0;
}

// ----------------------------------------------------------
//  Diagnostics
// ----------------------------------------------------------

void I2CController::dump_regs() const
{
    std::cout << "=== I2CController Register Dump ===\n"
              << std::hex << std::setfill('0')
              << "  CONTROL [0x00] = 0x" << std::setw(8) << i2c_reg.CONTROL.raw
              << "  (EN="    << i2c_reg.CONTROL.bits.EN
              << " SPEED="   << i2c_reg.CONTROL.bits.SPEED << ")\n"

              << "  STATUS  [0x04] = 0x" << std::setw(8) << i2c_reg.STATUS.raw
              << "  (BUSY="  << i2c_reg.STATUS.bits.BUSY
              << " DONE="    << i2c_reg.STATUS.bits.DONE
              << " ERR="     << i2c_reg.STATUS.bits.ERR
              << " NACK="    << i2c_reg.STATUS.bits.NACK << ")\n"

              << "  TXDATA  [0x08] = 0x" << std::setw(8) << i2c_reg.TXDATA.raw
              << "  (ADDR=0x" << static_cast<int>(i2c_reg.TXDATA.bits.SLAVE_ADDR)
              << " DATA=0x"   << static_cast<int>(i2c_reg.TXDATA.bits.DATA)
              << " RW="       << i2c_reg.TXDATA.bits.RW_BIT << ")\n"

              << "  RXDATA  [0x0C] = 0x" << std::setw(8) << i2c_reg.RXDATA.raw
              << "  (DATA=0x" << static_cast<int>(i2c_reg.RXDATA.bits.DATA)
              << " VALID="    << i2c_reg.RXDATA.bits.VALID << ")\n"

              << "  CMD     [0x10] = 0x" << std::setw(8) << i2c_reg.CMD.raw << std::endl
              << "  state   = " << state_name(state) << std::endl
              << "===================================\n";
}

const char* I2CController::state_name(State s)
{
    switch (s) {
    case State::IDLE:      return "IDLE";
    case State::START:     return "START";
    case State::SEND_ADDR: return "SEND_ADDR";
    case State::TRANSFER:  return "TRANSFER";
    case State::STOP:      return "STOP";
    default:               return "UNKNOWN";
    }
}
