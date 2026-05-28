#include "i2c_bus.hpp"
#include "Ii2c_slavedevice.hpp"

#include <iostream>
#include <sstream>
#include <iomanip>


bool I2CBus::attach(I2CSlaveDevice* device)
{
    if (!device) return false;

    uint8_t addr = device->getaddress();

    if (devices.count(addr)) {
        std::cerr << "[I2CBus] attach failed, device with [address:0x"
                  << std::hex << static_cast<int>(addr)
                  << "] already in use\n";
        return false;
    }

    devices[addr] = device;
    std::cout << "[I2CBus] device attached at [address:0x"
              << std::hex << static_cast<int>(addr) <<"]" << std::endl;
    return true;
}

void I2CBus::detach(uint8_t address)
{
    if (auto it = devices.find(address); it != devices.end()) {
        devices.erase(it);
        std::cout << "[I2CBus] detached device at 0x"
                  << std::hex << static_cast<int>(address) << std::endl;
    }

    // If we detach the currently active device, abort the transaction
    if (active_addr == address) {
        attached_device  = nullptr;
        active_addr   = 0;
        read_mode     = false;
    }
}


// mode = true for read, false for write
bool I2CBus::start(uint8_t address, bool mode) 
{
    // Abort any previous transaction that wasn't properly stopped
    if (attached_device) {
        std::cerr << "[I2CBus] WARNING: START without prior STOP — forcing stop\n";
        stop();
    }

    I2CSlaveDevice* dev = find(address);
    if (!dev) {
        std::cerr << "[I2CBus] START: no device is set at 0x"
                  << std::hex << static_cast<int>(address) << " (NACK)\n";
        return false;
    }

    attached_device  = dev;
    active_addr   = address;
    read_mode     = mode;

    std::cout << "[I2CBus] START → 0x"
              << std::hex << static_cast<int>(address)
              << (read_mode ? " [READ]\n" : " [WRITE]\n");
    return true;
}

bool I2CBus::write(uint8_t reg, uint8_t value)
{
    if (!attached_device) {
        std::cerr << "[I2CBus] WRITE: no active device\n";
        return false;
    }
    if (read_mode) {
        std::cerr << "[I2CBus] WRITE: bus opened in read mode\n";
        return false;
    }

    bool write_done = attached_device->write(reg, value);
    std::cout << "[I2CBus] WRITE reg=0x" << std::hex << static_cast<int>(reg)
              << " val=0x" << static_cast<int>(value)
              << (write_done ? " ACK\n" : " NACK\n");
    return write_done;
}

bool I2CBus::read(uint8_t reg, uint8_t& out_value)
{
    if (!attached_device) {
        std::cerr << "[I2CBus] READ: no active device\n";
        return false;
    }
    if (!read_mode) {
        std::cerr << "[I2CBus] READ: bus opened in write mode\n";
        return false;
    }

    bool read_done = attached_device->read(reg, out_value);
    std::cout << "[I2CBus] READ  reg=0x" << std::hex << static_cast<int>(reg)
              << " val=0x" << static_cast<int>(out_value)
              << (read_done ? " ACK\n" : " NACK\n");
    return read_done;
}

void I2CBus::stop()
{
    std::cout << "[I2CBus] STOP  (was active: 0x"
              << std::hex << static_cast<int>(active_addr) << ")\n";
    attached_device  = nullptr;
    active_addr   = 0x00;
    read_mode     = false;
}

// ----------------------------------------------------------
//  Diagnostics
// ----------------------------------------------------------

std::string I2CBus::dump_devices() const
{
    std::ostringstream oss;
    oss << "I2CBus device registry (" << std::dec << devices.size() << " device(s)):\n";
    for (auto& [addr, dev] : devices) {
        oss << "  0x" << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<int>(addr)
            << "  →  " << dev->getname() << std::endl;
    }
    return oss.str();
}
