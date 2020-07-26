extern "C"{
    void app_main(void);
}

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"

#define PIN_TXD  (GPIO_NUM_4)
#define PIN_RXD  (GPIO_NUM_5)
#define PIN_IRQ  (GPIO_NUM_13)
#define PIN_RTS  (UART_PIN_NO_CHANGE)
#define PIN_CTS  (UART_PIN_NO_CHANGE)

#define BUF_SIZE (1024)

bool interrupt = false;
typedef void (* gpio_intr_handler_fn_t)(uint32_t intr_mask, bool high, void *arg);
void IRAM_ATTR irq_intr(void *arg){
    //printf("irq\n");
    interrupt = true;
    
}

void app_main()
{
    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = {
        .baud_rate = 57600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
        .use_ref_tick = false
    };
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, PIN_TXD, PIN_RXD, PIN_RTS, PIN_CTS);
    uart_driver_install(UART_NUM_1, BUF_SIZE * 2, 0, 0, NULL, 0);

    gpio_set_direction(PIN_IRQ, GPIO_MODE_INPUT);
    gpio_set_intr_type(PIN_IRQ, GPIO_INTR_POSEDGE);
    gpio_intr_enable(PIN_IRQ);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(PIN_IRQ, irq_intr, nullptr);

    // wait for R502 to prepare itself
    vTaskDelay(500 / portTICK_PERIOD_MS);

    uint8_t data[16] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x07, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1B };
    printf("transferred data\n");
    for(int i = 0; i < sizeof(data); i++){
        printf("0x%X\n", data[i]);
    }
    uart_write_bytes(UART_NUM_1, (char *)data, sizeof(data));
    while (1) {
        if(interrupt){
            interrupt = false;
            printf("irq\n");
        }
        // Read data from the UART
        int len = uart_read_bytes(UART_NUM_1, data, BUF_SIZE, 20 / portTICK_RATE_MS);
        if(len > 0){
            printf("response\n");
            for(int i = 0; i < len; i++){
                printf("0x%x\n", data[i]);
            }
            printf("\n");
        }
    }
}
