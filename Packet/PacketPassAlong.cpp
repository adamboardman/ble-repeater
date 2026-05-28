#include "PacketPassAlong.h"
#include <vector>

#include "BinaryWriter.h"

PacketPassAlong::PacketPassAlong() : PacketBase(type_unknown), hasPayload(false) {
}

PacketPassAlong::PacketPassAlong(const uint8_t type) : PacketBase(type), hasPayload(false) {
}

PacketPassAlong::PacketPassAlong(const uint8_t type, const uint8_t ttl, const uint64_t timestamp, const uint8_t flags,
                                 const uint64_t sender, const uint64_t recipient)
    : PacketBase(type, ttl, timestamp, flags, sender, recipient), hasPayload(false) {
}

PacketPassAlong::PacketPassAlong(const uint8_t type, const uint8_t version, const bool hasPayload, BinaryReader &reader)
    : PacketBase(type, version, reader), hasPayload(hasPayload) {
    if (hasPayload) {
        const auto payload_len = reader.read_remainder_len();
        if (const auto payload_data = reader.read_data(payload_len)) {
            //print_named_data("payload_data", payload_data, payload_len);
            payload = std::string(reinterpret_cast<const char *>(payload_data), payload_len);
        } else {
            payload = "";
        }
    }
}

void PacketPassAlong::setPayload(const std::string &value) {
    payload = value;
}

const std::string &PacketPassAlong::getPayload() const {
    return payload;
}

std::size_t PacketPassAlong::getPacketHash() const {
    const std::size_t hash_payload = std::hash<std::string>{}(payload);
    std::vector<uint8_t> meta_buffer;
    const BinaryWriter writer(meta_buffer);
    writer.write_uint8(getPacketType());
    //ignore ttl for hash as we want to ignore the same message going around again
    writer.write_uint8(getPacketFlags());
    writer.write_uint64(getPacketTimestamp());
    writer.write_uint64(getPacketSenderId());
    writer.write_uint64(getPacketRecipientId());
    std::string meta_string;
    meta_string.assign(reinterpret_cast<const char *>(meta_buffer.data()), meta_buffer.size());
    const std::size_t hash_meta = std::hash<std::string>{}(meta_string);
    return hash_payload ^ (hash_meta << 1);
}

void PacketPassAlong::writePacket(std::vector<uint8_t> &vector) {
    PacketBase::writePacket(vector);
}

void PacketPassAlong::writePacketPayload(BinaryWriter &writer) {
    PacketBase::writePacketPayload(writer);
    if (hasPayload) {
        writer.write_data(payload, payload.size());
    }
}
