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
     * \retval ESP_OK: successful
     *         ESP_ERR_INVALID_STATE: Error sending or recieving via UART
     *         ESP_ERR_INVALID_SIZE: Not all data was sent out
     *         ESP_ERR_NOT_FOUND: No response from the module
     *         ESP_ERR_INVALID_CRC: Checksum failed
     *         ESP_ERR_INVALID_RESPONSE: Invalid data received
     */
    esp_err_t vfy_pass(const std::array<uint8_t, 4> &pass, 
        R502_conf_code_t &res);
private:
    static const char *TAG;

    void set_headers(R502_DataPackage_t &package, R502_pid_t pid,
        uint16_t length);

    void fill_checksum(R502_DataPackage_t &package);

    bool verify_checksum(const R502_DataPackage_t &package);

    /**
     * \brief Verify the header fields of the package are correct
     * \param pkg package to verify
     * \param pid Expected package ID
     * \param length Expected value of length field
     * \retval ESP_OK: successful
     *         ESP_ERR_INVALID_RESPONSE: package header is incorrect
     */
    esp_err_t verify_headers(const R502_DataPackage_t &pkg, R502_pid_t pid, 
        uint16_t length);

    /**
     * \brief Send a command to the module, and read its acknowledgement
     * \param pkg data to send
     * \param receivePkg package to read response data into
     * \retval ESP_OK: successful
     *         ESP_ERR_INVALID_STATE: Error sending or recieving via UART
     *         ESP_ERR_INVALID_SIZE: Not all data was sent out
     *         ESP_ERR_NOT_FOUND: No response from the module
     */
    esp_err_t send_command_package(const R502_DataPackage_t &pkg,
        R502_DataPackage_t &receivePkg);

    uint16_t conv_8_to_16(const uint8_t in[2]);
    void conv_16_to_8(const uint16_t in, uint8_t out[2]);

    /**
     * \brief Return the total number of bytes in a filled package
     * \param pkg Package to measure
     * This uses the filled length parameter, so it doesn't need to know what
     * type of package data it is. But length needs to be properly filled before
     * calling
     * Includes start, adder, pid, length, data and checksum bytes
     */
    uint16_t package_length(const R502_DataPackage_t &pkg);

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

    bool initialized = false;

    const uint8_t start[2] = {0xEF, 0x01};
    uint8_t adder[4] = {0xFF, 0xFF, 0xFF, 0xFF};
    const int read_delay = 20; //ms
};