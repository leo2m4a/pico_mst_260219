#ifndef AT24C32_H
#define AT24C32_H

#include "hardware/i2c.h"
#include <stdint.h>
#include <stddef.h>

#define AT24C32_PAGE_SIZE   32
#define AT24C32_ADDR        0x57    // default (A0=A1=A2=0)
#define SCH_RLY_SIZE        (24 * 4)
#define SCH_RLY_EEPROM_ADDR 0x0000  // starting word address in EEPROM

int at24c32_write_page(i2c_inst_t *i2c, uint8_t dev_addr, uint16_t word_addr, const uint8_t *data, size_t len);
int at24c32_read(i2c_inst_t *i2c, uint8_t dev_addr, uint16_t word_addr, uint8_t *data, size_t len);
int at24c32_save_sch_rly(i2c_inst_t *i2c, uint8_t dev_addr, const uint8_t *sch_rly);
int at24c32_load_sch_rly(i2c_inst_t *i2c, uint8_t dev_addr, uint8_t *sch_rly);

#endif
