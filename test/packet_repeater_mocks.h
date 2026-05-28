#pragma once

#include <cstdint>
#include <vector>

typedef struct {
} i2c_hw_t;

struct i2c_inst {
    i2c_hw_t *hw;
    bool restart_on_next;
};

typedef struct i2c_inst i2c_inst_t;

typedef unsigned int uint;

enum {
    PICO_OK = 0,
    PICO_ERROR_NONE = 0,
    PICO_ERROR_TIMEOUT = -1,
    PICO_ERROR_GENERIC = -2,
    PICO_ERROR_NO_DATA = -3,
};

typedef struct {
    uint16_t start_group_handle;
    uint16_t end_group_handle;
    uint16_t uuid16;
    uint8_t uuid128[16];
} gatt_client_service_t;

typedef uint16_t hci_con_handle_t;
#define BD_ADDR_LEN 6
typedef uint8_t bd_addr_t[BD_ADDR_LEN];

typedef enum {
    // Public Device Address
    BD_ADDR_TYPE_LE_PUBLIC = 0,
    // Random Device Address
    BD_ADDR_TYPE_LE_RANDOM = 1,
    // Public Identity Address (Corresponds to Resolved Private Address)
    BD_ADDR_TYPE_LE_PUBLIC_IDENTITY = 2,
    // Random (static) Identity Address (Corresponds to Resolved Private Address)
    BD_ADDR_TYPE_LE_RANDOM_IDENTITY = 3,
    // internal BTstack addr types for Classic connections
    BD_ADDR_TYPE_SCO = 0xfc,
    BD_ADDR_TYPE_ACL = 0xfd,
    BD_ADDR_TYPE_UNKNOWN = 0xfe, // also used as 'invalid'
} bd_addr_type_t;

typedef struct {
    uint16_t start_handle;
    uint16_t value_handle;
    uint16_t end_handle;
    uint16_t properties;
    uint16_t uuid16;
    uint8_t uuid128[16];
} gatt_client_characteristic_t;

void print_named_data(const char *name, const uint8_t *data, uint16_t data_size);

void populate_array_from_string(uint8_t *uint_array, const std::string &str);

void populate_vector_from_string(std::vector<uint8_t> *uint_vector, const std::string &str);

void populate_string_from_string(std::string *str_out, const std::string &str);

void gap_local_bd_addr(bd_addr_t address_buffer);

typedef enum {
    HCI_ROLE_MASTER = 0,
    HCI_ROLE_SLAVE = 1,
    HCI_ROLE_INVALID = 0xff,
} hci_role_t;

typedef struct btstack_linked_item {
    struct btstack_linked_item *next; // <-- next element in list, or NULL
} btstack_linked_item_t;

typedef struct {
    btstack_linked_item_t *item;

    void (*callback)(void *context);

    void *context;
} btstack_context_callback_registration_t;

typedef void (*btstack_packet_handler_t)(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);

typedef struct {
    btstack_linked_item_t item;
    btstack_packet_handler_t callback;
    hci_con_handle_t con_handle;
    uint16_t attribute_handle;
} gatt_client_notification_t;

#define ERROR_CODE_SUCCESS                                            0x00u
#define ERROR_CODE_UNKNOWN_CONNECTION_IDENTIFIER                      0x02u

void gatt_client_stop_listening_for_characteristic_value_updates(gatt_client_notification_t *notification);

uint8_t att_server_notify(hci_con_handle_t con_handle, uint16_t attribute_handle, const uint8_t *value,
                          uint16_t value_len);

uint8_t att_server_request_to_send_notification(btstack_context_callback_registration_t *callback_registration,
                                                hci_con_handle_t con_handle);

uint8_t gatt_client_write_value_of_characteristic_without_response(hci_con_handle_t con_handle, uint16_t value_handle,
                                                                   uint16_t value_length, uint8_t *value);

uint8_t gatt_client_request_to_write_without_response(btstack_context_callback_registration_t *callback_registration,
                                                      hci_con_handle_t con_handle);

typedef struct {
} hci_connection_t;

void gatt_client_listen_for_characteristic_value_updates(gatt_client_notification_t *notification,
                                                         btstack_packet_handler_t callback, hci_con_handle_t con_handle,
                                                         gatt_client_characteristic_t *characteristic);

int gap_read_rssi(hci_con_handle_t con_handle);

inline char *bd_addr_to_str(const bd_addr_t addr) {
    return nullptr;
}

hci_connection_t *hci_connection_for_handle(hci_con_handle_t con_handle);
