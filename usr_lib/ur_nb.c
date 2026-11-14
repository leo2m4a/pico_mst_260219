// ur_nb.c — non-blocking UART（中斷驅動 RX / TX ring buffer）
#include "ur_nb.h"

#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/sync.h"

#define TX_MASK (UR_NB_TX_SIZE - 1)
#define RX_MASK (UR_NB_RX_SIZE - 1)

static uart_inst_t *s_uart;

static uint8_t           s_tx_buf[UR_NB_TX_SIZE];
static volatile uint16_t s_tx_head;     // 寫入端（主迴圈）
static volatile uint16_t s_tx_tail;     // 讀出端（ISR / pump）

static uint8_t           s_rx_buf[UR_NB_RX_SIZE];
static volatile uint16_t s_rx_head;     // 寫入端（ISR）
static volatile uint16_t s_rx_tail;     // 讀出端（主迴圈）

static volatile uint32_t s_rx_overflow;
static volatile uint32_t s_tx_dropped;

// 把 TX ring 的資料填進硬體 FIFO，填到 FIFO 滿或 ring 空為止。
// ring 還有資料就開 TX 中斷，空了就關（否則 FIFO 空時會一直進中斷）。
// 呼叫時必須已關中斷，或在 UART ISR 內。
static void tx_pump(void)
{
    while (s_tx_tail != s_tx_head && uart_is_writable(s_uart)) {
        uart_get_hw(s_uart)->dr = s_tx_buf[s_tx_tail];
        s_tx_tail = (s_tx_tail + 1) & TX_MASK;
    }
    uart_set_irqs_enabled(s_uart, true, s_tx_tail != s_tx_head);
}

static void ur_nb_irq(void)
{
    // RX：FIFO → ring
    while (uart_is_readable(s_uart)) {
        uint8_t c = (uint8_t)uart_get_hw(s_uart)->dr;
        uint16_t next = (s_rx_head + 1) & RX_MASK;
        if (next == s_rx_tail) {
            s_rx_overflow++;           // 主迴圈太久沒取，丟掉
            continue;
        }
        s_rx_buf[s_rx_head] = c;
        s_rx_head = next;
    }
    // TX：ring → FIFO
    tx_pump();
}

void ur_nb_init(uart_inst_t *uart, uint baud, uint tx_pin, uint rx_pin)
{
    s_uart = uart;
    s_tx_head = s_tx_tail = 0;
    s_rx_head = s_rx_tail = 0;

    uart_init(uart, baud);
    gpio_set_function(tx_pin, GPIO_FUNC_UART);
    gpio_set_function(rx_pin, GPIO_FUNC_UART);
    uart_set_fifo_enabled(uart, true);

    int irq = (uart == uart0) ? UART0_IRQ : UART1_IRQ;
    irq_set_exclusive_handler(irq, ur_nb_irq);
    irq_set_enabled(irq, true);
    uart_set_irqs_enabled(uart, true, false);   // 先只開 RX；有資料要送時才開 TX
}

uint16_t ur_nb_tx_free(void)
{
    return (uint16_t)((s_tx_tail - s_tx_head - 1) & TX_MASK);
}

bool ur_nb_send(const uint8_t *data, uint16_t len)
{
    if (len == 0) return true;

    uint32_t irq_s = save_and_disable_interrupts();
    if (len > ur_nb_tx_free()) {
        restore_interrupts(irq_s);
        s_tx_dropped++;
        return false;
    }
    for (uint16_t i = 0; i < len; i++) {
        s_tx_buf[s_tx_head] = data[i];
        s_tx_head = (s_tx_head + 1) & TX_MASK;
    }
    // 先直接填 FIFO：PL011 的 TX 中斷是「FIFO 水位往下穿過門檻」才觸發，
    // FIFO 本來就空時只開中斷不會進來，所以這裡要先手動送一批。
    tx_pump();
    restore_interrupts(irq_s);
    return true;
}

int ur_nb_getc(void)
{
    if (s_rx_tail == s_rx_head) return -1;
    uint8_t c = s_rx_buf[s_rx_tail];
    s_rx_tail = (s_rx_tail + 1) & RX_MASK;
    return c;
}

uint32_t ur_nb_rx_overflow(void) { return s_rx_overflow; }
uint32_t ur_nb_tx_dropped(void)  { return s_tx_dropped; }
