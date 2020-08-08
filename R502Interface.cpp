#include "R502Interface.hpp"

const char *R502Interface::TAG = "R502";

void R502Interface::init(uart_port_t _uart_num, gpio_num_t _pin_txd, gpio_num_t _pin_rxd, gpio_num_t _pin_irq){
    pin_txd = _pin_txd;
    pin_rxd = _pin_rxd;
    pin_irq = _pin_irq;
    uart_num = _uart_num;
    pin_rts = (gpio_num_t)UART_PIN_NO_CHANGE;
    pin_cts = (gpio_num_t)UART_PIN_NO_CHANGE;

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
    uart_param_config(uart_num, &uart_config);
    uart_set_pin(uart_num, pin_txd, pin_rxd, pin_rts, pin_cts);
    uart_driver_install(uart_num, BUF_SIZE * 2, 0, 0, NULL, 0);

    gpio_set_direction(pin_irq, GPIO_MODE_INPUT);
    gpio_set_intr_type(pin_irq, GPIO_INTR_POSEDGE);
    gpio_intr_enable(pin_irq);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(pin_irq, irq_intr, this);

    // wait for R502 to prepare itself
    vTaskDelay(200 / portTICK_PERIOD_MS);
}

void R502Interface::deinit(){
    uart_driver_delete(uart_num);
}

void IRAM_ATTR R502Interface::irq_intr(void *arg){
    R502Interface *me = (R502Interface *)arg;
    me->interrupt++;
}

void R502Interface::busy_delay(int64_t microseconds){
    // wait
    int64_t time_start = esp_timer_get_time();
    while(esp_timer_get_time() < time_start + microseconds);
}

esp_err_t R502Interface::vfyPass(const std::array<uint8_t, 4> &pass, R502_conf_code_t &res){
    R502_DataPackage_t packet;
    packet.start[0] = 0xEF;
    packet.start[1] = 0x01;

    packet.adder[0] = 0xFF;
    packet.adder[1] = 0xFF;
    packet.adder[2] = 0xFF;
    packet.adder[3] = 0xFF;

    packet.pid = R502_pid_command;

    packet.length[0] = (sizeof(R502_VfyPwd_t) >> 8) & 0xff;
    packet.length[1] = sizeof(R502_VfyPwd_t) & 0xff;

    packet.data.vfy_pwd.instr_code = R502_ic_vfy_pwd;
    for(int i = 0; i < 4; i++){
        packet.data.vfy_pwd.password[i] = pass[i];
    }
    int sum = packet.pid + packet.length[0] + packet.length[1];
    sum += packet.data.vfy_pwd.instr_code;
    for (int i = 0; i < 4; i++){
        sum+= packet.data.vfy_pwd.password[i];
    }
    packet.data.vfy_pwd.checksum[0] = (sum >> 8) & 0xff;
    packet.data.vfy_pwd.checksum[1] = sum & 0xff;

    int len_err = 0;
    int data_err = 0;

    //printf("transferred data\n");
    //for(int i = 0; i < sizeof(data); i++){
        //printf("0x%X 0x%X\n", data[i], *((uint8_t *)(&packet) + i));
    //}

    uint8_t receive[BUF_SIZE] = { 0 };
    R502_DataPackage_t receivePackage;
    uint8_t expected_receive[12] = {0xef, 0x01, 0xff, 0xff, 0xff, 0xff, 0x07, 0x00, 0x03, 0x00, 0x00, 0x0a};

    uart_flush(UART_NUM_1);
    //ESP_LOGI(TAG, "uart_write_bytes(UART_NUM_1, (char *)&packet, %d);", sizeof(packet) - sizeof(packet.data) + (packet.length[0] << 8) + packet.length[1]);
    uart_write_bytes(UART_NUM_1, (char *)&packet, sizeof(packet) - sizeof(packet.data) + (packet.length[0] << 8) + packet.length[1]);

    //busy_delay(5000);

    //uart_write_bytes(UART_NUM_1, (char *)data, sizeof(data));

    //busy_delay(50000);
    vTaskDelay(10 / portTICK_PERIOD_MS);

    int len = uart_read_bytes(UART_NUM_1, (uint8_t *)&receivePackage, sizeof(receivePackage), 20 / portTICK_RATE_MS);

    // verify response
    // Do the checksum first, this kinda verifies all the others in one go, faster
    // checksum
    sum = receivePackage.pid + receivePackage.length[0] + receivePackage.length[1];
    sum += receivePackage.data.default_ack.conf_code;
    if((receivePackage.data.default_ack.checksum[0] << 8) + receivePackage.data.default_ack.checksum[1] != sum){
        return ESP_ERR_INVALID_CRC;
    }

    // start
    if(receivePackage.start[0] != 0xEF || receivePackage.start[1] != 0x01){
        return ESP_ERR_INVALID_RESPONSE;
    }

    // module address
    if(receivePackage.adder[0] != 0xFF || receivePackage.adder[1] != 0xFF || receivePackage.adder[2] != 0xFF || receivePackage.adder[3] != 0xFF){
        return ESP_ERR_INVALID_RESPONSE;
    }

    // pid
    if(receivePackage.pid != R502_pid_ack){
        return ESP_ERR_INVALID_RESPONSE;
    }

    // length
    if((receivePackage.length[0] << 8) + receivePackage.length[1] != sizeof(R502_Ack_t)){
        return ESP_ERR_INVALID_RESPONSE;
    }

    res = (R502_conf_code_t)receivePackage.data.default_ack.conf_code;
    return ESP_OK;


    // read out confirmation code

    //if(len != 12){
        //len_err++;
    //}
    //else{
        //int res = memcmp(receive, expected_receive, sizeof(expected_receive));
        //if(res != 0){
            //data_err++;
            //printf("Received\tExpected\n");
            //for(int i = 0; i < len; i++){
                //printf("0x%X\t\t0x%X\n", receive[i], expected_receive[i]);
            //}
        //}
    //}

    //printf("%d len_errs\n", len_err);
    //printf("%d data_errs\n", data_err);

    //while (1) {
        //if(interrupt > 0){
            //interrupt = 0;
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