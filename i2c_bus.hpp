#pragma once

#include <cstdint>
#include <unordered_map>
#include <string>

class I2CSlaveDevice;

class I2CBus {
private:

    inline I2CSlaveDevice* find(uint8_t address)
    {
        auto it = devices.find(address);
        return (it != devices.end()) ? it->second : nullptr;
    }

    std::unordered_map<uint8_t, I2CSlaveDevice*> devices;
    I2CSlaveDevice* attached_device  = nullptr;
    uint8_t      active_addr   = 0x00;
    bool         read_mode     = false;
public:
    I2CBus() {};
    ~I2CBus() {};

    // Non-copyable — bus is a shared resource
    I2CBus(const I2CBus&)            = delete;
    I2CBus& operator=(const I2CBus&) = delete;

     
    /*
        * Bus API:
        *  - attach() / detach() to manage slave devices on the bus
        *  - start() / write() / read() / stop() for transactions
    */
    bool attach(I2CSlaveDevice* device);
    void detach(uint8_t address);

   /*
        * Transaction API (called by I2CController):
         - start() initiates a transaction to a slave address with a specified direction (read/write).
            read_mode=true indicates a read transaction, false indicates a write transaction.
         - write() and read() perform register accesses on the active slave device during a transaction.
         - stop() ends the transaction and releases the bus.
         - Each method returns a boolean indicating success (ACK) or failure (NACK) of the operation.
   */

    bool start(uint8_t address, bool read_mode);
    bool write(uint8_t reg, uint8_t value);
    bool read(uint8_t reg, uint8_t& out_value);
    void stop();

    // ----------------------------------------------------------
    //  Diagnostics
    // ----------------------------------------------------------
    bool        is_busy()       const { return attached_device  != nullptr; }
    uint8_t     active_address() const { return active_addr; }
    std::string dump_devices()  const;

};

