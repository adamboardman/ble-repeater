#pragma once

#include <cstdint>
#include <string>
#include <vector>

class Peer {
public:
    Peer();

    Peer(uint64_t id, const std::string &peer_name);

    void updateName(const std::string &peer_name);

    uint64_t getId() const;

    std::string &getName();

    void setId(uint64_t new_id);

    void updateNoisePublicKey(const std::vector<uint8_t> &new_public_key);

    std::vector<uint8_t> &getPublicKey();

    [[nodiscard]] uint8_t getAnnounceTtl() const;

    void setAnnounceTtl(uint8_t ttl);

    void setConnectionHandle(uint16_t hci_con_handle);

private:
    uint64_t id{};
    std::string name{};
    std::vector<uint8_t> public_key{};
    uint8_t max_ttl{};
    uint16_t connection_handle{};
};

