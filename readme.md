# I2C Subsystem Model

A transaction-level simulation of an I2C subsystem in C++17.
Models a memory-mapped I2C master controller, a bus abstraction,
and pluggable slave devices — no electrical waveform simulation required.

---

## Folder Structure

```
i2c/
├── Reg_i2c.hpp                  # Controller MMIO register definitions
│                                # (CONTROL, STATUS, TXDATA, RXDATA, CMD)
├── i2c_bus.hpp                  # I2CBus — routes transactions by slave address
├── i2c_bus.cpp
├── i2c_controller.hpp           # I2CController — MMIO master + state machine
├── i2c_controller.cpp
├── i2c_slave_device.hpp         # I2CSlaveDevice — abstract base for all slaves
├── i2c_slave_device.cpp
├── Reg_tmp_sens.hpp             # TempSensor register map + offset constants
├── temp_sensor.hpp              # TempSensor — concrete slave device
└── main.cpp                     # Wire-up + smoke tests (Tests A–D)
```

---

## Architecture

```
┌─────────────────────────────────────┐
│         Software (main.cpp)         │
│    mmio_write() / mmio_read()       │
└──────────────┬──────────────────────┘
               │ MMIO
┌──────────────▼──────────────────────┐
│          I2CController              │
│  Reg: CONTROL  STATUS  TXDATA       │
│       RXDATA   CMD                  │
│  State: IDLE → START → SEND_ADDR   │
│         → TRANSFER → STOP          │
└──────────────┬──────────────────────┘
               │ start() / write() / read() / stop()
┌──────────────▼──────────────────────┐
│             I2CBus                  │
│  unordered_map<addr, device*>       │
│  routes transactions by address     │
└──────┬───────────────┬──────────────┘
       │               │
┌──────▼──────┐  ┌─────▼───────┐
│ TempSensor  │  │ TempSensor  │  ← any I2CSlaveDevice subclass
│  addr=0x48  │  │  addr=0x50  │
└─────────────┘  └─────────────┘
```

---

## Register Map

### I2C Controller (MMIO)

| Offset | Register | Access | Description                      |
|--------|----------|--------|----------------------------------|
| 0x00   | CONTROL  | R/W    | bit0=EN, bit[2:1]=SPEED          |
| 0x04   | STATUS   | RO     | bit0=BUSY, bit1=DONE, bit2=ERR   |
| 0x08   | TXDATA   | W      | [14:8]=ADDR [7:0]=DATA [15]=RW   |
| 0x0C   | RXDATA   | RO     | [7:0]=DATA [8]=VALID             |
| 0x10   | CMD      | W      | [3:0]=opcode (see below)         |

### CMD Opcodes

| Value | Name  | Description               |
|-------|-------|---------------------------|
| 0x0   | NOP   | No operation              |
| 0x1   | START | Begin transaction         |
| 0x2   | STOP  | End transaction           |
| 0x3   | WRITE | Send byte from TXDATA     |
| 0x4   | READ  | Receive byte into RXDATA  |

### TempSensor Registers

| Offset | Name         | Access | Description                   |
|--------|--------------|--------|-------------------------------|
| 0x00   | DEV_ID       | RO     | Device identifier (0x55)      |
| 0x01   | STATUS       | RO     | bit0=DATA_READY               |
| 0x02   | CURRENT_TEMP | RO     | Signed temperature in C       |
| 0x03   | DATA         | RW     | General purpose data register |

---

## Transaction Flow

### Write Transaction

```
1. mmio_write(CONTROL, 0x1)             enable controller
2. mmio_write(TXDATA,  addr<<8)         set slave address, RW=0
3. mmio_write(CMD,     START)           bus.start(addr, write)
4. mmio_write(TXDATA,  reg<<8 | data)   set register + data byte
5. mmio_write(CMD,     WRITE)           bus.write(reg, data)
6. mmio_write(CMD,     STOP)            bus.stop()
```

### Read Transaction

```
1. mmio_write(TXDATA,  addr<<8 | RW=1)  set slave address, RW=1
2. mmio_write(CMD,     START)           bus.start(addr, read)
3. mmio_write(TXDATA,  reg<<8)          set register to read from
4. mmio_write(CMD,     READ)            bus.read(reg, out)
5. mmio_read (RXDATA)                   get received byte
6. mmio_write(CMD,     STOP)            bus.stop()
```

---

## State Machine

```
      CMD=START             CMD=WRITE/READ          CMD=STOP
IDLE ──────────► START ──► SEND_ADDR ──────────► TRANSFER ──► IDLE
 ▲                              │                                │
 └──────────────────────────────┴────────────────────────────────┘
                          NACK / error → IDLE
```

---

## Build and Run

```bash
# first way
# Build - direct way
g++ -std=c++17 -Wall -Wextra \
    -o i2c_sim \
    main.cpp i2c_bus.cpp i2c_controller.cpp Ii2c_slavedevice.cpp
# Run
./i2c_sim


# second way
# use cmake 
mkdir build && cd build
cmake ..
cmake --build .
./bin/i2c_sim          # run directly
# or
ctest --output-on-failure   # run via CTest
```

> **Windows (MSYS2 / MinGW):** run `chcp 65001` first for correct UTF-8 output.

---

## Tests

| Test   | What it covers                                   | Expected result   |
|--------|--------------------------------------------------|-------------------|
| Test A | Write `0x42` to DATA reg of sensor @ `0x48`     | DONE=1, ERR=0     |
| Test B | Read CURRENT\_TEMP from sensor @ `0x48`         | DATA=37, VALID=1  |
| Test C | START to unknown address `0x77`                 | ERR=1, NACK=1     |
| Test D | Two sensors on bus, write to each independently | Isolated correctly|

---

## How to Add a New Slave Device

**Step 1** — Create a register map in a new header:

```cpp
// devices/Reg_pressure.hpp
struct PressureSensor_RegMap {
    uint8_t dev_id;     // 0x00  RO
    uint8_t status;     // 0x01  RO
    uint8_t pressure;   // 0x02  RO
};
namespace PressureReg {
    constexpr uint8_t DEV_ID    = offsetof(PressureSensor_RegMap, dev_id);
    constexpr uint8_t PRESSURE  = offsetof(PressureSensor_RegMap, pressure);
}
```

**Step 2** — Subclass `I2CSlaveDevice`:

```cpp
class PressureSensor : public I2CSlaveDevice {
protected:
    bool on_read (uint8_t reg, uint8_t& out) const override { ... }
    bool on_write(uint8_t reg, uint8_t val)  override       { ... }
};
```

**Step 3** — Attach to bus in `main.cpp`:

```cpp
PressureSensor ps(0x60);
bus.attach(&ps);
```

> No changes needed to `I2CBus`, `I2CController`, or any existing file.

---

## Design Decisions

| Decision | Reason |
|---|---|
| Transaction-level modeling | No electrical simulation required per spec |
| `enum class RegOffset` | Scoped, type-safe, compiler warns on missing case |
| `offsetof` for slave reg offsets | Struct is single source of truth — offsets never drift |
| Forward declaration of `I2CSlaveDevice` in bus header | Breaks circular include, reduces compile coupling |
| `inline find()` in `I2CBus` | One-liner on hot path — called on every transaction |
| Pure virtual `on_write` / `on_read` | Forces every subclass to handle its own registers explicitly |
| Non-copyable bus and controller | Shared resource — accidental copy would split state |
