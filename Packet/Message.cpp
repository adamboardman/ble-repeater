#include "Message.h"

extern void print_named_data(const char *name, const uint8_t *data, uint16_t data_size);

Message::Message() : PacketBase(type_message), sender_peer(nullptr) {
}

Message::Message(const uint8_t ttl, const uint64_t timestamp, const uint8_t packet_flags, const uint64_t sender)
    : PacketBase(type_message, ttl, timestamp, packet_flags, sender), sender_peer(nullptr) {
}

Message::Message(const uint8_t ttl, const uint64_t timestamp, const uint8_t packet_flags, const uint64_t sender,
                 const uint64_t recipient)
    : PacketBase(type_message, ttl, timestamp, packet_flags, sender, recipient),
      sender_peer(nullptr) {
}

Message::Message(const uint8_t type, const uint8_t version, BinaryReader &reader)
    : PacketBase(type, version, reader),
      sender_peer(nullptr) {

    message_flags = reader.read_uint8();
    message_timestamp = reader.read_uint64();

    const auto message_id_len = reader.read_uint8();
    if (const auto message_id_data = reader.read_data(message_id_len)) {
        print_named_data("message_id", message_id_data, message_id_len);
        message_id = std::string(reinterpret_cast<const char *>(message_id_data), message_id_len);
    } else {
        message_id = "";
    }

    const auto sender_nickname_len = reader.read_uint8();
    if (const auto sender_nick_data = reader.read_data(sender_nickname_len)) {
        print_named_data("sender_nick", sender_nick_data, sender_nickname_len);
        sender_nickname = std::string(reinterpret_cast<const char *>(sender_nick_data), sender_nickname_len);
    } else {
        sender_nickname = "";
    }

    const auto content_len = reader.read_uint16();
    if (const auto content_data = reader.read_data(content_len)) {
        print_named_data("content", content_data, content_len);
        content = std::string(reinterpret_cast<const char *>(content_data), content_len);
    } else {
        content = "";
    }
}

void Message::setMessageFlags(const uint8_t flags) {
    message_flags = flags;
}

void Message::setMessageTimestamp(uint64_t value) {
    message_timestamp = value;
}

void Message::setMessageId(const std::string &string) {
    message_id = string;
}

void Message::setSenderNickname(const std::string &string) {
    sender_nickname = string;
}

void Message::setContent(const std::string &string) {
    content = string;
}

void Message::setEncryptedContent(const std::string &string) {
    encrypted_content = string;
}

void Message::setRecipientNickname(const std::string &string) {
    recipient_nickname = string;
}

uint8_t Message::getMessageFlags() const {
    return message_flags;
}

uint64_t Message::getMessageTimestamp() const {
    return message_timestamp;
}

const std::string &Message::getMessageId() const {
    return message_id;
}

const std::string &Message::getSenderNickname() const {
    return sender_nickname;
}

const std::string &Message::getContent() const {
    return content;
}

const std::string &Message::getRecipientNickname() const {
    return recipient_nickname;
}

void Message::setSenderPeer(Peer *peer) {
    sender_peer = peer;
}

Peer *Message::getSenderPeer() const {
    return sender_peer;
}

void Message::writePacket(std::vector<uint8_t> &vector) {
    PacketBase::writePacket(vector);
}

void Message::writePacketPayload(BinaryWriter &writer) {
    PacketBase::writePacketPayload(writer);

    writer.write_uint8(getMessageFlags());
    writer.write_uint64(getMessageTimestamp());

    auto &id = getMessageId();
    const auto id_len = static_cast<uint8_t>(std::min(static_cast<size_t>(255), id.size()));
    writer.write_uint8(id_len);
    writer.write_data(id, id_len);

    auto &sender = getSenderNickname();
    const auto sender_len = static_cast<uint8_t>(std::min(static_cast<size_t>(255), sender.size()));
    writer.write_uint8(sender_len);
    writer.write_data(sender, sender_len);

    auto &content = getContent();
    const auto content_len = static_cast<uint16_t>(std::min(static_cast<size_t>(65535), content.size()));
    writer.write_uint16(content_len);
    writer.write_data(content, content_len);

    if (hasSenderPeerID()) {
        const auto peerId = getSenderPeer()->getId();
        constexpr uint8_t size = sizeof(uint64_t) * 2;
        writer.write_uint8(size);
        writer.write_uint64(peerId);
    }
}
