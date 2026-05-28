#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "BinaryReader.h"
#include "BinaryWriter.h"
#include "PacketTypes.h"

class Base {
public:
    explicit Base(uint8_t type);

    Base(uint8_t type, uint8_t ttl, uint64_t timestamp, uint8_t flags);

    Base(uint8_t type, BinaryReader &reader);

    virtual ~Base() = default;

    [[nodiscard]] uint8_t getPacketType() const;

    [[nodiscard]] uint8_t getPacketTtl() const;

    [[nodiscard]] uint64_t getPacketTimestamp() const;

    [[nodiscard]] uint64_t getPacketTimestampMs() const;

    [[nodiscard]] uint8_t getPacketFlags() const;

    void setPacketTtl(uint8_t ttl);

    void setPacketTimestamp(uint64_t timestamp);

    void setPacketFlags(uint8_t flags);

    virtual void writePacketPayload(BinaryWriter &writer);

    virtual void writePacket(std::vector<uint8_t> &vector);

protected:
    void setPacketType(PacketType type);

private:
    uint8_t packet_type = 0;
    uint8_t packet_ttl = 0;
    uint64_t packet_timestamp = 0;
    uint8_t packet_flags = 0;
};
