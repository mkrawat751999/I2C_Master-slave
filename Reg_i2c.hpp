#pragma once

#include <cstdint>

// ============================================================
//  I2C Controller — Memory-Mapped Register Definitions
//
//  Base address is supplied externally; these structs describe
//  the layout at that base.
//
//  Register map:
//    Offset  Register   Description
//    0x00    CONTROL    Enable / configuration
//    0x04    STATUS     Status / error reporting
//    0x08    TXDATA     Data or address to transmit
//    0x0C    RXDATA     Received data  (read-only)
//    0x10    CMD        Command register
// ============================================================

// ------------------------------------------------------------
//  0x00  CONTROL — Enable / configuration
// ------------------------------------------------------------
struct I2C_CONTROL_REG {
    union {
        uint32_t raw;

        struct {
            uint32_t EN          : 1;   // [0]    1 = controller enabled
            uint32_t SPEED       : 2;   // [2:1]  00=standard(100kHz)
            uint32_t ADDR_MODE   : 1;   // [3]    0 = 7-bit, 1 = 10-bit
            uint32_t ACK_EN      : 1;   // [4]    1 = send ACK on receive
            uint32_t RESERVED    : 27;  // [31:5] reserved, write 0
        } bits;
    };
};


// ------------------------------------------------------------
//  0x04  STATUS — Status / error reporting  (read-only)
// ------------------------------------------------------------
struct I2C_STATUS_REG {
    union {
        uint32_t raw;

        struct {
            uint32_t BUSY        : 1;   // [0]    1 = transaction in progress
            uint32_t DONE        : 1;   // [1]    1 = last operation complete
            uint32_t ERR         : 1;   // [2]    1 = error occurred
            uint32_t NACK        : 1;   // [3]    1 = slave sent NACK
            uint32_t ARB_LOST    : 1;   // [4]    1 = arbitration lost
            uint32_t BUS_BUSY    : 1;   // [5]    1 = bus held by another master
            uint32_t RESERVED    : 26;  // [31:6] reserved
        } bits;
    };
};


// ------------------------------------------------------------
//  0x08  TXDATA — Byte to transmit (or slave address)
// ------------------------------------------------------------
struct I2C_TXDATA_REG {
    union {
        uint32_t raw;

        struct {
            uint32_t DATA        : 8;   // [7:0]   data byte to send
            uint32_t SLAVE_ADDR  : 7;   // [14:8]  7-bit slave address
            uint32_t RW_BIT      : 1;   // [15]    0 = write, 1 = read
            uint32_t RESERVED    : 16;  // [31:16] reserved, write 0
        } bits;
    };
};


// ------------------------------------------------------------
//  0x0C  RXDATA — Received byte  (read-only)
// ------------------------------------------------------------
struct I2C_RXDATA_REG {
    union {
        uint32_t raw;

        struct {
            uint32_t DATA        : 8;   // [7:0]  received byte
            uint32_t VALID       : 1;   // [8]    1 = data is valid / new
            uint32_t RESERVED    : 23;  // [31:9] reserved
        } bits;
    };
};


// ------------------------------------------------------------
//  0x10  CMD — Command register (write-only)
// ------------------------------------------------------------

// Command codes written to CMD.OP
enum class I2C_CMD_OP : uint32_t {
    NOP         = 0x0,  // no operation
    START       = 0x1,  // generate START condition
    STOP        = 0x2,  // generate STOP  condition
    WRITE       = 0x3,  // send byte from TXDATA
    READ        = 0x4,  // receive byte into RXDATA
};

struct I2C_CMD_REG {
    union {
        uint32_t raw;

        struct {
            uint32_t OP          : 4;   // [3:0]  command opcode (I2C_CMD_OP)
            uint32_t RESERVED    : 28;  // [31:4] reserved, write 0
        } bits;
    };
};


// ============================================================
//  I2C_REGISTER_BLOCK
//
//  Place a volatile pointer to this struct at the controller's
//  base address to access all registers through one handle.
//
//  Usage:
//      volatile I2C_REGISTER_BLOCK* i2c =
//          reinterpret_cast<volatile I2C_REGISTER_BLOCK*>(I2C_BASE_ADDR);
//
//      i2c->CONTROL.bits.EN   = 1;
//      i2c->CMD.bits.OP       = static_cast<uint32_t>(I2C_CMD_OP::START);
//      uint8_t rx = i2c->RXDATA.bits.DATA;
// ============================================================

typedef struct I2C_REGISTER_BLOCK {
    I2C_CONTROL_REG  CONTROL;   // 0x00
    I2C_STATUS_REG   STATUS;    // 0x04
    I2C_TXDATA_REG   TXDATA;    // 0x08
    I2C_RXDATA_REG   RXDATA;    // 0x0C
    I2C_CMD_REG      CMD;       // 0x10
} I2C_REGISTER_BLOCK, *PI2C_REGISTER_BLOCK;

static_assert(sizeof(I2C_REGISTER_BLOCK) == 0x14,
              "I2C_REGISTER_BLOCK size mismatch — check struct packing");
