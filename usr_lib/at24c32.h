#ifndef AT24C32_H
#define AT24C32_H

#include "hardware/i2c.h"
#include <stdint.h>
#include <stddef.h>

#define AT24C32_PAGE_SIZE   32
#define AT24C32_ADDR        0x57    // default (A0=A1=A2=0)
#define SCH_RLY_SIZE        (24 * 4)
#define SCH_RLY_EEPROM_ADDR 0x0080  // starting word address in EEPROM
#define SYSINFO_EEPROM_ADDR 0x0000  // starting word address in EEPROM

#define SCH_RLY_PWD_H 0x16
#define SCH_RLY_PWD_L 0x88
// Per-relay schedule record used by load_sch_from_eep(): one 32-byte page per relay
//   +0x00..0x0F  schedule bytes 0..15
//   +0x10..0x17  schedule bytes 16..23
//   +0x18, 0x19  signature SCH_RLY_PWD_H, SCH_RLY_PWD_L
// Relay n record starts at SCH_RLY0_EEPROM_BASE + SCH_RLY_EEPROM_STRIDE*n
// (relays 0..3 -> 0x0100..0x017F, clear of SYSINFO and SCH_RLY_EEPROM_ADDR)
#define SCH_RLY0_EEPROM_BASE   0x0100
#define SCH_RLY_EEPROM_STRIDE  AT24C32_PAGE_SIZE
#define SCH_RLY0_EEPROM_ADDRL  (SCH_RLY0_EEPROM_BASE)
#define SCH_RLY0_EEPROM_ADDRH  (SCH_RLY0_EEPROM_BASE + 0x10)

#ifdef __cplusplus
extern "C" {
#endif

int at24c32_write_page(i2c_inst_t *i2c, uint8_t dev_addr, uint16_t word_addr, const uint8_t *data, size_t len);
int at24c32_read(i2c_inst_t *i2c, uint8_t dev_addr, uint16_t word_addr, uint8_t *data, size_t len);
int at24c32_save_sch_rly(i2c_inst_t *i2c, uint8_t dev_addr, const uint8_t *sch_rly);
int at24c32_load_sch_rly(i2c_inst_t *i2c, uint8_t dev_addr, uint8_t *sch_rly);

#ifdef __cplusplus
}
#endif

#endif
