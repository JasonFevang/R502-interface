#include "R502Interface.hpp"

const char *R502Interface::TAG = "R502";

esp_err_t R502Interface::init(uart_port_t _uart_num, gpio_num_t _pin_txd, gpio_num_t _pin_rxd, gpio_num_t _pin_irq){
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
    esp_err_t err = uart_param_config(uart_num, &uart_config);
    if(err) return err;
    err = uart_set_pin(uart_num, pin_txd, pin_rxd, pin_rts, pin_cts);
    if(err) return err;
    err = uart_driver_install(uart_num, BUF_SIZE * 2, 0, 0, NULL, 0);
    if(err) return err;

    err = gpio_set_direction(pin_irq, GPIO_MODE_INPUT);
    if(err) return err;
    err = gpio_set_intr_type(pin_irq, GPIO_INTR_POSEDGE);
    if(err) return err;
    err = gpio_intr_enable(pin_irq);
    if(err) return err;
    err = gpio_install_isr_service(0);
    if(err) return err;
    err = gpio_isr_handler_add(pin_irq, irq_intr, this);
    if(err) return err;

    // wait for R502 to prepare itself
    vTaskDelay(200 / portTICK_PERIOD_MS);
    return ESP_OK;
}

esp_err_t R502Interface::deinit(){
    esp_err_t err = uart_driver_delete(uart_num);
    if(err) return err;
    err = gpio_isr_handler_remove(pin_irq);
    if(err) return err;
    gpio_uninstall_isr_service();
    return ESP_OK;
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

void R502Interface::fill_checksum(R502_DataPackage_t &package){
    int length = conv_8_to_16(package.length);
    int sum = package.pid + package.length[0] + package.length[1];
    uint8_t *itr = (uint8_t *)&package.data;
    for(int i = 0; i < length; i++){
        sum += *itr;
        itr++;
    }
    *itr = (sum >> 8) & 0xff;
    itr++;
    *itr = sum & 0xff;
}

bool R502Interface::verify_checksum(R502_DataPackage_t &package){
    int length = conv_8_to_16(package.length);
    int sum = package.pid + package.length[0] + package.length[1];
    uint8_t *itr = (uint8_t *)&package.data;
    for(int i = 0; i < length; i++){
        sum += *itr;
        itr++;
    }
    int checksum = *itr << 8;
    itr++;
    checksum += *itr;

    return (sum == checksum);
}

void R502Interface::set_headers(R502_DataPackage_t &package, R502_pid_t pid, uint16_t length){
    memcpy(package.start, start, sizeof(start));
    memcpy(package.adder, adder, sizeof(adder));

    package.pid = pid;

    package.length[0] = (length >> 8) & 0xff;
    package.length[1] = length & 0xff;
}

esp_err_t R502Interface::vfyPass(const std::array<uint8_t, 4> &pass, R502_conf_code_t &res){
    R502_DataPackage_t package;
    R502_VfyPwd_t *data = &package.data.vfy_pwd;

    set_headers(package, R502_pid_command, sizeof(R502_VfyPwd_t));
    data->instr_code = R502_ic_vfy_pwd;
    for(int i = 0; i < 4; i++){
        data->password[i] = pass[i];
    }
    fill_checksum(package);
    //int sum = package.pid + package.length[0] + package.length[1];
    //sum += package.data.vfy_pwd.instr_code;
    //for (int i = 0; i < 4; i++){
        //sum+= package.data.vfy_pwd.password[i];
    //}
    //package.data.vfy_pwd.checksum[0] = (sum >> 8) & 0xff;
    //package.data.vfy_pwd.checksum[1] = sum & 0xff;

    //printf("transferred data\n");
    //for(int i = 0; i < sizeof(data); i++){
        //printf("0x%X 0x%X\n", data[i], *((uint8_t *)(&package) + i));
    //}

    R502_DataPackage_t receivePackage = { 0 };
    esp_err_t err = sendCommandPackage(package, receivePackage);
    if(err != ESP_OK){
        // resend logic here
    }
    
    // delay

    //uart_flush(UART_NUM_1);
    uart_write_bytes(UART_NUM_1, (char *)&package, sizeof(package) - sizeof(package.data) + (package.length[0] << 8) + package.length[1]);

    //busy_delay(5000);

    //uart_write_bytes(UART_NUM_1, (char *)data, sizeof(data));

    //busy_delay(50000);
    vTaskDelay(10 / portTICK_PERIOD_MS);

    int len = uart_read_bytes(UART_NUM_1, (uint8_t *)&receivePackage, sizeof(receivePackage), 20 / portTICK_RATE_MS);

    // verify response
    // Do the checksum first, this kinda verifies all the others in one go, faster
    // checksum
    if(!verify_checksum(receivePackage)){
        return ESP_ERR_INVALID_CRC;
    }
    //sum = receivePackage.pid + receivePackage.length[0] + receivePackage.length[1];
    //sum += receivePackage.data.default_ack.conf_code;
    //if((receivePackage.data.default_ack.checksum[0] << 8) + receivePackage.data.default_ack.checksum[1] != sum){
        //return ESP_ERR_INVALID_CRC;
    //}

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
}

esp_err_t R502Interface::sendCommandPackage(const R502_DataPackage_t &package, R502_DataPackage_t &receivePackage){
    int len = uart_write_bytes(UART_NUM_1, (char *)&package, sizeof(package) - sizeof(package.data) + conv_8_to_16(package.length));
    if(len == -1){
        // parameter error
        return ESP_ERR_INVALID_STATE;
    }
    else if(len != sizeof(package)){
        // not all data transferred
        return ESP_ERR_INVALID_SIZE;
    }

    //busy_delay(5000);

    //busy_delay(50000);
    vTaskDelay(10 / portTICK_PERIOD_MS);

    len = uart_read_bytes(UART_NUM_1, (uint8_t *)&receivePackage, sizeof(receivePackage), read_delay / portTICK_RATE_MS);
    if(len == -1){
        // parameter error
        return ESP_ERR_INVALID_STATE;
    }
    if(len != sizeof(receivePackage)){
        return ESP_ERR_INVALID_RESPONSE;
    }
}

uint16_t R502Interface::conv_8_to_16(const uint8_t in[2]){
    return (in[0] << 8) + in[1];
}