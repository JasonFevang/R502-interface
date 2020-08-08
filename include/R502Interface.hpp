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
    esp_err_t init(uart_port_t _uart_num, gpio_num_t _pin_txd, gpio_num_t _pin_rxd, gpio_num_t _pin_irq);
    esp_err_t deinit();

    esp_err_t vfyPass(const std::array<uint8_t, 4> &pass, R502_conf_code_t &res);
private:
    static const char *TAG;

    void set_headers(R502_DataPackage_t &package, R502_pid_t pid, uint16_t length);
    void fill_checksum(R502_DataPackage_t &package);
    bool verify_checksum(const R502_DataPackage_t &package);
    //esp_err_t sendPackage(R502_DataPackage_t &package, R502_DataPackage_t &receivePackage);
    esp_err_t sendCommandPackage(const R502_DataPackage_t &package, R502_DataPackage_t &receivePackage);
    uint16_t conv_8_to_16(const uint8_t in[2]);

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

    const uint8_t start[2] = {0xEF, 0x01};
    uint8_t adder[4] = {0xFF, 0xFF, 0xFF, 0xFF};
    const int read_delay = 20; //ms
};