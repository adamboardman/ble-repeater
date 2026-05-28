#pragma once

#include <map>
#include <set>

#include "BleConnection.h"
#include "../Packet/Message.h"
#include "../Packet/Peer.h"
#include "../Packet/Announce.h"
#include "../Packet/PacketPassAlong.h"

inline auto build_time_ms = 1755685519; //Occasionally update this, us since 1970
inline auto two_seconds_in_us = 1000 * 1000 * 2;
inline auto two_minutes_in_us = 1000 * 1000 * 60 * 5;
inline auto five_minutes_in_us = 1000 * 1000 * 60 * 5;
inline auto ten_minutes_in_us = 1000 * 1000 * 60 * 10;
inline auto ten_minutes_in_ms = 1000 * 60 * 10;

class BleConnectionTracker {
public:
    BleConnection &connectionForConnHandle(hci_con_handle_t connection_handle);

    Message *storeMessageAndReturnIfNew(Message &message);

    PacketPassAlong *storePacketAndReturnIfNew(PacketPassAlong &pass_along);

    Message *messageWithId(const std::string &id);

    Peer *peerWithId(uint64_t id);

    Peer &checkSenderInPeers(uint64_t sender);

    void enqueueTargetedPacket(Base *packet, BleConnection *to_connection);

    void enqueueBroadcastPacket(Base *packet);

    void enqueueBroadcastPacket(Base *packet, BleConnection *from_connection, Peer *from_peer);

    void addAvailablePeer(const bd_addr_t &bt_address, bd_addr_type_t bt_address_type,
                          service_uuid_check_status services, int8_t rssi);

    void reportConnection(uint16_t handle, const bd_addr_t &addr, bd_addr_type_t address_type);

    void reportConnection(uint16_t handle, const bd_addr_t &addr, bd_addr_type_t address_type, uint8_t role);

    void reportDisconnection(uint16_t handle);

    std::vector<BleConnection *> getConnectableNeighbours();

    bool requestNextRssi(bool restart);

    void printStats();

    bool SendPacketToConnection(Base &packet, BleConnection &ble_connection);

    bool havePacketsToSend();

    void sendPackets();

    void possiblyUpdateTimeOffset(uint64_t timestamp_ms);

    [[nodiscard]] uint64_t getTimeMs() const;

    void setConnectionStarted(const BleConnection *neighbour);

    void setupAnnounceIfNeeded();

    void announceToConnections();

    [[nodiscard]] size_t getTargetedPacketsToSendSize() const;

    PacketBase *getAnyPacket();

    void cleanupStaleItems();

    [[nodiscard]] size_t getConnectionsCount() const;

    void setConnectionHandleForPeer(uint16_t con_handle, Peer *peer);

    Peer *peerWithConnectionHandle(uint16_t con_handle);

    BleConnection *getConnectionForAddress(const uint8_t *address);

    void notifyRawPacket(hci_con_handle_t con_handle);

    void writeRawPacket(hci_con_handle_t con_handle);

    hci_con_handle_t getAnyDuplicateHandle();

private:
    //Store of peer data
    std::map<uint64_t, Peer> peers{};
    //Store of message data
    std::map<std::string, Message> messages{};
    //Store of pass along packets
    std::map<std::size_t, PacketPassAlong> packets{};
    //Store of self announcing data
    Announce announce{};
    //Store of active and disconnected connections
    std::map<hci_con_handle_t, BleConnection> connections{};
    //Store of potential connections
    std::map<std::string, BleConnection> available_neighbours{};
    //Store of raw packets to send
    std::multimap<hci_con_handle_t, std::vector<uint8_t> > raw_packet_to_notify{};
    std::multimap<hci_con_handle_t, std::vector<uint8_t> > raw_packet_to_write{};

    uint64_t timestamp_offset_ms{};

    std::map<hci_con_handle_t, Peer *> handle_peer_map{};
    std::multimap<Base *, BleConnection *> packets_connections_sent_list{};
    std::multimap<Base *, Peer *> packets_peers_sent_list{};
    std::vector<Base *> broadcast_packets_to_send_list{};
    std::multimap<Base *, BleConnection *> targeted_packets_to_send_list{};
};
