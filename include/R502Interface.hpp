#include <stdio.h>
#include <cstring>
#include <array>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"

#include "R502Definitions.hpp"

#define BUF_SIZE (1024)

class R502Interface {
public:
    void init(uart_port_t _uart_num, gpio_num_t _pin_txd, gpio_num_t _pin_rxd, gpio_num_t _pin_irq);
    void deinit();

    esp_err_t vfyPass(const std::array<uint8_t, 4> &pass, R502_conf_code_t &res);
    void test1();
private:
    static const char *TAG;
    void busy_delay(int64_t microseconds);
    static void IRAM_ATTR irq_intr(void *arg);

    gpio_num_t pin_txd;
    gpio_num_t pin_rxd;
    gpio_num_t pin_irq;

    // not used
    gpio_num_t pin_rts;
    gpio_num_t pin_cts;

    uart_port_t uart_num;
    int interrupt = 0;
};