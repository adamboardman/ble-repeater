#pragma once

#include "PacketBase.h"


class PacketPassAlong : public PacketBase {
public:
    PacketPassAlong();

    explicit PacketPassAlong(uint8_t type);

    explicit PacketPassAlong(uint8_t type, uint8_t ttl, uint64_t timestamp, uint8_t flags, uint64_t sender,
                             uint64_t recipient);

    PacketPassAlong(uint8_t type, uint8_t version, bool hasPayload, BinaryReader &reader);

    void setPayload(const std::string &value);

    [[nodiscard]] const std::string &getPayload() const;

    [[nodiscard]] std::size_t getPacketHash() const;

    void writePacket(std::vector<uint8_t> &vector) override;

    void writePacketPayload(BinaryWriter &writer) override;

private:
    std::string payload;
    bool hasPayload;
};
