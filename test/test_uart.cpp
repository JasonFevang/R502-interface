#include <limits.h>
#include "unity.h"
#include <array>
#include "R502Interface.hpp"

#define PIN_TXD  (GPIO_NUM_4)
#define PIN_RXD  (GPIO_NUM_5)
#define PIN_IRQ  (GPIO_NUM_13)
#define PIN_RTS  (UART_PIN_NO_CHANGE)
#define PIN_CTS  (UART_PIN_NO_CHANGE)

TEST_CASE("VfyPwd", "[command]")
{
    R502Interface R502;
    esp_err_t err = R502.init(UART_NUM_1, PIN_TXD, PIN_RXD, PIN_IRQ);
    TEST_ASSERT_EQUAL(err, ESP_OK);
    std::array<uint8_t, 4> pass = {0x00, 0x00, 0x00, 0x00};
    R502_conf_code_t res = R502_fail;
    err = R502.vfyPass(pass, res);
    TEST_ASSERT_EQUAL(err, ESP_OK);
    TEST_ASSERT_EQUAL(res, R502_ok);
    pass[3] = 0x01;
    err = R502.vfyPass(pass, res);
    TEST_ASSERT_EQUAL(err, ESP_OK);
    TEST_ASSERT_EQUAL(res, R502_err_wrong_pass);
    err = R502.deinit();
    TEST_ASSERT_EQUAL(err, ESP_OK);
}

TEST_CASE("Init deinit", "[system]")
{
    R502Interface R502;
    esp_err_t err = R502.init(UART_NUM_1, GPIO_NUM_5, GPIO_NUM_16, GPIO_NUM_14);
    TEST_ASSERT_EQUAL(err, ESP_OK);
    err = R502.deinit();
    TEST_ASSERT_EQUAL(err, ESP_OK);
    err = R502.init(UART_NUM_1, PIN_TXD, PIN_RXD, PIN_IRQ);
    TEST_ASSERT_EQUAL(err, ESP_OK);
    std::array<uint8_t, 4> pass = {0x00, 0x00, 0x00, 0x00};
    R502_conf_code_t res = R502_fail;
    err = R502.vfyPass(pass, res);
    TEST_ASSERT_EQUAL(err, ESP_OK);
    TEST_ASSERT_EQUAL(res, R502_ok);
}