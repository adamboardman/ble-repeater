#include "Peer.h"

Peer::Peer() = default;

Peer::Peer(const uint64_t id, const std::string &peer_name) : id(id), name(peer_name) {
}

void Peer::updateName(const std::string &peer_name) {
    name = peer_name;
}

uint64_t Peer::getId() const {
    return id;
}

std::string &Peer::getName() {
    return name;
}

void Peer::setId(uint64_t new_id) {
    id = new_id;
}

void Peer::updateNoisePublicKey(const std::vector<uint8_t> &new_public_key) {
    public_key = new_public_key;
}

std::vector<uint8_t> &Peer::getPublicKey() {
    return public_key;
}

uint8_t Peer::getAnnounceTtl() const {
    return max_ttl;
}

void Peer::setAnnounceTtl(const uint8_t ttl) {
    max_ttl = ttl;
}

void Peer::setConnectionHandle(uint16_t hci_con_handle) {
    connection_handle = hci_con_handle;
}
