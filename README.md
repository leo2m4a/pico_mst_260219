# pico_mst_260219

Raspberry Pi Pico (RP2040) firmware for a LoRa master node. The master periodically broadcasts beacons, polls sensor data from slave nodes, and displays status on an I2C LCD. UART is used for external communication and debugging.

> **This project is part of a master/slave pair and must be used together with the slave node project [pico_rx260101](https://github.com/leo2m4a/pico_rx260101).**

## Target Board

This project is designed for the **宅老大 Pico LoRa** development board, which integrates a Raspberry Pi Pico (RP2040) with an SX1262 LoRa module.

- [Purchase](https://www.makdev.net/p/store.html)
- [Pinout](https://www.makdev.net/p/pico-lora-mesh-solar-shield-pinout.html)

## Features

- LoRa RF communication via SX1262 (915 MHz, SF7, BW 125 kHz)
- Beacon broadcast and slave node polling
- Real-time clock via DS3231 (I2C)
- I2C LCD display
- UART communication (115200 baud)
- AT24C32 EEPROM (I2C, address `0x57`): relay schedules loaded at startup, 8-byte read/write over UART
- Dual button input (UP / DOWN)
- Manages slave nodes SID `0x81` and `0x83`

## Hardware

| Peripheral | Interface | Pins |
|---|---|---|
| SX1262 LoRa | SPI1 | MISO=12, MOSI=11, SCK=10, CS=3 |
| SX1262 control | GPIO | DIO1=20, RST=15, BUSY=2 |
| I2C LCD + DS3231 | I2C1 | SDA=6, SCL=7 |
| UART1 | UART | TX=8, RX=9 |
| Button UP | GPIO | GPIO18 |
| Button DOWN | GPIO | GPIO19 |

## Dependencies

### Pico SDK

This project requires **Pico SDK 2.2.0**. Follow the [official setup guide](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf) to install the SDK.

### RadioLib (Required — must be cloned manually)

This project uses [RadioLib](https://github.com/jgromes/RadioLib) by Jan Gromeš, licensed under the MIT License.

RadioLib is **not included** in this repository. Clone it into the project root before building:

```bash
git clone https://github.com/jgromes/RadioLib.git
```

The directory structure should look like this:

```
pico_mst_260219/
├── RadioLib/          <-- cloned here
├── usr_lib/
├── pico_mst_260219.cpp
├── CMakeLists.txt
└── ...
```

## Build

```bash
mkdir build
cd build
cmake ..
make
```

Flash the resulting `.uf2` file to the Pico by holding BOOTSEL and dragging the file onto the USB drive.

## RF Protocol

The device operates as a master node (MID `0x02`). It sends 12-byte beacon and poll packets to slave nodes and processes their responses. Command processing is handled in `mst_rf_cmd_proc.cpp`. UART command handling is in `mst_ur_proc.cpp`.

The full UART/RF packet format is documented in [`docs/pico_lora_cmd2604091.ods`](docs/pico_lora_cmd2604091.ods) (LibreOffice Calc spreadsheet).

### UART EEPROM commands (`0x83`)

Read or write 8 bytes of the AT24C32 at any word address. Byte numbers are offsets in the packet.

| Byte | Write request | Read request |
|---|---|---|
| 0 | `0x83` | `0x83` |
| 1 | `0x10` (length) | `0x08` (length) |
| 4 | `0x11` (write) | `0x35` (read) |
| 5–6 | Word address (high, low) | Word address (high, low) |
| 7–14 | 8 data bytes | — |

Replies are 16 bytes, starting `0x92`, `0x10`, MID `0x02`, `0x00`:

| Byte | Write reply | Read reply |
|---|---|---|
| 4 | `0x11` | `0x35` |
| 5 | Address high | `0x00` |
| 6 | Address low | data0 |
| 7–13 | data0–data6 | data1–data7 |
| 14 | data7 | `0x00` |
| 15 | `0x00` | `0x00` |

### EEPROM layout

| Address | Contents |
|---|---|
| `0x0000` | Reserved for system info |
| `0x0080` | Relay schedules, 96 bytes (`24 × 4`), read into `gSCH_RLY` at startup |

At startup the master loads the schedule from `0x0080` with no validity check.

## License

Project source code is provided as-is. RadioLib is copyright Jan Gromeš and contributors, used under the [MIT License](https://github.com/jgromes/RadioLib/blob/master/LICENSE).
