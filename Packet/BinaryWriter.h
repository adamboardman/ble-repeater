#pragma once

#include <cstdint>
#include <vector>
#include <string>

class BinaryWriter {
public:
    explicit BinaryWriter(std::vector<uint8_t> &vector) : vector(vector) {
    }

    void write_uint8(uint8_t value) const;

    void write_uint16(uint16_t value) const;

    void write_uint32(uint16_t value) const;

    void write_uint64(uint64_t value) const;

    void write_data(const uint8_t *data, uint16_t len) const;

    void write_data(const std::string &data, uint16_t len) const;

    [[nodiscard]] uint16_t test_only_current_pos() const;

    static uint8_t hexify(uint8_t nibble);

    void write_uint8_hex16(uint8_t value) const;

    void write_uint16_hex16(uint16_t value) const;

    void write_uint32_hex16(uint32_t value) const;

    void write_uint48_hex16(uint64_t value) const;

    void write_uint64_hex16(uint64_t value) const;

private:
    std::vector<uint8_t> &vector;
};
