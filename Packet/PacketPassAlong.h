#pragma once

#include "PacketBase.h"


class PacketPassAlong final : public PacketBase {
public:
    PacketPassAlong();

    explicit PacketPassAlong(uint8_t type, uint8_t ttl, uint64_t timestamp, uint8_t flags, uint64_t sender,
                             uint64_t recipient);


    void setPayload(std::string &value);

    [[nodiscard]] const std::string &getPayload() const;

    [[nodiscard]] std::size_t getPacketHash() const;

private:
    std::string payload;
};
