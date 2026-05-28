#pragma once

enum PacketType {
    type_unknown = 0,
    //Skip values used by other chat clients
    type_announce = 0x41,
    type_message = 0x42
};

enum PacketFlag {
    packet_flag_has_recipient = 0x01,
    packet_flag_has_signature = 0x02,
    packet_flag_is_compressed = 0x04,
    packet_flag_has_route = 0x08
};
