#include "BinaryWriter.h"

#include <cstring>

void BinaryWriter::write_uint8(uint8_t value) const {
    vector.push_back(value);
}

void BinaryWriter::write_uint16(const uint16_t value) const {
    vector.push_back(static_cast<uint8_t>(value >> 8));
    vector.push_back(static_cast<uint8_t>(value) & 0xFF);
}

void BinaryWriter::write_uint32(const uint16_t value) const {
    vector.reserve(vector.size() + 4);
    vector.push_back(static_cast<uint8_t>(value >> 24) & 0xFF);
    vector.push_back(static_cast<uint8_t>(value >> 16) & 0xFF);
    vector.push_back(static_cast<uint8_t>(value >> 8) & 0xFF);
    vector.push_back(static_cast<uint8_t>(value) & 0xFF);
}

void BinaryWriter::write_uint64(const uint64_t value) const {
    vector.reserve(vector.size() + 8);
    vector.push_back(static_cast<uint8_t>(value >> 56));
    vector.push_back(static_cast<uint8_t>(value >> 48) & 0xFF);
    vector.push_back(static_cast<uint8_t>(value >> 40) & 0xFF);
    vector.push_back(static_cast<uint8_t>(value >> 32) & 0xFF);
    vector.push_back(static_cast<uint8_t>(value >> 24) & 0xFF);
    vector.push_back(static_cast<uint8_t>(value >> 16) & 0xFF);
    vector.push_back(static_cast<uint8_t>(value >> 8) & 0xFF);
    vector.push_back(static_cast<uint8_t>(value) & 0xFF);
}

void BinaryWriter::write_data(const uint8_t *data, const uint16_t len) const {
    vector.reserve(vector.size() + len);
    for (int i = 0; i < len; i++) {
        vector.push_back(data[i]);
    }
}

void BinaryWriter::write_data(const std::string &data, uint16_t len) const {
    vector.reserve(vector.size() + len);
    for (int i = 0; i < len; i++) {
        vector.push_back(static_cast<uint8_t>(data[i]));
    }
}

uint16_t BinaryWriter::test_only_current_pos() const {
    return vector.size();
}

uint8_t BinaryWriter::hexify(uint8_t nibble) {
    if (nibble < 10) {
        nibble += '0';
    } else {
        nibble += 'a' - 10;
    }
    return nibble;
}

void BinaryWriter::write_uint8_hex16(const uint8_t value) const {
    const uint8_t high_nibble = hexify(value >> 4 & 0xF);
    vector.push_back(static_cast<uint8_t>(high_nibble));
    const uint8_t low_nibble = hexify(value & 0xF);
    vector.push_back(static_cast<uint8_t>(low_nibble));
}

void BinaryWriter::write_uint16_hex16(const uint16_t value) const {
    write_uint8_hex16(static_cast<uint8_t>(value >> 8));
    write_uint8_hex16(static_cast<uint8_t>(value));
}

void BinaryWriter::write_uint32_hex16(const uint32_t value) const {
    write_uint8_hex16(static_cast<uint8_t>(value >> 24));
    write_uint8_hex16(static_cast<uint8_t>(value >> 16));
    write_uint8_hex16(static_cast<uint8_t>(value >> 8));
    write_uint8_hex16(static_cast<uint8_t>(value));
}

void BinaryWriter::write_uint48_hex16(const uint64_t value) const {
    write_uint8_hex16(static_cast<uint8_t>(value >> 40));
    write_uint8_hex16(static_cast<uint8_t>(value >> 32));
    write_uint8_hex16(static_cast<uint8_t>(value >> 24));
    write_uint8_hex16(static_cast<uint8_t>(value >> 16));
    write_uint8_hex16(static_cast<uint8_t>(value >> 8));
    write_uint8_hex16(static_cast<uint8_t>(value));
}

void BinaryWriter::write_uint64_hex16(const uint64_t value) const {
    write_uint8_hex16(static_cast<uint8_t>(value >> 56));
    write_uint8_hex16(static_cast<uint8_t>(value >> 48));
    write_uint8_hex16(static_cast<uint8_t>(value >> 40));
    write_uint8_hex16(static_cast<uint8_t>(value >> 32));
    write_uint8_hex16(static_cast<uint8_t>(value >> 24));
    write_uint8_hex16(static_cast<uint8_t>(value >> 16));
    write_uint8_hex16(static_cast<uint8_t>(value >> 8));
    write_uint8_hex16(static_cast<uint8_t>(value));
}
