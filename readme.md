Folder Structure

i2c/
│             
│
├── i2c_bus.hpp                  # I2CBus — routes transactions by slave address
├── i2c_bus.cpp
│
|── Reg_i2c.hpp                  # Controller MMIO register definitions(I2C_CONTROL_REG, STATUS, TXDATA,RXDATA,CMD)                                 
├── i2c_controller.hpp           # I2CController — MMIO master + state machine
├── i2c_controller.cpp
│
├── i2c_slave_device.hpp         # I2CSlaveDevice — abstract base for all slaves
├── i2c_slave_device.cpp
│
│── Reg_tmp_sens.hpp         # TempSensor register map + offset constants
│── temp_sensor.hpp          # TempSensor — concrete slave device
│
└── main.cpp                     # Wire-up + Test (Tests A–D)



Architecture

┌─────────────────────────────────────┐
 │           Software (main.cpp)        │
 │    mmio_write() / mmio_read()        │
 └──────────────┬──────────────────────┘
                │ MMIO
 ┌──────────────▼──────────────────────┐
 │         I2CController               │
 │  Reg: CONTROL STATUS TXDATA         │
 │       RXDATA  CMD                   │
 │  State: IDLE→START→SEND_ADDR        │
 │         →TRANSFER→STOP              │
 └──────────────┬──────────────────────┘
                │ start() / write() / read() / stop()
 ┌──────────────▼──────────────────────┐
 │             I2CBus                  │
 │   unordered_map<addr, device*>      │
 │   routes transactions by address    │
 └──────┬──────────────┬───────────────┘
        │              │
 ┌──────▼──────┐ ┌─────▼───────┐
 │ TempSensor  │ │ TempSensor  │   ← any I2CSlaveDevice subclass
 │  addr=0x48  │ │  addr=0x50  │
 └─────────────┘ └─────────────┘

Build
Build = g++ -std=c++17 -Wall -Wextra -o i2c_sim  main.cpp i2c_bus.cpp i2c_controller.cpp Ii2c_slavedevice.cpp
run = ./i2c_sim