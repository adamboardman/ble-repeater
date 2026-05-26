#include "int_types.h"
#include "ProtocolWriter.h"

#include "Announce.h"
#include "BinaryWriter.h"
#include "PacketTypes.h"
#include "PacketPassAlong.h"

void ProtocolWriter::writePacket(std::vector<uint8_t> &vector, const PacketBase *packet_base) {
    if (packet_base == nullptr) {
        return;
    }
    const BinaryWriter writer(vector);

    writer.write_uint8(1); //version
    writer.write_uint8(packet_base->getPacketType());

    //we only do relaying of packets so if we are writing one we want to reduce the ttl
    writer.write_uint8(packet_base->getPacketTtl()-1);

    writer.write_uint64(packet_base->getPacketTimestamp());
    writer.write_uint8(packet_base->getPacketFlags());
    writer.write_uint64(packet_base->getPacketSenderId());

    std::vector<uint8_t> payload;
    switch (packet_base->getPacketType()) {
        case type_announce: {
            if (const auto announce = static_cast<const Announce*>(packet_base); announce != nullptr) {
                const BinaryWriter payload_writer(payload);
                payload_writer.write_data(announce->getName(),announce->getName().size());
            }
            break;
        }
        case type_message: {
            if (const auto message = static_cast<const Message*>(packet_base); message != nullptr) {
                writeMessagePayload(payload, *message);
            }
            break;
        }
        default: {
            if (const auto pass_along = static_cast<const PacketPassAlong*>(packet_base); pass_along != nullptr) {
                const BinaryWriter payload_writer(payload);
                payload_writer.write_data(pass_along->getPayload(),pass_along->getPayload().size());
            }
            break;
        }
    }
    const auto payload_len = static_cast<uint16_t>(std::min(static_cast<size_t>(65535), payload.size()));
    writer.write_uint16(payload_len);

    writer.write_data(payload.data(), payload_len);
}

void ProtocolWriter::writeMessagePayload(std::vector<uint8_t> &vector, const Message &message) {
    const BinaryWriter writer(vector);

    writer.write_uint8(message.getMessageFlags());
    writer.write_uint64(message.getMessageTimestamp());

    auto &id = message.getMessageId();
    const auto id_len = static_cast<uint8_t>(std::min(static_cast<size_t>(255), id.size()));
    writer.write_uint8(id_len);
    writer.write_data(id, id_len);

    auto &sender = message.getSenderNickname();
    const auto sender_len = static_cast<uint8_t>(std::min(static_cast<size_t>(255), sender.size()));
    writer.write_uint8(sender_len);
    writer.write_data(sender, sender_len);

    auto &content = message.getContent();
    const auto content_len = static_cast<uint16_t>(std::min(static_cast<size_t>(65535), content.size()));
    writer.write_uint16(content_len);
    writer.write_data(content, content_len);

    if (message.hasSenderPeerID()) {
        const auto peerId = message.getSenderPeer()->getId();
        constexpr uint8_t size = sizeof(uint64_t) * 2;
        writer.write_uint8(size);
        writer.write_uint64(peerId);
    }
}
