#include "PacketPassAlong.h"
#include <vector>

#include "BinaryWriter.h"

PacketPassAlong::PacketPassAlong() : PacketBase(type_unknown) {
}

PacketPassAlong::PacketPassAlong(const uint8_t type, const uint8_t ttl, const uint64_t timestamp, const uint8_t flags,
                                 const uint64_t sender, const uint64_t recipient)
    : PacketBase(type, ttl, timestamp, flags, sender, recipient) {
}

void PacketPassAlong::setPayload(std::string &value) {
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
