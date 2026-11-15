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
#include "at24c32.h"
#include "global_def.h"
#include "main.h"
#include "mst_ur_proc.h"
#include "mst_rf_cmd_proc.h"
#include "ur_nb.h"   // [non-blocking UART]

void update_ds(unsigned char);
void read_ds(void);
bool rtc_sync(void);
bool rtc_read_date(void);
void load_sch_from_eep(uint8_t rly_no,uint8_t *dst_rly_sch,uint8_t *def_rly_sch);
const char * days[7] = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};

// rf_state: 0 = 接收中, 1 = 發射中（transmit() 進行中）
// [fix] 加 volatile：主迴圈寫、ISR 讀
volatile uint8_t rf_state = 0;
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
        if(time->min >= 59)
        {
            time->min = 0;
            ret |= 0x2;
            if(time->hour >= 23)
            {
                time->hour = 0;
                ret |= 0x4;
            }
            else
            {
              time->hour++;
            }
        }
        else
        {
          time->min++;
        }
    }
    else
    {
        // [fix] 原本沒有這個分支：sec 只在 59 時歸零，其餘時候不會前進，
        //       Stru_curTime.sec 一直停在開機時讀到的值，beacon 的秒數也就固定不動。
        time->sec++;
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

// 1 = overflow: inval > 9999, outval[3] is not a single digit
if(inval > 9999)
{return 1;}
return 0;
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
        {
          // [fix] 原本在這裡呼叫 radio->startReceive()：
          //   1. 在 ISR 裡做 SPI，會和主迴圈正在進行的 radio->transmit() 搶 SPI。
          //   2. transmit() 是 blocking，自己會輪詢 DIO1 等 TxDone；ISR 先把 IRQ 清掉、
          //      切到 RX，transmit() 可能等不到 DIO1 而回報 TX timeout，
          //      結束時再呼叫 finishTransmit() 把 radio 切回 standby → master 收不到。
          // 現在 ISR 只標記 TxDone，切回 RX 一律在主迴圈 transmit() 之後做。
          rf_state = 0;
        }
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
      
      // [timer] 系統時鐘 = 100 ms timer + Stru_curTime 的 時/分/秒。
      //         gPARAM_fcnt_1s 不再自己從開機起算，改成等於目前的「秒」，
      //         beacon（秒 = 0 / 16 / 32）和之後的 TDMA 都以同一個時鐘為準。
      sub_timeadv(&Stru_curTime);
      gPARAM_fcnt_1s = Stru_curTime.sec;
      actflag_set(0x1);  
      if((gPARAM_fcnt_1s == 0)||(gPARAM_fcnt_1s == 16)||(gPARAM_fcnt_1s == 32))
      {
        actflag_set(0x10);//bcn
      }
      else if(gPARAM_fcnt_1s % 4 == 2)
      {
        actflag_set(0x40);//display
      }
      else if(gPARAM_fcnt_1s % 2 == 1)
      {
        if(gPARAM_APOLL_en == 1)
          {
            if(gPARAM_APOLL_cnt >= gPARAM_APOLL_cnt_lmt)
            {gPARAM_APOLL_cnt=0;}
            else
            {gPARAM_APOLL_cnt++;}
             actflag_set(0x80);//auto pool??
          }
      }
    }  
    else
    {gPARAM_fcnt_100ms++;}

    return true;
}
// [non-blocking UART] 封包解析從 ISR 移到主迴圈。
//   原本 ISR 一邊拆封包一邊寫 gPARAM_UR1RX_buf，主迴圈同時在讀同一塊 buffer；
//   而且沒有逾時，中間掉一個 byte 後面的封包全部錯位。
//   現在 ISR（ur_nb.c）只把 byte 放進 ring，這裡依原本規則組封包：
//     byte0 = cmd, byte1 = 長度（0 或 >= 32 視為錯誤，重新開始）
//   並加上 byte 間逾時：超過 UR_FRAME_TIMEOUT_US 沒收到下一個 byte 就丟掉半包重來。
//   上一包還沒處理完（旗標 0x2 未清）時先不解析，資料留在 ring 裡等。
#define UR_FRAME_TIMEOUT_US 20000u   // 20 ms（115200 bps 下一個 byte 約 87 us）

void ur_frame_poll(void)
{
  static uint32_t last_us = 0;
  int c;

  if ((gPARAM_actflag & 0x2) == 0x2) { return; }   // 上一包還沒處理

  if ((gPARAM_UR1RX_buf_ptr != 0) && ((time_us_32() - last_us) > UR_FRAME_TIMEOUT_US))
  {
    gPARAM_UR1RX_buf_ptr = 0;                       // 逾時：丟掉半包
  }

  while ((c = ur_nb_getc()) >= 0)
  {
    uint8_t uctemp = (uint8_t)c;
    last_us = time_us_32();

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
      actflag_set(0x02);
      return;                                       // 一次一包，剩下的留在 ring
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
    // 922.5 MHz：台灣 LoRa 免執照頻段 920–925 MHz 的中間。master / slave 必須相同。
    state = radio->begin(922.5,125.0,7,5,0x12,20);
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
    // [timer] 負數 = 每 100 ms 固定觸發（對齊絕對時間，晚了下一拍會補回）。
    //         正數是「上一次 callback 結束後再等 100 ms」，每一拍的延遲都會累積，時鐘會越走越慢。
    add_repeating_timer_ms(-100, repeating_timer_callback, NULL, &timer);
    // For more examples of timer use see https://github.com/raspberrypi/pico-examples/tree/master/timer

    // Set up our UART
    // [non-blocking UART] uart_init / 腳位 / 中斷都在 ur_nb_init() 裡（RX、TX 都用中斷 + ring buffer）
    ur_nb_init(UART_ID, BAUD_RATE, UART_TX_PIN, UART_RX_PIN);
    {
      static const uint8_t hello[] = " Hello, UART!\r\n";
      ur_nb_send(hello, sizeof(hello) - 1);
    }
    
    // For more examples of UART use see https://github.com/raspberrypi/pico-examples/tree/master/uart

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);

    //param initial
    param_init();
    
    lcd_init();
    lcd_set_cursor(0,3);
    lcd_string(FW_VERSION);   // 版號定義在 global_def.h
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
    {
      // [fix] Stru_curTime 會在 timer ISR（sub_timeadv）裡被更新，主迴圈寫入時先關中斷
      uint32_t irq_s = save_and_disable_interrupts();
      Stru_curTime.sec=ds3231_data.seconds;
      Stru_curTime.min=ds3231_data.minutes;
      Stru_curTime.hour=ds3231_data.hours;
      restore_interrupts(irq_s);
    }
    rtc_sync();   // [timer] 開機：對齊 RTC 的秒邊界，之後由 100 ms timer 接手
    
        } else {
    lcd_set_cursor(0,12);
    lcd_string("E");
        }

    // load relay schedule from AT24C32 EEPROM
    // 1) 96-byte block at SCH_RLY_EEPROM_ADDR is the default for every relay
    at24c32_load_sch_rly(I2C_PORT, AT24C32_ADDR, gSCH_RLY);
    // 2) a relay's own record (valid signature) overrides its 24 bytes
    for (uint8_t rly = 0; rly < 4; rly++)
    {
      load_sch_from_eep(rly, &gSCH_RLY[24*rly], &gSCH_RLY[24*rly]);
    }

    while (true) {
      ur_frame_poll();   // [non-blocking UART] 從 RX ring 組封包，完成時設旗標 0x2
      if (receivedFlag) {
            // 關閉中斷，避免處理過程中被打斷
            gpio_set_irq_enabled(RADIO_DIO1_PIN, GPIO_IRQ_EDGE_RISE, false);
            
            // 清除標誌
            receivedFlag = false;
            // [fix] 依實際收到的長度讀取（原本固定 16 bytes，slave 的 poll 回應是 20 bytes，
            //       後 4 bytes（含 DS18B20 低位元組）被截掉）。
            //   - getPacketLength()：取得剛收到的封包長度（SX126x GetRxBufferStatus）
            //   - readData(buf, len)：實際讀 min(len, 封包長度)；len = 0 會整包讀完，
            //     但 LoRa 最長 255 bytes，所以要先限制在 buffer 大小內，避免寫爆。
            size_t rx_len = radio->getPacketLength();
            if (rx_len > sizeof(gPARAM_RFRX_buf)) { rx_len = sizeof(gPARAM_RFRX_buf); }
            memset(&gPARAM_RFRX_buf[0], 0, sizeof(gPARAM_RFRX_buf));   // 清掉上一包較長封包的殘留
            state = radio->readData(&gPARAM_RFRX_buf[0], rx_len);
            memcpy(&gPARAM_RFRX_buf_lat[0],&gPARAM_RFRX_buf[0],sizeof(gPARAM_RFRX_buf_lat));
            if (state == RADIOLIB_ERR_NONE) {
              //rf cmd process
              ret = mst_rf_cmd_process(&gPARAM_RFRX_buf_lat[0],&gPARAM_UR1TX_buf[0],&gPARAM_UR1TX_buf_leng,&gDisp_BUF_2[0]);
              if((ret & 0x1) == 0x1)
              {
                // [non-blocking UART] 直接放進 TX ring（不會等）。
                // 原本設旗標 0x4 晚點才送；若期間 UART 命令的回應也寫進 gPARAM_UR1TX_buf，前一包會被蓋掉。
                ur_nb_send(gPARAM_UR1TX_buf, gPARAM_UR1TX_buf_leng);
              }//uart
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
        actflag_clr(0x1);
        //toggle led
        if((gPARAM_fcnt_1s & 0x1) == 0x1){ gpio_put(LED_PIN, 1); }
        else {gpio_put(LED_PIN, 0);}
        //
        lcd_set_cursor(0,0);
        lcd_hex2dec((uint8_t)(gPARAM_fcnt_1s&0xff));
        {
          // [timer] RTC 不再每分鐘介入，只在 RTC_SYNC_HOUR_A / _B 的第 RTC_SYNC_MIN 分校正一次
          //         （預設 00:05 與 12:05）。其餘時間由 100 ms timer 自己走。
          static uint8_t rtc_sync_done_hour = 0xff;   // 這個時段是否已校正過
          uint32_t irq_s = save_and_disable_interrupts();
          uint8_t n_hour = Stru_curTime.hour, n_min = Stru_curTime.min, n_sec = Stru_curTime.sec;
          restore_interrupts(irq_s);

          if(((n_hour == RTC_SYNC_HOUR_A) || (n_hour == RTC_SYNC_HOUR_B)) && (n_min >= RTC_SYNC_MIN))
          {
            if(rtc_sync_done_hour != n_hour)
            {
              rtc_sync_done_hour = n_hour;
              if(rtc_sync()) { read_ds(); }
            }
          }
          // [timer] 換日：100 ms timer 只推進 時/分/秒，月/日 在午夜過後向 RTC 讀一次。
          //         等到 00:00:RTC_DATE_SEC 才讀，是因為系統時鐘和 RTC 可能差 1 秒左右，
          //         太早讀的話 RTC 可能還停在前一天的 23:59:59。這裡只更新日期，不動 時/分/秒。
          static bool rtc_date_done = false;
          if((n_hour == 0) && (n_min == 0))
          {
            if(!rtc_date_done && (n_sec >= RTC_DATE_SEC))
            {
              rtc_date_done = true;
              if(rtc_read_date()) { read_ds(); }
            }
          }
          else
          {
            rtc_date_done = false;
          }
          if(n_sec == 0)
          {
            read_ds();   // 每分鐘更新 LCD 上的 月/日/時/分（只顯示，不讀 RTC）
          }
        }
      }
      // [fix] gPARAM_RFTX_buf 只有一個，host 命令、beacon、自動輪詢都會寫它，再設旗標 0x20 等送出。
      //       還有封包等著送（0x20）時，這三個來源都先不動，等這一包送完的下一圈再處理，
      //       否則後寫的會蓋掉前一包（原本 host 的命令可能被 beacon 蓋掉而沒送出）。
      #define RF_TX_PENDING() ((gPARAM_actflag & 0x20) == 0x20)
      if(((gPARAM_actflag & 0x2) == 0x2) && !RF_TX_PENDING())//urrx
      {
        actflag_clr(0x02);
        //process urrx
        #if 0
        lcd_set_cursor(2,0);
        lcd_hex((uint8_t)(gPARAM_UR1RX_buf[0]&0xff));
        lcd_hex((uint8_t)(gPARAM_UR1RX_buf[1]&0xff));
        lcd_hex((uint8_t)(gPARAM_UR1RX_buf[3]&0xff));
        #endif
        #if 1
          ret = mst_ur_cmd_process(&gPARAM_UR1RX_buf[0],&gPARAM_UR1TX_buf[0],&gPARAM_UR1TX_buf_leng,&gPARAM_RFTX_buf[0],&gPARAM_RFTX_buf_leng);
          
          #if 1
          lcd_set_cursor(1,12);
          lcd_hex((uint8_t)(ret&0xff));
          lcd_hex((uint8_t)(gPARAM_UR1TX_buf_leng&0xff));
          #endif
          if((ret & 0x1) == 0x1)
          {
            ret &= ~0x1;
            //send urrsp
            // [non-blocking UART] 直接放進 TX ring，理由同上
            ur_nb_send(gPARAM_UR1TX_buf, gPARAM_UR1TX_buf_leng);
          }
           if((ret & 0x2) == 0x2)
          {
            ret &= ~0x2;
            //send rf,merge to 0x20 
            actflag_set(0x20);
            //uart_write_blocking(UART_ID, gPARAM_UR1TX_buf, gPARAM_UR1TX_buf_leng);
          }
          if((ret & 0x4) == 0x4)//actions
          {
            ret &= ~0x4;
            if(gPARAM_ACT_DATA[0] == 0)
            {
              update_ds(0);
              // [timer] host 校時：時間已寫進 RTC，系統時鐘也從這一刻起算新的一秒，
              //         並立刻多發一次 beacon，讓 slave 不用等到下一個排定的 beacon。
              {
                uint32_t irq_s = save_and_disable_interrupts();
                gPARAM_fcnt_100ms = 0;
                gPARAM_fcnt_1s = Stru_curTime.sec;
                restore_interrupts(irq_s);
              }
              actflag_set(0x10);
              read_ds();
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
        // [non-blocking UART] 已不再使用（改成組好回應就直接 ur_nb_send），保留以防其他地方設這個旗標
        actflag_clr(0x4);
        ur_nb_send(gPARAM_UR1TX_buf, gPARAM_UR1TX_buf_leng);
      }
      else if(((gPARAM_actflag & 0x10) == 0x10) && !RF_TX_PENDING())
      {
        actflag_clr(0x10);
        //compose bcn
        {
          // [fix] 一次取完 sec/min/hour，避免剛好在 59→00 進位時被 ISR 打斷，讀到不一致的時間
          uint32_t irq_s = save_and_disable_interrupts();
          uint8_t b_sec = Stru_curTime.sec, b_min = Stru_curTime.min, b_hour = Stru_curTime.hour;
          restore_interrupts(irq_s);
          mst_compose_rf_bcn(&u8_temp16[0],b_sec,b_min,b_hour);
        }
        //copy to txbuf
        memcpy(&gPARAM_RFTX_buf[0],&u8_temp16[0],12);gPARAM_RFTX_buf_leng = 12;
        actflag_set(0x20);//send
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
        actflag_clr(0x20);
        //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<RF SEND
        //lcd_set_cursor(1,8);
        //lcd_hex(gPARAM_RFTX_buf[0]);lcd_hex(gPARAM_RFTX_buf_leng);
        rf_state = 1;  //must before transmit
        //state = radio->transmit((uint8_t*)&gPARAM_RFTX_buf[0], gPARAM_RFTX_buf_leng);
        state = radio->transmit(&gPARAM_RFTX_buf[0], gPARAM_RFTX_buf_leng);
        // [fix] transmit() 結束後 radio 在 standby，必須由主迴圈切回 RX
        //       （原本靠 ISR 裡的 startReceive()，已移除）。
        //       放在 LCD 之前：LCD 寫入很慢，太晚切回 RX 會漏收 slave 的回應。
        rf_state = 0;
        radio->startReceive();
        
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
         actflag_clr(0x40);
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
      else if(((gPARAM_actflag & 0x80) == 0x80) && !RF_TX_PENDING())
      {
        actflag_clr(0x80);
        //compose poll
        gPARAM_RFTX_buf_leng = mst_compose_rf_poll(&gPARAM_RFTX_buf[0],gPARAM_APOLL_SID[gPARAM_APOLL_cnt]);
        
          lcd_set_cursor(3,0);
          //lcd_hex(gPARAM_RFTX_buf[0]);
          //lcd_hex(gPARAM_RFTX_buf[2]);
          lcd_hex(gPARAM_RFTX_buf[3]);
          
        actflag_set(0x20);
      }
      //  printf("Hello, world!\n");
        sleep_ms(1);
    }
    
}

void update_ds(unsigned char mode)
{
 // [fix] 先取一份 Stru_curTime 快照（ISR 會改它），下面都用快照
 uint32_t irq_s = save_and_disable_interrupts();
 TimeData t = Stru_curTime;
 restore_interrupts(irq_s);
 if(ds3231_read_current_time(&ds3231, &ds3231_data) == 0) {
            //printf("No data is received\n");
    if(mode == 0)
    {
     ds3231_data.seconds = t.sec;
     ds3231_data.minutes = t.min;
     ds3231_data.hours = t.hour;     
     ds3231_data.month = t.month;
     ds3231_data.date = t.day;
     ds3231_data.day = t.dow;
     ds3231_data.year = t.year;     
    
    ds3231_configure_time(&ds3231, &ds3231_data);
    }   
else if(mode == 1 )
{
    ds3231_data.year = t.year;
    ds3231_data.month = t.month;
    ds3231_data.date = t.day;    
ds3231_data.day = t.dow;       
    ds3231_configure_time(&ds3231, &ds3231_data);  
}  
  }
}

// [timer] 只負責把 Stru_curTime 的 月/日/時/分 顯示到 LCD，不再讀 RTC。
//         （原本每分鐘讀一次 DS3231 覆寫時、分；現在 RTC 只在 rtc_sync() 介入。）
void read_ds(void)
{
    uint32_t irq_s = save_and_disable_interrupts();
    TimeData t = Stru_curTime;
    restore_interrupts(irq_s);
    lcd_set_cursor(0,8);
    lcd_hex2dec((uint8_t)((t.month)&0xff));
    lcd_hex2dec((uint8_t)((t.day)&0xff));
    lcd_hex2dec((uint8_t)((t.hour)&0xff));
    lcd_hex2dec((uint8_t)((t.min)&0xff));
}

// [timer] 午夜過後只向 RTC 讀日期（月/日/星期/年），不動 時/分/秒，不會擋住主迴圈。
//   回傳 true = 已更新；false = RTC 讀不到，日期維持不變。
bool rtc_read_date(void)
{
    if(ds3231_read_current_time(&ds3231, &ds3231_data) != 0) { return false; }
    uint32_t irq_s = save_and_disable_interrupts();
    Stru_curTime.month = ds3231_data.month;
    Stru_curTime.day   = ds3231_data.date;
    Stru_curTime.dow   = ds3231_data.day;
    Stru_curTime.year  = ds3231_data.year;
    restore_interrupts(irq_s);
    return true;
}

// [timer] 用 RTC 校正系統時鐘（開機、每天 00:05 / 12:05 各一次）。
//   DS3231 只能讀到「秒」，隨便一個時間點去讀會有最多 1 秒的誤差，
//   所以先等 RTC 的秒數跳動（秒邊界），在那一刻把 時/分/秒/月/日 寫進系統時鐘，
//   並把 100 ms 計數歸零，兩邊的秒邊界就對齊了（誤差約幾 ms）。
//   代價：會擋住主迴圈最多約 1 秒；期間收到的 RF 封包會等這裡結束後才處理。
//   回傳 true = 已校正；false = RTC 讀不到或沒在走，系統時鐘維持不變。
bool rtc_sync(void)
{
    if(ds3231_read_current_time(&ds3231, &ds3231_data) != 0) { return false; }
    uint8_t s0 = ds3231_data.seconds;
    absolute_time_t timeout = make_timeout_time_ms(1200);
    do
    {
        sleep_ms(2);
        if(ds3231_read_current_time(&ds3231, &ds3231_data) != 0) { return false; }
    } while((ds3231_data.seconds == s0) && !time_reached(timeout));
    if(ds3231_data.seconds == s0) { return false; }

    uint32_t irq_s = save_and_disable_interrupts();
    gPARAM_fcnt_100ms  = 0;
    Stru_curTime.sec   = ds3231_data.seconds;
    Stru_curTime.min   = ds3231_data.minutes;
    Stru_curTime.hour  = ds3231_data.hours;
    Stru_curTime.month = ds3231_data.month;
    Stru_curTime.day   = ds3231_data.date;
    gPARAM_fcnt_1s     = Stru_curTime.sec;
    restore_interrupts(irq_s);
    return true;
}

void load_sch_from_eep(uint8_t rly_no,uint8_t *dst_rly_sch,uint8_t *def_rly_sch)
{
  uint8_t dtemp_l[16];
  uint8_t dtemp_h[16];
  uint16_t word_addr;
  int i;
  for(i=0;i<24;i++)
  {
    *(dst_rly_sch+i) = *(def_rly_sch+i);
  }
  word_addr = SCH_RLY0_EEPROM_ADDRH + SCH_RLY_EEPROM_STRIDE*rly_no;
  at24c32_read(I2C_PORT, AT24C32_ADDR, word_addr,&dtemp_h[0],16);
  if((dtemp_h[8] ==SCH_RLY_PWD_H)&&(dtemp_h[9] ==SCH_RLY_PWD_L))
  {
    word_addr = SCH_RLY0_EEPROM_ADDRL + SCH_RLY_EEPROM_STRIDE*rly_no;
  at24c32_read(I2C_PORT, AT24C32_ADDR, word_addr,&dtemp_l[0],16);
  for(i=0;i<16;i++)
  {
    *(dst_rly_sch+i) = dtemp_l[i];
  }
  for(i=0;i<8;i++)
  {
    *(dst_rly_sch+i+16) = dtemp_h[i];
  }
  }
}