#include <ranges>
#include <algorithm>
#include <cstdlib>
#include <limits>
#include <malloc.h>

#include "Debugging.h"
#include "BleConnectionTracker.h"

#include "../include/int_types.h"
#include "../Packet/ProtocolWriter.h"
#include "../Packet/Announce.h"
#include "../Packet/BinaryWriter.h"

#ifdef MOCK_PICO_PI
#include "../test/packet_repeater_mocks.h"
#include "../test/pico_pi_mocks.h"
#else
#include "pico/time.h"
#include "hardware/timer.h"
#include "ble/att_server.h"
#endif

extern BleConnectionTracker *connection_tracker_ptr;
static btstack_context_callback_registration_t notify_context_callback_registration;
static btstack_context_callback_registration_t write_context_callback_registration;

void packet_can_send_notification_handler(void *context) {
    LOG_DEBUG("packet_can_send_notification_handler(0x%02x)\n", context);
#if __WORDSIZE == 64
    hci_con_handle_t con_handle = reinterpret_cast<uint64_t>(context) & 0xffff;
#else
    auto con_handle = reinterpret_cast<uint32_t>(context);
#endif
    if (connection_tracker_ptr)
        connection_tracker_ptr->notifyRawPacket(con_handle);
}

void packet_can_write_without_response_handler(void *context) {
    LOG_DEBUG("packet_can_write_without_response_handler(0x%02x)\n", context);
#if __WORDSIZE == 64
    hci_con_handle_t con_handle = reinterpret_cast<uint64_t>(context) & 0xffff;
#else
    auto con_handle = reinterpret_cast<uint32_t>(context);
#endif
    if (connection_tracker_ptr)
        connection_tracker_ptr->writeRawPacket(con_handle);
}

BleConnection &BleConnectionTracker::connectionForConnHandle(const hci_con_handle_t connection_handle) {
    if (const auto search = connections.find(connection_handle); search != connections.end()) {
        return connections[connection_handle];
    }
    connections[connection_handle].setConnectionHandle(connection_handle);
    return connections[connection_handle];
}

const Message *BleConnectionTracker::storeMessageAndReturnIfNew(Message &message) {
    const auto id = message.getMessageId();
    if (const auto search = messages.find(id); search != messages.end()) {
        return nullptr; //message was found so it not new
    }
    messages[id] = std::move(message);
    return &messages[id];
}

const PacketPassAlong *BleConnectionTracker::storePacketAndReturnIfNew(PacketPassAlong &pass_along) {
    const auto id = pass_along.getPacketHash();
    if (const auto search = packets.find(id); search != packets.end()) {
        return nullptr; //packet was found so it not new
    }
    packets[id] = std::move(pass_along);
    return &packets[id];
}

Message *BleConnectionTracker::messageWithId(const std::string &id) {
    if (const auto search = messages.find(id); search != messages.end()) {
        return &search->second;
    }
    return nullptr;
}

Peer *BleConnectionTracker::peerWithId(const uint64_t id) {
    if (const auto search = peers.find(id); search != peers.end()) {
        return &search->second;
    }
    return nullptr;
}

Peer &BleConnectionTracker::checkSenderInPeers(const uint64_t sender) {
    if (const auto search = peers.find(sender); search == peers.end()) {
        peers[sender].setId(sender);
    }
    return peers[sender];
}

void BleConnectionTracker::enqueueTargetedPacket(const PacketBase *packet, BleConnection *to_connection) {
    if (packet->getPacketTtl() > 0) {
        auto con = &connectionForConnHandle(to_connection->getConnectionHandle());
        targeted_packets_to_send_list.emplace(packet, con);
    }
}

void BleConnectionTracker::enqueueBroadcastPacket(const PacketBase *packet) {
    if (packet->getPacketTtl() > 0) {
        broadcast_packets_to_send_list.push_back(packet);
    }
}

void BleConnectionTracker::enqueueBroadcastPacket(const PacketBase *packet, BleConnection *from_connection,
                                                  Peer *from_peer) {
    packets_connections_sent_list.emplace(packet, from_connection);
    packets_peers_sent_list.emplace(packet, from_peer);
    enqueueBroadcastPacket(packet);
}

void BleConnectionTracker::addAvailablePeer(const bd_addr_t &bt_address, const bd_addr_type_t bt_address_type,
                                            const service_uuid_check_status services, const int8_t rssi) {
    const auto address = std::string(reinterpret_cast<const char *>(bt_address), BD_ADDR_LEN);
    available_neighbours[address].setBleAddress(bt_address, bt_address_type);
    available_neighbours[address].setServices(services);
    available_neighbours[address].setRssi(rssi);
    available_neighbours[address].setTimestamp(time_us_64());
}

void BleConnectionTracker::reportConnection(const uint16_t handle, const bd_addr_t &addr,
                                            const bd_addr_type_t address_type) {
    const auto address = std::string(reinterpret_cast<const char *>(addr), BD_ADDR_LEN);
    if (const auto item = available_neighbours.find(address); item != available_neighbours.end()) {
        connections[handle] = item->second;
        available_neighbours.erase(address);
    }
    connections[handle].setConnectionHandle(handle);
    connections[handle].setConnected(true);
    connections[handle].setBleAddress(addr, address_type);
    connections[handle].setTimestamp(time_us_64());
    available_neighbours.erase(address);
}

void handle_gatt_client_value_update_event(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);

void BleConnectionTracker::reportConnection(const uint16_t handle, const bd_addr_t &addr,
                                            const bd_addr_type_t address_type, const uint8_t role) {
    reportConnection(handle, addr, address_type);
    auto &connection = connections[handle];
    connection.setRole(role);

    if (role == HCI_ROLE_MASTER) {
        gatt_client_characteristic_t le_characteristic;
        //gatt_client_listen_for_characteristic_value_updates only reads the value_handle
        le_characteristic.value_handle = connection.getPacketCharacteristicValueHandle();
        gatt_client_listen_for_characteristic_value_updates(connection.getNotificationListener(),
                                                            handle_gatt_client_value_update_event, handle,
                                                            &le_characteristic);
    }
}

void BleConnectionTracker::reportDisconnection(const uint16_t handle) {
    auto &removed_connection = connections[handle];
    removed_connection.setNotificationEnabled(false);
    removed_connection.setConnected(false);

    if (removed_connection.getRole() == HCI_ROLE_MASTER) {
        gatt_client_stop_listening_for_characteristic_value_updates(removed_connection.getNotificationListener());
    }
    auto announced_to_connection = [this, removed_connection](const auto &item) {
        const auto &[packet, connection] = item;

        return packet == &announce && connection == &removed_connection;
    };
    const auto handle_peers_removed = handle_peer_map.erase(handle);
    const auto announce_packets_removed = std::erase_if(packets_connections_sent_list, announced_to_connection);
    LOG_DEBUG("disconnection - removed announce packets: %d, handle_peers_removed: %d\n", announce_packets_removed,
              handle_peers_removed);
}

std::vector<BleConnection *> BleConnectionTracker::getConnectableNeighbours() {
    std::vector<BleConnection *> neighbours;
    const auto now = time_us_64();
    for (auto &connection: available_neighbours | std::views::values) {
        if ((timestamp_offset_ms >= build_time_ms || connection.isRepeater()) &&
            connection.isConnected() == false && !connection.isRandom()) {
            neighbours.push_back(&connection);
        }
    }
    return neighbours;
}

bool BleConnectionTracker::requestNextRssi(const bool restart) {
    static auto iterator = connections.begin();
    if (iterator == connections.end() || restart) {
        iterator = connections.begin();
    }
    auto [handle, connection] = *iterator;
    gap_read_rssi(handle); //requested and will call back
    ++iterator;
    return (iterator != connections.end());
}

extern "C" char __StackLimit; // NOLINT(*-reserved-identifier)
extern "C" char __bss_end__; // NOLINT(*-reserved-identifier)

void BleConnectionTracker::printStats() {
    const auto total_heap = &__StackLimit - &__bss_end__;
    const auto used_heap = mallinfo().uordblks;
    const auto free_heap = total_heap - used_heap;

    const uint32_t time_ms = time_us_64() / 1000u;
    const uint32_t seconds = time_ms / 1000u;
    const uint32_t minutes = seconds / 60;
    const uint32_t hours = minutes / 60;

    const uint16_t p_ms = time_ms - (seconds * 1000u);
    const uint16_t p_seconds = seconds - (minutes * 60);
    const uint16_t p_minutes = minutes - (hours * 60u);
    LOG_DEBUG("[%02u:%02u:%02u.%03u]", hours, p_minutes, p_seconds, p_ms);
    LOG_DEBUG("%d %d:%d", timestamp_offset_ms>0, used_heap, free_heap);

    auto active_connections_count = 0;
    for (auto &connection: connections | std::views::values) {
        if (connection.isConnected()) {
            LOG_DEBUG("{0x%x:%d-%s}", connection.getConnectionHandle(), connection.getRole(),
                      bd_addr_to_str(connection.getAddress()));
            active_connections_count++;
        }
    }
    LOG_DEBUG("con: %d/%d, avail: %d, messages:%d, packets:%d, broadcast: %d, targeted: %d\n", active_connections_count,
              connections.size(), available_neighbours.size(), messages.size(), packets.size(),
              broadcast_packets_to_send_list.size(),
              targeted_packets_to_send_list.size());
}

#pragma GCC push_options
#pragma GCC optimize ("O0")

bool BleConnectionTracker::SendPacketToConnection(const PacketBase &packet, BleConnection &ble_connection) {
    std::vector<uint8_t> packet_data;
    ProtocolWriter::writePacket(packet_data, &packet);

    if (packet_data.size() > ble_connection.getMtu()) {
        //TODO - implement fragment creation
        assert(0);
    }

    const uint16_t con_handle = ble_connection.getConnectionHandle();
    std::string peer_string{};
    uint64_t sender_id = 0;
    const auto peer = peerWithConnectionHandle(con_handle);
    if (peer && !peer->getName().empty()) {
        peer_string.append(peer->getName());
    }
    if (peer) {
        sender_id = peer->getId();
    }
    hci_connection_t *hci_connection = hci_connection_for_handle(con_handle);

    LOG_DEBUG("SendPacketToConnection - type(%d), peer(%s:0x%" PRIx64 "), hci_connection_for_handle(0x%x), hc(0x%x)\n",
              packet.getPacketType(), peer_string.c_str(), sender_id,
              con_handle, hci_connection);
    uint8_t status = 0;
    ble_connection.setHasData(true);
    if (ble_connection.getRole() == HCI_ROLE_SLAVE) {
        raw_packet_to_notify.emplace(con_handle, packet_data);
        notify_context_callback_registration.callback = &packet_can_send_notification_handler;
        notify_context_callback_registration.context = reinterpret_cast<void *>(con_handle);
        status = att_server_request_to_send_notification(&notify_context_callback_registration, con_handle);
    } else {
        raw_packet_to_write.emplace(con_handle, packet_data);
        write_context_callback_registration.callback = &packet_can_write_without_response_handler;
        write_context_callback_registration.context = reinterpret_cast<void *>(con_handle);
        status = gatt_client_request_to_write_without_response(&write_context_callback_registration, con_handle);
    }
    if (status) {
        LOG_DEBUG("SendPacketToConnection - Write without response failed, status 0x%02x.\n", status);
        sleep_ms(20);
        return false;
    }
    sleep_ms(20);
    return true;
}

bool BleConnectionTracker::havePacketsToSend() {
    return (!(broadcast_packets_to_send_list.empty() && targeted_packets_to_send_list.empty()));
}

void BleConnectionTracker::sendPackets() {
    if (broadcast_packets_to_send_list.empty() && targeted_packets_to_send_list.empty()) {
        return;
    }
    auto available = [](const BleConnection &connection) {
        return connection.isConnected() && connection.getPacketCharacteristicValueHandle() > 0;
    };
    auto available_connections = connections | std::views::values | std::views::filter(available);

    auto packet_needed = [this](const std::pair<const PacketBase *, const BleConnection *> &targeted) {
        auto [begin, end] = packets_connections_sent_list.equal_range(targeted.first);
        auto matches_connection = [targeted](const std::pair<const PacketBase *, const BleConnection *> &sent_to) {
            return sent_to.second->getConnectionHandle() == targeted.second->getConnectionHandle();
        };
        return std::ranges::find_if(begin, end, matches_connection) == end;
    };
    std::set<const PacketBase *> targeted_packets_to_remove;
    for (const auto &[packet, connection]: targeted_packets_to_send_list | std::views::filter(packet_needed)) {
        LOG_DEBUG("Sending Targeted Packet %p, 0x%x\n", packet, connection->getConnectionHandle());
        SendPacketToConnection(*packet, *connection);
        packets_connections_sent_list.emplace(packet, connection);
        targeted_packets_to_remove.emplace(packet);
    }
    for (auto packet: targeted_packets_to_remove) {
        auto [being, end] = targeted_packets_to_send_list.equal_range(packet);
        targeted_packets_to_send_list.erase(being, end);
    }

    std::set<const PacketBase *> broadcast_packets_to_remove;
    for (auto packet: broadcast_packets_to_send_list) {
        bool packet_sent = false;
        // LOG_DEBUG("Sending Broadcast Packet %p\n", packet);
        for (auto &connection: available_connections) {
            bool sendable = true;
            for (auto search = packets_connections_sent_list.find(packet);
                 search != packets_connections_sent_list.end(); ++search) {
                if (search->second->getConnectionHandle() == connection.getConnectionHandle()) {
                    // LOG_DEBUG("Not Sendable 0x%x\n", connection.getConnectionHandle());
                    sendable = false;
                }
            }
            if (sendable) {
                SendPacketToConnection(*packet, connection);
                packets_connections_sent_list.emplace(packet, &connection);
                packet_sent = true;
            }
        }

        if (packet_sent) {
            broadcast_packets_to_remove.emplace(packet);
        }
    }
    auto sent = [broadcast_packets_to_remove](const PacketBase *packet) {
        return broadcast_packets_to_remove.contains(packet);
    };
    const auto ret = std::ranges::remove_if(broadcast_packets_to_send_list, sent);
    broadcast_packets_to_send_list.erase(ret.begin(), ret.end());
}
#pragma GCC pop_options

constexpr uint64_t MESSAGE_TIMEOUT = 300000L; //5mins in ms

void BleConnectionTracker::possiblyUpdateTimeOffset(const uint64_t timestamp_ms) {
    const uint64_t our_now = getTimeMs();
    const int64_t diff = static_cast<int64_t>(timestamp_ms) - static_cast<int64_t>(our_now);
    const auto diff_abs = llabs(diff);
    if (diff_abs > MESSAGE_TIMEOUT || timestamp_ms > our_now) {
        timestamp_offset_ms += diff;
    }
}

uint64_t BleConnectionTracker::getTimeMs() const {
    return timestamp_offset_ms + (time_us_64() / 1000); //time in ms
}

void BleConnectionTracker::setConnectionStarted(const BleConnection *neighbour) {
    const auto address = std::string(reinterpret_cast<const char *>(neighbour->getAddress()), BD_ADDR_LEN);
    available_neighbours.erase(address);
}

void BleConnectionTracker::setupAnnounceIfNeeded() {
    if (announce.getPacketTtl() == 3) {
        return;
    }
    uint64_t sender{};
    bd_addr_t local_addr;
    gap_local_bd_addr(local_addr);
    memcpy(&sender, local_addr, BD_ADDR_LEN);
    announce.setPacketTtl(3);
    announce.setPacketFlags(0);
    announce.setPacketSenderId(sender);

    std::vector<uint8_t> buffer;
    const BinaryWriter writer(buffer);
    writer.write_data(reinterpret_cast<const uint8_t *>(packet_service_name), strlen(packet_service_name));
    writer.write_data(":",1);
    writer.write_uint8_hex16(local_addr[BD_ADDR_LEN - 2]);
    writer.write_uint8_hex16(local_addr[BD_ADDR_LEN - 1]);
    announce.setName(std::string(reinterpret_cast<const char *>(buffer.data()), buffer.size()));
}

void BleConnectionTracker::announceToConnections() {
    setupAnnounceIfNeeded();

    auto available = [&](const BleConnection &connection) {
        return connection.isConnected()
               && connection.getPacketCharacteristicValueHandle() > 0
               && timestamp_offset_ms >= build_time_ms;
    };
    auto available_connections = connections | std::views::values | std::views::filter(available);

    auto announced_to = packets_connections_sent_list.equal_range(&announce);
    auto not_announced_to = [announced_to](const BleConnection &connection) {
        auto matches_connection = [connection](const std::pair<const PacketBase *, const BleConnection *> &sent_to) {
            // LOG_DEBUG("announced_to connection(0x%x), available connection(0x%x)\n",sent_to.second->getConnectionHandle(), connection.getConnectionHandle());
            return sent_to.second->getConnectionHandle() == connection.getConnectionHandle();
        };
        const auto ret = std::ranges::find_if(announced_to.first, announced_to.second, matches_connection) ==
                         announced_to.second;
        // LOG_DEBUG("not_announced_to: %d\n",ret);
        return ret;
    };
    for (auto connection: available_connections | std::views::filter(not_announced_to)) {
        LOG_DEBUG("Announce To Connection(0x%x)\n", connection.getConnectionHandle());
        announce.setPacketTimestamp(getTimeMs());
        enqueueTargetedPacket(&announce, &connection);
    }
}

size_t BleConnectionTracker::getTargetedPacketsToSendSize() const {
    return targeted_packets_to_send_list.size();
}

PacketBase *BleConnectionTracker::getAnyPacket() {
    if (!packets.empty()) {
        return &packets.begin()->second;
    }
    return nullptr;
}

void BleConnectionTracker::cleanupStaleItems() {
    auto now = getTimeMs();

    auto packet_connection_stale = [now](const auto &item) {
        const auto &[packet, connection] = item;
        return packet->getPacketTimestamp() + ten_minutes_in_ms < now ||
               !connection->isConnected() && connection->getTimestampMs() + ten_minutes_in_ms < now;
    };
    const auto packets_connections_removed = std::erase_if(packets_connections_sent_list, packet_connection_stale);
    auto packet_peer_stale = [now](const auto &item) {
        const auto &[packet, peer] = item;
        return packet->getPacketTimestampMs() + ten_minutes_in_ms < now;
    };
    const auto packets_peers_sent_list_removed = std::erase_if(packets_peers_sent_list, packet_peer_stale);
    auto packet_stale = [now](const auto &packet) {
        return packet->getPacketTimestampMs() + ten_minutes_in_ms < now;
    };
    const auto broadcast_packets_removed = std::erase_if(broadcast_packets_to_send_list, packet_stale);
    const auto targeted_packets_removed = std::erase_if(targeted_packets_to_send_list, packet_connection_stale);
    auto connection_stale = [now](const auto &item) {
        const auto &[key, connection] = item;
        return !connection.isConnected() && connection.getTimestampMs() + ten_minutes_in_ms < now;
    };
    const auto connections_removed = std::erase_if(connections, connection_stale);
    const auto available_neighbours_removed = std::erase_if(available_neighbours, connection_stale);
    const auto messages_removed = std::erase_if(messages, [now](const auto &item) {
        const auto &[key, message] = item;
        return message.getPacketTimestampMs() + ten_minutes_in_ms < now;
    });
    const auto packets_removed = std::erase_if(packets, [now](const auto &item) {
        const auto &[key, packet] = item;
        return packet.getPacketTimestampMs() + ten_minutes_in_ms < now;
    });

    LOG_DEBUG(
        "Cleanup items removed: connections(%d), neighbours(%d), messages(%d), packets(%d), packet con(%d), packet peer(%d), broadcast(%d), targeted(%d)\n",
        connections_removed, available_neighbours_removed, messages_removed, packets_removed,
        packets_connections_removed, packets_peers_sent_list_removed, broadcast_packets_removed,
        targeted_packets_removed);
}

size_t BleConnectionTracker::getConnectionsCount() const {
    return connections.size() + available_neighbours.size();
}

void BleConnectionTracker::setConnectionHandleForPeer(const uint16_t con_handle, Peer *peer) {
    handle_peer_map[con_handle] = peer;
}

Peer *BleConnectionTracker::peerWithConnectionHandle(const uint16_t con_handle) {
    if (const auto search = handle_peer_map.find(con_handle); search != handle_peer_map.end()) {
        return handle_peer_map[con_handle];
    }
    return nullptr;
}

BleConnection *BleConnectionTracker::getConnectionForAddress(const uint8_t *address) {
    auto search = [&](const BleConnection &connection) {
        return connection.isConnected() && memcmp(connection.getAddress(), address,BD_ADDR_LEN) == 0;
    };
    if (auto found = connections | std::views::values | std::views::filter(search); !found.empty()) {
        return &*found.begin();
    }
    return nullptr;
}

void BleConnectionTracker::notifyRawPacket(const hci_con_handle_t con_handle) {
    auto packets_for_handle = raw_packet_to_notify.equal_range(con_handle);
    if (packets_for_handle.first != raw_packet_to_notify.end()) {
        const auto connection = connections[con_handle];
        assert(connection.hasData());
        const auto &data = packets_for_handle.first->second;
        const auto ret = att_server_notify(con_handle, connection.getPacketCharacteristicValueHandle(), data.data(),
                                           data.size());
        LOG_DEBUG("notifyRawPacket(0x%x) ret:%d\n", con_handle, ret);
        if (ret == ERROR_CODE_SUCCESS || ret == ERROR_CODE_UNKNOWN_CONNECTION_IDENTIFIER) {
            raw_packet_to_notify.erase(packets_for_handle.first);
        }
    }
    packets_for_handle = raw_packet_to_notify.equal_range(con_handle);
    if (packets_for_handle.first == raw_packet_to_notify.end()) {
        auto connection = connections[con_handle];
        LOG_DEBUG("notifyRawPacket(0x%x) no more data\n", con_handle);
        connection.setHasData(false);
    } else {
        notify_context_callback_registration.callback = &packet_can_send_notification_handler;
        notify_context_callback_registration.context = reinterpret_cast<void *>(con_handle);
        LOG_DEBUG("notifyRawPacket(0x%x) request to send notification\n", con_handle);
        att_server_request_to_send_notification(&notify_context_callback_registration, con_handle);
    }
}

void BleConnectionTracker::writeRawPacket(const hci_con_handle_t con_handle) {
    auto packets_for_handle = raw_packet_to_write.equal_range(con_handle);
    if (packets_for_handle.first != raw_packet_to_write.end()) {
        const auto connection = connections[con_handle];
        assert(connection.hasData());
        auto &data = packets_for_handle.first->second;
        const auto ret = gatt_client_write_value_of_characteristic_without_response(
            con_handle, connection.getPacketCharacteristicValueHandle(), data.size(), data.data());
        LOG_DEBUG("writeRawPacket(0x%x) ret:%d\n", con_handle, ret);
        if (ret == ERROR_CODE_SUCCESS || ret == ERROR_CODE_UNKNOWN_CONNECTION_IDENTIFIER) {
            raw_packet_to_write.erase(packets_for_handle.first);
        }
    }
    packets_for_handle = raw_packet_to_write.equal_range(con_handle);
    if (packets_for_handle.first == raw_packet_to_write.end()) {
        auto connection = connections[con_handle];
        LOG_DEBUG("writeRawPacket(0x%x) no more data\n", con_handle);
        connection.setHasData(false);
    } else {
        write_context_callback_registration.callback = &packet_can_write_without_response_handler;
        write_context_callback_registration.context = reinterpret_cast<void *>(con_handle);
        LOG_DEBUG("writeRawPacket(0x%x) request to write without response\n", con_handle);
        gatt_client_request_to_write_without_response(&write_context_callback_registration, con_handle);
    }
}

hci_con_handle_t BleConnectionTracker::getAnyDuplicateHandle() {
    std::map<Peer *, hci_con_handle_t> reversed;
    for (auto &[handle, peer]: handle_peer_map) {
        if (reversed[peer] != 0) {
            //return the earlier connection - appears to get gazumped by the more recent one
            LOG_DEBUG(("rp ls: %d, h ls: %d \n"), connections[reversed[peer]].getTimestamp(),
                      connections[handle].getTimestamp());
            if (connections[reversed[peer]].getTimestamp() < connections[handle].getTimestamp()) {
                return reversed[peer];
            } //else
            return handle;
        }
        reversed[peer] = handle;
    }
    return 0;
}
