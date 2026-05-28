#pragma once

#include <cstdint>
#include <cstring>
#include <string>

// ============================================================
//  I2CSlaveDevice  — Base Interface class for all I2C slave devices
//
//  Subclass this to implement a concrete device.
//  Override on_write() and on_read() for custom behaviour.
//
// ============================================================


class I2CSlaveDevice {
private:
    uint8_t     address;
    std::string name;
    uint8_t     dev_id;    // value returned when reading DEV_ID register

protected:

    /*
    * Called when a write is received to a register.
    * Subclass can implement custom behavior.
    */
    virtual void on_write(uint8_t reg, uint8_t value) = 0;
    virtual bool on_read(uint8_t reg, uint8_t& out_value) const = 0;

public:
    // 'address' : 7-bit I2C slave address
    // 'dev_id'  : value returned when reading DEV_ID register
    // 'name'    : human-readable label for diagnostics
    explicit I2CSlaveDevice(uint8_t address,
                         uint8_t dev_id ,
                         std::string name): address(address), name(std::move(name)), dev_id(dev_id){}; 

    virtual ~I2CSlaveDevice() {};

    // Non-copyable
    I2CSlaveDevice(const I2CSlaveDevice&)            = delete;
    I2CSlaveDevice& operator=(const I2CSlaveDevice&) = delete;

    // ----------------------------------------------------------
    //  Identity
    // ----------------------------------------------------------
    uint8_t            getaddress() const { return address; }
    const std::string& getname()    const { return name; }
    uint8_t            getdev_id()  const { return dev_id; }

    /*
        bus-facing interface
        read/write are called by the I2CBus when the master initiates a transaction targeting this device.
        return true for ACK, false for NACK.
    */
    bool write(uint8_t reg, uint8_t value);
    bool read(uint8_t reg, uint8_t& out_value) const;

};

