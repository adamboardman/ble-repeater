#include <iomanip>
#include <ranges>
#include <cstdio>
#include <vector>

#include "Debugging.h"
#include "include/int_types.h"
#include "btstack.h"
#include "hci_dump_embedded_stdout.h"
#include "pico/cyw43_arch.h"
#include "pico/btstack_cyw43.h"
#include "hardware/adc.h"
#include "pico/stdlib.h"
#include "pico/float.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "ble_repeater.h"
#include "hci.h"
#include "gap.h"
#include "pico/unique_id.h"
#include "Packet/BinaryReader.h"
#include "Packet/ProtocolProcessor.h"
#include "Packet/ProtocolWriter.h"
#include "BLE/BleConnection.h"
#include "hardware/sync.h"
#include <cinttypes>

#include "Packet/BinaryWriter.h"
#include "BLE/BleConnectionTracker.h"
#include "hardware/clocks.h"
#include "hardware/pll.h"
#include "pico/binary_info/code.h"

const uint EXIT_GPIO_PIN = 28;
const uint SMOKE_SEEN_PIN = 17;
const uint LIFE_CHECK_PIN = 18;


#define BLE_SCAN_PERIOD_MS 2000

#define BLE_ADVERTISING_MAX_LENGTH 31
#define BLE_FLAG_LE_LIMITED_DISC_MODE (0x01) // LE Limited Discoverable Mode
#define BLE_FLAG_LE_GENERAL_DISC_MODE (0x02) // LE General Discoverable Mode
#define BLE_FLAG_BR_EDR_NOT_SUPPORTED (0x04) // BR/EDR not supported
#define BLE_FLAG_LE_BR_EDR_CONTROLLER (0x08) // Simultaneous LE and BR/EDR, Controller
#define BLE_FLAG_LE_BR_EDR_HOST (0x10) // Simultaneous LE and BR/EDR, Host
#define BLE_FLAGS_LE_ONLY_LIMITED_DISC_MODE (BLE_FLAG_LE_LIMITED_DISC_MODE | BLE_FLAG_BR_EDR_NOT_SUPPORTED)
#define BLE_FLAGS_LE_ONLY_GENERAL_DISC_MODE (BLE_FLAG_LE_GENERAL_DISC_MODE | BLE_FLAG_BR_EDR_NOT_SUPPORTED)

#define BLE_GAP_ADV_TYPE_CONNECTABLE_SCANNABLE_UNDIRECTED 0x01 /**< Connectable and scannable undirected advertising events. */



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

bool keep_running = true;
bool smoke_seen = false;
bool life_check = false;
uint16_t global_activity = 0;
bool scanning = false;
bool connection_in_progress = false;
uint32_t disconnection_started_at = 0;
bool discover_primary_services = false;
bool discover_characteristics_for_service = false;

BleConnectionTracker connection_tracker;
BleConnectionTracker *connection_tracker_ptr = &connection_tracker;
ProtocolProcessor processor(connection_tracker);

static btstack_timer_source_t stop_scan_timer_source;
static btstack_packet_callback_registration_t hci_event_callback_registration;
static btstack_packet_callback_registration_t sm_event_callback_registration;

uint16_t att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t *buffer,
                           uint16_t buffer_size) {
    UNUSED(connection_handle);
    global_activity++;
    LOG_DEBUG("att_read_callback(%02x,%02x,%d,buffer,%d)\n", connection_handle, att_handle, offset, buffer_size);

    uint16_t size_used_or_needed = 0;
    switch (att_handle) {
        case ATT_CHARACTERISTIC_a8d99167_e58c_4a0c_9565_e2f1a7fbc05d_01_CLIENT_CONFIGURATION_HANDLE:
            size_used_or_needed = 2;
            memset(&buffer[offset], 0, buffer_size - offset);
            break;
        case ATT_CHARACTERISTIC_a8d99167_e58c_4a0c_9565_e2f1a7fbc05d_01_VALUE_HANDLE:
            LOG_DEBUG("read - from a packet client - we have nothing to say just now\n");
            memset(&buffer[offset], 0, buffer_size - offset);
            break;
        case ATT_CHARACTERISTIC_a8d99167_e58c_4a0c_9565_e2f1a7fbc05d_01_USER_DESCRIPTION_HANDLE:
            LOG_DEBUG("read - from a packet client - characteristic description - unknown if this is needed\n");
            memset(&buffer[offset], 0, buffer_size - offset);
            break;
        default:
            LOG_DEBUG("attempt to read undefined att_handle: %02x\n", att_handle);
            break;
    }
    //print_named_data("att read buffer out", buffer + offset, std::min(buffer_size, size_used_or_needed));
    return size_used_or_needed;
}

int att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode,
                       uint16_t offset, uint8_t *buffer, uint16_t buffer_size) {
    UNUSED(transaction_mode);
    // UNUSED(offset);
    // UNUSED(buffer_size);
    global_activity++;

    LOG_DEBUG("att_write_callback(%02x,0x%02x,%d,%d,buffer,%d)\n", connection_handle, att_handle, transaction_mode,
              offset, buffer_size);
    //print_named_data("att write buffer in", buffer, buffer_size);

    if (transaction_mode != ATT_TRANSACTION_MODE_NONE) {
        switch (transaction_mode) {
            case ATT_TRANSACTION_MODE_ACTIVE:
                LOG_DEBUG("att_write_callback - ATT_TRANSACTION_MODE_ACTIVE\n");
                break;
            case ATT_TRANSACTION_MODE_EXECUTE:
                LOG_DEBUG("att_write_callback - ATT_TRANSACTION_MODE_EXECUTE\n");
                break;
            case ATT_TRANSACTION_MODE_CANCEL:
                LOG_DEBUG("att_write_callback - ATT_TRANSACTION_MODE_CANCEL\n");
                break;
            case ATT_TRANSACTION_MODE_VALIDATE:
                LOG_DEBUG("att_write_callback - ATT_TRANSACTION_MODE_VALIDATE\n");
                break;
            default:
                break;
        }
        return 0;
    }
    auto &context = connection_tracker.connectionForConnHandle(connection_handle);

    switch (att_handle) {
        case ATT_CHARACTERISTIC_a8d99167_e58c_4a0c_9565_e2f1a7fbc05d_01_CLIENT_CONFIGURATION_HANDLE: {
            context.setNotificationEnabled(
                little_endian_read_16(buffer, 0) == GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION);
            if (context.getNotificationEnabled()) {
                switch (att_handle) {
                    case ATT_CHARACTERISTIC_a8d99167_e58c_4a0c_9565_e2f1a7fbc05d_01_CLIENT_CONFIGURATION_HANDLE:
                        context.setPacketCharacteristicValueHandle(
                            ATT_CHARACTERISTIC_a8d99167_e58c_4a0c_9565_e2f1a7fbc05d_01_VALUE_HANDLE);
                        break;
                    default:
                        break;
                }
            }
            break;
        }
        case ATT_CHARACTERISTIC_a8d99167_e58c_4a0c_9565_e2f1a7fbc05d_01_VALUE_HANDLE: {
            LOG_DEBUG("write from a packet connection - offset: %d, buffer_size: %d\n", offset, buffer_size);
            print_named_data("att write buffer in", buffer, buffer_size);
            processor.processWrite(context, offset, buffer, buffer_size);
            break;
        }
        default:
            LOG_DEBUG("BLE Service: Unsupported write to 0x%02x, len %u\n", att_handle, buffer_size);
            break;
    }
    return 0;
}

static uint8_t adv_data[BLE_ADVERTISING_MAX_LENGTH];
uint8_t adv_data_len = 0;

void populate_advertising_data() {
    //ADV_IND - It allows for up to 31 bytes of advertising data in the main packet payload.

    //0xXX (the length of the subsequent data, {d0(type), d01-dXX}
    adv_data[adv_data_len++] = 0x2;
    adv_data[adv_data_len++] = BLUETOOTH_DATA_TYPE_FLAGS;
    adv_data[adv_data_len++] = BLE_FLAGS_LE_ONLY_GENERAL_DISC_MODE;

    adv_data[adv_data_len++] = 17;
    adv_data[adv_data_len++] = BLUETOOTH_DATA_TYPE_INCOMPLETE_LIST_OF_128_BIT_SERVICE_CLASS_UUIDS;
    memcpy(&adv_data[adv_data_len], packet_service_uuid_reversed, sizeof(packet_service_uuid_reversed));
    adv_data_len += sizeof(packet_service_uuid_reversed);

    const uint8_t name_len = strlen(packet_service_name);
    const auto fitable_name_len = std::min(static_cast<uint8_t>(BLE_ADVERTISING_MAX_LENGTH - adv_data_len - 2),
                                           name_len);
    adv_data[adv_data_len++] = fitable_name_len + 1;
    if (fitable_name_len == name_len) {
        adv_data[adv_data_len++] = BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME;
    } else {
        adv_data[adv_data_len++] = BLUETOOTH_DATA_TYPE_SHORTENED_LOCAL_NAME;
    }
    memcpy(&adv_data[adv_data_len], packet_service_name, fitable_name_len);
    adv_data_len += fitable_name_len;
}

void setup_advertisements() {
    populate_advertising_data();

    // setup advertisements - units each of 625us
    constexpr uint16_t adv_int_min = 30000 / 625; // 0.03 second
    constexpr uint16_t adv_int_max = 40000 / 625; // 0.04 second
    constexpr uint8_t adv_type = 0;
    bd_addr_t null_addr = {};
    gap_advertisements_set_params(adv_int_min, adv_int_max, adv_type, 0, null_addr, 0x07, 0x00);
    gap_advertisements_set_data(adv_data_len, adv_data);
    gap_advertisements_enable(1);
}

void start_scanning_for_local_nodes() {
    LOG_DEBUG("start_scanning_for_local_nodes\n");
    gap_start_scan();
    scanning = true;

    btstack_run_loop_set_timer(&stop_scan_timer_source, BLE_SCAN_PERIOD_MS);
    btstack_run_loop_add_timer(&stop_scan_timer_source);
}

void setup_scanning() {
    //scan type passive or active
    //100% (scan interval = scan window)
    //could change to only scan once every 10 mins or so - we don't expect a static set
    //of devices strategically placed to appear and disappear too often
    gap_set_scan_params(0, 0x0030, 0x0030, 0);
    gap_set_scan_duplicate_filter(true);
    start_scanning_for_local_nodes();
}

static const char *ad_types[] = {
    "",
    "Flags",
    "Incomplete List of 16-bit Service Class UUIDs",
    "Complete List of 16-bit Service Class UUIDs",
    "Incomplete List of 32-bit Service Class UUIDs",
    "Complete List of 32-bit Service Class UUIDs",
    "Incomplete List of 128-bit Service Class UUIDs",
    "Complete List of 128-bit Service Class UUIDs",
    "Shortened Local Name",
    "Complete Local Name",
    "Tx Power Level",
    "",
    "",
    "Class of Device",
    "Simple Pairing Hash C",
    "Simple Pairing Randomizer R",
    "Device ID",
    "Security Manager TK Value",
    "Slave Connection Interval Range",
    "",
    "List of 16-bit Service Solicitation UUIDs",
    "List of 128-bit Service Solicitation UUIDs",
    "Service Data",
    "Public Target Address",
    "Random Target Address",
    "Appearance",
    "Advertising Interval"
};

static const char *flags[] = {
    "LE Limited Discoverable Mode",
    "LE General Discoverable Mode",
    "BR/EDR Not Supported",
    "Simultaneous LE and BR/EDR to Same Device Capable (Controller)",
    "Simultaneous LE and BR/EDR to Same Device Capable (Host)",
    "Reserved",
    "Reserved",
    "Reserved"
};

static service_uuid_check_status check_for_uuid(const uint8_t *advertisement_data, uint8_t adv_size) {
    ad_context_t context;
    bool found_uuid = false;
    bool found_name = false;
    for (ad_iterator_init(&context, adv_size, (uint8_t *) advertisement_data); ad_iterator_has_more(&context);
         ad_iterator_next(&context)) {
        const uint8_t data_type = ad_iterator_get_data_type(&context);
        uint8_t size = ad_iterator_get_data_len(&context);
        const uint8_t *data = ad_iterator_get_data(&context);

        int i;
        switch (data_type) {
            case BLUETOOTH_DATA_TYPE_INCOMPLETE_LIST_OF_128_BIT_SERVICE_CLASS_UUIDS:
            case BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_128_BIT_SERVICE_CLASS_UUIDS:
            case BLUETOOTH_DATA_TYPE_LIST_OF_128_BIT_SERVICE_SOLICITATION_UUIDS:
                found_uuid = 0 == memcmp(packet_service_uuid_reversed, data, sizeof(packet_service_uuid_reversed));
                break;
            case BLUETOOTH_DATA_TYPE_SHORTENED_LOCAL_NAME:
            case BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME:
                found_name = 0 == memcmp(packet_service_name, data, strlen(packet_service_name));

                break;
            default:
                break;
        }
    }
    if (found_uuid) {
        if (found_name) {
            return ServiceUUIDAndNameFound;
        }
        return ServiceUUIDFound;
    }
    return ServiceUUIDNotFound;
}

static void dump_advertisement_data(const uint8_t *advertisement_data, uint8_t adv_size) {
    ad_context_t context;
    bd_addr_t address;
    uint8_t uuid_128[16];
    for (ad_iterator_init(&context, adv_size, (uint8_t *) advertisement_data); ad_iterator_has_more(&context);
         ad_iterator_next(&context)) {
        uint8_t data_type = ad_iterator_get_data_type(&context);
        uint8_t size = ad_iterator_get_data_len(&context);
        const uint8_t *data = ad_iterator_get_data(&context);

        if (data_type > 0 && data_type < 0x1B) {
            LOG_DEBUG("    %s: ", ad_types[data_type]);
        }
        int i;
        // Assigned Numbers GAP

        switch (data_type) {
            case BLUETOOTH_DATA_TYPE_FLAGS:
                // show only first octet, ignore rest
                for (i = 0; i < 8; i++) {
                    if (data[0] & (1 << i)) {
                        LOG_DEBUG("%s; ", flags[i]);
                    }
                }
                break;
            case BLUETOOTH_DATA_TYPE_INCOMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS:
            case BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS:
            case BLUETOOTH_DATA_TYPE_LIST_OF_16_BIT_SERVICE_SOLICITATION_UUIDS:
                for (i = 0; i < size; i += 2) {
                    LOG_DEBUG("%04X ", little_endian_read_16(data, i));
                }
                break;
            case BLUETOOTH_DATA_TYPE_INCOMPLETE_LIST_OF_32_BIT_SERVICE_CLASS_UUIDS:
            case BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_32_BIT_SERVICE_CLASS_UUIDS:
            case BLUETOOTH_DATA_TYPE_LIST_OF_32_BIT_SERVICE_SOLICITATION_UUIDS:
                for (i = 0; i < size; i += 4) {
                    LOG_DEBUG("%04" PRIx32, little_endian_read_32(data, i));
                }
                break;
            case BLUETOOTH_DATA_TYPE_INCOMPLETE_LIST_OF_128_BIT_SERVICE_CLASS_UUIDS:
            case BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_128_BIT_SERVICE_CLASS_UUIDS:
            case BLUETOOTH_DATA_TYPE_LIST_OF_128_BIT_SERVICE_SOLICITATION_UUIDS:
                reverse_128(data, uuid_128);
                LOG_DEBUG("%s", uuid128_to_str(uuid_128));
                break;
            case BLUETOOTH_DATA_TYPE_SHORTENED_LOCAL_NAME:
            case BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME:
                for (i = 0; i < size; i++) {
                    LOG_DEBUG("%c", (char) (data[i]));
                }
                break;
            case BLUETOOTH_DATA_TYPE_TX_POWER_LEVEL:
                LOG_DEBUG("%d dBm", *(int8_t *) data);
                break;
            case BLUETOOTH_DATA_TYPE_SLAVE_CONNECTION_INTERVAL_RANGE:
                LOG_DEBUG("Connection Interval Min = %u ms, Max = %u ms", little_endian_read_16(data, 0) * 5 / 4,
                          little_endian_read_16(data, 2) * 5 / 4);
                break;
            case BLUETOOTH_DATA_TYPE_SERVICE_DATA:
                for (i = 0; i < size; i += 2) {
                    LOG_DEBUG("%02X ", data[i]);
                }
                break;
            case BLUETOOTH_DATA_TYPE_PUBLIC_TARGET_ADDRESS:
            case BLUETOOTH_DATA_TYPE_RANDOM_TARGET_ADDRESS:
                reverse_bd_addr(data, address);
                LOG_DEBUG("%s", bd_addr_to_str(address));
                break;
            case BLUETOOTH_DATA_TYPE_APPEARANCE:
                // https://developer.bluetooth.org/gatt/characteristics/Pages/CharacteristicViewer.aspx?u=org.bluetooth.characteristic.gap.appearance.xml
                LOG_DEBUG("%02X", little_endian_read_16(data, 0));
                break;
            case BLUETOOTH_DATA_TYPE_ADVERTISING_INTERVAL:
                LOG_DEBUG("%u ms", little_endian_read_16(data, 0) * 5 / 8);
                break;
            case BLUETOOTH_DATA_TYPE_3D_INFORMATION_DATA:
                for (i = 0; i < size; i += 2) {
                    LOG_DEBUG("%02X ", data[i]);
                }
                break;
            case BLUETOOTH_DATA_TYPE_MANUFACTURER_SPECIFIC_DATA: // Manufacturer Specific Data
            case BLUETOOTH_DATA_TYPE_CLASS_OF_DEVICE:
            case BLUETOOTH_DATA_TYPE_SIMPLE_PAIRING_HASH_C:
            case BLUETOOTH_DATA_TYPE_SIMPLE_PAIRING_RANDOMIZER_R:
            case BLUETOOTH_DATA_TYPE_DEVICE_ID:
            case BLUETOOTH_DATA_TYPE_SECURITY_MANAGER_OUT_OF_BAND_FLAGS:
            default:
                LOG_DEBUG("    Advertising Data Type 0x%2x not handled yet", data_type);
                break;
        }
        LOG_DEBUG("\n");
    }
    LOG_DEBUG("\n");
}

static void printUUID(uint8_t *uuid128, uint16_t uuid16) {
    if (uuid16) {
        LOG_DEBUG("%04x", uuid16);
    } else {
        LOG_DEBUG("%s", uuid128_to_str(uuid128));
    }
}

static void dump_service(gatt_client_service_t *service) {
    LOG_DEBUG("    * service: [0x%04x-0x%04x], uuid ", service->start_group_handle, service->end_group_handle);
    printUUID(service->uuid128, service->uuid16);
    LOG_DEBUG("\n");
}

static void dump_characteristic(gatt_client_characteristic_t *characteristic) {
    LOG_DEBUG("    * characteristic: [0x%04x-0x%04x-0x%04x], properties 0x%02x, uuid ",
              characteristic->start_handle, characteristic->value_handle, characteristic->end_handle,
              characteristic->properties);
    printUUID(characteristic->uuid128, characteristic->uuid16);
    LOG_DEBUG("\n");
}

static void handle_gatt_client_event(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    UNUSED(packet_type);
    UNUSED(channel);
    UNUSED(size);
    global_activity++;
    LOG_DEBUG("handle_gatt_client_event(%d,%d,packet,%d)\n", packet_type, channel, size);
    // print_named_data("client_event packet", packet, size);

    gatt_client_service_t service;
    gatt_client_characteristic_t characteristic;
    const auto client_event_type = hci_event_packet_get_type(packet);
    LOG_DEBUG("client_event_type 0x%x\n", client_event_type);
    switch (client_event_type) {
        case GATT_EVENT_MTU: {
            const auto con_handle = gatt_event_mtu_get_handle(packet);
            BleConnection &connection = connection_tracker.connectionForConnHandle(con_handle);
            connection.setMtu(gatt_event_mtu_get_MTU(packet));
            LOG_DEBUG("GATT MTU: %d\n", gatt_event_mtu_get_MTU(packet));
            break;
        }
        case GATT_EVENT_SERVICE_QUERY_RESULT: {
            //0xa1
            const auto con_handle = gatt_event_service_query_result_get_handle(packet);
            BleConnection &connection = connection_tracker.connectionForConnHandle(con_handle);
            gatt_event_service_query_result_get_service(packet, &service);
            // dump_service(&service);
            if (!service.uuid16) {
                connection.storeHandlesIfServiceMatches(service);
            }
            break;
        }
        case GATT_EVENT_CHARACTERISTIC_QUERY_RESULT: {
            //0xa2
            const auto con_handle = gatt_event_characteristic_query_result_get_handle(packet);
            BleConnection &connection = connection_tracker.connectionForConnHandle(con_handle);
            gatt_event_characteristic_query_result_get_characteristic(packet, &characteristic);
            // dump_characteristic(&characteristic);
            if (!characteristic.uuid16) {
                connection.storeHandlesIfCharacteristicMatches(characteristic);
            }
            break;
        }
        case GATT_EVENT_QUERY_COMPLETE: {
            //0xa0
            const auto con_handle = gatt_event_query_complete_get_handle(packet);
            const BleConnection &connection = connection_tracker.connectionForConnHandle(con_handle);
            const auto service_id = gatt_event_query_complete_get_service_id(packet);
            const auto connection_id = gatt_event_query_complete_get_connection_id(packet);
            const auto status = gatt_event_query_complete_get_att_status(packet);
            hci_connection_t *hci_connection = hci_connection_for_handle(con_handle);
            LOG_DEBUG("hci_connection_for_handle(0x%x) - 0x%x\n", con_handle, hci_connection);
            LOG_DEBUG("GATT_EVENT_QUERY_COMPLETE handle: 0x%x, service: %d, conn: %d, status: %d\n", con_handle,
                      service_id,
                      connection_id, status);
            discover_primary_services = false;
            discover_characteristics_for_service = false;
            if (connection.canAndNeedToDiscoverPacketCharacteristicsQuery(service)) {
                LOG_DEBUG("gatt_client_discover_characteristics_for_service\n");
                discover_characteristics_for_service = true;
                gatt_client_discover_characteristics_for_service_by_uuid128(
                    handle_gatt_client_event, con_handle, &service, packet_characteristic_uuid);
            }
            break;
        }
        default:
            break;
    }
}

void handle_gatt_client_value_update_event(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    global_activity++;
    LOG_DEBUG("handle_gatt_client_value_update_event(%d,%d,packet,%d)\n", packet_type, channel, size);
    //print_named_data("client_value_update_event packet", packet, size);

    const auto client_value_update_event_type = hci_event_packet_get_type(packet);
    LOG_DEBUG("client_value_update_event_type 0x%x\n", client_value_update_event_type);
    switch (client_value_update_event_type) {
        case GATT_EVENT_NOTIFICATION: {
            //0xa7
            auto handle = gatt_event_notification_get_handle(packet);
            auto service_id = gatt_event_notification_get_service_id(packet);
            auto connection_id = gatt_event_notification_get_connection_id(packet);
            auto value_handle = gatt_event_notification_get_value_handle(packet);
            auto value_length = gatt_event_notification_get_value_length(packet);
            auto value = gatt_event_notification_get_value(packet);
            auto connection = connection_tracker.connectionForConnHandle(handle);
            LOG_DEBUG("handle 0x%x, %d, %d, %d, %d\n", handle, service_id, connection_id, value_handle, value_length);
            processor.processWrite(connection, 0, value, value_length);
            break;
        }
        case GATT_EVENT_INDICATION: {
            auto handle = gatt_event_indication_get_handle(packet);
            auto service_id = gatt_event_indication_get_service_id(packet);
            auto connection_id = gatt_event_indication_get_connection_id(packet);
            auto value_handle = gatt_event_indication_get_value_handle(packet);
            auto value_length = gatt_event_indication_get_value_length(packet);
            auto value = gatt_event_indication_get_value(packet);
            LOG_DEBUG("handle 0x%x, %d, %d, %d, %d\n", handle, service_id, connection_id, value_handle, value_length);
            print_named_data("client_value_update_event indication", value, value_length);
        }
    }
}


void hci_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    UNUSED(size);
    UNUSED(channel);
    global_activity++;
    LOG_DEBUG("hci_packet_handler(0x%02x,%d,data,%d)\n", packet_type, channel, size);

    //ProtocolProcessor::print_named_data("hci_packet", packet, size);

    if (packet_type != HCI_EVENT_PACKET) return;

    const uint8_t event_type = hci_event_packet_get_type(packet);
    LOG_DEBUG("event_type: 0x%02x\n", event_type);
    switch (event_type) {
        case BTSTACK_EVENT_STATE: {
            //0x60
            if (btstack_event_state_get_state(packet) != HCI_STATE_WORKING) return;
            bd_addr_t local_addr;
            gap_local_bd_addr(local_addr);
            LOG_DEBUG("BTstack up and running on: %s\n", bd_addr_to_str(local_addr));
            gap_set_max_number_peripheral_connections(4); //will bump up sometime?
            setup_advertisements();
            setup_scanning();
            break;
        }
        case HCI_EVENT_DISCONNECTION_COMPLETE: {
            //0x05
            const auto handle = hci_event_disconnection_complete_get_connection_handle(packet);
            hci_connection_t *hci_connection = hci_connection_for_handle(handle);
            LOG_DEBUG("hci_connection_for_handle(0x%x) - 0x%x\n", handle, hci_connection);
            LOG_DEBUG("Disconnected with handle 0x%x\n", handle);
            connection_tracker.reportDisconnection(handle);

            const auto reason = hci_event_disconnection_complete_get_reason(packet);
            LOG_DEBUG("LE Connection disconnect, reason 0x%02x ", reason);
            switch (reason) {
                case 0x08:
                    LOG_DEBUG("BLE_HCI_CONNECTION_TIMEOUT\n");
                    break;
                case 0x12:
                    LOG_DEBUG("BLE_HCI_STATUS_CODE_INVALID_BTLE_COMMAND_PARAMETERS\n");
                    break;
                case 0x13:
                    LOG_DEBUG("BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION\n");
                    break;
                case 0x14:
                    LOG_DEBUG("BLE_HCI_REMOTE_DEV_TERMINATION_DUE_TO_LOW_RESOURCES\n");
                    break;
                case 0x15:
                    LOG_DEBUG("BLE_HCI_REMOTE_DEV_TERMINATION_DUE_TO_POWER_OFF\n");
                    break;
                case 0x16:
                    LOG_DEBUG("BLE_HCI_LOCAL_HOST_TERMINATED_CONNECTION\n");
                    break;
                default:
                    LOG_DEBUG("\n");
            }
            break;
        }
        case SM_EVENT_PAIRING_STARTED: //0xd4
            LOG_DEBUG("Pairing Started\n");
            break;
        case SM_EVENT_PAIRING_COMPLETE: //0xd5
            LOG_DEBUG("Pairing Complete\n");
            break;
        case GAP_EVENT_RSSI_MEASUREMENT: {
            //0xde
            const auto rssi = static_cast<int8_t>(gap_event_rssi_measurement_get_rssi(packet));
            const uint16_t handle = gap_event_rssi_measurement_get_con_handle(packet);
            const auto role = gap_get_role(handle);
            auto connection = connection_tracker.connectionForConnHandle(handle);
            connection.setRssi(rssi);
            LOG_DEBUG("RSSI(0x%x): %d, role: %d\n", handle, rssi, role);
            break;
        }
        case BTSTACK_EVENT_NR_CONNECTIONS_CHANGED: {
            //0x61 - part of connect[3]
            const uint8_t conn_count = btstack_event_nr_connections_changed_get_number_connections(packet);
            LOG_DEBUG("Number of connections changed: %d\n", conn_count);
            break;
        }
        case HCI_EVENT_LE_META: {
            const auto sub_code = hci_event_le_meta_get_subevent_code(packet);
            print_named_data("hci_packet", packet, size);
            LOG_DEBUG("HCI_EVENT_LE_META 0x%02x\n", sub_code);
            switch (sub_code) {
                case HCI_SUBEVENT_LE_CONNECTION_COMPLETE: {
                    const auto handle = hci_subevent_le_connection_complete_get_connection_handle(packet);
                    hci_connection_t *hci_connection = hci_connection_for_handle(handle);
                    LOG_DEBUG("hci_connection_for_handle(0x%x) - 0x%x\n", handle, hci_connection);
                    const auto role = hci_subevent_le_connection_complete_get_role(packet);
                    const auto address_type = static_cast<bd_addr_type_t>(
                        hci_subevent_le_connection_complete_get_peer_address_type(packet));
                    bd_addr_t address;
                    hci_subevent_le_connection_complete_get_peer_address(packet, address);
                    LOG_DEBUG("Connected on handle 0x%x with %s address %s, role: %d...\n", handle,
                              address_type == 0 ? "public" : "random",
                              bd_addr_to_str(address), role);
                    connection_tracker.reportConnection(handle, address, address_type, role);
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case GAP_EVENT_ADVERTISING_REPORT: {
            // 0xda
            const auto address_type = static_cast<bd_addr_type_t>(
                gap_event_advertising_report_get_address_type(packet));
            bd_addr_t address;
            gap_event_advertising_report_get_address(packet, address);
            const auto adv_event_type = gap_event_advertising_report_get_advertising_event_type(packet);
            const auto rssi = static_cast<int8_t>(gap_event_advertising_report_get_rssi(packet));
            const auto length = gap_event_advertising_report_get_data_length(packet);
            const auto data = gap_event_advertising_report_get_data(packet);
            //print_named_data("advertising data", data, length);
            //dump_advertisement_data(data, length);
            const auto services = check_for_uuid(data, length);
            if (services & ServiceUUIDFound) {
                connection_tracker.addAvailablePeer(address, address_type, services, rssi);
            }
            LOG_DEBUG("GAP_EVENT_ADVERTISING_REPORT type:%d rssi: %d with %s address %s services: 0b%b\n",
                      adv_event_type,
                      rssi, address_type == 0 ? "public" : "random", bd_addr_to_str(address), services);
            break;
        }
        case HCI_EVENT_META_GAP: {
            //0xe7
            // wait for connection complete
            LOG_DEBUG("HCI_EVENT_META_GAP subevent: 0x%02x\n", hci_event_gap_meta_get_subevent_code(packet));

            if (hci_event_gap_meta_get_subevent_code(packet) != GAP_SUBEVENT_LE_CONNECTION_COMPLETE) break;

            gap_advertisements_enable(1); //re-enable adverts

            const auto con_handle = gap_subevent_le_connection_complete_get_connection_handle(packet);
            hci_connection_t *hci_connection = hci_connection_for_handle(con_handle);
            LOG_DEBUG("HCI_EVENT_META_GAP hci_connection_for_handle(0x%x) - 0x%x\n", con_handle, hci_connection);

            const auto address_type = static_cast<bd_addr_type_t>(
                gap_subevent_le_connection_complete_get_peer_address_type(packet));
            bd_addr_t address;
            const auto role = gap_subevent_le_connection_complete_get_role(packet);
            gap_subevent_le_connection_complete_get_peer_address(packet, address);
            connection_tracker.reportConnection(con_handle, address, address_type, role);
            connection_in_progress = false;
            discover_primary_services = true;
            const auto err = gatt_client_discover_primary_services_by_uuid128(
                &handle_gatt_client_event, con_handle, packet_service_uuid);
            LOG_DEBUG("Connection complete - discover primary services - err: %d\n", err);
            break;
        }
        case GATT_EVENT_CAN_WRITE_WITHOUT_RESPONSE: {
            //0xAC
            const hci_con_handle_t con_handle = gatt_event_can_write_without_response_get_handle(packet);
            hci_connection_t *hci_connection = hci_connection_for_handle(con_handle);
            LOG_DEBUG("GATT_EVENT_CAN_WRITE_WITHOUT_RESPONSE hci_connection_for_handle(0x%x) - 0x%x\n", con_handle,
                      hci_connection);
            break;
        }
        case GATT_EVENT_QUERY_COMPLETE: {
            //0xa0
            switch (const auto status = gatt_event_query_complete_get_att_status(packet)) {
                case ATT_ERROR_INSUFFICIENT_ENCRYPTION:
                    LOG_DEBUG("GATT Query result: Insufficient Encryption\n");
                    break;
                case ATT_ERROR_INSUFFICIENT_AUTHENTICATION:
                    LOG_DEBUG("GATT Query result: Insufficient Authentication\n");
                    break;
                case ATT_ERROR_BONDING_INFORMATION_MISSING:
                    LOG_DEBUG("GATT Query result: Bonding Information Missing\n");
                    break;
                case ATT_ERROR_SUCCESS:
                    LOG_DEBUG("GATT Query result: OK\n");
                    break;
                default:
                    LOG_DEBUG("GATT Query result: 0x%02x\n", status);
                    break;
            }
            break;
        }
        case HCI_EVENT_NUMBER_OF_COMPLETED_PACKETS: //0x13
        case HCI_EVENT_TRANSPORT_PACKET_SENT: //0x6e
            //ignore
            break;
        default:
            print_named_data("hci_packet unknown", packet, size);
            break;
    }
}

void att_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    UNUSED(size);
    UNUSED(channel);
    global_activity++;
    LOG_DEBUG("att_packet_handler(0x%02x,%d,data,%d)\n", packet_type, channel, size);
    if (packet_type != HCI_EVENT_PACKET) return;

    //print_named_data("att_packet", packet, size);

    uint8_t event_type = hci_event_packet_get_type(packet);
    LOG_DEBUG("event_type: 0x%02x\n", event_type);
    switch (event_type) {
        case ATT_EVENT_CAN_SEND_NOW: {
            //0xb7
            break;
        }
        case GATT_EVENT_CAN_WRITE_WITHOUT_RESPONSE:
            // hci_con_handle_t gatt_event_can_write_without_response_get_handle(packet);
            // hci_connection_t *hci_connection = hci_connection_for_handle(con_handle);
            // LOG_DEBUG("hci_connection_for_handle(0x%x) - 0x%x\n",con_handle,hci_connection);
            break;
        case ATT_EVENT_CONNECTED: {
            //0xb3
            //att_packet:b30901 8a91a10fea5a 4000
            const auto address_type = static_cast<bd_addr_type_t>(att_event_connected_get_address_type(packet));
            bd_addr_t address;
            const hci_con_handle_t handle = att_event_connected_get_handle(packet);
            hci_connection_t *hci_connection = hci_connection_for_handle(handle);
            LOG_DEBUG("hci_connection_for_handle(0x%x) - 0x%x\n", handle, hci_connection);
            att_event_connected_get_address(packet, address);
            LOG_DEBUG("Connected on handle 0x%x with %s address %s ...\n", handle,
                      address_type == 0 ? "public" : "random",
                      bd_addr_to_str(address));
            connection_tracker.reportConnection(handle, address, address_type);
            break;
        }
        case ATT_EVENT_DISCONNECTED: {
            //0xb4
            const hci_con_handle_t handle = att_event_disconnected_get_handle(packet);
            hci_connection_t *hci_connection = hci_connection_for_handle(handle);
            LOG_DEBUG("hci_connection_for_handle(0x%x) - 0x%x\n", handle, hci_connection);
            LOG_DEBUG("Disconnected with handle 0x%x\n", handle);
            connection_tracker.reportDisconnection(handle);
            break;
        }
        default:
            break;
    }
}

static void sm_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    UNUSED(channel);
    UNUSED(size);
    global_activity++;
    LOG_DEBUG("sm_packet_handler(0x%02x,%d,data,%d)\n", packet_type, channel, size);
    if (packet_type != HCI_EVENT_PACKET) return;

    //print_named_data("sm_packet", packet, size);

    const uint8_t event_packet_type = hci_event_packet_get_type(packet);
    LOG_DEBUG("event_packet_type: 0x%02x\n", event_packet_type);
}

static void stop_scan_handler(struct btstack_timer_source *ts) {
    global_activity++;
    LOG_DEBUG(".");

    gap_stop_scan();
    scanning = false;

    // Restart timer
    //btstack_run_loop_set_timer(ts, BLE_SCAN_PERIOD_MS);
    //btstack_run_loop_add_timer(ts);
}

bool connect_to_first_neighbour() {
    auto neighbours = connection_tracker.getConnectableNeighbours();
    if (const auto item = neighbours.begin(); item != neighbours.end()) {
        const auto neighbour = *item;
        const auto address = neighbour->getAddress();
        if (connection_tracker.getConnectionForAddress(address)) {
            return false;
        }
        const auto err = gap_connect(address, neighbour->getAddressType());
        LOG_DEBUG("gap_connect: %s, err: 0x%x\n", bd_addr_to_str(address), err);
        if (err == 0) {
            connection_tracker.setConnectionStarted(neighbour);
            connection_in_progress = true;
            return true;
        }
        sleep_ms(20);
    }
    return false;
}

void gpio_callback(const uint gpio, const uint32_t events) {
    global_activity++;
    LOG_DEBUG("gpio_callback gpio: %d, events: %d\n", gpio, events);
    if (gpio == EXIT_GPIO_PIN && (events & GPIO_IRQ_EDGE_FALL)) {
        keep_running = false;
    }
    if (gpio == SMOKE_SEEN_PIN && (events & GPIO_IRQ_EDGE_FALL)) {
        smoke_seen = true;
    }
    if (gpio == LIFE_CHECK_PIN && (events & GPIO_IRQ_EDGE_FALL)) {
        life_check = true;
    }
}

void reduce_clock(const uint32_t speed) {
    set_sys_clock_khz(speed * KHZ, false);
}

void handlePinActivity() {
    std::vector<uint8_t> outBuffer;
    const uint64_t timestamp_ms = connection_tracker.getTimeMs();
    if (smoke_seen || life_check) {
        bd_addr_t local_addr;
        gap_local_bd_addr(local_addr);
        uint64_t sender{};
        memcpy(&sender, local_addr, BD_ADDR_LEN);

        const BinaryWriter buffer(outBuffer);
        buffer.write_uint8('[');
        buffer.write_uint16_hex16(timestamp_ms / 1000); //hexified seconds
        buffer.write_uint8(']');
        if (life_check) {
            const std::string still_alive_string = " Still alive";
            buffer.write_data(still_alive_string, still_alive_string.size());
            life_check = false;
        }
        if (smoke_seen) {
            const std::string smoke_seen_string = " Smoke seen";
            buffer.write_data(smoke_seen_string, smoke_seen_string.size());
            smoke_seen = false;
        }
        const std::string messageContentString(reinterpret_cast<const char *>(outBuffer.data()), outBuffer.size());

        Message message(7, timestamp_ms, 0, sender);
        message.setContent(messageContentString);
        outBuffer.clear();
        message.setMessageFlags(0);
        buffer.write_uint32(timestamp_ms);
        buffer.write_uint64(sender);
        const std::string messageIdString(reinterpret_cast<const char *>(outBuffer.data()), outBuffer.size());
        message.setMessageId(messageIdString);
        message.setMessageTimestamp(timestamp_ms);

        std::vector<uint8_t> name_buffer;
        const BinaryWriter name_writer(name_buffer);
        name_writer.write_data(reinterpret_cast<const uint8_t *>(packet_service_name), strlen(packet_service_name));
        name_writer.write_data(":",1);
        name_writer.write_uint8_hex16(local_addr[BD_ADDR_LEN - 2]);
        name_writer.write_uint8_hex16(local_addr[BD_ADDR_LEN - 1]);
        message.setSenderNickname(std::string(reinterpret_cast<const char *>(name_buffer.data()), name_buffer.size()));

        if (const Message *messageIfNew = connection_tracker.storeMessageAndReturnIfNew(message)) {
            //our messages will always be new
            connection_tracker.enqueueBroadcastPacket(messageIfNew);
            global_activity++;
        }
    }
}

void setup_gpio_callback() {
    gpio_init(EXIT_GPIO_PIN);
    gpio_set_dir(EXIT_GPIO_PIN, GPIO_IN);
    gpio_pull_up(EXIT_GPIO_PIN);

    gpio_init(SMOKE_SEEN_PIN);
    gpio_set_dir(SMOKE_SEEN_PIN, GPIO_IN);
    gpio_pull_up(SMOKE_SEEN_PIN);

    gpio_init(LIFE_CHECK_PIN);
    gpio_set_dir(LIFE_CHECK_PIN, GPIO_IN);
    gpio_pull_up(LIFE_CHECK_PIN);

    gpio_set_irq_enabled_with_callback(EXIT_GPIO_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    gpio_set_irq_enabled_with_callback(SMOKE_SEEN_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    gpio_set_irq_enabled_with_callback(LIFE_CHECK_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
}

int main() {
    bi_decl(bi_program_description(
        "BLE Packet Repeater - designed to be spread around to extend the range of packet messages."));
    bi_decl(bi_1pin_with_name(EXIT_GPIO_PIN, "Switch - pull to ground to exit the loop and return to usb-disk mode"));
    bi_decl(bi_1pin_with_name(SMOKE_SEEN_PIN, "Switch - pull to ground to indicate smoke has been detected"));
    bi_decl(bi_1pin_with_name(LIFE_CHECK_PIN, "Switch - pull to ground to send a still alive message"));

    //reduce_clock(18); //Meshtastic folk have managed to get things working this slow
#if (PICO_RP2040)
    reduce_clock(63); //slower than this and things get funky in CYW43 land
#endif
    stdio_init_all();

    setup_gpio_callback();

    sleep_ms(1000); //wait for logging to be attached - remove this once we are working ok
    printf("ble_repeater()\n");

    printf("cyw43_arch_init()\n");
    // initialize CYW43 driver architecture (will enable BT if/because CYW43_ENABLE_BLUETOOTH == 1)
    if (cyw43_arch_init()) {
        printf("failed to initialise cyw43_arch - nothing doing spin forever\n");
        while (keep_running) {
            __wfi();
        }
    }

    printf("l2cap_init()\n");
    l2cap_init();

    printf("Initialize GATT client\n");
    gatt_client_init();

    printf("sm_init()\n");
    sm_init();
    sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);

    // printf("enable btstack hci logging\n");
    // hci_dump_init(hci_dump_embedded_stdout_get_instance());

    //printf("att_server_init()\n");
    att_server_init(profile_data, att_read_callback, att_write_callback);

    //printf("inform about BTstack state");
    hci_event_callback_registration.callback = &hci_packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    //printf("inform about security manager state");
    sm_event_callback_registration.callback = &sm_packet_handler;
    sm_add_event_handler(&sm_event_callback_registration);

    //printf("register for ATT event");
    att_server_register_packet_handler(att_packet_handler);

    // setup one-shot btstack timer used to stop scanning - started whenever we do a scan
    stop_scan_timer_source.process = &stop_scan_handler;

    printf("turn on bluetooth\n");
    if (const auto err = hci_power_control(HCI_POWER_ON); err != 0) {
        printf("Failed to power on Bluetooth - nothing doing spin forever\n");
        while (keep_running) {
            __wfi();
        }
    }

    auto lastSleepOrActivity = time_us_32();
    auto lastAnnounce = time_us_32();
    uint32_t lastFlash = 0;
    auto lastScan = time_us_32();
    auto lastRssiUpdate = time_us_32();
    bool rssi_update_in_progress = false;
    auto lastCleanup = time_us_32() + five_minutes_in_us; //gives 15mins before first cleanup
    auto last_activity = global_activity;
    while (keep_running) {
        const auto loopStart = time_us_32();
        printAvailableLogging();
        handlePinActivity();
        if ((loopStart - lastAnnounce) > two_seconds_in_us) {
            connection_tracker.announceToConnections();
            printAvailableLogging();
            lastAnnounce = time_us_32();
        }
        if (!scanning && !connection_in_progress && !discover_primary_services && !discover_characteristics_for_service
            && disconnection_started_at < loopStart - two_seconds_in_us) {
            if (const auto connecting = connect_to_first_neighbour(); !connecting) {
                // if (const auto duplicate = connection_tracker.getAnyDuplicateHandle()) {
                //     if (gap_disconnect(duplicate) == ERROR_CODE_SUCCESS) {
                //         disconnection_started_at = time_us_32();
                //     }
                // }
            }
        }
        if (last_activity != global_activity) {
            connection_tracker.printStats();
        }
        printAvailableLogging();
        connection_tracker.sendPackets();
        printAvailableLogging();

        if ((loopStart - lastFlash) > two_seconds_in_us) {
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, true);
            sleep_ms(20);
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, false);
            lastFlash = time_us_32();
        }
        if ((loopStart - lastRssiUpdate) > two_minutes_in_us) {
            rssi_update_in_progress = connection_tracker.requestNextRssi(true);
            lastRssiUpdate = time_us_32();
        } else if (rssi_update_in_progress) {
            rssi_update_in_progress = connection_tracker.requestNextRssi(false);
        }
        if ((loopStart - lastScan) > five_minutes_in_us) {
            start_scanning_for_local_nodes();
            lastScan = time_us_32();
        }
        if ((loopStart - lastCleanup) > ten_minutes_in_us) {
            connection_tracker.cleanupStaleItems();
            lastCleanup = time_us_32();
        }

        bool slept = false;
        //nothing happened in the last 2seconds so lets sleep
        while ((loopStart - lastSleepOrActivity) > two_seconds_in_us && last_activity == global_activity) {
            connection_tracker.printStats();
            LOG_DEBUG("We can sleep\n");
            printAvailableLogging();
#if (PICO_RP2040)
            const auto interrupts = *reinterpret_cast<io_rw_32 *>(PPB_BASE + M0PLUS_NVIC_ISER_OFFSET);
#endif
            const auto time_before = static_cast<uint32_t>(time_us_64() & 0xffffffff);
            __wfi(); //similar power consumption to sleep_ms, but less logging, but also less led flashing
            const auto time_after = static_cast<uint32_t>(time_us_64() & 0xffffffff);
#if (PICO_RP2040)
            LOG_DEBUG("slept: %" PRIu32 "us, int: 0b%" PRIb32 "\n", time_after - time_before, interrupts);
#else
            LOG_DEBUG("slept: %" PRIu32 "us\n", time_after - time_before);
#endif
            printAvailableLogging();
            slept = true;
            //wfe - 1us sleep
            //wfi - infinite sleep (sometimes)
            //wfi - 10ms,44ms,44ms,44ms,44ms,44ms,44ms,44ms,44ms,infinite sleep (other times)
        }
        if (slept || last_activity != global_activity) {
            lastSleepOrActivity = time_us_32();
            last_activity = global_activity;
        }
        if (!slept) {
            sleep_ms(40);
        }
    }
}
