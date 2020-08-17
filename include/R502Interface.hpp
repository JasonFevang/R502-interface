#include <stdio.h>
#include <cstring>
#include <array>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
#include <functional>
#include <cmath> // for min and max

#include "R502Definitions.hpp"

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
    typedef std::function<void(std::array<uint8_t, max_data_len * 2> &data, 
        int data_len)> up_image_cb_t;

    /**
     * \brief initialize interface, must call first
     * \param _uart_num The uart hardware port to use for communication
     * \param _pin_txd Pin to transmit to R502
     * \param _pin_rxd Pin to receive from R502
     * \param _pin_irq Pin to receive inturrupt requests from R502 on
     */
    esp_err_t init(uart_port_t _uart_num, gpio_num_t _pin_txd, 
        gpio_num_t _pin_rxd, gpio_num_t _pin_irq, 
        R502_baud_t _baud = R502_baud_57600);

    /**
     * \brief Deinitialize interface, free hardware uart and gpio resources
     */
    esp_err_t deinit();

    /**
     * \brief Return pointer to 4 byte length module address
     */
    uint8_t *get_module_address();

    /**
     * \brief Set callback for each received data frame of the fingerprint image
     * when calling up_image
     */
    void set_up_image_cb(up_image_cb_t _up_image_cb);

    /// System Commands ///

    /**
     * \brief Verify provided password
     * \param pass 4 byte password to verify
     * \param res OUT confirmation code provided by the R502
     * \retval ESP_OK: successful
     *         ESP_ERR_INVALID_STATE: Error sending or recieving via UART
     *         ESP_ERR_INVALID_SIZE: Not all data was sent out
     *         ESP_ERR_NOT_FOUND: No response from the module
     *         ESP_ERR_INVALID_CRC: Checksum failed
     *         ESP_ERR_INVALID_RESPONSE: Invalid data received
     */
    esp_err_t vfy_pass(const std::array<uint8_t, 4> &pass, 
        R502_conf_code_t &res);

    /**
     * \brief Set baud rate for communication with R502 module
     * \param baud baud rate to set
     * \param res OUT confirmation code provided by the R502
     * \retval See vfy_pass for description of all possible return values
     */
    esp_err_t set_baud_rate(R502_baud_t baud, R502_conf_code_t &res);

    /**
     * \brief Set security level for fingerprint detection with R502 module
     * \param security_level number from 1 to 5
     * \param res OUT confirmation code provided by the R502
     * \retval See vfy_pass for description of all possible return values
     */
    esp_err_t set_security_level(uint8_t security_level, R502_conf_code_t &res);
    
    /**
     * \brief Set data package length for transferring images with up_image and
     * down_image 
     * \param data_length length to set the R502 to communicate with
     * \param res OUT confirmation code provided by the R502
     * \retval See vfy_pass for description of all possible return values
     * 
     * \note Can only set data package length to 128 or 256. Setting to 64 or 32
     * only sets the module to 128. This is contrary to the documentation, and
     * perhaps this is an exception for only my module, so I've left the 32 and
     * 64 byte options accessible
     */
    esp_err_t set_data_package_length(R502_data_len_t data_length,
        R502_conf_code_t &res);

    /**
     * \brief Read system parameters
     * \param res OUT confirmation code provided by the R502
     * \param sys_para OUT data structure to be filled with system parameters
     * \retval See vfy_pass for description of all possible return values
     */
    esp_err_t read_sys_para(R502_conf_code_t &res, R502_sys_para_t &sys_para);

    /**
     * \brief Read current valid template number
     * \param res OUT confirmation code provided by the R502
     * \param template_num OUT for the returned template number
     * \retval See vfy_pass for description of all possible return values
     */
    esp_err_t template_num(R502_conf_code_t &res, uint16_t &template_num);

    /// Fingerprint Processing Commands ///

    /**
     * \brief Detect finger, and store finger in imageBuffer
     * \param res OUT confirmation code
     * \retval See vfy_pass for description of all possible return values
     */
    esp_err_t gen_image(R502_conf_code_t &res);

    /**
     * \brief Upload the image in img_buffer to upper computer
     * \param res OUT confirmation code
     * \retval See vfy_pass for description of all possible return values
     */
    esp_err_t up_image(R502_conf_code_t &res);

private:
    static const char *TAG;

    /**
     * \brief Set system parameters baud rate, security level, and 
     * data package length
     * \param parameter_num Select which parameter to be set
     * \param value The value to set the parameter to
     * \param res OUT confirmation code provided by the R502
     * \retval See vfy_pass for description of all possible return values
     * 
     * Set to private and broken out to three public methods instead
     *  Parameter Num       | Value Meaning
     *  ------------------- | -----------------------------------------------
     *  Baud Control        | N=[1,2,4,6,12]. Baud rate = N*9600 
     *  Security Level      | [1,2,3,4,5]. At 1 FAR is high, at 5 FRR is high
     *  Data Package Length | [0,1,2,3] corresponds to 32, 64, 128, 256 bytes
     */
    esp_err_t set_sys_para(R502_para_num parameter_num, int value, 
        R502_conf_code_t &res);

    /**
     * \brief Send a command to the module, and read its acknowledgement
     * \param pkg data to send
     * \param receivePkg OUT package to read response data into
     * \param data_rec_length number of data bytes to receive into
     * receivePkg.data
     * \param read_delay_ms Max number of ms to wait for a response
     * \retval ESP_OK: successful
     *         ESP_ERR_INVALID_STATE: Error sending or recieving via UART
     *         ESP_ERR_INVALID_SIZE: Not all data was sent out
     *         ESP_ERR_NOT_FOUND: No response from the module
               ESP_ERR_INVALID_RESPONSE: Not enough bytes received
               ESP_ERR_INVALID_CRC: Response had failed CRC
     */
    esp_err_t send_command_package(const R502_DataPkg_t &pkg,
        R502_DataPkg_t &receivePkg, int data_rec_length, 
        int read_delay_ms = default_read_delay);

    /**
     * \brief Send a filled package to the module
     * \param pkg A filled package, depends on length being filled
     * \retval ESP_OK: successful
               ESP_ERR_INVALID_ARG: Package length not set correctly
     *         ESP_ERR_INVALID_STATE: Error sending or recieving via UART
     *         ESP_ERR_INVALID_SIZE: Not all data was sent out
     */
    esp_err_t send_package(const R502_DataPkg_t &pkg);

    /**
     * \brief Receive a package from the module
     * \param rec_pkg OUT Package to be filled
     * \param data_length Expected amount of data to be read
     * \param read_delay_ms Max number of ms to wait for a response
     * \retval ESP_OK: successful
     *         ESP_ERR_INVALID_STATE: Error sending or recieving via UART
     *         ESP_ERR_NOT_FOUND: No data was received
     *         ESP_ERR_INVALID_SIZE: Less than data_rec_length was received
     */
    esp_err_t receive_package(const R502_DataPkg_t &rec_pkg, 
        int data_length, int read_delay_ms = default_read_delay);

    void set_headers(R502_DataPkg_t &package, R502_pid_t pid,
        uint16_t length);

    void fill_checksum(R502_DataPkg_t &package);

    bool verify_checksum(const R502_DataPkg_t &package);

    /**
     * \brief Verify the header fields of the package are correct
     * \param pkg package to verify
     * \param length Expected value of length field
     * \retval ESP_OK: successful
     *         ESP_ERR_INVALID_RESPONSE: package header is incorrect
     */
    esp_err_t verify_headers(const R502_DataPkg_t &pkg, uint16_t length);

    /**
     * \brief Return the total number of bytes in a filled package
     * \param pkg Package to measure
     * This uses the filled length parameter, so it doesn't need to know what
     * type of package data it is. But length needs to be properly filled before
     * calling
     * Includes start, adder, pid, length, data and checksum bytes
     */
    uint16_t package_length(const R502_DataPkg_t &pkg);

    uint16_t conv_8_to_16(const uint8_t in[2]);
    void conv_16_to_8(const uint16_t in, uint8_t out[2]);

    void busy_delay(int64_t microseconds);

    static void IRAM_ATTR irq_intr(void *arg);

    // callbacks
    up_image_cb_t up_image_cb = nullptr;

    // parameters
    uint8_t adder[4] = {0xFF, 0xFF, 0xFF, 0xFF};

    // shouldn't need these here. Try not to store data members for parameters
    R502_baud_t cur_baud;
    int cur_data_pkg_len = 128;

    bool initialized = false;
    int interrupt = 0;

    uart_port_t uart_num;
    gpio_num_t pin_txd;
    gpio_num_t pin_rxd;
    gpio_num_t pin_irq;

    // not used
    gpio_num_t pin_rts;
    gpio_num_t pin_cts;

    // Private constants
    const uint8_t start[2] = {0xEF, 0x01};
    static const uint16_t system_identifier_code = 9;
    static const int default_read_delay = 200; // ms
    static const int read_delay_gen_image = 2000; // ms
    // The slowest speed is transfering 256 byte payload at 9600 baud
    static const int min_uart_buffer_size = 256;
    static const int header_size = 
        sizeof(R502_DataPkg_t) - sizeof(R502_DataPkg_t::data);
};