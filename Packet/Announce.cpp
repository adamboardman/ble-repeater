#include "Announce.h"

#include <utility>

Announce::Announce()
    : PacketBase(type_announce) {
}

void Announce::setName(std::string value) {
    name = std::move(value);
}

const std::string &Announce::getName() const {
    return name;
}
