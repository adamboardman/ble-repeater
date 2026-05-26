#pragma once

#include <vector>

#include "Message.h"

class ProtocolWriter {
public:
    static void writePacket(std::vector<uint8_t> &vector, const PacketBase *packet_base);

    static void writeMessagePayload(std::vector<uint8_t> &vector, const Message &message);
};
