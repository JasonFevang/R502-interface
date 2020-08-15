#include "R502Interface.hpp"

const char *R502Interface::TAG = "R502";

esp_err_t R502Interface::init(uart_port_t _uart_num, gpio_num_t _pin_txd, 
    gpio_num_t _pin_rxd, gpio_num_t _pin_irq)
{
    if(initialized){
        return ESP_OK;
    }
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
    initialized = true;
    return ESP_OK;
}

esp_err_t R502Interface::deinit()
{
    if(initialized){
        initialized = false;
        esp_err_t err_uart_driver = uart_driver_delete(uart_num);
        esp_err_t err_isr_remove = gpio_isr_handler_remove(pin_irq);
        gpio_uninstall_isr_service();
        if(err_uart_driver) return err_uart_driver;
        if(err_isr_remove) return err_isr_remove;
    }
    return ESP_OK;
}

void IRAM_ATTR R502Interface::irq_intr(void *arg)
{
    R502Interface *me = (R502Interface *)arg;
    me->interrupt++;
}

uint8_t *R502Interface::get_module_address(){
    return adder;
}

esp_err_t R502Interface::vfy_pass(const std::array<uint8_t, 4> &pass, 
    R502_conf_code_t &res)
{
    R502_DataPackage_t pkg;
    R502_VfyPwd_t *data = &pkg.data.vfy_pwd;

    // Fill package
    set_headers(pkg, R502_pid_command, sizeof(R502_VfyPwd_t));
    data->instr_code = R502_ic_vfy_pwd;
    for(int i = 0; i < 4; i++){
        data->password[i] = pass[i];
    }
    fill_checksum(pkg);

    // Send package, get response
    R502_DataPackage_t receive_pkg;
    esp_err_t err = send_command_package(pkg, receive_pkg);
    if(err) return err;
    
    // Return result
    res = (R502_conf_code_t)receive_pkg.data.general_ack.conf_code;
    return ESP_OK;
}

esp_err_t R502Interface::read_sys_para(R502_conf_code_t &res, 
    R502_sys_para_t &sys_para)
{
    R502_DataPackage_t pkg;
    R502_GeneralCommand_t *data = &pkg.data.general;

    // Fill package
    set_headers(pkg, R502_pid_command, sizeof(R502_GeneralCommand_t));
    data->instr_code = R502_ic_read_sys_para;
    fill_checksum(pkg);

    // Send package, get response
    R502_DataPackage_t receive_pkg;
    R502_ReadSysParaAck_t *receive_data = &receive_pkg.data.read_sys_para_ack;
    esp_err_t err = send_command_package(pkg, receive_pkg);
    if(err) return err;

    // Return result
    res = (R502_conf_code_t)receive_data->conf_code;

    if(sys_para.system_identifier_code != system_identifier_code){
        ESP_LOGW(TAG, "sys_para system identifier is %d not %d", 
            sys_para.system_identifier_code, system_identifier_code);
    }

    sys_para.status_register = conv_8_to_16(receive_data->data + 0);
    sys_para.system_identifier_code = conv_8_to_16(receive_data->data + 2);
    sys_para.finger_library_size = conv_8_to_16(receive_data->data + 4);
    sys_para.security_level = conv_8_to_16(receive_data->data + 6);
    memcpy(sys_para.device_address, receive_data->data+8, 4);
    sys_para.data_packet_size = conv_8_to_16(receive_data->data + 12);
    sys_para.baud_setting = conv_8_to_16(receive_data->data + 14);

    return ESP_OK;
}

esp_err_t R502Interface::template_num(R502_conf_code_t &res, 
    uint16_t &template_num)
{
    R502_DataPackage_t pkg;
    R502_GeneralCommand_t *data = &pkg.data.general;

    // Fill package
    set_headers(pkg, R502_pid_command, sizeof(R502_GeneralCommand_t));
    data->instr_code = R502_ic_template_num;
    fill_checksum(pkg);

    // Send package, get response
    R502_DataPackage_t receive_pkg;
    R502_TemplateNumAck_t *receive_data = &receive_pkg.data.template_num_ack;
    esp_err_t err = send_command_package(pkg, receive_pkg);
    if(err) return err;

    // Return result
    res = (R502_conf_code_t)receive_data->conf_code;
    template_num = conv_8_to_16(receive_data->template_num);

    return ESP_OK;
}

esp_err_t R502Interface::send_command_package(const R502_DataPackage_t &pkg,
    R502_DataPackage_t &receive_pkg)
{
    int len = uart_write_bytes(UART_NUM_1, (char *)&pkg, package_length(pkg));
    if(len == -1){
        ESP_LOGE(TAG, "uart write error, parameter error");
        return ESP_ERR_INVALID_STATE;
    }
    else if(len != package_length(pkg)){
        // not all data transferred
        ESP_LOGE(TAG, "uart write error, wrong number of bytes written");
        return ESP_ERR_INVALID_SIZE;
    }

    len = uart_read_bytes(UART_NUM_1, (uint8_t *)&receive_pkg, 
        sizeof(receive_pkg), read_delay / portTICK_RATE_MS);

    if(len == -1){
        ESP_LOGE(TAG, "uart read error, parameter error");
        return ESP_ERR_INVALID_STATE;
    }
    else if(len == 0){
        ESP_LOGE(TAG, "uart read error, R502 not found");
        return ESP_ERR_NOT_FOUND;
    }

    // Verify response
    if(!verify_checksum(receive_pkg)){
        return ESP_ERR_INVALID_CRC;
    }
    return verify_headers(receive_pkg, R502_pid_ack, len - 9);
}

void R502Interface::busy_delay(int64_t microseconds)
{
    // wait
    int64_t time_start = esp_timer_get_time();
    while(esp_timer_get_time() < time_start + microseconds);
}

void R502Interface::fill_checksum(R502_DataPackage_t &package)
{
    int data_length = conv_8_to_16(package.length) - 2; // -2 for the 2 byte CS
    int sum = package.pid + package.length[0] + package.length[1];
    uint8_t *itr = (uint8_t *)&package.data;
    for(int i = 0; i < data_length; i++){
        sum += *itr++;
    }
    // Now itr is pointing to the first checksum byte
    *itr = (sum >> 8) & 0xff;
    *++itr = sum & 0xff;
}

bool R502Interface::verify_checksum(const R502_DataPackage_t &package)
{
    int data_length = conv_8_to_16(package.length) - 2; // -2 for the 2 byte CS
    int sum = package.pid + package.length[0] + package.length[1];
    uint8_t *itr = (uint8_t *)&package.data;
    for(int i = 0; i < data_length; i++){
        sum += *itr++;
    }
    int checksum = *itr << 8;
    checksum += *++itr;

    return (sum == checksum);
}

esp_err_t R502Interface::verify_headers(const R502_DataPackage_t &pkg, 
    R502_pid_t pid, uint16_t length)
{
    // start
    if(memcmp(pkg.start, start, sizeof(start)) != 0){
        ESP_LOGE(TAG, "Response has invalid start");
        return ESP_ERR_INVALID_RESPONSE;
    }

    // module address
    if(memcmp(pkg.adder, adder, sizeof(adder)) != 0){
        ESP_LOGE(TAG, "Response has invalid adder");
        return ESP_ERR_INVALID_RESPONSE;
    }

    // pid
    if(pkg.pid != pid){
        ESP_LOGE(TAG, "Response has invalid pid");
        return ESP_ERR_INVALID_RESPONSE;
    }

    // length
    if(conv_8_to_16(pkg.length) != length){
        ESP_LOGE(TAG, "Response has invalid length");
        return ESP_ERR_INVALID_RESPONSE;
    }

    return ESP_OK;
}

void R502Interface::set_headers(R502_DataPackage_t &package, R502_pid_t pid,
    uint16_t length)
{
    memcpy(package.start, start, sizeof(start));
    memcpy(package.adder, adder, sizeof(adder));

    package.pid = pid;

    package.length[0] = (length >> 8) & 0xff;
    package.length[1] = length & 0xff;
}


uint16_t R502Interface::conv_8_to_16(const uint8_t in[2])
{
    return (in[0] << 8) + in[1];
}

void R502Interface::conv_16_to_8(const uint16_t in, uint8_t out[2])
{
    out[0] = (in >> 8) & 0xff;
    out[1] = in & 0xff;
}

uint16_t R502Interface::package_length(const R502_DataPackage_t &pkg){
    return sizeof(pkg) - sizeof(pkg.data) + conv_8_to_16(pkg.length);
}