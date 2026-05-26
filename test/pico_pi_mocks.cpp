#include <catch2/catch_test_macros.hpp>
#include <vector>
#include <cstring>

#include "Debugging.h"
#include "../Packet/ProtocolWriter.h"
#include "pico_pi_mocks.h"

#include "../Packet/BinaryWriter.h"

int lastAddress;
int last_length_read;
int last_length_written;
long mock_data_read = 0;
std::vector<uint8_t> mock_read_data;
std::vector<uint8_t> mock_write_data;
bool use_expected_write_data = false;
std::vector<uint8_t> mock_expected_write_data;
int mock_expected_write_read_at;
int waitTime;
uint64_t mock_now = 0;
char __StackLimit = 0; // NOLINT(*-reserved-identifier)
char __bss_end__ = 0; // NOLINT(*-reserved-identifier)

void reset_for_test(const i2c_inst_t *i2c) {
    lastAddress = 0;
    last_length_read = 0;
    last_length_written = 0;
    mock_data_read = 0;
    mock_read_data.clear();
    mock_write_data.clear();
    mock_expected_write_data.clear();
    mock_expected_write_read_at = 0;
    use_expected_write_data = false;
}

void set_read_data(std::vector<uint8_t> &data) {
    mock_read_data = data;
}

void set_expected_write_data(std::vector<uint8_t> &data) {
    mock_expected_write_data = data;
    use_expected_write_data = true;
}

int i2c_read_blocking(i2c_inst_t *i2c, uint8_t addr, uint8_t *dst, size_t len, bool nostop) {
    lastAddress = addr;
    for (int i = 0; i < len; i++) {
        try {
            dst[i] = mock_read_data.at(mock_data_read);
        } catch (...) {
            LOG_DEBUG("Attempted to read past mock data [%d] - stick a breakpoint here to track it down", i);
        }
        mock_data_read++;
    }
    last_length_read = len;
    return PICO_ERROR_NONE;
}

int i2c_write_blocking(i2c_inst_t *i2c, uint8_t addr, const uint8_t *src, size_t len, bool nostop) {
    lastAddress = addr;
    for (int i = 0; i < len; i++) {
        if (use_expected_write_data) {
            try {
                REQUIRE((int) src[i] == (int) mock_expected_write_data.at(mock_expected_write_read_at++));
            } catch (...) {
                LOG_DEBUG("Attempted to read past mock data [%d] - stick a breakpoint here to track it down", i);
            }
        }
        mock_write_data.push_back(src[i]);
    }
    last_length_written = len;
    return PICO_ERROR_NONE;
}


void sleep_us(int us) {
    waitTime += us;
}

void sleep_ms(int ms) {
    waitTime += ms * 1000;
}

void set_mock_time(const uint64_t now) {
    mock_now = now;
}

uint64_t time_us_64() {
    return mock_now;
}

uint32_t save_and_disable_interrupts() {
    return 0;
}

void restore_interrupts([[maybe_unused]] uint32_t status) {
}

std::vector<uint8_t> mock_sent_data;

void reset_sent_for_test() {
    mock_sent_data.clear();
}

void print_named_data(const char *name, const uint8_t *data, const uint16_t data_size) {
    LOG_DEBUG("%s:", name);
    for (int i = 0; i < data_size; i++) {
        LOG_DEBUG("%02x", data[i]);
    }
    LOG_DEBUG("\n");
    LOG_DEBUG("%s[c]:", name);
    for (int i = 0; i < data_size; i++) {
        if (data[i] >= 0x20 && data[i] < 0x7e) {
            LOG_DEBUG("%c", data[i]);
        } else {
            LOG_DEBUG(" ");
        }
    }
    LOG_DEBUG("\n");
}

void populate_array_from_string(uint8_t *uint_array, const std::string &str) {
    const uint8_t datalen = str.length() / 2;
    for (int i = 0, j = 0; i < datalen; i++, j++) {
        uint_array[i] = (str[j] & '@' ? str[j] + 9 : str[j]) << 4;
        j++;
        uint_array[i] |= (str[j] & '@' ? str[j] + 9 : str[j]) & 0xF;
    }
}

void populate_vector_from_string(std::vector<uint8_t> *uint_vector, const std::string &str) {
    const uint8_t datalen = str.length() / 2;
    for (int i = 0, j = 0; i < datalen; i++, j++) {
        uint_vector->at(i) = (str[j] & '@' ? str[j] + 9 : str[j]) << 4;
        j++;
        uint_vector->at(i) |= (str[j] & '@' ? str[j] + 9 : str[j]) & 0xF;
    }
}

void gap_local_bd_addr(bd_addr_t address_buffer) {
}

void gatt_client_listen_for_characteristic_value_updates(gatt_client_notification_t *notification,
                                                         btstack_packet_handler_t callback, hci_con_handle_t con_handle,
                                                         gatt_client_characteristic_t *characteristic) {
}

void handle_gatt_client_value_update_event(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    LOG_DEBUG("handle_gatt_client_value_update_event(%d,%d,packet,%d)\n", packet_type, channel, size);
}

void gatt_client_stop_listening_for_characteristic_value_updates(gatt_client_notification_t *notification) {
}

int gap_read_rssi(hci_con_handle_t con_handle) {
    return 0;
}

hci_connection_t *hci_connection_for_handle(hci_con_handle_t con_handle) {
    return nullptr;
}

uint8_t att_server_request_to_send_notification(btstack_context_callback_registration_t *callback_registration,
                                                hci_con_handle_t con_handle) {
    callback_registration->callback(callback_registration->context);
    return 0;
}

uint8_t gatt_client_request_to_write_without_response(btstack_context_callback_registration_t *callback_registration,
                                                      hci_con_handle_t con_handle) {
    callback_registration->callback(callback_registration->context);
    return 0;
}

uint8_t att_server_notify(hci_con_handle_t con_handle, uint16_t attribute_handle, const uint8_t *value,
                          uint16_t value_len) {
    BinaryWriter writer(mock_sent_data);
    writer.write_data(value, value_len);
    return 0;
}

uint8_t gatt_client_write_value_of_characteristic_without_response(hci_con_handle_t con_handle, uint16_t value_handle,
                                                                   uint16_t value_length, uint8_t *value) {
    BinaryWriter writer(mock_sent_data);
    writer.write_data(value, value_length);
    return 0;
}
