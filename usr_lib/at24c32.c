#include "at24c32.h"
#include "pico/stdlib.h"

#define WRITE_DELAY_MS 10

// Write up to one page (32 bytes). word_addr must be page-aligned.
int at24c32_write_page(i2c_inst_t *i2c, uint8_t dev_addr, uint16_t word_addr, const uint8_t *data, size_t len) {
    uint8_t buf[2 + AT24C32_PAGE_SIZE];
    buf[0] = (word_addr >> 8) & 0xFF;
    buf[1] = word_addr & 0xFF;
    for (size_t i = 0; i < len; i++) buf[2 + i] = data[i];
    if (i2c_write_blocking(i2c, dev_addr, buf, 2 + len, false) < 0) return -1;
    sleep_ms(WRITE_DELAY_MS);
    return 0;
}

// Sequential read from word_addr.
int at24c32_read(i2c_inst_t *i2c, uint8_t dev_addr, uint16_t word_addr, uint8_t *data, size_t len) {
    uint8_t addr[2] = { (word_addr >> 8) & 0xFF, word_addr & 0xFF };
    if (i2c_write_blocking(i2c, dev_addr, addr, 2, true) < 0) return -1;
    if (i2c_read_blocking(i2c, dev_addr, data, len, false) < 0) return -1;
    return 0;
}

// Save gSCH_RLY[96] to EEPROM starting at SCH_RLY_EEPROM_ADDR (3 pages).
int at24c32_save_sch_rly(i2c_inst_t *i2c, uint8_t dev_addr, const uint8_t *sch_rly) {
    for (int page = 0; page < (SCH_RLY_SIZE / AT24C32_PAGE_SIZE); page++) {
        uint16_t addr = SCH_RLY_EEPROM_ADDR + page * AT24C32_PAGE_SIZE;
        if (at24c32_write_page(i2c, dev_addr, addr, sch_rly + page * AT24C32_PAGE_SIZE, AT24C32_PAGE_SIZE) < 0)
            return -1;
    }
    return 0;
}

// Load gSCH_RLY[96] from EEPROM.
int at24c32_load_sch_rly(i2c_inst_t *i2c, uint8_t dev_addr, uint8_t *sch_rly) {
    return at24c32_read(i2c, dev_addr, SCH_RLY_EEPROM_ADDR, sch_rly, SCH_RLY_SIZE);
}
