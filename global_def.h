// SPI Defines
// We are going to use SPI 0, and allocate it to the following GPIO pins
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define SPI_PORT spi1
#define PIN_MISO 12
#define PIN_CS   13
#define PIN_SCK  10
#define PIN_MOSI 11

// I2C defines
// This example will use I2C0 on GPIO8 (SDA) and GPIO9 (SCL) running at 400KHz.
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define I2C_PORT i2c1
#define I2C_SDA 6
#define I2C_SCL 7

// UART defines
// By default the stdout UART is `uart0`, so we will use the second one
#define UART_ID uart1
#define BAUD_RATE 115200

// Use pins 4 and 5 for UART1
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define UART_TX_PIN 8
#define UART_RX_PIN 9

#define UART_EXT 21
// NSS pin
#define RADIO_CS_PIN 3
// DIO1 pin (中斷引腳)
#define RADIO_DIO1_PIN 20
// Reset pin
#define RADIO_RST_PIN 15
// Busy pin
#define RADIO_BUSY_PIN 2

//------ADC
// 分壓電阻配置 (單位: MΩ)
#define VOLTAGE_R1 1.5  // 下電阻 (連接 GND)
#define VOLTAGE_R2 1.0  // 上電阻 (連接外部電壓)

// 電壓校正係數 (根據實際測量調整)
// 計算方法: 校正係數 = 實際電壓 / 顯示電壓
// 例如: 實際 3.5V 顯示為 3.37V，則 校正係數 = 3.5 / 3.37 = 1.0386
#define VOLTAGE_CALIBRATION 1.0386  // 預設校正係數

#define GPIO_BTN_U_PIN 18
#define GPIO_BTN_D_PIN 19

#define MY_MID 0x02
#define MY_SID1 0x81
#define MY_SID2 0x83
//const char * days[7] = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};

// time資料結構（使用 packed 避免記憶體對齊問題）
typedef struct {
    uint8_t sec;
    uint8_t min;
    uint8_t hour;
    uint8_t caldone;
    uint8_t year;
    uint8_t month;
    uint8_t day;
    uint8_t dow;
//    uint32_t device_id;      // 裝置唯一 ID (4 bytes)
//    uint32_t u32data0;       //  (4 bytes)
//    uint32_t u32data1;          //  (4 bytes)
//    uint16_t checksum;       // CRC-16 校驗碼 (2 bytes)
} __attribute__((packed)) TimeData;  // 總共 14 bytes

// beacon資料結構（使用 packed 避免記憶體對齊問題）
typedef struct {
    uint8_t hdr;
    uint8_t length;
    uint8_t mid;
    uint8_t sid;
    uint8_t subcmd;
    uint8_t sec;
    uint8_t min;
    uint8_t hour;
    uint8_t fcnt;
    uint8_t data9;
    uint8_t data10;
    uint8_t data11;
//    uint32_t device_id;      // 裝置唯一 ID (4 bytes)
//    uint32_t u32data0;       //  (4 bytes)
//    uint32_t u32data1;          //  (4 bytes)
//    uint16_t checksum;       // CRC-16 校驗碼 (2 bytes)
} __attribute__((packed)) BcnData;  // 總共 14 bytes

// 感測器資料結構（使用 packed 避免記憶體對齊問題）
typedef struct {
    uint8_t hdr;
    uint8_t length;
    uint8_t mid;
    uint8_t sid;
    uint8_t subcmd;
    uint8_t addr;
    uint8_t data6;
    uint8_t data7;
//    uint32_t device_id;      // 裝置唯一 ID (4 bytes)
//    uint32_t u32data0;       //  (4 bytes)
//    uint32_t u32data1;          //  (4 bytes)
//    uint16_t checksum;       // CRC-16 校驗碼 (2 bytes)
} __attribute__((packed)) PollData;  // 總共 14 bytes

typedef struct {
    uint8_t dig100;
    uint8_t dig10;
    uint8_t dig1;
    uint8_t dot1;
    uint8_t dot10;
} __attribute__((packed)) dec5Dig;  // 總共  bytes

typedef struct {
    uint8_t dig100;
    uint8_t dig10;
    uint8_t dig1;
} __attribute__((packed)) dec3Dig;  // 總共  bytes

typedef struct {
    uint8_t rcv_ts;
    uint8_t sid;
    uint8_t temp[4];
    uint8_t humd[4];
    uint8_t rly;
    uint8_t adc0[4];
} __attribute__((packed)) slv_Data;  // 總共  bytes