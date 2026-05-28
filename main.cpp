#include <iostream>
#include <cassert>
#include <cstdint>

#include "i2c_bus.hpp"
#include "i2c_controller.hpp"
#include "Ii2c_slavedevice.hpp"
#include "temp_sensor.hpp"
#include "Reg_i2c.hpp"

// ============================================================
//  Helpers
// ============================================================

// Builds the TXDATA value from a slave address, register index,
// data byte and RW flag — mirrors I2C_TXDATA_REG bitfield layout.
//
//   [15]    RW      1=read, 0=write
//   [14:8]  ADDR    7-bit slave address
//   [7:0]   DATA    data byte  (or register index after START)
static uint32_t make_txdata(uint8_t addr, uint8_t data, bool read = false)
{
    uint32_t v = 0;
    v |= static_cast<uint32_t>(data);
    v |= static_cast<uint32_t>(addr) << 8;
    if (read) v |= (1u << 15);
    return v;
}

static void print_separator(const char* title)
{
    std::cout << "\n────────────────────────────────────────\n"
              << "  " << title << std::endl
              << "────────────────────────────────────────\n";
}

// ============================================================
//  Test A — Write transaction
//  Write 0x42 to register 0x02 (DATA) of TempSensor @ 0x48
// ============================================================
void test_write_transaction(I2CController& ctrl, TempSensor& sensor)
{
    print_separator("TEST A: Write transaction");

    // Enable controller
    ctrl.mmio_write(0x00, 0x1);                            // CONTROL.EN = 1

    // Set slave address and direction (write), then START
    ctrl.mmio_write(0x08, make_txdata(0x48, 0x00, false)); // TXDATA: addr=0x48 RW=0
    ctrl.mmio_write(0x10, static_cast<uint32_t>(I2C_CMD_OP::START));

    // Write 0x42 to endpoint register 0x02
    // After START, upper byte of TXDATA is interpreted as register index where we have to write.
    ctrl.mmio_write(0x08, make_txdata(0x03, 0x42, false)); // reg=0x03, data=0x42
    ctrl.mmio_write(0x10, static_cast<uint32_t>(I2C_CMD_OP::WRITE));

    // STOP
    ctrl.mmio_write(0x10, static_cast<uint32_t>(I2C_CMD_OP::STOP));

    // Check STATUS: DONE=1, ERR=0
    uint32_t status = ctrl.mmio_read(0x04);
    I2C_STATUS_REG s; s.raw = status;
    std::cout << "\n[TEST A] STATUS: BUSY=" << s.bits.BUSY
              << " DONE=" << s.bits.DONE
              << " ERR="  << s.bits.ERR << std::endl;
    assert(s.bits.DONE == 1 && "Test A: DONE should be set");
    assert(s.bits.ERR  == 0 && "Test A: ERR should be clear");
    assert(sensor.get_data_reg() == 0x42 && "Test A: sensor DATA reg should be 0x42");
    std::cout << "[TEST A] PASSED\n";
}

// ============================================================
//  Test B — Read transaction
//  Read register 0x03 (TEMP_RAW) from TempSensor @ 0x48
// ============================================================
void test_read_transaction(I2CController& ctrl, TempSensor& sensor)
{
    print_separator("TEST B: Read transaction");

    sensor.set_temperature(37);   // inject 37 °C

    // START with read flag set
    ctrl.mmio_write(0x08, make_txdata(0x48, 0x00, true));   // addr=0x48 RW=1
    ctrl.mmio_write(0x10, static_cast<uint32_t>(I2C_CMD_OP::START));

    // READ register 0x02 (TEMP_RAW)
    ctrl.mmio_write(0x08, make_txdata(TempReg::CURRENT_TEMP, 0x00, true));
    ctrl.mmio_write(0x10, static_cast<uint32_t>(I2C_CMD_OP::READ));

    ctrl.mmio_write(0x10, static_cast<uint32_t>(I2C_CMD_OP::STOP));

    // Retrieve RXDATA
    uint32_t rxraw = ctrl.mmio_read(0x0C);
    I2C_RXDATA_REG rx; rx.raw = rxraw;
    std::cout << "[TEST B] RXDATA: DATA=0x" << std::hex
              << static_cast<int>(rx.bits.DATA)
              << " VALID=" << rx.bits.VALID << std::endl;

    assert(rx.bits.VALID == 1       && "Test B: VALID should be set");
    assert(rx.bits.DATA  == 37      && "Test B: DATA should be 37");
    std::cout << "[TEST B] PASSED\n";
}

// ============================================================
//  Test C — Address NACK
//  Transaction to an address with no registered device
// ============================================================
void test_nack(I2CController& ctrl)
{
    print_separator("TEST C: NACK on unknown address");

    ctrl.mmio_write(0x08, make_txdata(0x77, 0x00, false));  // nobody @ 0x77
    ctrl.mmio_write(0x10, static_cast<uint32_t>(I2C_CMD_OP::START));

    uint32_t status = ctrl.mmio_read(0x04);
    I2C_STATUS_REG s; s.raw = status;
    std::cout << "[TEST C] STATUS: ERR=" << s.bits.ERR
              << " NACK=" << s.bits.NACK << std::endl;
    assert(s.bits.ERR  == 1 && "Test C: ERR should be set");
    assert(s.bits.NACK == 1 && "Test C: NACK should be set");
    std::cout << "[TEST C] PASSED\n";
}

// ============================================================
//  Test D — Two devices on the same bus
//  Attach a second TempSensor @ 0x50, write to it independently
// ============================================================
void test_two_devices(I2CController& ctrl, I2CBus& bus)
{
    print_separator("TEST D: Two devices on bus");

    // address and initial temprature
    TempSensor sensor2(0x50, 20);
    bus.attach(&sensor2);

    std::cout << bus.dump_devices();

    // Write to sensor2
    ctrl.mmio_write(0x08, make_txdata(0x50, 0x00, false));
    ctrl.mmio_write(0x10, static_cast<uint32_t>(I2C_CMD_OP::START));
    ctrl.mmio_write(0x08, make_txdata(0x03, 0xBE, false));  // DATA reg ← 0xBE
    ctrl.mmio_write(0x10, static_cast<uint32_t>(I2C_CMD_OP::WRITE));
    ctrl.mmio_write(0x10, static_cast<uint32_t>(I2C_CMD_OP::STOP));

    // Verify directly via endpoint
    uint8_t val = 0;
    sensor2.read(0x03, val);
    std::cout << "[TEST D] sensor2 DATA reg = 0x" << std::hex
              << static_cast<int>(val) << std::endl;
    assert(val == 0xBE && "Test D: sensor2 DATA should be 0xBE");
    std::cout << "[TEST D] PASSED\n";

    bus.detach(0x50);
}

// ============================================================
//  main
// ============================================================
int main()
{
    std::cout << "╔══════════════════════════════════════╗\n"
              << "║   LET THE TEST BEGIN YEAHH!          ║\n"
              << "╚══════════════════════════════════════╝\n";

    /*
        * Test setup:
        *  - Create an I2C bus and Master controller
        *  - Create a TempSensor device at address 0x48 and inject an initial temperature in celsius
        *  - Attach the sensor to the bus
    */
    I2CBus        bus;
    I2CController ctrl(bus);
    TempSensor    sensor1(0x48, 25);
    bus.attach(&sensor1);
    std::cout << std::endl << bus.dump_devices();



    // ── Run tests 1 ────────────────────────────────────────────
    test_write_transaction(ctrl, sensor1);

    // ── Run tests 1 ────────────────────────────────────────────
    test_read_transaction(ctrl, sensor1);

    // ── Run tests 1 ────────────────────────────────────────────
    test_nack(ctrl);

    // ── Run tests 1 ────────────────────────────────────────────
    test_two_devices(ctrl, bus);

    // // ── Final register dump ──────────────────────────────────
    // print_separator("Final register state");
    // ctrl.dump_regs();


    std::cout << "\n✓ All tests passed\n";
    return 0;
}
