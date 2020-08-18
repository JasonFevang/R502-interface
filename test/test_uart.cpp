#include <limits.h>
#include "unity.h"
#include <array>
#include "R502Interface.hpp"
#include "esp_err.h"
#include "esp_log.h"
#include "esp32/rom/uart.h"

#define PIN_TXD  (GPIO_NUM_4)
#define PIN_NC_1 (GPIO_NUM_16)
#define PIN_NC_2 (GPIO_NUM_17)
#define PIN_NC_3 (GPIO_NUM_18)
#define PIN_RXD  (GPIO_NUM_5)
#define PIN_IRQ  (GPIO_NUM_13)
#define PIN_RTS  (UART_PIN_NO_CHANGE)
#define PIN_CTS  (UART_PIN_NO_CHANGE)

#define TAG "TestR502"

// The object under test
static R502Interface R502;

void tearDown(){
    R502.deinit();
}

void wait_with_message(char *message){
    printf(message);
    uart_rx_one_char_block();
}

static int up_image_size = 0;
void up_image_callback(std::array<uint8_t, max_data_len * 2> &data, 
    int data_len)
{
    // this is where you would store or otherwise do something with the image
    up_image_size += data_len;

    //int box_width = 16;
    //int total = 0;
    //while(total < data_len){
        //for(int i = 0; i < box_width; i++){
            //printf("0x%02X ", data[total]);
            //total++;
        //}
        //printf("\n");
    //}
    // Check that all bytes have the lower four bytes 0
    for(int i = 0; i < data_len; i++){
        TEST_ASSERT_EQUAL(0, data[i] & 0x0f);
    }
}

TEST_CASE("Not connected", "[initialization]")
{
    esp_err_t err = R502.init(UART_NUM_1, PIN_NC_1, PIN_NC_2, PIN_NC_3);
    TEST_ESP_OK(err);
    std::array<uint8_t, 4> pass = {0x00, 0x00, 0x00, 0x00};
    R502_conf_code_t conf_code = R502_fail;
    err = R502.vfy_pass(pass, conf_code);
    TEST_ASSERT_EQUAL(ESP_ERR_NOT_FOUND, err);
}

TEST_CASE("Reinitialize", "[initialization]")
{
    esp_err_t err = R502.init(UART_NUM_1, PIN_NC_1, PIN_NC_2, PIN_NC_3);
    TEST_ESP_OK(err);
    err = R502.deinit();
    TEST_ESP_OK(err);
    err = R502.init(UART_NUM_1, PIN_TXD, PIN_RXD, PIN_IRQ);
    TEST_ESP_OK(err);
    std::array<uint8_t, 4> pass = {0x00, 0x00, 0x00, 0x00};
    R502_conf_code_t conf_code = R502_fail;
    err = R502.vfy_pass(pass, conf_code);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(conf_code, R502_ok);
}

TEST_CASE("VfyPwd", "[system command]")
{
    esp_err_t err = R502.init(UART_NUM_1, PIN_TXD, PIN_RXD, PIN_IRQ);
    TEST_ESP_OK(err);
    std::array<uint8_t, 4> pass = {0x00, 0x00, 0x00, 0x00};
    R502_conf_code_t conf_code = R502_fail;
    err = R502.vfy_pass(pass, conf_code);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);
    pass[3] = 0x01;
    err = R502.vfy_pass(pass, conf_code);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_err_wrong_pass, conf_code);
    err = R502.deinit();
    TEST_ESP_OK(err);
}

TEST_CASE("ReadSysPara", "[system command]")
{
    esp_err_t err = R502.init(UART_NUM_1, PIN_TXD, PIN_RXD, PIN_IRQ);
    TEST_ESP_OK(err);
    R502_sys_para_t sys_para;
    R502_conf_code_t conf_code;
    err = R502.read_sys_para(conf_code, sys_para);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(R502.get_module_address(), 
        sys_para.device_address, 4);
    printf("status_register %d\n", sys_para.status_register);
    printf("system_identifier_code %d\n", sys_para.system_identifier_code);
    printf("finger_library_size %d\n", sys_para.finger_library_size);
    printf("security_level %d\n", sys_para.security_level);
    printf("device_address %x%x%x%x\n", sys_para.device_address[0], 
        sys_para.device_address[1], sys_para.device_address[2], 
        sys_para.device_address[3]);
    printf("data_package_length %d\n", sys_para.data_package_length);
    printf("baud_setting %d\n", sys_para.baud_setting);
}

TEST_CASE("SetBaudRate", "[system command]")
{
    esp_err_t err = R502.init(UART_NUM_1, PIN_TXD, PIN_RXD, PIN_IRQ);
    TEST_ESP_OK(err);

    R502_conf_code_t conf_code = R502_fail;
    R502_baud_t current_baud = R502_baud_57600;
    R502_sys_para_t sys_para;
    err = R502.read_sys_para(conf_code, sys_para);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);
    current_baud = sys_para.baud_setting;

    // 9600
    err = R502.set_baud_rate(R502_baud_9600, conf_code);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);

    // 19200
    err = R502.set_baud_rate(R502_baud_19200, conf_code);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);

    // 38400
    err = R502.set_baud_rate(R502_baud_38400, conf_code);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);

    // 57600
    err = R502.set_baud_rate(R502_baud_57600, conf_code);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);

    // 115200
    err = R502.set_baud_rate(R502_baud_115200, conf_code);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);

    // error baud rate
    conf_code = R502_fail;
    err = R502.set_baud_rate((R502_baud_t)9600, conf_code);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, err);
    TEST_ASSERT_EQUAL(R502_fail, conf_code);

    // rest baud rate to what it was before testing
    err = R502.set_baud_rate(current_baud, conf_code);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);
}

TEST_CASE("SetSecurityLevel", "[system command]")
{
    esp_err_t err = R502.init(UART_NUM_1, PIN_TXD, PIN_RXD, PIN_IRQ);
    TEST_ESP_OK(err);

    R502_conf_code_t conf_code = R502_fail;
    int current_security_level = 0;
    R502_sys_para_t sys_para;
    err = R502.read_sys_para(conf_code, sys_para);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);
    current_security_level = sys_para.security_level;

    for(int sec_lev = 1; sec_lev <= 5; sec_lev++){
        err = R502.set_security_level(sec_lev, conf_code);
        TEST_ESP_OK(err);
        TEST_ASSERT_EQUAL(R502_ok, conf_code);
        err = R502.read_sys_para(conf_code, sys_para);
        TEST_ESP_OK(err);
        TEST_ASSERT_EQUAL(R502_ok, conf_code);
        TEST_ASSERT_EQUAL(sys_para.security_level, sec_lev);
    }

    conf_code = R502_fail;
    err = R502.set_security_level(6, conf_code);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, err);
    TEST_ASSERT_EQUAL(R502_fail, conf_code);

    err = R502.set_security_level(current_security_level, conf_code);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);
}

TEST_CASE("SetDataPackageLength", "[system command]")
{
    // Can only set to 128 and 256 bytes, not 32 or 64
    esp_err_t err = R502.init(UART_NUM_1, PIN_TXD, PIN_RXD, PIN_IRQ);
    TEST_ESP_OK(err);

    R502_conf_code_t conf_code = R502_fail;
    R502_data_len_t current_data_length = R502_data_len_32;
    R502_sys_para_t sys_para;
    err = R502.read_sys_para(conf_code, sys_para);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);
    current_data_length = sys_para.data_package_length;

    err = R502.set_data_package_length(R502_data_len_64, conf_code);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);
    err = R502.read_sys_para(conf_code, sys_para);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);
    // this should be 64bytes, but my module only goes down to 128
    // Leaving assert in so if it changes with another module I will know
    TEST_ASSERT_EQUAL(R502_data_len_128, sys_para.data_package_length);

    err = R502.set_data_package_length(R502_data_len_256, conf_code);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);
    err = R502.read_sys_para(conf_code, sys_para);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);
    TEST_ASSERT_EQUAL(R502_data_len_256, sys_para.data_package_length);

    err = R502.set_data_package_length(R502_data_len_128, conf_code);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);
    err = R502.read_sys_para(conf_code, sys_para);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);
    TEST_ASSERT_EQUAL(R502_data_len_128, sys_para.data_package_length);

    // reset to starting data length
    err = R502.set_data_package_length(current_data_length, conf_code);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);
}

TEST_CASE("TemplateNum", "[system command]")
{
    esp_err_t err = R502.init(UART_NUM_1, PIN_TXD, PIN_RXD, PIN_IRQ);
    TEST_ESP_OK(err);
    R502_conf_code_t conf_code;
    uint16_t template_num = 65535;
    err = R502.template_num(conf_code, template_num);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);
}

TEST_CASE("GenImage", "[fingerprint processing command]")
{
    esp_err_t err = R502.init(UART_NUM_1, PIN_TXD, PIN_RXD, PIN_IRQ);
    TEST_ESP_OK(err);
    R502_conf_code_t conf_code;
    err = R502.gen_image(conf_code);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_err_no_finger, conf_code);
}

TEST_CASE("GenImageSuccess", "[fingerprint processing command][userInput]")
{
    esp_err_t err = R502.init(UART_NUM_1, PIN_TXD, PIN_RXD, PIN_IRQ);
    TEST_ESP_OK(err);
    R502_conf_code_t conf_code;

    wait_with_message("Place finger on sensor, then press enter\n");

    err = R502.gen_image(conf_code);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);
}

TEST_CASE("UpImage", "[fingerprint processing command]")
{
    esp_err_t err = R502.init(UART_NUM_1, PIN_TXD, PIN_RXD, PIN_IRQ);
    TEST_ESP_OK(err);

    // read and save current data_package_length and baud rate
    R502_sys_para_t sys_para;
    R502_conf_code_t conf_code;
    err = R502.read_sys_para(conf_code, sys_para);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);
    R502_data_len_t starting_data_len = sys_para.data_package_length;

    // Attempt up_image without setting the callback
    err = R502.up_image(starting_data_len, conf_code);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, err);

    // reset counter to measure size of the uploaded image
    R502.set_up_image_cb(up_image_callback);

    up_image_size = 0;

    err = R502.up_image(starting_data_len, conf_code);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);
    TEST_ASSERT_EQUAL(image_size, up_image_size);
}

TEST_CASE("UpImageAdvanced", "[fingerprint processing command]")
{
    esp_err_t err = R502.init(UART_NUM_1, PIN_TXD, PIN_RXD, PIN_IRQ);
    TEST_ESP_OK(err);
    R502.set_up_image_cb(up_image_callback);

    // read and save current data_package_length and baud rate
    R502_sys_para_t sys_para;
    R502_conf_code_t conf_code;
    err = R502.read_sys_para(conf_code, sys_para);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);
    R502_data_len_t starting_data_len = sys_para.data_package_length;
    R502_baud_t starting_baud = sys_para.baud_setting;

    
    R502_baud_t test_bauds[5] = { R502_baud_9600, R502_baud_19200, 
        R502_baud_38400, R502_baud_57600, R502_baud_115200 };
    R502_data_len_t test_data_lens[2] = { R502_data_len_128, 
        R502_data_len_256 };

    // Loop through all bauds and data lengths to test all combinations
    for(int baud_i = 0; baud_i < 5; baud_i++){
        ESP_LOGI(TAG, "Test baud %d", test_bauds[baud_i]);
        err = R502.set_baud_rate(test_bauds[baud_i], conf_code);
        TEST_ESP_OK(err);
        TEST_ASSERT_EQUAL(R502_ok, conf_code);

        for(int data_i = 0; data_i < 2; data_i++){
            ESP_LOGI(TAG, "Test data len %d", test_data_lens[data_i]);
            // reset counter to measure size of the uploaded image
            up_image_size = 0;

            err = R502.set_data_package_length(test_data_lens[data_i], 
                conf_code);
            TEST_ESP_OK(err);
            TEST_ASSERT_EQUAL(R502_ok, conf_code);

            err = R502.up_image(test_data_lens[data_i], conf_code);
            TEST_ESP_OK(err);
            TEST_ASSERT_EQUAL(R502_ok, conf_code);
            TEST_ASSERT_EQUAL(image_size, up_image_size);
        }
    }

    // reset the data package length and baud settings to default
    err = R502.set_data_package_length(starting_data_len, conf_code);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);
    err = R502.set_baud_rate(starting_baud, conf_code);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, conf_code);
}