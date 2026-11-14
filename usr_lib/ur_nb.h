// ur_nb.h — non-blocking UART（中斷驅動 RX / TX ring buffer）
//
// 原本：
//   TX  uart_write_blocking()：FIFO（32 bytes）滿了就在主迴圈裡等。
//   RX  ISR 直接拆封包、寫進 gPARAM_UR1RX_buf，主迴圈同時在讀同一塊 buffer。
// 現在：
//   ISR 只搬 byte：RX FIFO → RX ring，TX ring → TX FIFO。
//   ur_nb_send() 把整包放進 TX ring 立刻返回，不會等。
//   封包解析移到主迴圈（ur_frame_poll，在 pico_mst_260219.cpp）。
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "hardware/uart.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UR_NB_TX_SIZE 512   // 必須是 2 的冪次
#define UR_NB_RX_SIZE 256   // 必須是 2 的冪次

// 初始化 UART、腳位、中斷（取代 uart_init + irq_set_exclusive_handler + uart_set_irq_enables）
void ur_nb_init(uart_inst_t *uart, uint baud, uint tx_pin, uint rx_pin);

// 把 len bytes 放進 TX ring 後立刻返回。
// 空間不夠時整包不送、回傳 false（不會只送一半）。
bool ur_nb_send(const uint8_t *data, uint16_t len);

// 取一個收到的 byte；沒有資料回傳 -1
int ur_nb_getc(void);

uint16_t ur_nb_tx_free(void);      // TX ring 剩餘空間
uint32_t ur_nb_rx_overflow(void);  // RX ring 滿而丟掉的 byte 數（除錯用）
uint32_t ur_nb_tx_dropped(void);   // 因空間不足被丟掉的封包數（除錯用）

#ifdef __cplusplus
}
#endif
