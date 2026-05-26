#pragma once

#include "PacketBase.h"

class Announce final : public PacketBase {
public:
    Announce();

    void setName(std::string value);

    [[nodiscard]] const std::string &getName() const;

private:
    std::string name;
};
