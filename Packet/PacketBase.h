#pragma once

#include <cstdint>
#include <string>

#include "PacketTypes.h"

class PacketBase {
public:
    explicit PacketBase(uint8_t type);

    PacketBase(uint8_t type, uint8_t ttl, uint64_t timestamp, uint8_t flags, uint64_t sender);

    PacketBase(uint8_t type, uint8_t ttl, uint64_t timestamp, uint8_t flags, uint64_t sender, uint64_t recipient);

    virtual ~PacketBase() = default;

    [[nodiscard]] uint8_t getPacketType() const;

    [[nodiscard]] uint8_t getPacketTtl() const;

    [[nodiscard]] uint64_t getPacketTimestamp() const;

    [[nodiscard]] uint64_t getPacketTimestampMs() const;

    [[nodiscard]] uint8_t getPacketFlags() const;

    [[nodiscard]] uint64_t getPacketSenderId() const;

    [[nodiscard]] uint64_t getPacketRecipientId() const;

    [[nodiscard]] const std::string &getPacketSignature() const;

    [[nodiscard]] bool hasPacketRecipient() const {
        return (packet_flags & packet_flag_has_recipient) != 0;
    }

    void setPacketTtl(uint8_t ttl);

    void setPacketTimestamp(uint64_t timestamp);

    void setPacketFlags(uint8_t flags);

    void setPacketSenderId(uint64_t senderId);

protected:
    void setPacketType(PacketType type);

private:
    uint8_t packet_type = 0;
    uint8_t packet_ttl = 0;
    uint64_t packet_timestamp = 0;
    uint8_t packet_flags = 0;
    uint64_t packet_sender_id = 0;
    uint64_t packet_recipient_id = 0;
};
