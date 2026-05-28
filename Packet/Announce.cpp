#include "Announce.h"

#include <utility>

#include "Debugging.h"

Announce::Announce()
    : PacketPassAlong(type_announce) {
}

Announce::Announce(const uint8_t type, const uint8_t version, BinaryReader &reader)
    : PacketPassAlong(type, version, false, reader) {

    const auto name_len = reader.read_uint8();
    const auto name_data = reader.read_data(name_len);
    if (name_data) {
        name = std::string(reinterpret_cast<const char *>(name_data), name_len);
        LOG_DEBUG("name_len: %d\n", name_len);
    } else {
        name = "";
    }
}

void Announce::setName(std::string value) {
    name = std::move(value);
}

const std::string &Announce::getName() const {
    return name;
}

void Announce::writePacket(std::vector<uint8_t> &vector) {
    PacketPassAlong::writePacket(vector);
}

void Announce::writePacketPayload(BinaryWriter &writer) {
    PacketPassAlong::writePacketPayload(writer);

    const auto name_len = static_cast<uint8_t>(std::min(static_cast<size_t>(255), name.size()));
    writer.write_uint8(name_len);
    writer.write_data(name, name_len);
}
