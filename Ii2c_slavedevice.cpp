#include "Ii2c_slavedevice.hpp"

#include <sstream>
#include <iomanip>

// ----------------------------------------------------------
//  Bus-facing interface
// ----------------------------------------------------------

bool I2CSlaveDevice::write(uint8_t reg, uint8_t value)
{
    on_write(reg, value);   // notify subclass
    return true;            // ACK
}

bool I2CSlaveDevice::read(uint8_t reg, uint8_t& out_value) const
{
    // Give subclass first chance to supply a live/computed value
    if (on_read(reg, out_value)) {
        return true;    // subclass handled it
    }

    return false;        // ACK
}

