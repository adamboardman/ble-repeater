#pragma once

#include <vector>

#include "Base.h"
#include "../BLE/BleConnection.h"
#include "../BLE/BleConnectionTracker.h"

class ProtocolProcessor {
public:
    explicit ProtocolProcessor(BleConnectionTracker &ble_connection_tracker)
        : ble_connection_tracker(ble_connection_tracker) {
    }

    static const char *stringForType(uint8_t type);

    void updateOrStorePeerNameFromAnnouncement(Announce &announce, BleConnection &connection) const;

    bool processMessage(Base &message, const uint8_t *payload, uint16_t payload_length) const;

    void processWrite(BleConnection &connection, uint16_t offset, const uint8_t *buffer, uint16_t buffer_size) const;

private:
    BleConnectionTracker &ble_connection_tracker;
};
