#include "PacketBase.h"

#include <string>
#include <utility>
#include <vector>

#include "BinaryWriter.h"
#include "Debugging.h"

PacketBase::PacketBase(const uint8_t type)
    : Base(type) {
}

PacketBase::PacketBase(const uint8_t type, const uint8_t ttl, const uint64_t timestamp, const uint8_t flags,
                       const uint64_t sender)
    : Base(type, ttl, timestamp, flags),
      packet_sender_id(sender) {
}

PacketBase::PacketBase(const uint8_t type, const uint8_t ttl, const uint64_t timestamp, const uint8_t flags,
                       const uint64_t sender, const uint64_t recipient)
    : Base(type, ttl, timestamp, flags),
      packet_sender_id(sender),
      packet_recipient_id(recipient) {
}

PacketBase::PacketBase(const uint8_t type, const uint8_t version, BinaryReader &reader)
    : Base(type, reader) {
    packet_sender_id = reader.read_uint64();
    if (hasPacketRecipient()) {
        packet_recipient_id = reader.read_uint64();
    }
}

uint64_t PacketBase::getPacketSenderId() const {
    return packet_sender_id;
}

void PacketBase::setPacketSenderId(const uint64_t senderId) {
    packet_sender_id = senderId;
}

uint64_t PacketBase::getPacketRecipientId() const {
    return packet_recipient_id;
}

void PacketBase::writePacket(std::vector<uint8_t> &vector) {
    Base::writePacket(vector);
}

void PacketBase::writePacketPayload(BinaryWriter &writer) {
    writer.write_uint64(getPacketSenderId());
    if (hasPacketRecipient()) {
        writer.write_uint64(getPacketRecipientId());
    }
}
