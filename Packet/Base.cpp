#include "Base.h"

#include <cinttypes>
#include <string>
#include <utility>
#include <vector>
#include "assert.h"
#include "BinaryWriter.h"
#include "Debugging.h"
#include "int_types.h"

Base::Base(const uint8_t type)
    : packet_type(type) {
}

Base::Base(const uint8_t type, const uint8_t ttl, const uint64_t timestamp, const uint8_t flags)
    : packet_type(type),
      packet_ttl(ttl),
      packet_timestamp(timestamp),
      packet_flags(flags) {
}

Base::Base(const uint8_t type, BinaryReader &reader)
    : packet_type(type) {
    packet_ttl = reader.read_uint8();
    LOG_DEBUG("ttl: %d\n", packet_ttl);
    packet_timestamp = reader.read_uint64();
    LOG_DEBUG("timestamp: 0x%" PRIx64 "\n", packet_timestamp);
    packet_flags = reader.read_uint8();
    LOG_DEBUG("flags: %d\n", packet_flags);
}

uint8_t Base::getPacketType() const {
    return packet_type;
}

uint8_t Base::getPacketTtl() const {
    return packet_ttl;
}

uint64_t Base::getPacketTimestamp() const {
    return packet_timestamp;
}

uint64_t Base::getPacketTimestampMs() const {
    return packet_timestamp / 1000;
}

uint8_t Base::getPacketFlags() const {
    return packet_flags;
}

void Base::setPacketTtl(const uint8_t ttl) {
    packet_ttl = ttl;
}

void Base::setPacketTimestamp(const uint64_t timestamp) {
    packet_timestamp = timestamp;
}

void Base::setPacketFlags(const uint8_t flags) {
    packet_flags = flags;
}

void Base::setPacketType(PacketType type) {
    packet_type = type;
}

void Base::writePacket(std::vector<uint8_t> &vector) {
    BinaryWriter writer(vector);

    writer.write_uint8(1); //version
    writer.write_uint8(getPacketType());

    writer.write_uint8(getPacketTtl());

    writer.write_uint64(getPacketTimestamp());
    writer.write_uint8(getPacketFlags());

    writePacketPayload(writer);
}

void Base::writePacketPayload(BinaryWriter &writer) {
    ASSERT_DEBUG(false);
}
