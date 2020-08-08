#include <limits.h>
#include "unity.h"
#include <array>
#include "R502Interface.hpp"
#include "esp_err.h"
#include "esp_log.h"

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

TEST_CASE("VfyPwd", "[command]")
{
    esp_err_t err = R502.init(UART_NUM_1, PIN_TXD, PIN_RXD, PIN_IRQ);
    TEST_ESP_OK(err);
    std::array<uint8_t, 4> pass = {0x00, 0x00, 0x00, 0x00};
    R502_conf_code_t res = R502_fail;
    err = R502.vfy_pass(pass, res);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_ok, res);
    pass[3] = 0x01;
    err = R502.vfy_pass(pass, res);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(R502_err_wrong_pass, res);
    err = R502.deinit();
    TEST_ESP_OK(err);
}

TEST_CASE("Not connected", "[system]")
{
    esp_err_t err = R502.init(UART_NUM_1, PIN_NC_1, PIN_NC_2, PIN_NC_3);
    TEST_ESP_OK(err);
    std::array<uint8_t, 4> pass = {0x00, 0x00, 0x00, 0x00};
    R502_conf_code_t res = R502_fail;
    err = R502.vfy_pass(pass, res);
    TEST_ASSERT_EQUAL(ESP_ERR_NOT_FOUND, err);
}
TEST_CASE("Reinitialize", "[system]")
{
    esp_err_t err = R502.init(UART_NUM_1, PIN_NC_1, PIN_NC_2, PIN_NC_3);
    TEST_ESP_OK(err);
    err = R502.deinit();
    TEST_ESP_OK(err);
    err = R502.init(UART_NUM_1, PIN_TXD, PIN_RXD, PIN_IRQ);
    TEST_ESP_OK(err);
    std::array<uint8_t, 4> pass = {0x00, 0x00, 0x00, 0x00};
    R502_conf_code_t res = R502_fail;
    err = R502.vfy_pass(pass, res);
    TEST_ESP_OK(err);
    TEST_ASSERT_EQUAL(res, R502_ok);
}