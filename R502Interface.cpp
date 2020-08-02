#include "R502Interface.hpp"


void R502Interface::init(){
    // do nothing
}

void IRAM_ATTR R502Interface::irq_intr(void *arg){
    R502Interface *me = (R502Interface *)arg;
    me->interrupt = true;
    
}

void R502Interface::busy_delay(int64_t microseconds){
    // wait
    int64_t time_start = esp_timer_get_time();
    while(esp_timer_get_time() < time_start + microseconds);
}

void R502Interface::test1(){
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
    gpio_isr_handler_add(PIN_IRQ, irq_intr, this);

    // wait for R502 to prepare itself
    vTaskDelay(500 / portTICK_PERIOD_MS);

    int len_err = 0;
    int data_err = 0;

    const uint8_t data[16] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x07, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1B };
    //printf("transferred data\n");
    //for(int i = 0; i < sizeof(data); i++){
        //printf("0x%X\n", data[i]);
    //}

    uint8_t receive[BUF_SIZE] = { 0 };
    uint8_t expected_receive[12] = {0xef, 0x01, 0xff, 0xff, 0xff, 0xff, 0x07, 0x00, 0x03, 0x00, 0x00, 0x0a};

    uart_flush(UART_NUM_1);
    uart_write_bytes(UART_NUM_1, (char *)data, sizeof(data));

    //busy_delay(5000);

    //uart_write_bytes(UART_NUM_1, (char *)data, sizeof(data));

    //busy_delay(50000);
    vTaskDelay(10 / portTICK_PERIOD_MS);

    int len = uart_read_bytes(UART_NUM_1, receive, BUF_SIZE, 20 / portTICK_RATE_MS);

    if(len != 12){
        len_err++;
    }
    else{
        int res = memcmp(receive, expected_receive, sizeof(expected_receive));
        if(res != 0){
            data_err++;
            printf("Received\tExpected\n");
            for(int i = 0; i < len; i++){
                printf("0x%X\t\t0x%X\n", receive[i], expected_receive[i]);
            }
        }
    }

    printf("%d len_errs\n", len_err);
    printf("%d data_errs\n", data_err);

    //while (1) {
        //if(interrupt){
            //interrupt = false;
            //printf("irq\n");
        //}
        //// Read data from the UART
        //int len = uart_read_bytes(UART_NUM_1, data, BUF_SIZE, 20 / portTICK_RATE_MS);
        //if(len > 0){
            //printf("response\n");
            //for(int i = 0; i < len; i++){
                //printf("0x%x\n", data[i]);
            //}
            //printf("\n");
        //}
    //}
}