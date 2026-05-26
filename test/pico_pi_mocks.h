#ifndef PICO_PI_MOCKS_H
#define PICO_PI_MOCKS_H

#include <cstdint>
#include <vector>
#include "packet_repeater_mocks.h"


extern int lastAddress;
extern int last_length_read;
extern int last_length_written;
extern long mock_data_read;
extern std::vector<uint8_t> mock_write_data;
extern std::vector<uint8_t> mock_read_data;

void reset_for_test(const i2c_inst_t *i2c);

void set_read_data(std::vector<uint8_t> &data);

void set_expected_write_data(std::vector<uint8_t> &data);

int i2c_read_blocking(i2c_inst_t *i2c, uint8_t addr, uint8_t *dst, size_t len, bool nostop);

int i2c_write_blocking(i2c_inst_t *i2c, uint8_t addr, const uint8_t *src, size_t len, bool nostop);

void sleep_us(int us);

void sleep_ms(int ms);

void set_mock_time(uint64_t now);

uint64_t time_us_64();

uint32_t save_and_disable_interrupts();

void restore_interrupts(uint32_t status);

void reset_sent_for_test();

extern std::vector<uint8_t> mock_sent_data;


#endif // PICO_PI_MOCKS_H
