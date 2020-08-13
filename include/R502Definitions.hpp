/**
 * \file R502Definitions.hpp
 * \brief Defines data types and enums to faciliate communication with the R502 
 * module
 */

#pragma once
#include <stdint.h>


/**
 * \brief Package identifiers
 */
typedef enum {
    R502_pid_command = 0x01,
    R502_pid_data = 0x02,
    R502_pid_ack = 0x07,
    R502_pid_end_of_data = 0x08
} R502_pid_t;

/**
 * \brief Instruction codes sent with each command
 */
typedef enum {
    R502_ic_gen_img = 0x1,
    R502_ic_img_2_tz = 0x2,
    R502_ic_match = 0x3,
    R502_ic_search = 0x4,
    R502_ic_reg_model = 0x5,
    R502_ic_store = 0x6,
    R502_ic_load_char = 0x7,
    R502_ic_up_char = 0x8,
    R502_ic_down_char = 0x9,
    R502_ic_up_image = 0xA,
    R502_ic_down_image = 0xB,
    R502_ic_delet_char = 0xC,
    R502_ic_empty = 0xD,
    R502_ic_set_sys_para = 0xE,
    R502_ic_read_sys_para = 0xF,
    R502_ic_set_pwd = 0x12,
    R502_ic_vfy_pwd = 0x13,
    R502_ic_get_random_code = 0x14,
    R502_ic_set_adder = 0x15,
    R502_ic_control = 0x17,
    R502_ic_write_notepad = 0x18,
    R502_ic_read_notepad = 0x19,
    R502_ic_template_num = 0x1D,
    R502_ic_led_config = 0x35
} R502_instr_code_t;

/**
 * \brief Confirmation codes returned by the R502 module in acknowledge packages
 */
typedef enum {
    R502_fail = -1,
    R502_ok = 0x0,
    R502_err_receive = 0x1,
    R502_err_no_finger = 0x2,
    R502_err_fail_to_enroll = 0x3,
    R502_err_disorderly_image = 0x6,
    R502_err_no_character_pointer = 0x7,
    R502_err_no_match = 0x8,
    R502_err_not_found = 0x9,
    R502_err_combine = 0xA,
    R502_err_page_id_out_of_range = 0xB,
    R502_err_reading_template = 0xC,
    R502_err_uploading_template = 0xD,
    R502_err_receiving_data = 0xE,
    R502_err_uploading_image = 0xF,
    R502_err_deleting_template = 0x10,
    R502_err_clear_library = 0x11,
    R502_err_wrong_pass = 0x13,
    R502_err_no_valid_primary_image = 0x15,
    R502_err_writing_flash = 0x18,
    R502_err_no_definition = 0x19,
    R502_err_invalid_reg_num = 0x1A,
    R502_err_wrong_reg_config = 0x1B,
    R502_err_wrong_page_num = 0x1C,
    R502_err_comm_port = 0x1D,
} R502_conf_code_t;

struct R502_sys_para_t {
    uint8_t status_register[2];
    uint8_t system_identifier_code[2];
    uint8_t finger_library_size[2];
    uint8_t security_level[2];
    uint8_t device_address[4];
    uint8_t data_packet_size[2];
    uint8_t baud_setting[2];
};

struct R502_sys_para_2_t {
    uint16_t status_register;
    uint16_t system_identifier_code;
    uint16_t finger_library_size;
    uint16_t security_level;
    uint8_t device_address[4];
    uint16_t data_packet_size;
    uint16_t baud_setting;
};

/**
 * \brief Data section of the VfyPwd command
 */
struct R502_VfyPwd_t {
    uint8_t instr_code; //!< instruction code
    uint8_t password[4]; //!< User-provided password
    uint8_t checksum[2]; //!< checksum
};

/**
 * \brief Data section of the SetPwd command
 */
struct R502_SetPwd_t {
    uint8_t instr_code; //!< instruction code
    uint8_t password[4]; //!< User-provided password
    uint8_t checksum[2]; //!< checksum
};

/**
 * \brief Data section of a general acknowledge package from R502
 * 
 * Many commands have this same acknowledge format, reuse this
 */
struct R502_Ack_t {
    uint8_t conf_code; //!< confirmation code
    uint8_t checksum[2]; //!< checksum
};

/**
 * \brief General data structure storing any type of package
 */
struct R502_DataPackage_t {
    uint8_t start[2]; //!< Two byte header, all packages are the same
    uint8_t adder[4]; //!< Module address, default 0xff, 0xff, 0xff, 0xff
    uint8_t pid; //!< Package id
    uint8_t length[2]; //!< length in bytes of data section of this package
    union {
        R502_VfyPwd_t vfy_pwd;
        R502_SetPwd_t set_pwd;
        R502_Ack_t default_ack;
    } data; //!< Data and checksum of the package
};