#include <stdio.h>
#include "pico/stdlib.h"

#include "global_def.h"
#include "i2c_lcd.h"
#include "ds3231.h"
#include "hardware/i2c.h"

//sub function
void i2c_write_byte_lcd(uint8_t val) {
//#ifdef i2c_default
//    i2c_write_blocking(i2c_default, addr, &val, 1, false);
//    
//#endif
i2c_write_blocking(I2C_PORT, gPARAM_lcd_i2caddr, &val, 1, false);
}

void lcd_toggle_enable(uint8_t val) {
    // Toggle enable pin on LCD display
    // We cannot do this too quickly or things don't work
#define DELAY_US 600
    sleep_us(DELAY_US);
    i2c_write_byte_lcd(val | LCD_ENABLE_BIT);
    sleep_us(DELAY_US);
    i2c_write_byte_lcd(val & ~LCD_ENABLE_BIT);
    sleep_us(DELAY_US);
}

// The display is sent a byte as two separate nibble transfers
void lcd_send_byte(uint8_t val, int mode) {
    uint8_t high = mode | (val & 0xF0) | LCD_BACKLIGHT;
    uint8_t low = mode | ((val << 4) & 0xF0) | LCD_BACKLIGHT;

    i2c_write_byte_lcd(high);
    lcd_toggle_enable(high);
    i2c_write_byte_lcd(low);
    lcd_toggle_enable(low);
}

void lcd_clear(void) {
    lcd_send_byte(LCD_CLEARDISPLAY, LCD_COMMAND);
}

// go to location on LCD
void lcd_set_cursor(int line, int position) {
    //int val = (line == 0) ? 0x80 + position : 0xC0 + position;
    int val;
    if(line == 0){val=0x80+position;}
    else if(line == 1){val=0xc0+position;}
    else if(line == 2){val=0x94+position;}
    else if(line == 3){val=0xd4+position;}
    else              {val=0x80+position;}
    lcd_send_byte(val, LCD_COMMAND);
}

static inline void lcd_char(char val) {
    lcd_send_byte(val, LCD_CHARACTER);
}

void lcd_string(char *s) {
    while (*s!='\0') {
        lcd_char(*s++);
    }
}

void lcd_hex(uint8_t bchar) {
  uint8_t bNum[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
  uint8_t bHexh,bHexl;
  
  bHexh = (bchar>>4)&0x0f;
  lcd_char(*(bNum+bHexh));
  bHexl = (bchar>>0)&0x0f;
  lcd_char(*(bNum+bHexl));
}

void lcd_hex2dec(uint8_t hex) {
  uint8_t bNum[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
  uint8_t bHexh,bHexl;
  uint8_t dig10,dig1;
  
  conv_hex2dec_max100(hex,&dig10,&dig1);
  lcd_char(*(bNum+dig10));
  
  lcd_char(*(bNum+dig1));
}

void lcd_init(void) {
    lcd_send_byte(0x03, LCD_COMMAND);
    lcd_send_byte(0x03, LCD_COMMAND);
    lcd_send_byte(0x03, LCD_COMMAND);
    lcd_send_byte(0x02, LCD_COMMAND);

    lcd_send_byte(LCD_ENTRYMODESET | LCD_ENTRYLEFT, LCD_COMMAND);
    lcd_send_byte(LCD_FUNCTIONSET | LCD_2LINE, LCD_COMMAND);
    lcd_send_byte(LCD_DISPLAYCONTROL | LCD_DISPLAYON, LCD_COMMAND);
    lcd_clear();
}

void conv_hex2dec_max100(uint8_t hex,uint8_t *dig10,uint8_t *dig1)
{
  int i;
  uint8_t temp1;
  
  if(hex >99)
  {
    //error
    *(dig10+0) = 15;
    *(dig1+0)  = 15;
  }
  else if(hex >= 90) {temp1 = hex -90;*(dig10+0) = 9;}
  else if(hex >= 80) {temp1 = hex -80;*(dig10+0) = 8;}
  else if(hex >= 70) {temp1 = hex -70;*(dig10+0) = 7;}
  else if(hex >= 60) {temp1 = hex -60;*(dig10+0) = 6;}
  else if(hex >= 50) {temp1 = hex -50;*(dig10+0) = 5;}
  else if(hex >= 40) {temp1 = hex -40;*(dig10+0) = 4;}
  else if(hex >= 30) {temp1 = hex -30;*(dig10+0) = 3;}
  else if(hex >= 20) {temp1 = hex -20;*(dig10+0) = 2;}
  else if(hex >= 10) {temp1 = hex -10;*(dig10+0) = 1;}
  else              {temp1 = hex ;*(dig10+0) = 0;}
  *(dig1+0)  = temp1;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief               Library function to read a specific I2C register adress.
 * 
 * @param[in] i2c       I2C instance used.
 * @param[in] dev_addr  Adress of the I2C device.
 * @param[in] reg_addr  Register adress to be read.
 * @param[in] length    length of the data in bytes to be read.
 * @param[out] data     Buffer to store the read data.
 * @return              0 if succesful, -1 if i2c failure.    
 */
int i2c_read_reg_ds3231(uint8_t dev_addr, 
    uint8_t reg_addr, size_t length, uint8_t * data) 
{
    if(!length) 
        return -1;
    uint8_t reg = reg_addr; 
    if(i2c_write_blocking(I2C_PORT, dev_addr, &reg, 1, true) == PICO_ERROR_GENERIC) {
        return -1;
    }
    if(i2c_read_blocking(I2C_PORT, dev_addr, data, length, false) == PICO_ERROR_GENERIC) {
        return -1;
    }
    return 0;
}

/**
 * @brief               Library function to write to a specific I2C register adress.
 * 
 * @param[in] i2c       I2C instance used.
 * @param[in] dev_addr  Adress of the I2C device.
 * @param[in] reg_addr  Register adress to be written.
 * @param[in] length    Length of the data to be written in bytes.
 * @param[in] data      Pointer to the data buffer.
 * @return              0 if succesful, -1 if i2c failure.
 */
int i2c_write_reg_ds3231(uint8_t dev_addr, 
    uint8_t reg_addr, size_t length, uint8_t * data)
{
    if(!length) 
        return -1;
    uint8_t messeage[length + 1];
    messeage[0] = reg_addr;
    for(int i = 0; i < length; i++) {
        messeage[i + 1] = data[i];
    }
    if(i2c_write_blocking(I2C_PORT, dev_addr, messeage, (length + 1), false) == PICO_ERROR_GENERIC)
        return -1;
    return 0;
}

/**
 * @brief           Library function that takes an 8 bit unsigned integer and converts into
 *  Binary Coded Decimal number that can be written to DS3231 registers.
 * 
 * @param[in] data  Number to be converted.
 * @return          Number in BCD form.
 */
uint8_t bin_to_bcd(const uint8_t data) {
    uint8_t ones_digit = (uint8_t)(data % 10);
    uint8_t twos_digit = (uint8_t)(data - ones_digit) / 10;
    return ((twos_digit << 4) + ones_digit);
}

/**
 * @brief           Library function that takes an 8 bit unsigned integer and converts it into
 * Binary Coded Decimal number where bit 5 and 6 represent the AM/PM characteristics.
 * 
 * @param[in] data  Number to converted.
 * @return          Number in BCD form with AM/PM bits.
 */
uint8_t bin_to_bcd_am_pm(uint8_t data) {
    uint8_t temp = bin_to_bcd(data);
    uint8_t am_pm = 0x00;
    if(data > 12) {
        am_pm = 0x01;
        data -= 12;
    }
    temp |= (am_pm << 5);
    return temp;
}

/**
 * @brief                   Initiliaze ds3231 struct and specify which I2C instance is going to be used.
 * 
 * @param[out] rtc          Pointer to the ds3231 struct.
 * @param[in] i2c           Chosen I2C instance.
 * @param[in] dev_addr      DS3231 device adress. Leave 0 for default adress.
 * @param[in] eeprom_addr   EEPROM device adress. Leave 0 for default adress. IF an incorrect adress is entered, it is set to the default adress.
 * @return                  0 if succesful. 
 */
int ds3231_init(ds3231_t * rtc, i2c_inst_t * i2c, uint8_t dev_addr, uint8_t eeprom_addr) {
    rtc->am_pm_mode = false;
    rtc->i2c = I2C_PORT;
    if(dev_addr)
        rtc->ds3231_addr = dev_addr;
    else 
        rtc->ds3231_addr = DS3231_DEVICE_ADRESS;

    switch (eeprom_addr)
    {
    case AT24C32_EEPROM_ADRESS_0:
        rtc->at24c32_addr = AT24C32_EEPROM_ADRESS_0;
        break;
    case AT24C32_EEPROM_ADRESS_1:
        rtc->at24c32_addr = AT24C32_EEPROM_ADRESS_1;    
        break;
    case AT24C32_EEPROM_ADRESS_2:
        rtc->at24c32_addr = AT24C32_EEPROM_ADRESS_2;
        break;
    case AT24C32_EEPROM_ADRESS_3:
        rtc->at24c32_addr = AT24C32_EEPROM_ADRESS_3;
        break;
    case AT24C32_EEPROM_ADRESS_4:
        rtc->at24c32_addr = AT24C32_EEPROM_ADRESS_4;
        break;
    case AT24C32_EEPROM_ADRESS_5:
        rtc->at24c32_addr = AT24C32_EEPROM_ADRESS_5;
        break;
    case AT24C32_EEPROM_ADRESS_6:
        rtc->at24c32_addr = AT24C32_EEPROM_ADRESS_6;
        break;
    case AT24C32_EEPROM_ADRESS_7:
        rtc->at24c32_addr = AT24C32_EEPROM_ADRESS_7;
        break;
    default:
        rtc->at24c32_addr = AT24C32_EEPROM_ADRESS_0;
        break;
    }
    return 0;
}

/**
 * @brief               Configure the current time in DS3231.
 * 
 * @param[in] rtc       DS3231 struct.
 * @param[in] data      Data struct that holds the time that will be set in DS3231.
 * @return              0 if succesful.
 */
int ds3231_configure_time(ds3231_t * rtc, ds3231_data_t * data) {
    uint8_t temp[7] = {0, 0, 0, 0, 0, 0, 0};
    if(i2c_read_reg_ds3231(rtc->ds3231_addr, DS3231_SECONDS_REG, 7, temp)) 
        return -1;
    
    /* Checking if time values are within correct ranges. */
    if(data->seconds > 59) 
        data->seconds = 59;

    if(data->minutes > 59) 
        data->minutes = 59;

    if(rtc->am_pm_mode) {
        if(data->hours > 12)
            data->hours = 12;
        else if(data->hours < 1)
            data->hours = 1;
    } else {
        if(data->hours > 23) 
            data->hours = 23;
    }

    if(data->day > 7) 
        data->day = 7;
    else if(data->day < 1)
        data->day = 1;
    
    if(data->date > 31) 
        data->date = 31;
    else if(data->date < 1)
        data->date = 1;

    if(data->month > 12) 
        data->month = 12;
    else if(data->month < 1)
        data->month = 1;
    
    if(data->year > 99) 
        data->year = 99;
        

    temp[0] = bin_to_bcd(data->seconds);

    temp[1] = bin_to_bcd(data->minutes);

    if(rtc->am_pm_mode) {
        temp[2] = bin_to_bcd_am_pm(data->hours);
        temp[2] |= (0x01 << 6);
    } else {
        temp[2] = bin_to_bcd(data->hours);
        temp[2] &= ~(0x01 << 6);
    }

    temp[3] = bin_to_bcd(data->day);

    temp[4] = bin_to_bcd(data->date);

    temp[5] = bin_to_bcd(data->month);

    if(data->century)
        temp[5] |= (0x01 << 7);
    
    temp[6] = bin_to_bcd(data->year);
    
    if(i2c_write_reg_ds3231( rtc->ds3231_addr, DS3231_SECONDS_REG, 7, temp)) 
        return -1;
    return 0;
}

/**
 * @brief               Enable or disable AM/PM mode of DS3231. By default, it is disabled
 * 
 * @param[in] rtc       DS3231 struct.           
 * @param[in] enable    If true, AM_PM mode is enabled. If false, 24-Hour mode is enabled.
 * @return              0 if succeful.
 */
int ds3231_enable_am_pm_mode(ds3231_t * rtc, bool enable) {
    uint8_t temp = 0;
    if(i2c_read_reg_ds3231(rtc->ds3231_addr, DS3231_HOURS_REG, 1, &temp))
        return -1;
    if(enable) {
        temp |= (0x01 << 6);
        rtc->am_pm_mode = true;
    } else {
        rtc->am_pm_mode = false;
        temp &= ~(0x01 << 6);
    }
    if(i2c_write_reg_ds3231( rtc->ds3231_addr, DS3231_HOURS_REG, 1, &temp))
        return -1;
    return 0;
}

/**
 * @brief               Reads the timekeeping registers and converts time to real units.
 * 
 * @param[in]   rtc     ds3231 struct.    
 * @param[out]  data    data struct to save converted time units.
 * @return              0 if succesful. 
 */
int ds3231_read_current_time(ds3231_t * rtc, ds3231_data_t * data) {
    uint8_t raw_data[7];
    if(i2c_read_reg_ds3231(rtc->ds3231_addr, DS3231_SECONDS_REG, 7, raw_data)) 
        return -1;
    
    data->seconds = 10 * ((raw_data[0] & 0x70) >> 4) + (raw_data[0] & 0x0F);

    data->minutes = 10 * ((raw_data[1] & 0x70) >> 4) + (raw_data[1] & 0x0F);
    
    if(rtc->am_pm_mode) {
        data->hours   = 10 * ((raw_data[2] & 0x10) >> 4) + (raw_data[2] & 0x0F);
        data->am_pm = ((raw_data[2] & 0x20) >> 5);
    } else {
        data->hours   = 10 * ((raw_data[2] & 0x30) >> 4) + (raw_data[2] & 0x0F);
    }

    data->day     = (raw_data[3] & (0x07));

    data->date    = 10 * ((raw_data[4] & 0x30) >> 4) + (raw_data[4] & 0x0F);

    data->month   = 10 * ((raw_data[5] & 0x10) >> 4) + (raw_data[5] & 0x0F);

    data->century = (raw_data[5] & (0x01 << 7)) >> 7;

    data->year    = 10 * ((raw_data[6] & 0xF0) >> 4) + (raw_data[6] & 0x0F);

    return 0;
}
