#include "Debugging.h"
#include "int_types.h"

#include "ProtocolProcessor.h"

#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <string>
#include <cinttypes>

#include "BinaryReader.h"
#include "PacketTypes.h"
#include "ProtocolWriter.h"
#include "libdeflate.h"
#include "PacketPassAlong.h"

extern void print_named_data(const char *name, const uint8_t *data, uint16_t data_size);

const char *ProtocolProcessor::stringForType(const uint8_t type) {
    switch (type) {
        case type_announce:
            return "Announce";
        case type_message:
            return "Message";
        default:
            return "UnknownType";
    }
}

void ProtocolProcessor::updateOrStorePeerNameFromAnnouncement(const uint64_t sender, const uint8_t *payload,
                                                              uint16_t payload_length, uint8_t ttl,
                                                              BleConnection &connection) const {
    auto &peer = ble_connection_tracker.checkSenderInPeers(sender);
    std::string peer_name;
    peer_name.assign(reinterpret_cast<const char *>(payload), payload_length);
    peer.updateName(peer_name);
    if (ttl >= peer.getAnnounceTtl()) {
        peer.setAnnounceTtl(ttl);
        ble_connection_tracker.setConnectionHandleForPeer(connection.getConnectionHandle(), &peer);
    }
}

bool ProtocolProcessor::processMessage(Message &message, const uint8_t *payload, uint16_t payload_length) const {
    //Cope with the new broken message format that contains only plain text
    bool broken_message = true;
    for (int i = 0; i < payload_length; i++) {
        if (payload[i] < 0x20 && payload[i] != '\r' && payload[i] != '\n') {
            broken_message = false;
            break;
        }
    }
    if (broken_message) {
        message.setContent(std::string(reinterpret_cast<const char *>(payload), payload_length));
        return true;
    }

    if (payload_length < 13) {
        LOG_DEBUG("Payload too small to be a message: %d\n", payload_length);
        return false;
    }

    BinaryReader reader(0, payload, payload_length);
    message.setMessageFlags(reader.read_uint8());
    message.setMessageTimestamp(reader.read_uint64());

    const auto id_len = reader.read_uint8();
    const auto id = reader.read_data(id_len);
    if (id == nullptr) {
        LOG_DEBUG("Data corrupted: invalid message id\n");
        return false;
    }
    print_named_data("message id", id, id_len);
    message.setMessageId(std::string(reinterpret_cast<const char *>(id), id_len));

    const auto sender_len = reader.read_uint8();
    const auto sender = reader.read_data(sender_len);
    if (sender == nullptr) {
        LOG_DEBUG("Data corrupted: invalid sender nickname id\n");
        return false;
    }
    print_named_data("sender", sender, sender_len);
    message.setSenderNickname(std::string(reinterpret_cast<const char *>(sender), sender_len));

    const auto content_len = reader.read_uint16();
    const auto content = reader.read_data(content_len);
    if (content == nullptr) {
        LOG_DEBUG("Data corrupted: invalid content\n");
        return false;
    }
    print_named_data("content", content, content_len);
    if (message.isEncrypted()) {
        message.setEncryptedContent(std::string(reinterpret_cast<const char *>(content), content_len));
    } else {
        message.setContent(std::string(reinterpret_cast<const char *>(content), content_len));
    }

    if (message.hasRecipientNickname()) {
        const auto recipient_nickname_len = reader.read_uint8();
        const auto recipient_nickname = reader.read_data(recipient_nickname_len);
        if (recipient_nickname == nullptr) {
            LOG_DEBUG("Data corrupted: invalid recipient_nickname\n");
            return false;
        }
        print_named_data("recipient_nickname", recipient_nickname, recipient_nickname_len);
        message.setRecipientNickname(std::string(reinterpret_cast<const char *>(recipient_nickname),
                                                 recipient_nickname_len));
    }

    if (message.hasSenderPeerID()) {
        const auto sender_peer_id_len = reader.read_uint8();
        const auto sender_peer_id = reader.read_data(sender_peer_id_len);
        if (sender_peer_id == nullptr) {
            LOG_DEBUG("Data corrupted: invalid sender_peer_id\n");
            return false;
        }
        print_named_data("sender_peer_id", sender_peer_id, sender_peer_id_len);
        if (sender_peer_id_len == 16) {
            uint64_t peer_id = 0;
            for (int shift_by = 56, i = 0; i < sender_peer_id_len; shift_by -= 8, i++) {
                uint8_t uint8 = (sender_peer_id[i] & '@' ? sender_peer_id[i] + 9 : sender_peer_id[i]) << 4;
                i++;
                uint8 |= (sender_peer_id[i] & '@' ? sender_peer_id[i] + 9 : sender_peer_id[i]) & 0xF;
                peer_id |= static_cast<uint64_t>(uint8) << shift_by;
            }
            auto &peer = ble_connection_tracker.checkSenderInPeers(peer_id);
            message.setSenderPeer(&peer);
        }
    }

    return true;
}

void ProtocolProcessor::processWrite(BleConnection &connection, const uint16_t offset, const uint8_t *buffer,
                                     const uint16_t buffer_size) const {
    BinaryReader reader(offset, buffer, buffer_size);
    const auto version = reader.read_uint8();
    if (version < 1 || version > 2) {
        LOG_DEBUG("Unknown Protocol Version: %d\n", version);
        return;
    }
    const auto type = reader.read_uint8();
    LOG_DEBUG("type: %d (%s)\n", type, stringForType(type));

    const auto ttl = reader.read_uint8();
    LOG_DEBUG("ttl: %d\n", ttl);
    const auto timestamp_ms = reader.read_uint64(); //time in ms since 1970
    LOG_DEBUG("timestamp: 0x%" PRIx64 "\n", timestamp_ms);
    const auto packet_flags = reader.read_uint8();
    LOG_DEBUG("flags: %d\n", packet_flags);
    const auto sender = reader.read_uint64();
    LOG_DEBUG("sender: 0x%" PRIx64 "\n", sender);
    __uint32_t payload_length = 0;
    if (version == 1) {
        payload_length = reader.read_uint16();
        LOG_DEBUG("payload length: %d\n", payload_length);
    } else if (version == 2) {
        payload_length = reader.read_uint32();
        LOG_DEBUG("payload length: %d\n", payload_length);
    }
    uint64_t recipient = 0;
    if (packet_flags & packet_flag_has_recipient) {
        recipient = reader.read_uint64();
        LOG_DEBUG("recipient: 0x%" PRIx64 "\n", recipient);
    }
    uint16_t originalSize = 0;
    if (packet_flags & packet_flag_is_compressed) {
        //compressed gives original size before payload
        originalSize = reader.read_uint16();
        LOG_DEBUG("originalSize: %d\n", originalSize);
        payload_length -= 2;
        //removes the two extra chars added to cover these bytes for simpler clients who don't unpack the payload
    }
    //payload
    auto payload = reader.read_data(payload_length);

    std::vector<uint8_t> decompressed;
    if (payload && packet_flags & packet_flag_is_compressed) {
        decompressed.reserve(originalSize);

        // Decompress using zlib
        size_t decompressedSize;
        const auto d = libdeflate_alloc_decompressor();
        const auto err = libdeflate_zlib_decompress(d, payload, payload_length, decompressed.data(), originalSize,
                                                    &decompressedSize);
        libdeflate_free_decompressor(d);

        if (err == LIBDEFLATE_SUCCESS && decompressedSize > 0) {
            payload = decompressed.data();
            payload_length = decompressedSize;
        }

        if (decompressedSize != static_cast<int>(originalSize) || err != LIBDEFLATE_SUCCESS) {
            LOG_DEBUG("decompressed to something other than the original size - err: %d\n", err);
        }
    } else if (payload) {
        print_named_data("packet payload", payload, payload_length);
    } else {
        LOG_DEBUG("payload not readable\n");
        return;
    }

    switch (type) {
        case type_message: {
            auto &peer = ble_connection_tracker.checkSenderInPeers(sender);
            if (Message message(ttl, timestamp_ms, packet_flags, sender, recipient);
                processMessage(message, payload, payload_length)) {
                if (const auto stored_message = ble_connection_tracker.storeMessageAndReturnIfNew(message)) {
                    ble_connection_tracker.enqueueBroadcastPacket(stored_message, &connection, &peer);
                }
            }
            break;
        }
        case type_announce: {
            //store a peer as we might need to pass along a message later
            updateOrStorePeerNameFromAnnouncement(sender, payload, payload_length, ttl, connection);
            ble_connection_tracker.possiblyUpdateTimeOffset(timestamp_ms);
            //continue into pass along
        }

        default:
            break;
    }
}
