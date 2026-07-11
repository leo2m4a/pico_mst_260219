#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/i2c.h"
#include "hardware/timer.h"
#include "hardware/uart.h"
#include "PicoHal.h"
#include <RadioLib.h>
#include <usr_lib/i2c_lcd.h>
#include <usr_lib/ds3231.h>
#include "global_def.h"
#include "main.h"
#include "mst_ur_proc.h"
#include "mst_rf_cmd_proc.h"

void update_ds(unsigned char);
void read_ds(void);
const char * days[7] = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};

uint8_t rf_state = 0;
// 全域變數宣告
PicoHal* hal = nullptr;
Module* module = nullptr;
SX1262* radio = nullptr;

// 中斷標誌
volatile bool receivedFlag = false;

TimeData Stru_curTime;

BcnData bcn;
dec5Dig TS_val;
uint8_t u8_temp16[16];
//slv_data rcv_slv_data[8];

    /* Set your current time. */
    ds3231_t ds3231;
    ds3231_data_t ds3231_data = {
        .seconds = 0,
        .minutes = 13,
        .hours = 21,
        .am_pm = false,
        .day = 7,
        .date = 15,
        .month = 2,
        .century = 1,
        .year = 26
    };
//sub function
uint8_t sub_timeadv(TimeData * time)
{
    uint8_t ret = 0;

    if(time->sec == 59)
    {
        time->sec = 0;
        ret |= 0x1;
        if(time->min == 59)
        {
            time->min = 0;
            ret |= 0x2;
            if(time->hour == 23)
            {
                time->hour = 0;
                ret |= 0x4;
            }
        }
    }
    return ret;
}

void param_init(void)
{
    int i;
    gPARAM_fcnt_100ms = 0;
    gPARAM_fcnt_1s = 0;
    gPARAM_actflag = 0;
    for(i=0;i<32;i++)
    {
      gPARAM_UR1TX_buf[i]=0;
      gPARAM_UR1RX_buf[i]=0;
      gPARAM_RFTX_buf[i]=0;
      gPARAM_RFRX_buf[i]=0;
    }
    Stru_curTime.hour = 0;
    Stru_curTime.min = 0;
    Stru_curTime.sec = 0;
    Stru_curTime.caldone=0;
   // gPARAM_APOLL_SID[8] ;
    gPARAM_APOLL_en = 0;
    gPARAM_APOLL_cnt=0;
    gPARAM_APOLL_cnt_lmt = 3;
}

void system_init(void)
{
    // Add your system initialisation code here

}
uint8_t conv4byte_4decdig(uint16_t inval,uint8_t *outval)
{
  int i;
  int stop;
  uint16_t u16temp;
  uint8_t rlt_dig[4];
  
  u16temp = inval;
  stop = 0;
  if(u16temp>=1000)
  {
    i=0;
    while(stop == 0)
    {
      
      u16temp-=1000;
      i++;
      if(u16temp<1000)
      {stop = 1;}
    }
    rlt_dig[3] = i;
  }
  else{
    rlt_dig[3] = 0;
  }
  
  stop = 0;
  if(u16temp>=100)
  {
    i=0;
    while(stop == 0)
    {
      
      u16temp-=100;
      i++;
      if(u16temp<100)
      {stop = 1;}
    }
    rlt_dig[2] = i;
  }
  else{
    rlt_dig[2] = 0;
  }  
  
  stop = 0;
  if(u16temp>=10)
  {
    i=0;
    while(stop == 0)
    {
      
      u16temp-=10;
      i++;
      if(u16temp<10)
      {stop = 1;}
    }
    rlt_dig[1] = i;
  }
  else{
    rlt_dig[1] = 0;
  }  
rlt_dig[0] = u16temp;
*(outval+0) = rlt_dig[0];
*(outval+1) = rlt_dig[1];
*(outval+2) = rlt_dig[2];
*(outval+3) = rlt_dig[3];

}
uint8_t conv1byte_3dig(unsigned char srchex_l,dec3Dig *dig)
{
  uint8_t val;
  uint8_t dig100;
  val = srchex_l;
  if(val >= 200)
  {
    val-=200;dig->dig100 = 2;
  }
  else if(val >= 100)
  {
    val-=100;dig->dig100 = 1;
  }
  else
  {
    dig->dig100 = 0;
  }
  
  if(val >= 90)
  {
    val-=90;dig->dig10 = 9;
  }
  else if(val >= 80)
  {
    val-=80;dig->dig10 = 8;
  }
  else if(val >= 70)
  {
    val-=70;dig->dig10 = 7;
  }
  else if(val >= 60)
  {
    val-=60;dig->dig10 = 6;
  }
    else if(val >= 50)
  {
    val-=50;dig->dig10 = 5;
  }
  else if(val >= 40)
  {
    val-=40;dig->dig10 = 4;
  }
  else if(val >= 30)
  {
    val-=30;dig->dig10 = 3;
  }
  else if(val >= 20)
  {
    val-=20;dig->dig10 = 2;
  }
  else if(val >= 10)
  {
    val-=10;dig->dig10 = 1;
  }
  else
  {
    dig->dig10 = 0;
  }
  dig->dig1 = val;
  return 0;
}

uint8_t conv2byte_5dig(unsigned char srchex_h,unsigned char srchex_l,dec5Dig *dig5)
{
  dec3Dig dig3,dig3a;
  unsigned char uctemp1;
  
  conv1byte_3dig(srchex_l,&dig3);
  uctemp1 = srchex_h + dig3.dig100;
  conv1byte_3dig(uctemp1,&dig3a);
  dig5->dig100 = dig3a.dig100;
  dig5->dig10 = dig3a.dig10;
  dig5->dig1 = dig3a.dig1;
  dig5->dot1 = dig3.dig10;
  dig5->dot10 = dig3.dig1;
  return 0;
  
}
// 中斷服務程式
void irqRadio(uint gpio, uint32_t events) {
    // 檢查是否為 DIO1 引腳中斷
    if (gpio == RADIO_DIO1_PIN) {
        if((rf_state == 0))
        {
        receivedFlag = true;
        //rf_state = 2;
        }
        else
        {rf_state = 0;radio->startReceive(); }
    }
}

int64_t alarm_callback(alarm_id_t id, void *user_data) {
    // Put your timeout handler code in here
    return 0;
}

bool repeating_timer_callback(__unused struct repeating_timer *t) {
//    printf("Repeat at %lld\n", time_us_64());

    if(gPARAM_fcnt_100ms >= 9)
    {
      gPARAM_fcnt_100ms=0;
      
      if(gPARAM_fcnt_1s >= 59)
      {
        gPARAM_fcnt_1s = 0;
        sub_timeadv(&Stru_curTime);
      }
      else
      {gPARAM_fcnt_1s++;}
    
      gPARAM_actflag |= 0x1;  
      if((gPARAM_fcnt_1s == 0)||(gPARAM_fcnt_1s == 16)||(gPARAM_fcnt_1s == 32))
      {
        gPARAM_actflag |= 0x10;//bcn
      }
      else if(gPARAM_fcnt_1s % 4 == 2)
      {
        gPARAM_actflag |= 0x40;//display
      }
      else if(gPARAM_fcnt_1s % 2 == 1)
      {
        if(gPARAM_APOLL_en == 1)
          {
            if(gPARAM_APOLL_cnt >= gPARAM_APOLL_cnt_lmt)
            {gPARAM_APOLL_cnt=0;}
            else
            {gPARAM_APOLL_cnt++;}
             gPARAM_actflag |= 0x80;//auto pool??
          }
      }
    }  
    else
    {gPARAM_fcnt_100ms++;}

    return true;
}
// RX interrupt handler
void on_uart_rx() {
  unsigned char uctemp;

    while (uart_is_readable(UART_ID)) {
      uctemp = uart_getc(UART_ID);
      if(gPARAM_UR1RX_buf_ptr == 0)
      {
        gPARAM_UR1RX_buf[gPARAM_UR1RX_buf_ptr] = uctemp;
        gPARAM_UR1RX_buf_ptr++;
      }
      else if(gPARAM_UR1RX_buf_ptr == 1)
      {
        gPARAM_UR1RX_buf[gPARAM_UR1RX_buf_ptr] = uctemp;
        gPARAM_UR1RX_buf_leng = uctemp;
        if((gPARAM_UR1RX_buf_leng >=32)|(gPARAM_UR1RX_buf_leng ==0))
        {gPARAM_UR1RX_buf_ptr = 0;gPARAM_UR1RX_buf_leng = 32;}
        else
        {gPARAM_UR1RX_buf_ptr++;}

      }
      else if(gPARAM_UR1RX_buf_ptr >= (gPARAM_UR1RX_buf_leng-1))
      {
         gPARAM_UR1RX_buf[gPARAM_UR1RX_buf_ptr] = uctemp;
         gPARAM_UR1RX_buf_cnt = gPARAM_UR1RX_buf_ptr;
        gPARAM_UR1RX_buf_ptr = 0;
        gPARAM_actflag |= 0x02;
      }
      else
      {
        gPARAM_UR1RX_buf[gPARAM_UR1RX_buf_ptr] = uctemp;
        gPARAM_UR1RX_buf_ptr++;
      }
    }
}

int main()
{
    struct repeating_timer timer;
    const uint LED_PIN = PICO_DEFAULT_LED_PIN;
    int state;
    PollData poll_slv;
    uint8_t ret;

    stdio_init_all();

    // SPI initialisation. This example will use SPI at 1MHz.
    spi_init(SPI_PORT, 1000*1000);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_CS,   GPIO_FUNC_SIO);
    gpio_set_function(PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    
    // Chip select is active-low, so we'll initialise it to a driven-high state
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);
    // 設定 GPIO 中斷 for radio
    gpio_set_irq_enabled_with_callback(RADIO_DIO1_PIN, GPIO_IRQ_EDGE_RISE, true, &irqRadio);
    // Create a new instance of the Pico HAL
    // SPI0, MISO 16, MOSI 19, SCK 18
    hal = new PicoHal(spi1, PIN_MISO, PIN_MOSI, PIN_SCK, 1000 * 1000);
    // Create a new instance of the radio module
    // NSS 17, DIO1 15, RST 21, BUSY 6
    module = new Module(hal, RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN,RADIO_BUSY_PIN);
    // Create a new instance of the SX1262 radio
    radio = new SX1262(module);
    state = radio->begin(915.0,125.0,7,5,0x12,20);
    if (state != RADIOLIB_ERR_NONE) {
        //printf("Failed to initialize radio, code %d\n", state);
        while (true);
    }   
    state = radio->startReceive();    
    // For more examples of SPI use see https://github.com/raspberrypi/pico-examples/tree/master/spi

    // I2C Initialisation. Using it at 400Khz.
    i2c_init(I2C_PORT, 400*1000);
    
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    // For more examples of I2C use see https://github.com/raspberrypi/pico-examples/tree/master/i2c

    // Timer example code - This example fires off the callback after 2000ms
    //add_alarm_in_ms(2000, alarm_callback, NULL, false);
    add_repeating_timer_ms(100, repeating_timer_callback, NULL, &timer);
    // For more examples of timer use see https://github.com/raspberrypi/pico-examples/tree/master/timer

    // Set up our UART
    uart_init(UART_ID, BAUD_RATE);
    // Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    
    // Use some the various UART functions to send out data
    // In a default system, printf will also output via the default UART
    // Select correct interrupt for the UART we are using
    int UART_IRQ = UART_ID == uart0 ? UART0_IRQ : UART1_IRQ;

    // And set up and enable the interrupt handlers
    irq_set_exclusive_handler(UART_IRQ, on_uart_rx);
    irq_set_enabled(UART_IRQ, true);

    // Now enable the UART to send interrupts - RX only
    uart_set_irq_enables(UART_ID, true, false);    
    // Send out a string, with CR/LF conversions
    uart_puts(UART_ID, " Hello, UART!\n");
    
    // For more examples of UART use see https://github.com/raspberrypi/pico-examples/tree/master/uart

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);

    //param initial
    param_init();
    
    lcd_init();
    lcd_set_cursor(0,3);
    lcd_string("m002");
    //lcd_set_cursor(1,0);lcd_string("X");
    //lcd_set_cursor(2,0);lcd_string("Y");
    //lcd_set_cursor(3,0);lcd_string("Z");

    //DS
    /* Initiliaze ds3231 struct. */
    ds3231_init(&ds3231, i2c_default, DS3231_DEVICE_ADRESS, AT24C32_EEPROM_ADRESS_0);
    sleep_ms(200);
    
    //ds3231_configure_time(&ds3231, &ds3231_data);
    //ds3231_data.hours = 0;
     /* Read the time registers of DS3231. */
        if(ds3231_read_current_time(&ds3231, &ds3231_data) == 0) {
            //printf("No data is received\n");
    lcd_set_cursor(0,8);
    lcd_hex2dec((uint8_t)((ds3231_data.month)&0xff));
    lcd_hex2dec((uint8_t)((ds3231_data.date)&0xff));
    lcd_hex2dec((uint8_t)((ds3231_data.hours)&0xff));
    lcd_hex2dec((uint8_t)((ds3231_data.minutes)&0xff));
    lcd_string("DS");
    //update_time
    Stru_curTime.sec=ds3231_data.seconds;
    Stru_curTime.min=ds3231_data.minutes;
    Stru_curTime.hour=ds3231_data.hours;
    
        } else {
    lcd_set_cursor(0,12);
    lcd_string("E");
        }

    while (true) {
      if (receivedFlag) {
            // 關閉中斷，避免處理過程中被打斷
            gpio_set_irq_enabled(RADIO_DIO1_PIN, GPIO_IRQ_EDGE_RISE, false);
            
            // 清除標誌
            receivedFlag = false;
            state = radio->readData(&gPARAM_RFRX_buf[0], 16);
            memcpy(&gPARAM_RFRX_buf_lat[0],&gPARAM_RFRX_buf[0],16);
            if (state == RADIOLIB_ERR_NONE) {
              //rf cmd process
              ret = mst_rf_cmd_process(&gPARAM_RFRX_buf_lat[0],&gPARAM_UR1TX_buf[0],&gPARAM_UR1TX_buf_leng,&gDisp_BUF_2[0]);
              if((ret & 0x1) == 0x1)
              {gPARAM_actflag |= 0x4;}//uart
                 //debug
                 //lcd_set_cursor(3,8);
                 //lcd_hex(ret);
                 //lcd_hex(gPARAM_UR1TX_buf_leng);
                 //lcd_hex(gPARAM_RFRX_buf[0]);
                 //lcd_hex(gPARAM_RFRX_buf[4]);

                 //printf("Frequency Error: %.2f Hz\n", radio.getFrequencyError());
                //printf("Packet Length: %d bytes\n", radio.getPacketLength());
            } else {
                //printf("Failed to read packet, code %d\n", state);
            }
                //lcd_set_cursor(1,8);
                //lcd_hex(gPARAM_fcnt_1s);
            state = radio->startReceive();            
            // 重新啟用中斷
            gpio_set_irq_enabled(RADIO_DIO1_PIN, GPIO_IRQ_EDGE_RISE, true);
            rf_state = 0;
      }
      
      if((gPARAM_actflag & 0x1) == 0x1)
      {
        gPARAM_actflag &= 0xfffffffe;
        //toggle led
        if((gPARAM_fcnt_1s & 0x1) == 0x1){ gpio_put(LED_PIN, 1); }
        else {gpio_put(LED_PIN, 0);}
        //
        lcd_set_cursor(0,0);
        lcd_hex2dec((uint8_t)(gPARAM_fcnt_1s&0xff));
        if(( gPARAM_fcnt_1s == 5))
        {
          read_ds();
        }
      }
      if((gPARAM_actflag & 0x2) == 0x2)//urrx
      {
        gPARAM_actflag &= ~0x02;
        //process urrx
        #if 0
        lcd_set_cursor(2,0);
        lcd_hex((uint8_t)(gPARAM_UR1RX_buf[0]&0xff));
        lcd_hex((uint8_t)(gPARAM_UR1RX_buf[1]&0xff));
        lcd_hex((uint8_t)(gPARAM_UR1RX_buf[3]&0xff));
        #endif
        #if 1
          ret = mst_ur_cmd_process(&gPARAM_UR1RX_buf[0],&gPARAM_UR1TX_buf[0],&gPARAM_UR1TX_buf_leng,&gPARAM_RFTX_buf[0],&gPARAM_RFTX_buf_leng);
          
          #if 0
          lcd_set_cursor(1,12);
          lcd_hex((uint8_t)(ret&0xff));
          lcd_hex((uint8_t)(gPARAM_UR1TX_buf_leng&0xff));
          #endif
          if((ret & 0x1) == 0x1)
          {
            ret &= ~0x1;
            //send urrsp
            
            gPARAM_actflag |= 0x4;
          }
           if((ret & 0x2) == 0x2)
          {
            ret &= ~0x2;
            //send rf,merge to 0x20 
            gPARAM_actflag |= 0x20;
            //uart_write_blocking(UART_ID, gPARAM_UR1TX_buf, gPARAM_UR1TX_buf_leng);
          }
          if((ret & 0x4) == 0x4)//actions
          {
            ret &= ~0x4;
            if(gPARAM_ACT_DATA[0] == 0)
            {
              update_ds(0);
            }
            else if(gPARAM_ACT_DATA[0] == 1)
            {
              update_ds(1);
            }
          }
        #endif
      }
      else if((gPARAM_actflag & 0x4) == 0x4)
      {
        gPARAM_actflag &= ~0x4;
        uart_write_blocking(UART_ID, gPARAM_UR1TX_buf, gPARAM_UR1TX_buf_leng);
      }
      else if((gPARAM_actflag & 0x10) == 0x10)
      {
        gPARAM_actflag &= ~0x10;
        //compose bcn
        mst_compose_rf_bcn(&u8_temp16[0],Stru_curTime.sec,Stru_curTime.min,Stru_curTime.hour);
        //copy to txbuf
        memcpy(&gPARAM_RFTX_buf[0],&u8_temp16[0],12);gPARAM_RFTX_buf_leng = 12;
        gPARAM_actflag |= 0x20;//send
        #if 0
        bcn.hdr = 0x55;
        bcn.length = 0x8;
        bcn.mid = 0x01;
        bcn.sid = 0x81;
        bcn.sec = Stru_curTime.sec;
        bcn.min = Stru_curTime.min;
        bcn.hour = Stru_curTime.hour;
        bcn.fcnt = gPARAM_fcnt_1s;
        
        rf_state = 1;
        //state = radio->transmit((uint8_t*)&bcn, sizeof(BcnData));
        state = radio->transmit((uint8_t *)&u8_temp16[0], sizeof(BcnData));
          
          if (state == RADIOLIB_ERR_NONE) {
              printf("success!\n");
          } else {
              printf("failed, code %d\n", state);
          }          
          //state = radio->startReceive(); 
        lcd_set_cursor(1,0);
        lcd_hex((uint8_t)(gPARAM_fcnt_1s&0xff));
        //lcd_hex((u8_temp16[0]&0xff));
        //lcd_hex((u8_temp16[4]&0xff));
        //lcd_hex((u8_temp16[7]&0xff));
        //lcd_hex((u8_temp16[5]&0xff));
        #endif
      }
      else if((gPARAM_actflag & 0x20) == 0x20)
      {
        gPARAM_actflag &= ~0x20;
        //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<RF SEND
        //lcd_set_cursor(1,8);
        //lcd_hex(gPARAM_RFTX_buf[0]);lcd_hex(gPARAM_RFTX_buf_leng);
        rf_state = 1;  //must before transmit
        //state = radio->transmit((uint8_t*)&gPARAM_RFTX_buf[0], gPARAM_RFTX_buf_leng);
        state = radio->transmit(&gPARAM_RFTX_buf[0], gPARAM_RFTX_buf_leng);
        
          if (state == RADIOLIB_ERR_NONE) {
              printf("success!\n");
          } else {
              printf("failed, code %d\n", state);
          }            
        //compose poll
        lcd_set_cursor(1,0);
        lcd_hex((uint8_t)(gPARAM_fcnt_1s&0xff));
        lcd_hex((uint8_t)(gPARAM_RFTX_buf_leng&0xff));
        //#if 0
        //poll_slv.hdr = 0xa9;
        //poll_slv.length = 0x8;
        //poll_slv.mid = 0x01;
        //poll_slv.sid = 0x81;
        //poll_slv.subcmd = 0;
        //poll_slv.addr = 0;
        //poll_slv.data6 = 0;
        //poll_slv.data7 = 0;
        //rf_state = 1;  //must before transmit
        //state = radio->transmit((uint8_t*)&poll_slv, sizeof(PollData));
        //
        //  if (state == RADIOLIB_ERR_NONE) {
        //      printf("success!\n");
        //  } else {
        //      printf("failed, code %d\n", state);
        //  }          
        //  //state = radio->startReceive(); 
        //   lcd_set_cursor(0,6);
        //lcd_hex((uint8_t)(gPARAM_fcnt_1s&0xff));
        //#endif
      }
      else if((gPARAM_actflag & 0x40) == 0x40)//periodic
      {
         gPARAM_actflag &= ~0x40;
         //display periodically
         if(!(gDisp_BUF_2[0] == 0))
         {
           lcd_set_cursor(1,6);
          lcd_hex(gDisp_BUF_2[1]);
          lcd_hex(gDisp_BUF_2[2]);
          lcd_hex(gDisp_BUF_2[3]);
          lcd_hex(gDisp_BUF_2[4]);
          lcd_hex(gDisp_BUF_2[5]);
          gDisp_BUF_2[0] = 0;
         //conv2byte_5dig(gPARAM_RFRX_buf_lat[6],gPARAM_RFRX_buf_lat[5],&TS_val);
         //lcd_hex(TS_val.dig100+0x30);
         //lcd_hex(gPARAM_RRX_buf_lat[6]);
         }
         if(!(gDisp_BUF_3[0] == 0))
         {
           lcd_set_cursor(2,4);
          lcd_hex(gDisp_BUF_3[1]);
          lcd_hex(gDisp_BUF_3[2]);
          lcd_hex(gDisp_BUF_3[3]);
          lcd_hex(gDisp_BUF_3[4]);
          gDisp_BUF_3[0] = 0;
           
         }
      }
      else if((gPARAM_actflag & 0x80) == 0x80)
      {
        gPARAM_actflag &= ~0x80;
        //compose poll
        gPARAM_RFTX_buf_leng = mst_compose_rf_poll(&gPARAM_RFTX_buf[0],gPARAM_APOLL_SID[gPARAM_APOLL_cnt]);
        
          lcd_set_cursor(3,0);
          //lcd_hex(gPARAM_RFTX_buf[0]);
          //lcd_hex(gPARAM_RFTX_buf[2]);
          lcd_hex(gPARAM_RFTX_buf[3]);
          
        gPARAM_actflag|=0x20;
      }
      //  printf("Hello, world!\n");
        sleep_ms(1);
    }
    
}

void update_ds(unsigned char mode)
{
 if(ds3231_read_current_time(&ds3231, &ds3231_data) == 0) {
            //printf("No data is received\n");
    if(mode == 0)
    {
     ds3231_data.seconds = Stru_curTime.sec;
     ds3231_data.minutes = Stru_curTime.min;
     ds3231_data.hours = Stru_curTime.hour;     
     ds3231_data.month = Stru_curTime.month;
     ds3231_data.date = Stru_curTime.day;
     ds3231_data.day = Stru_curTime.dow;
     ds3231_data.year = Stru_curTime.year;     
    
    ds3231_configure_time(&ds3231, &ds3231_data);
    }   
else if(mode == 1 )
{
    ds3231_data.year = Stru_curTime.year;
    ds3231_data.month = Stru_curTime.month;
    ds3231_data.date = Stru_curTime.day;    
ds3231_data.day = Stru_curTime.dow;       
    ds3231_configure_time(&ds3231, &ds3231_data);  
}  
  }
}

void read_ds(void)
{
    if(ds3231_read_current_time(&ds3231, &ds3231_data) == 0) {
    //update_time
    Stru_curTime.month=ds3231_data.month;
    Stru_curTime.day=ds3231_data.date;
    Stru_curTime.min=ds3231_data.minutes;
    Stru_curTime.hour=ds3231_data.hours;
    lcd_set_cursor(0,8);
    lcd_hex2dec((uint8_t)((Stru_curTime.month)&0xff));
    lcd_hex2dec((uint8_t)((Stru_curTime.day)&0xff));
    lcd_hex2dec((uint8_t)((Stru_curTime.hour)&0xff));
    lcd_hex2dec((uint8_t)((Stru_curTime.min)&0xff));    
        }
}