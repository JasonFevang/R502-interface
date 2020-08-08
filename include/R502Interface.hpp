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

/**
 * @mainpage ESP32 R502 Interface
 * The R502 is a fingerprint indentification module, developed by GROW
 * Technology.
 * The offical documentation can be downloaded here: 
 * https://www.dropbox.com/sh/epucei8lmoz7xpp/AAAmon04b1DiSOeh1q4nAhzAa?dl=0
 * 
 * This is an ESP-IDF component developed for the esp32 to interface with the
 * R502 module via UART
 */

/**
 * \brief Provides command-level api to interact with the R502 fingerprint
 * scanner module
 */
class R502Interface {
public:
    /**
     * \brief initialize interface, must call first
     * \param _uart_num The uart hardware port to use for communication
     * \param _pin_txd Pin to transmit to R502
     * \param _pin_rxd Pin to receive from R502
     * \param _pin_irq Pin to receive inturrupt requests from R502 on
     */
    esp_err_t init(uart_port_t _uart_num, gpio_num_t _pin_txd, 
        gpio_num_t _pin_rxd, gpio_num_t _pin_irq);

    /**
     * \brief Deinitialize interface, free hardware uart and gpio resources
     */
    esp_err_t deinit();

    /**
     * \brief Verify provided password
     * \param pass 4 byte password to verify
     * \param res confirmation code provided by the R502
     */
    esp_err_t vfyPass(const std::array<uint8_t, 4> &pass, 
        R502_conf_code_t &res);

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