#include <catch2/catch_test_macros.hpp>
#include <cstdarg>
#include <cstdint>
#include <cstring>
#include <vector>

#include "Debugging.h"
#include "pico_pi_mocks.h"
#include "../Packet/BinaryReader.h"
#include "../Packet/ProtocolProcessor.h"

extern BleConnectionTracker *connection_tracker_ptr;

TEST_CASE("AnnounceToConnections", "[ann1]") {
    BleConnectionTracker tracker;
    connection_tracker_ptr = &tracker;
    tracker.possiblyUpdateTimeOffset(1755685519+40);
    const bd_addr_t addr{};
    tracker.reportConnection(1,addr,BD_ADDR_TYPE_LE_RANDOM);
    BleConnection &connection = tracker.connectionForConnHandle(1);
    connection.setPacketCharacteristicValueHandle(1);
    connection.setMtu(517);

    tracker.announceToConnections();

    REQUIRE(tracker.getTargetedPacketsToSendSize() == 1);

    tracker.sendPackets();

    tracker.announceToConnections();
    REQUIRE(tracker.getTargetedPacketsToSendSize() == 0);

    set_mock_time(1000*1000*60*15);
    connection.setConnected(false);

    REQUIRE(1 == tracker.getConnectionsCount());
    tracker.cleanupStaleItems();
    REQUIRE(0 == tracker.getConnectionsCount());
}

TEST_CASE("WriteMessageData", "[message]") {

    Message message(7, 1000, 0, 123456);
    message.setMessageFlags(0);
    message.setMessageTimestamp(1001);
    message.setMessageId("808080");
    message.setSenderNickname("TestSender");
    message.setContent("The Actual Message");

    std::vector<uint8_t> packetData;
    message.writePacket(packetData);
    print_named_data("packetData", packetData.data(), packetData.size());

    BinaryReader reader(0, packetData.data(), packetData.size());
    //Base
    REQUIRE(reader.read_uint8() == 1);//version
    REQUIRE(reader.read_uint8() == type_message);//type
    REQUIRE(reader.read_uint8() == 7);//ttl
    REQUIRE(reader.read_uint64() == 1000);//timestamp
    REQUIRE(reader.read_uint8() == 0);//flags
    //PacketBase
    REQUIRE(reader.read_uint64() == 123456);//sender
    //Message
    REQUIRE(reader.read_uint8() == 0);//flags
    REQUIRE(reader.read_uint64() == 1001);//timestamp
    auto messageIdLength = reader.read_uint8();
    REQUIRE(messageIdLength == 6);
    std::string messageId(reinterpret_cast<const char *>(reader.read_data(messageIdLength)), messageIdLength);
    REQUIRE(messageId == "808080");
    auto senderNickLength = reader.read_uint8();
    REQUIRE(senderNickLength == 10);
    std::string senderNick(reinterpret_cast<const char *>(reader.read_data(senderNickLength)), senderNickLength);
    REQUIRE(senderNick == "TestSender");
    auto payloadLength = reader.read_uint16();
    REQUIRE(payloadLength == 18);
    std::string content(reinterpret_cast<const char *>(reader.read_data(payloadLength)), payloadLength);
    REQUIRE(content == "The Actual Message");
}

const std::string message_data = "014207000000000000099a000000aeedd867cf2c00000000000000099a0c0000099a0000aeedd867cf2c11536d6f6b654465746563746f7265646165000d5b303030325d20426f6f746564";

TEST_CASE("MessageReadFromData", "[messageData]") {
    const uint8_t data_len = message_data.length() / 2;
    uint8_t uint_array[data_len];
    populate_array_from_string(uint_array, message_data);

    BleConnectionTracker tracker;
    connection_tracker_ptr = &tracker;
    const ProtocolProcessor processor(tracker);
    BleConnection &connectionTo = tracker.connectionForConnHandle(1);
    connectionTo.setConnected(true);
    connectionTo.setPacketCharacteristicValueHandle(1);
    connectionTo.setMtu(517);

    BleConnection &connectionFrom = tracker.connectionForConnHandle(2);
    connectionFrom.setConnected(true);
    connectionFrom.setPacketCharacteristicValueHandle(1);
    connectionFrom.setMtu(517);

    processor.processWrite(connectionFrom, 0, uint_array, sizeof(uint_array));

    const std::string msg_id = "0000099a0000aeedd867cf2c";
    std::string msg_id_string;
    populate_string_from_string(&msg_id_string, msg_id);

    const auto message = tracker.messageWithId(msg_id_string);
    REQUIRE(!message->getMessageId().empty());

    REQUIRE(message->getSenderNickname() == "SmokeDetectoredae");
    REQUIRE(message->getContent() == "[0002] Booted");
    REQUIRE(message->getSenderPeer() == nullptr);
}


TEST_CASE("WriteAndReadMessageData", "[message]") {

    Message messageIn(7, 3000, 0, 123456);
    messageIn.setMessageFlags(0);
    messageIn.setMessageTimestamp(3002);
    messageIn.setMessageId("808080");
    messageIn.setSenderNickname("TestSender");
    messageIn.setContent("The Actual Message");

    std::vector<uint8_t> packetData;
    messageIn.writePacket(packetData);

    BinaryReader reader(0, packetData.data(), packetData.size());
    //Base
    REQUIRE(reader.read_uint8() == 1);//version
    REQUIRE(reader.read_uint8() == type_message);//type
    REQUIRE(reader.read_uint8() == 7);//ttl
    REQUIRE(reader.read_uint64() == 3000);//timestamp
    REQUIRE(reader.read_uint8() == 0);//flags
    //PacketBase
    REQUIRE(reader.read_uint64() == 123456);//sender
    //Message
    REQUIRE(reader.read_uint8() == 0);//flags
    REQUIRE(reader.read_uint64() == 3002);//timestamp
    auto messageIdLength = reader.read_uint8();
    REQUIRE(messageIdLength == 6);
    std::string messageId(reinterpret_cast<const char *>(reader.read_data(messageIdLength)), messageIdLength);
    REQUIRE(messageId == "808080");
    auto senderNickLength = reader.read_uint8();
    REQUIRE(senderNickLength == 10);
    std::string senderNick(reinterpret_cast<const char *>(reader.read_data(senderNickLength)), senderNickLength);
    REQUIRE(senderNick == "TestSender");
    auto payloadLength = reader.read_uint16();
    REQUIRE(payloadLength == 18);
    std::string content(reinterpret_cast<const char *>(reader.read_data(payloadLength)), payloadLength);
    REQUIRE(content == "The Actual Message");

    BleConnectionTracker tracker;
    connection_tracker_ptr = &tracker;
    const ProtocolProcessor processor(tracker);
    BleConnection &connectionTo = tracker.connectionForConnHandle(1);
    connectionTo.setConnected(true);
    connectionTo.setPacketCharacteristicValueHandle(1);
    connectionTo.setMtu(517);

    BleConnection &connectionFrom = tracker.connectionForConnHandle(2);
    connectionFrom.setConnected(true);
    connectionFrom.setPacketCharacteristicValueHandle(1);
    connectionFrom.setMtu(517);

    processor.processWrite(connectionFrom, 0, packetData.data(), packetData.size());

    const auto message = tracker.messageWithId("808080");
    REQUIRE(!message->getMessageId().empty());

    REQUIRE(message->getSenderNickname() == "TestSender");
    REQUIRE(message->getContent() == "The Actual Message");
    REQUIRE(message->getSenderPeer() == nullptr);
}


TEST_CASE("AnnounceOnlyOnce","[Ann1]") {
    BleConnectionTracker tracker = {};
    connection_tracker_ptr = &tracker;
    const uint64_t timestamp = 0x198c702ff54 / 1000;
    tracker.possiblyUpdateTimeOffset(timestamp);

    BleConnection &connection1 = tracker.connectionForConnHandle(3);
    connection1.setConnected(true);
    connection1.setPacketCharacteristicValueHandle(7);
    connection1.setMtu(517);

    tracker.announceToConnections();

    reset_sent_for_test();
    tracker.sendPackets();

    REQUIRE(34 == mock_sent_data.size());

    BleConnection &connection2 = tracker.connectionForConnHandle(5);
    connection2.setConnected(true);
    connection2.setPacketCharacteristicValueHandle(7);
    connection2.setMtu(517);

    tracker.announceToConnections();

    reset_sent_for_test();
    tracker.sendPackets();

    REQUIRE(34 == mock_sent_data.size());

    set_mock_time(1000*1000*60*15);
}

TEST_CASE("AnnounceOnlyOnceNoSendCleanup","[Ann2]") {
    BleConnectionTracker tracker;
    connection_tracker_ptr = &tracker;
    constexpr uint64_t timestamp = 0x198c702ff54 / 1000;
    tracker.possiblyUpdateTimeOffset(timestamp);

    BleConnection &connection1 = tracker.connectionForConnHandle(3);
    connection1.setConnected(true);
    connection1.setPacketCharacteristicValueHandle(7);
    connection1.setMtu(517);

    tracker.announceToConnections();

    reset_sent_for_test();
    REQUIRE(0 == mock_sent_data.size());

    BleConnection &connection2 = tracker.connectionForConnHandle(5);
    connection2.setConnected(true);
    connection2.setPacketCharacteristicValueHandle(7);
    connection2.setMtu(517);

    tracker.announceToConnections();

    REQUIRE(0 == mock_sent_data.size());

    set_mock_time(timestamp + 1000*1000*60*15);

    REQUIRE(3 == tracker.getTargetedPacketsToSendSize());
    tracker.cleanupStaleItems();
    REQUIRE(0 == tracker.getTargetedPacketsToSendSize());
}
