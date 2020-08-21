#include "R502Interface.hpp"

const char *R502Interface::TAG = "R502";

esp_err_t R502Interface::init(uart_port_t _uart_num, gpio_num_t _pin_txd, 
    gpio_num_t _pin_rxd, gpio_num_t _pin_irq, 
    R502_baud_t _baud)
{
    if(initialized){
        return ESP_OK;
    }
    cur_baud = _baud;
    pin_txd = _pin_txd;
    pin_rxd = _pin_rxd;
    pin_irq = _pin_irq;
    uart_num = _uart_num;
    pin_rts = (gpio_num_t)UART_PIN_NO_CHANGE;
    pin_cts = (gpio_num_t)UART_PIN_NO_CHANGE;

    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = {
        .baud_rate = 9600*cur_baud,
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
    err = uart_driver_install(uart_num, 
        std::max<int>(sizeof(R502_DataPkg_t), min_uart_buffer_size), 0, 0, NULL,
        0);
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

        // reset up_image_cb
        up_image_cb = nullptr;
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

void R502Interface::set_up_image_cb(up_image_cb_t _up_image_cb){
    up_image_cb = _up_image_cb;
}

esp_err_t R502Interface::vfy_pass(const std::array<uint8_t, 4> &pass, 
    R502_conf_code_t &res)
{
    R502_DataPkg_t pkg;
    R502_VfyPwd_t *data = &pkg.data.vfy_pwd;

    // Fill package
    set_headers(pkg, R502_pid_command, sizeof(R502_VfyPwd_t));
    data->instr_code = R502_ic_vfy_pwd;
    for(int i = 0; i < 4; i++){
        data->password[i] = pass[i];
    }
    fill_checksum(pkg);

    // Send package, get response
    R502_DataPkg_t receive_pkg;
    R502_GeneralAck_t *receive_data = &receive_pkg.data.general_ack;
    esp_err_t err = send_command_package(pkg, receive_pkg, 
        sizeof(*receive_data));
    if(err) return err;
    
    // Return result
    res = (R502_conf_code_t)receive_data->conf_code;
    return ESP_OK;
}

esp_err_t R502Interface::set_sys_para(R502_para_num parameter_num, int value, 
    R502_conf_code_t &res)
{
    // validate input data
    esp_err_t err = ESP_OK;
    switch(parameter_num){
        case R502_para_num_baud_control:{
            if(value != 1 && value != 2 && value != 4 && value != 6 && 
                value != 12)
            {
                err = ESP_ERR_INVALID_ARG;
            }
            break;
        }
        case R502_para_num_security_level:{
            if(value != 1 && value != 2 && value != 3 && value != 4 && 
                value != 5)
            {
                err = ESP_ERR_INVALID_ARG;
            }
            break;
        }
        case R502_para_num_data_pkg_len:{
            if(value != 0 && value != 1 && value != 2 && value != 3){
                err = ESP_ERR_INVALID_ARG;
            }
            break;
        }
        default:{
            ESP_LOGE(TAG, "invalid parameter number, %d", parameter_num);
            return ESP_ERR_INVALID_ARG;
        }
    };
    if(err){
        ESP_LOGE(TAG, "invalid parameter value, %d. Param_num %d", value, 
            parameter_num);
        return err;
    }

    R502_DataPkg_t pkg;
    R502_SetSysPara_t *data = &pkg.data.set_sys_para;

    // Fill package
    set_headers(pkg, R502_pid_command, sizeof(R502_SetSysPara_t));
    data->instr_code = R502_ic_set_sys_para;
    data->parameter_number = parameter_num;
    data->contents = value;
    fill_checksum(pkg);

    // Send package, get response
    R502_DataPkg_t receive_pkg;
    R502_GeneralAck_t *receive_data = &receive_pkg.data.general_ack;
    err = send_command_package(pkg, receive_pkg, 
        sizeof(*receive_data));
    if(err) return err;

    // Return result
    res = (R502_conf_code_t)receive_data->conf_code;
    return ESP_OK;
}

esp_err_t R502Interface::set_baud_rate(R502_baud_t baud, R502_conf_code_t &res)
{
    esp_err_t err = set_sys_para(R502_para_num_baud_control, baud, res);
    if(err){
        ESP_LOGE(TAG, "set_sys_para err %s", esp_err_to_name(err));
        return err;
    }
    // remember the current baud rate
    cur_baud = baud;

    esp_err_t err_uart_driver = uart_driver_delete(uart_num);
    if(err_uart_driver){
        ESP_LOGE(TAG, "error deleting uart driver: %s",  
            esp_err_to_name(err));
    }
    // Reinitialize with the new baud rate
    uart_config_t uart_config = {
        .baud_rate = 9600*baud,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
        .use_ref_tick = false
    };
    err = uart_param_config(uart_num, &uart_config);
    if(err){
        ESP_LOGE(TAG, "error reconfiguring uart baud rate: %s",  
            esp_err_to_name(err));
    }

    err = uart_driver_install(uart_num, 
        std::max<int>(sizeof(R502_DataPkg_t), min_uart_buffer_size), 0, 0, NULL,
        0);
    if(err){
        ESP_LOGE(TAG, "error installing uart driver: %s",  
            esp_err_to_name(err));
    }
    return ESP_OK;
}

esp_err_t R502Interface::set_security_level(uint8_t security_level,
    R502_conf_code_t &res)
{
    esp_err_t err = set_sys_para(R502_para_num_security_level, 
        security_level, res);
    return err;
}

esp_err_t R502Interface::set_data_package_length(R502_data_len_t data_length,
    R502_conf_code_t &res)
{
    esp_err_t err = set_sys_para(R502_para_num_data_pkg_len, data_length, res);
    if(err) return err;
    return ESP_OK;
}

esp_err_t R502Interface::read_sys_para(R502_conf_code_t &res, 
    R502_sys_para_t &sys_para)
{
    R502_DataPkg_t pkg;
    R502_GeneralCommand_t *data = &pkg.data.general;

    // Fill package
    set_headers(pkg, R502_pid_command, sizeof(R502_GeneralCommand_t));
    data->instr_code = R502_ic_read_sys_para;
    fill_checksum(pkg);

    // Send package, get response
    R502_DataPkg_t receive_pkg;
    R502_ReadSysParaAck_t *receive_data = &receive_pkg.data.read_sys_para_ack;
    esp_err_t err = send_command_package(pkg, receive_pkg, 
        sizeof(*receive_data));
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
    sys_para.data_package_length = (R502_data_len_t)conv_8_to_16(receive_data->data + 12);
    sys_para.baud_setting = (R502_baud_t)conv_8_to_16(receive_data->data + 14);

    return ESP_OK;
}

esp_err_t R502Interface::template_num(R502_conf_code_t &res, 
    uint16_t &template_num)
{
    R502_DataPkg_t pkg;
    R502_GeneralCommand_t *data = &pkg.data.general;

    // Fill package
    set_headers(pkg, R502_pid_command, sizeof(R502_GeneralCommand_t));
    data->instr_code = R502_ic_template_num;
    fill_checksum(pkg);

    // Send package, get response
    R502_DataPkg_t receive_pkg;
    R502_TemplateNumAck_t *receive_data = &receive_pkg.data.template_num_ack;
    esp_err_t err = send_command_package(pkg, receive_pkg, 
        sizeof(*receive_data));
    if(err) return err;

    // Return result
    res = (R502_conf_code_t)receive_data->conf_code;
    template_num = conv_8_to_16(receive_data->template_num);

    return ESP_OK;
}

esp_err_t R502Interface::gen_image(R502_conf_code_t &res)
{
    R502_DataPkg_t pkg;
    R502_GeneralCommand_t *data = &pkg.data.general;

    // Fill package
    set_headers(pkg, R502_pid_command, sizeof(R502_GeneralCommand_t));
    data->instr_code = R502_ic_gen_img;
    fill_checksum(pkg);

    // Send package, get response
    R502_DataPkg_t receive_pkg;
    R502_GeneralAck_t *receive_data = &receive_pkg.data.general_ack;
    esp_err_t err = send_command_package(pkg, receive_pkg, 
        sizeof(*receive_data), read_delay_gen_image);
    if(err) return err;

    // Return result
    res = (R502_conf_code_t)receive_data->conf_code;
    return ESP_OK;
}

esp_err_t R502Interface::up_image(R502_data_len_t data_len, 
    R502_conf_code_t &res)
{
    R502_DataPkg_t pkg;
    R502_GeneralCommand_t *data = &pkg.data.general;

    if(!up_image_cb){
        ESP_LOGW(TAG, "up_image callback not set");
        return ESP_ERR_INVALID_STATE;
    }
    
    // TODO: Check stored parameters to see if the R502 has an image ready
    // to send. If not, still perform the transfer, but send a warning

    // Fill package
    set_headers(pkg, R502_pid_command, sizeof(R502_GeneralCommand_t));
    data->instr_code = R502_ic_up_image;
    fill_checksum(pkg);

    // Send package, get response
    R502_DataPkg_t receive_pkg;
    R502_GeneralAck_t *receive_data = &receive_pkg.data.general_ack;
    esp_err_t err = send_command_package(pkg, receive_pkg, 
        sizeof(*receive_data), read_delay_gen_image);
    if(err) return err;

    res = (R502_conf_code_t)receive_data->conf_code;
    if(res != R502_ok){ 
        // The esp side of things is ok, but the module isn't ready to send
        return ESP_OK;
    }
    int data_len_i = 0;
    switch(data_len){
        case R502_data_len_32:
            data_len_i = 32;
            break;
        case R502_data_len_64:
            data_len_i = 64;
            break;
        case R502_data_len_128:
            data_len_i = 128;
            break;
        case R502_data_len_256:
            data_len_i = 256;
            break;
        default:
            ESP_LOGE(TAG, "invalid data length, use enum");
            return ESP_ERR_INVALID_ARG;
    }

    // receive data packages
    R502_pid_t pid = R502_pid_data;
    std::array<uint8_t, R502_max_data_len * 2> data_cb_buffer;
    uint8_t *rec_data = receive_pkg.data.data.content;
    int bytes_received = 0;
    while(pid == R502_pid_data){
        err = receive_package(receive_pkg, 
            data_len_i+R502_cs_len+header_size);
        if(err) return err;
        bytes_received += data_len_i;

        pid = (R502_pid_t)receive_pkg.pid;

        // convert 4bit bytes to 8bit in an expanded buffer
        for(int i = 0; i < data_len_i; i++){
            // Low four bytes
            data_cb_buffer[i*2] = (rec_data[i] & 0xf) << 4;
            // High four bytes
            data_cb_buffer[i*2+1] = rec_data[i] & 0xf0;
        }

        // call callback
        up_image_cb(data_cb_buffer, data_len_i * 2);
    }
    ESP_LOGI(TAG, "bytes received %d", bytes_received);

    return ESP_OK;
}

esp_err_t R502Interface::send_command_package(const R502_DataPkg_t &pkg,
    R502_DataPkg_t &receive_pkg, int data_rec_length, int read_delay_ms)
{
    esp_err_t err = send_package(pkg);
    if(err) return err;
    return receive_package(receive_pkg, data_rec_length + header_size, 
        read_delay_ms);
}


esp_err_t R502Interface::send_package(const R502_DataPkg_t &pkg)
{
    int pkg_len = package_length(pkg);
    if(pkg_len < header_size + sizeof(R502_GeneralCommand_t) || 
        pkg_len > sizeof(R502_DataPkg_t))
    {
        ESP_LOGE(TAG, "package length not set correctly");
        return ESP_ERR_INVALID_ARG;
    }

    //printf("Send Data\n");
    //int printed = 0;
    //while(printed < pkg_len){
        //for(int i = 0; i < 8 && printed < pkg_len; i++){
            //printf("0x%02X ", *((uint8_t *)&pkg+printed));
            //printed++;
        //}
        //printf("\n");
    //}

    int len = uart_write_bytes(uart_num, (char *)&pkg, pkg_len);
    if(len == -1){
        ESP_LOGE(TAG, "uart write error, parameter error");
        return ESP_ERR_INVALID_STATE;
    }
    else if(len != package_length(pkg)){
        // not all data transferred
        ESP_LOGE(TAG, "uart write error, wrong number of bytes written");
        return ESP_ERR_INVALID_SIZE;
    }
    return ESP_OK;
}

esp_err_t R502Interface::receive_package(const R502_DataPkg_t &rec_pkg,
    int data_length, int read_delay_ms)
{
    int len = uart_read_bytes(uart_num, (uint8_t *)&rec_pkg, data_length,
        read_delay_ms / portTICK_RATE_MS);
    
    //ESP_LOGI(TAG, "received %d bytes", len);

    if(len == -1){
        ESP_LOGE(TAG, "uart read error, parameter error");
        return ESP_ERR_INVALID_STATE;
    }
    else if(len == 0){
        ESP_LOGE(TAG, "uart read error, R502 not found");
        return ESP_ERR_NOT_FOUND;
    }
    // This check is uncesseccary, it'll probably fail the crc cause it doesn't
    // write data into the last crc byte if this is the case
    else if(len < data_length){
        ESP_LOGE(TAG, "uart read error, not enough bytes read, %d < %d", 
            len, data_length);
        uart_flush(uart_num);
        return ESP_ERR_INVALID_RESPONSE;
    }

//    printf("response Data\n");
    //int printed = 0;
    //while(printed < len){
        //for(int i = 0; i < 8 && printed < len; i++){
            //printf("0x%02X ", *((uint8_t *)&rec_pkg+printed));
            //printed++;
        //}
        //printf("\n");
    //}

    // Verify response
    if(!verify_checksum(rec_pkg)){
        ESP_LOGE(TAG, "uart read error, invalid CRC"); 
        uart_flush(uart_num);
        return ESP_ERR_INVALID_CRC;
    }
    esp_err_t err = verify_headers(rec_pkg, len - header_size);
    if(err){
        uart_flush(uart_num);
    }
    return err;
}

void R502Interface::busy_delay(int64_t microseconds)
{
    // wait
    int64_t time_start = esp_timer_get_time();
    while(esp_timer_get_time() < time_start + microseconds);
}

void R502Interface::fill_checksum(R502_DataPkg_t &package)
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

bool R502Interface::verify_checksum(const R502_DataPkg_t &package)
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

esp_err_t R502Interface::verify_headers(const R502_DataPkg_t &pkg, 
    uint16_t length)
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
    if(pkg.pid != R502_pid_command && pkg.pid != R502_pid_data && 
        pkg.pid != R502_pid_ack && pkg.pid != R502_pid_end_of_data){
        ESP_LOGE(TAG, "Response has invalid pid, %d", pkg.pid);
        return ESP_ERR_INVALID_RESPONSE;
    }

    // length
    if(conv_8_to_16(pkg.length) != length){
        ESP_LOGE(TAG, "Response has invalid length, %d vs %dB received", 
            conv_8_to_16(pkg.length), length);
        return ESP_ERR_INVALID_RESPONSE;
    }

    return ESP_OK;
}

void R502Interface::set_headers(R502_DataPkg_t &package, R502_pid_t pid,
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

uint16_t R502Interface::package_length(const R502_DataPkg_t &pkg){
    return sizeof(pkg) - sizeof(pkg.data) + conv_8_to_16(pkg.length);
}