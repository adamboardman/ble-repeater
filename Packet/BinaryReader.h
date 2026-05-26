#pragma once

#include <cstdint>

class BinaryReader {
public:
    explicit BinaryReader(const uint16_t offset, const uint8_t *buffer, const uint16_t buffer_size) : offset(offset),
        buffer(buffer), buffer_size(buffer_size), pos(offset) {
    }

    uint8_t read_uint8();

    uint16_t read_uint16();

    uint32_t read_uint32();

    uint64_t read_uint64();

    const uint8_t *read_data(uint16_t len);

    [[nodiscard]] uint16_t test_only_current_pos() const;

private:
    const uint16_t offset;
    const uint8_t *buffer;
    const uint16_t buffer_size;
    uint16_t pos;
};
