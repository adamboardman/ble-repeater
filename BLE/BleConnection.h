#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <string>

#ifdef MOCK_PICO_PI
#include "../test/packet_repeater_mocks.h"
#else
#include "bluetooth.h"
#include "ble/gatt_client.h"
#endif

// PRIMARY_SERVICE, 298cfeca-a10d-49ee-8a74-e513547f7ef7
// CHARACTERISTIC, a8d99167-e58c-4a0c-9565-e2f1a7fbc05d, READ | WRITE | WRITE_WITHOUT_RESPONSE | NOTIFY | DYNAMIC

const uint8_t packet_service_uuid[] = {
    0x29, 0x8c, 0xfe, 0xca, 0xa1, 0x0d, 0x49, 0xee, 0x8a, 0x74, 0xe5, 0x13, 0x54, 0x7f, 0x7e, 0xf7
};
const uint8_t packet_service_uuid_reversed[] = {
    0xf7, 0x7e, 0x7f, 0x54, 0x13, 0xe5, 0x74, 0x8a, 0xee, 0x49, 0x0d, 0xa1, 0xca, 0xfe, 0x8c, 0x29
};
const auto packet_service_name = "Repeater";
const uint8_t packet_characteristic_uuid[] = {
    0xa8, 0xd9, 0x91, 0x67, 0xe5, 0x8c, 0x4a, 0x0c, 0x95, 0x65, 0xe2, 0xf1, 0xa7, 0xfb, 0xc0, 0x5d
};

enum service_uuid_check_status {
    ServiceUUIDNotFound = 0,
    ServiceUUIDFound = 0b1,
    ServiceUUIDAndNameFound = 0b11
};

class BleConnection {
public:
    void setConnectionHandle(uint16_t hci_con_handle);

    void setNotificationEnabled(bool cond);

    [[nodiscard]] bool getNotificationEnabled() const;

    void setPacketCharacteristicValueHandle(int handle);

    void setConnected(bool cond);

    void setBleAddress(const bd_addr_t &addr, bd_addr_type_t addr_type);

    [[nodiscard]] hci_con_handle_t getConnectionHandle() const;

    [[nodiscard]] bool hasData() const;

    [[nodiscard]] uint16_t getPacketCharacteristicValueHandle() const;

    void setHasData(bool);

    void setServices(service_uuid_check_status services);

    void setRssi(int8_t scan_rssi);

    void setTimestamp(uint64_t value);

    [[nodiscard]] const bd_addr_t &getAddress() const;

    [[nodiscard]] bd_addr_type_t getAddressType() const;

    [[nodiscard]] bool isConnected() const;

    void setMtu(uint16_t mtu);

    bool canAndNeedToDiscoverPacketCharacteristicsQuery(gatt_client_service_t &service) const;

    void storeHandlesIfServiceMatches(const gatt_client_service_t &service);

    void storeHandlesIfCharacteristicMatches(const gatt_client_characteristic_t &characteristic);

    [[nodiscard]] bool isRepeater() const;

    [[nodiscard]] bool isRandom() const;

    [[nodiscard]] uint64_t getTimestamp() const;

    [[nodiscard]] uint64_t getTimestampMs() const;

    [[nodiscard]] uint16_t getMtu() const;

    void setRole(uint8_t role);

    [[nodiscard]] uint8_t getRole() const;

    gatt_client_notification_t *getNotificationListener();

private:
    hci_con_handle_t connection_handle = 0;
    bd_addr_t bt_address{};
    bd_addr_type_t bt_address_type = BD_ADDR_TYPE_UNKNOWN;
    uint16_t bt_mtu = 0;
    uint16_t packet_service_start_group_handle = 0;
    uint16_t packet_service_end_group_handle = 0;
    uint16_t packet_characteristic_value_handle = 0;
    int notification_enabled = 0;
    bool has_data_to_send = false;
    bool connected = false;
    service_uuid_check_status services_found = ServiceUUIDFound;
    int8_t rssi = 0;
    uint8_t role = HCI_ROLE_INVALID;
    uint64_t last_seen_time = 0;
    gatt_client_notification_t notification_listener{};
};
