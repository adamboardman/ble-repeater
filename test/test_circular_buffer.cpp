#include <catch2/catch_test_macros.hpp>
#include <cstdarg>
#include <cstdint>
#include <cstring>
#include <vector>

#include "pico_pi_mocks.h"
#include "../CircularBuffer/CircularBuffer.h"

#define BUFFER_LIMIT 200

CircularBuffer<char> circular_buffer(BUFFER_LIMIT);
auto alphabet = "abcdefghijklmnopqrstuvwxyz";

TEST_CASE("Circular Buffer - Basics", "[cb1]") {
    circular_buffer.clear();
    REQUIRE(circular_buffer.empty() == true);

    circular_buffer.write('a');
    REQUIRE(circular_buffer.empty() == false);
    REQUIRE(circular_buffer.consume() == 'a');
    REQUIRE(circular_buffer.empty() == true);

    circular_buffer.write(reinterpret_cast<const char *>("text"));
    REQUIRE(circular_buffer.consume() == 't');
    REQUIRE(circular_buffer.consume() == 'e');
    REQUIRE(circular_buffer.consume() == 'x');
    circular_buffer.clear_if_empty();
    REQUIRE(circular_buffer.full() == false);
    REQUIRE(circular_buffer.empty() == false);
    REQUIRE(circular_buffer.consume() == 't');
    circular_buffer.clear_if_empty();
    REQUIRE(circular_buffer.empty() == true);

    circular_buffer.clear();
    char string[10];
    memcpy(string, alphabet, 10);
    circular_buffer.write(string);
    REQUIRE(circular_buffer.full() == false);
    REQUIRE(circular_buffer.consume() == 'a');
    REQUIRE(circular_buffer.consume() == 'b');
    REQUIRE(circular_buffer.consume() == 'c');

    circular_buffer.clear();
    REQUIRE(circular_buffer.empty() == true);
    char string_char[10];
    memcpy(string_char, alphabet, 10);
    string_char[9] = 0;
    circular_buffer.write(string_char);
    REQUIRE(circular_buffer.consume() == 'a');
    REQUIRE(circular_buffer.consume() == 'b');
    REQUIRE(circular_buffer.consume() == 'c');
    auto out_expected = "defghi";
    uint16_t out_count = 6;
    char out_block[10];
    auto out_len = circular_buffer.consume_block(out_block, 10);
    REQUIRE(out_block[0] == out_expected[0]);
    REQUIRE(out_block[1] == out_expected[1]);
    REQUIRE(out_block[2] == out_expected[2]);
    REQUIRE(out_block[3] == out_expected[3]);
    REQUIRE(out_block[4] == out_expected[4]);
    REQUIRE(out_block[5] == out_expected[5]);
    REQUIRE(out_block[6] == out_expected[6]);
    auto cmp_res = strcmp(out_block, out_expected);
    REQUIRE(cmp_res ==0);
    REQUIRE(out_len == out_count);
}

TEST_CASE("Circular Buffer - Wraparound", "[cb2]") {
    circular_buffer.clear();
    circular_buffer.write(alphabet);
    REQUIRE(circular_buffer.consume() == 'a');
    circular_buffer.write(alphabet);
    circular_buffer.write(alphabet);
    circular_buffer.write(alphabet);
    REQUIRE(circular_buffer.consume() == 'b');
    REQUIRE(circular_buffer.full() == false);
    circular_buffer.write(alphabet);
    REQUIRE(circular_buffer.full() == false);
    circular_buffer.write(alphabet);
    char out_block[10];
    auto out_len = circular_buffer.consume_block(out_block, 10);
    REQUIRE(out_len == 10);
    REQUIRE(circular_buffer.full() == false);
    circular_buffer.write(alphabet);
    REQUIRE(circular_buffer.full() == false);
    circular_buffer.write(alphabet);
    REQUIRE(circular_buffer.full() == false);
    circular_buffer.write(alphabet);
    REQUIRE(circular_buffer.full() == true);
    circular_buffer.write('A');
    REQUIRE(circular_buffer.full() == true);
    REQUIRE(circular_buffer.consume() == 'k');
    REQUIRE(circular_buffer.full() == false);
    out_len = circular_buffer.consume_block(out_block, 10);
    REQUIRE(out_len == 10);
    while (circular_buffer.consume() != 0) {
        REQUIRE(circular_buffer.full() == false);
    }
    REQUIRE(circular_buffer.full() == false);
    REQUIRE(circular_buffer.empty() == true);
}

TEST_CASE("Circular Buffer - Formatted", "[cb3]") {
    circular_buffer.clear();
    circular_buffer.writeF("an int: %d", 5);
    char out_block[10];
    auto out_len = circular_buffer.consume_block(out_block, 10);
    REQUIRE(out_len == 9);
    auto out_expected = "an int: 5";
    auto cmp_res = strcmp(out_block, out_expected);
    REQUIRE(cmp_res == 0);
    circular_buffer.write(alphabet);
    circular_buffer.write(alphabet);
    circular_buffer.write(alphabet);
    circular_buffer.write(alphabet);
    circular_buffer.write(alphabet);
    circular_buffer.write(alphabet);
    circular_buffer.writeF("Some random stuff to format: 0x%02x", 57005);
    REQUIRE(circular_buffer.full() == false);
    circular_buffer.write(alphabet);
    REQUIRE(circular_buffer.full() == true);
    out_len = circular_buffer.consume_block(out_block, 10);
    REQUIRE(out_len == 10);
    out_expected = "stuvwxyzab";
    cmp_res = strcmp(out_block, out_expected);
    REQUIRE(cmp_res == 0);
    out_len = circular_buffer.consume_block(out_block, 10);
    REQUIRE(out_len == 10);
    out_len = circular_buffer.consume_block(out_block, 10);
    REQUIRE(out_len == 10);
    out_len = circular_buffer.consume_block(out_block, 10);
    REQUIRE(out_len == 10);
    out_len = circular_buffer.consume_block(out_block, 10);
    REQUIRE(out_len == 10);
    out_len = circular_buffer.consume_block(out_block, 10);
    REQUIRE(out_len == 10);
    out_len = circular_buffer.consume_block(out_block, 10);
    REQUIRE(out_len == 10);
    out_len = circular_buffer.consume_block(out_block, 10);
    REQUIRE(out_len == 10);
    out_len = circular_buffer.consume_block(out_block, 10);
    REQUIRE(out_len == 10);
    out_len = circular_buffer.consume_block(out_block, 10);
    REQUIRE(out_len == 10);
    out_len = circular_buffer.consume_block(out_block, 10);
    REQUIRE(out_len == 10);
    out_len = circular_buffer.consume_block(out_block, 10);
    REQUIRE(out_len == 10);
    out_len = circular_buffer.consume_block(out_block, 10);
    REQUIRE(out_len == 10);
    out_len = circular_buffer.consume_block(out_block, 10);
    REQUIRE(out_len == 10);
    out_len = circular_buffer.consume_block(out_block, 10);
    REQUIRE(out_len == 10);
    out_len = circular_buffer.consume_block(out_block, 3);
    REQUIRE(out_len == 3);
    out_len = circular_buffer.consume_block(out_block, 10);
    REQUIRE(out_len == 10);
    out_len = circular_buffer.consume_block(out_block, 10);
    REQUIRE(out_len == 10);
    out_expected = "at: 0xdead";
    cmp_res = strcmp(out_block, out_expected);
    REQUIRE(cmp_res == 0);
}

TEST_CASE("Circular Buffer - Consume Line", "[cb4]") {
    circular_buffer.clear();
    circular_buffer.writeF("an int: %d\n", 5);
    circular_buffer.writeF("second line\n");
    circular_buffer.writeF("third\n");
    char out_block[11];
    auto out_len = circular_buffer.consume_line(out_block, 10);
    REQUIRE(out_len == 10);
    auto out_expected = "an int: 5\n";
    auto cmp_res = strcmp(out_block, out_expected);
    REQUIRE(cmp_res == 0);
    out_len = circular_buffer.consume_line(out_block, 10);
    REQUIRE(out_len == 10);
    out_expected = "second lin";
    cmp_res = strcmp(out_block, out_expected);
    REQUIRE(cmp_res == 0);
    out_len = circular_buffer.consume_line(out_block, 10);
    REQUIRE(out_len == 2);
    out_expected = "e\n";
    cmp_res = strcmp(out_block, out_expected);
    REQUIRE(cmp_res == 0);
    out_len = circular_buffer.consume_line(out_block, 10);
    REQUIRE(out_len == 6);
    out_expected = "third\n";
    cmp_res = strcmp(out_block, out_expected);
    REQUIRE(cmp_res == 0);
    out_len = circular_buffer.consume_line(out_block, 10);
    REQUIRE(out_len == 0);
    out_expected = "";
    cmp_res = strcmp(out_block, out_expected);
    REQUIRE(cmp_res == 0);
}

TEST_CASE("Circular Buffer - NoNewLines", "[cb4]") {
    circular_buffer.clear();
    circular_buffer.writeF("an int: %d", 5);
    circular_buffer.writeF("second line");
    circular_buffer.writeF("third");
    char out_block[11];
    auto out_len = circular_buffer.consume_line(out_block, 10);
    REQUIRE(out_len == 10);
    auto out_expected = "an int: 5s";
    auto cmp_res = strcmp(out_block, out_expected);
    REQUIRE(cmp_res == 0);
    out_len = circular_buffer.consume_line(out_block, 10);
    REQUIRE(out_len == 10);
    out_expected = "econd line";
    cmp_res = strcmp(out_block, out_expected);
    REQUIRE(cmp_res == 0);
    out_len = circular_buffer.consume_line(out_block, 10);
    REQUIRE(out_len == 5);
    out_expected = "third";
    cmp_res = strcmp(out_block, out_expected);
    REQUIRE(cmp_res == 0);
    out_len = circular_buffer.consume_line(out_block, 10);
    REQUIRE(out_len == 0);
    out_expected = "";
    cmp_res = strcmp(out_block, out_expected);
    REQUIRE(cmp_res == 0);
    out_len = circular_buffer.consume_line(out_block, 10);
    REQUIRE(out_len == 0);
    out_expected = "";
    cmp_res = strcmp(out_block, out_expected);
    REQUIRE(cmp_res == 0);
}

TEST_CASE("Circular Buffer - Consume Line With Wrap", "[cb4]") {
    circular_buffer.clear();
    circular_buffer.write(alphabet);
    circular_buffer.write(alphabet);
    circular_buffer.write(alphabet);
    circular_buffer.writeF("an int: %d\n", 5);
    circular_buffer.write(alphabet);
    circular_buffer.write(alphabet);
    circular_buffer.write(alphabet);
    circular_buffer.writeF("second line\n");
    circular_buffer.write(alphabet);
    circular_buffer.write(alphabet);
    circular_buffer.write(alphabet);
    circular_buffer.writeF("third\n");
    REQUIRE(circular_buffer.full() == true);
    char out_block[51];
    auto out_len = circular_buffer.consume_line(out_block, 50);
    REQUIRE(out_len == 25);
    auto out_expected = "lmnopqrstuvwxyzan int: 5\n";
    auto cmp_res = strcmp(out_block, out_expected);
    REQUIRE(cmp_res == 0);
    out_len = circular_buffer.consume_line(out_block, 10);
    REQUIRE(out_len == 10);
    out_expected = "abcdefghij";
    cmp_res = strcmp(out_block, out_expected);
    REQUIRE(cmp_res == 0);
    out_len = circular_buffer.consume_line(out_block, 50);
    REQUIRE(out_len == 50);
    out_expected = "klmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefgh";
    cmp_res = strcmp(out_block, out_expected);
    REQUIRE(cmp_res == 0);
    out_len = circular_buffer.consume_line(out_block, 50);
    REQUIRE(out_len == 30);
    out_expected = "ijklmnopqrstuvwxyzsecond line\n";
    cmp_res = strcmp(out_block, out_expected);
    REQUIRE(cmp_res == 0);
    out_len = circular_buffer.consume_line(out_block, 50);
    REQUIRE(out_len == 50);
    out_expected = "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwx";
    cmp_res = strcmp(out_block, out_expected);
    REQUIRE(cmp_res == 0);
    out_len = circular_buffer.consume_line(out_block, 10);
    REQUIRE(out_len == 10);
    out_expected = "yzabcdefgh";
    cmp_res = strcmp(out_block, out_expected);
    REQUIRE(cmp_res == 0);
    out_len = circular_buffer.consume_line(out_block, 50);
    REQUIRE(out_len == 24);
    out_expected = "ijklmnopqrstuvwxyzthird\n";
    cmp_res = strcmp(out_block, out_expected);
    REQUIRE(cmp_res == 0);
    REQUIRE(circular_buffer.empty() == true);
    out_len = circular_buffer.consume_line(out_block, 10);
    REQUIRE(out_len == 0);
    out_expected = "";
    cmp_res = strcmp(out_block, out_expected);
    REQUIRE(cmp_res == 0);
    REQUIRE(circular_buffer.empty() == true);
}

TEST_CASE("Circular Buffer - Consume Line With Wrap No New Lines", "[cb4]") {
    circular_buffer.clear();
    circular_buffer.write(alphabet);
    circular_buffer.write(alphabet);
    circular_buffer.write(alphabet);
    circular_buffer.writeF("an int: %d", 5);
    circular_buffer.write(alphabet);
    circular_buffer.write(alphabet);
    circular_buffer.write(alphabet);
    circular_buffer.writeF("second line");
    circular_buffer.write(alphabet);
    circular_buffer.write(alphabet);
    circular_buffer.write(alphabet);
    circular_buffer.writeF("third");
    REQUIRE(circular_buffer.full() == true);
    char out_block[51];
    auto out_len = circular_buffer.consume_line(out_block, 50);
    REQUIRE(out_len == 50);
    auto out_expected = "ijklmnopqrstuvwxyzan int: 5abcdefghijklmnopqrstuvw";
    auto cmp_res = strcmp(out_block, out_expected);
    REQUIRE(cmp_res == 0);
    out_len = circular_buffer.consume_line(out_block, 10);
    REQUIRE(out_len == 10);
    out_expected = "xyzabcdefg";
    cmp_res = strcmp(out_block, out_expected);
    REQUIRE(cmp_res == 0);
    out_len = circular_buffer.consume_line(out_block, 50);
    REQUIRE(out_len == 50);
    out_expected = "hijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzsecon";
    cmp_res = strcmp(out_block, out_expected);
    REQUIRE(cmp_res == 0);
    out_len = circular_buffer.consume_line(out_block, 50);
    REQUIRE(out_len == 30);
    out_expected = "d lineabcdefghijklmnopqrstuvwx";
    cmp_res = strcmp(out_block, out_expected);
    REQUIRE(cmp_res == 0);
    out_len = circular_buffer.consume_line(out_block, 50);
    REQUIRE(out_len == 50);
    out_expected = "yzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuv";
    cmp_res = strcmp(out_block, out_expected);
    REQUIRE(cmp_res == 0);
    out_len = circular_buffer.consume_line(out_block, 50);
    REQUIRE(out_len == 9);
    out_expected = "wxyzthird";
    cmp_res = strcmp(out_block, out_expected);
    REQUIRE(cmp_res == 0);
    out_len = circular_buffer.consume_line(out_block, 10);
    REQUIRE(out_len == 0);
    out_expected = "";
    cmp_res = strcmp(out_block, out_expected);
    REQUIRE(cmp_res == 0);
}

TEST_CASE("Circular Buffer - Consume Line With Wrap 2", "[cb4]") {
    circular_buffer.clear();
    circular_buffer.write(alphabet);
    circular_buffer.writeF("1\n");
    circular_buffer.write(alphabet);
    circular_buffer.writeF("2\n");
    circular_buffer.write(alphabet);
    circular_buffer.writeF("3\n");
    circular_buffer.write(alphabet);
    circular_buffer.writeF("4\n");
    circular_buffer.write(alphabet);
    circular_buffer.writeF("5\n");
    circular_buffer.write(alphabet);
    circular_buffer.writeF("6\n");
    circular_buffer.write(alphabet);
    circular_buffer.writeF("7");
    circular_buffer.write("alphabetical");
    //REQUIRE(circular_buffer.head == );
    REQUIRE(circular_buffer.full() == true);
    char out_block[51];
    while (!circular_buffer.empty()) {
        auto out_len = circular_buffer.consume_line(out_block, 50);
        printf("out_len: %d\n", out_len);
    }

}

TEST_CASE("Circular Buffer - Consume Line With Wrap 3 meets end", "[cb5]") {
    CircularBuffer<char> small_c_b(10);

    small_c_b.clear();
    small_c_b.writeF("1\n");
    small_c_b.writeF("2\n");
    small_c_b.writeF("3\n");
    small_c_b.writeF("4\n");
    small_c_b.writeF("5\n");
    small_c_b.writeF("6\n");
    small_c_b.writeF("7");
    REQUIRE(small_c_b.full() == true);
    char out_block[51];
    while (!small_c_b.empty()) {
        auto out_len = small_c_b.consume_line(out_block, 50);
        printf("out_len: %d\n", out_len);
    }

}