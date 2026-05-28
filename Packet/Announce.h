#pragma once

#include "PacketBase.h"
#include "PacketPassAlong.h"

class Announce final : public PacketPassAlong {
public:
    Announce();

    Announce(uint8_t type, uint8_t version, BinaryReader &reader);

    void setName(std::string value);

    [[nodiscard]] const std::string &getName() const;

    void writePacket(std::vector<uint8_t> &vector) override;

    void writePacketPayload(BinaryWriter &writer) override;

private:
    std::string name;
};
