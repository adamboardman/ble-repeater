#include "Debugging.h"
#include "BleConnection.h"

#include <cstring>

#ifdef MOCK_PICO_PI
#include "../test/packet_repeater_mocks.h"
#include "../test/pico_pi_mocks.h"
#else
#include "hardware/timer.h"
#endif

void BleConnection::setConnectionHandle(const uint16_t hci_con_handle) {
    connection_handle = hci_con_handle;
}

void BleConnection::setNotificationEnabled(const bool cond) {
    notification_enabled = cond;
}

bool BleConnection::getNotificationEnabled() const {
    return notification_enabled;
}

void BleConnection::setPacketCharacteristicValueHandle(const int handle) {
    packet_characteristic_value_handle = handle;
}

void BleConnection::setConnected(const bool cond) {
    connected = cond;
}

void BleConnection::setBleAddress(const bd_addr_t &addr, const bd_addr_type_t addr_type) {
    memcpy(bt_address, addr, BD_ADDR_LEN);
    bt_address_type = addr_type;
}

hci_con_handle_t BleConnection::getConnectionHandle() const {
    return connection_handle;
}

bool BleConnection::hasData() const {
    return has_data_to_send;
}

uint16_t BleConnection::getPacketCharacteristicValueHandle() const {
    return packet_characteristic_value_handle;
}

void BleConnection::setHasData(bool data) {
    has_data_to_send = data;
}

void BleConnection::setServices(service_uuid_check_status services) {
    services_found = services;
}

void BleConnection::setRssi(const int8_t scan_rssi) {
    rssi = scan_rssi;
}

void BleConnection::setTimestamp(const uint64_t value) {
    last_seen_time = value;
}

const bd_addr_t &BleConnection::getAddress() const {
    return bt_address;
}

bd_addr_type_t BleConnection::getAddressType() const {
    return bt_address_type;
}

bool BleConnection::isConnected() const {
    return connected;
}

void BleConnection::setMtu(uint16_t mtu) {
    bt_mtu = mtu;
}

void BleConnection::storeHandlesIfServiceMatches(const gatt_client_service_t &service) {
    const auto found = std::equal(std::begin(packet_service_uuid), std::end(packet_service_uuid),
                                  std::begin(service.uuid128));
    if (found) {
        LOG_DEBUG("Found Service UUID start group handle: %d\n", service.start_group_handle);
        packet_service_start_group_handle = service.start_group_handle;
        packet_service_end_group_handle = service.end_group_handle;
    }
}

void BleConnection::storeHandlesIfCharacteristicMatches(const gatt_client_characteristic_t &characteristic) {
    const auto found = std::equal(std::begin(packet_characteristic_uuid), std::end(packet_characteristic_uuid),
                                  std::begin(characteristic.uuid128));
    if (found) {
        LOG_DEBUG("Found Characteristic UUID value handle: %d\n", characteristic.value_handle);
        packet_characteristic_value_handle = characteristic.value_handle;
    }
}

bool BleConnection::isRepeater() const {
    return services_found == ServiceUUIDAndNameFound;
}

bool BleConnection::isRandom() const {
    return bt_address_type == BD_ADDR_TYPE_LE_RANDOM;
}

uint64_t BleConnection::getTimestamp() const {
    return last_seen_time;
}

uint64_t BleConnection::getTimestampMs() const {
    return last_seen_time / 1000;
}

uint16_t BleConnection::getMtu() const {
    return bt_mtu;
}

void BleConnection::setRole(uint8_t the_role) {
    role = the_role;
}

uint8_t BleConnection::getRole() const {
    return role;
}

gatt_client_notification_t *BleConnection::getNotificationListener() {
    return &notification_listener;
}

bool BleConnection::canAndNeedToDiscoverPacketCharacteristicsQuery(gatt_client_service_t &service) const {
    if (packet_service_start_group_handle > 0 && packet_characteristic_value_handle == 0) {
        LOG_DEBUG("populated service for query\n");
        service.uuid16 = 0;
        memcpy(service.uuid128, packet_service_uuid, sizeof(packet_service_uuid));
        service.start_group_handle = packet_service_start_group_handle;
        service.end_group_handle = packet_service_end_group_handle;
        return true;
    }
    return false;
}
