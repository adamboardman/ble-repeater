#include <catch2/catch_test_macros.hpp>
#include <cstdarg>
#include <cstdint>
#include <cstring>
#include <vector>

#include "Debugging.h"
#include "pico_pi_mocks.h"
#include "../Packet/BinaryReader.h"
#include "../Packet/BinaryWriter.h"
#include "../Packet/ProtocolProcessor.h"

extern void print_named_data(const char *name, const uint8_t *data, uint16_t data_size);

const std::string data2="01010300000198717c111400000419077f0222faf5ce6164616de89f19e772b12aea88b2450fbd9c76161304d656dbf817366830b44031e6264714089006b960e6168f5eba7964bf0be4875057b678a4a076314723884b2ef2ae66a23476df2bea4fe370304ed0053e80c61b2dc3b354970dda20282bd5dbd3decd0618d3d8cdc9667a41f538453d0385570018e5f60872eb89e1b7917ae7bc41e3bdae1e70f28b06924f51f38a19535096f9cc66969343f068147c514d3ec0e6400a96833ccb22b5ed3ed225f76efe74e14fecfd54c00612dc2d90f7f1d96930348547eb47dbda537ea95e712dc39377d4304103a01abe8cbbf21e8674b53654989a";
const std::string data3="01130100000198717c111405004b19077f0222faf5ceffffffffffffffff009c7801639064af6752faf5f51c83c2eefd752fec1205f7059806e6c9a5fdbb77c4885571ebf650de7dd3632d6d0fc67a1056c192989298cbc0c038a3b0465084c18181420000f12524171faedad93d658acf4f0107ea080387f17180794fad21cc6207951f2df239ecad83639ebe60992ade12b4ebdf9b46a4369d44d884f46a0ee57e2868ede3356fe5aafe423b9ea639fd674fa09bf42d88fa94c7f9b3c880b3d24c2ac5f9f3a809a41f16c73615bfe434e2f1bfbd168eec040b496b1402d6c6359a82be2c9ae4d6ae49f6a9cb0f6889483dbfd659aa74b70428f572";
const std::string data4="01130100000198717c110705004b19077f0222faf5ceffffffffffffffff009c7801639064af6752faf5f51c83c2eefd752fec1205f7059806e6c9a5fdbb77c4885571ebf650de7dd3632d6d0fc67a1056c192989298cbc0c038a3b046908dc18181420000ed7b240994fd85a4429eb67b4ecb7c76da3fb5b6986f3f8f9d10708e57427ea9f1eb2cfbb32d17cb3755db5a872a6b8648f334b51423ffbb46dffcf7044e0212dc480c9b194c4202f1d1b82b2ab00ce36c721d37297f9685b0412fb0bef1b3d9503cc4527a96d4fccf460abe8debfe2265e95da5163b49cafc932b045e3217564f14d0e42d23e80264a4f031f9b4139864e2fc4b6f2ec0";
const std::string data5="0104070000019893abb14c01004feddd326fb00c2b40ffffffffffffffff100000019893abb14b2431454645413636352d313837382d343644312d393538452d46314535353731413833383008616e6f6e32303134000568656c6c6f1065646464333236666230306332623430151bdfade3b1eec297f58a83862efcf098857199715a2de8a28de951489319c101adce8799dee33409c120137e272b60522f6354c1af553b2949b7125d62d849ca69dc7f419dd3e880cd5b57ec6b718241015e75974563ba00a33c8556ef62fe2f266f16e3bde1d313c6209c53549ede95dd8635eb10326bebaca5b1d9f3a406c110ade6bff69f17417ae182436a89";
const std::string data6="01040700000198941571c301004bc67ff7caf2952326ffffffffffffffff1000000198941571c12434453337323032392d323035422d344644432d413844362d343736333437303436423944046164616d000568656c6c6f10633637666637636166323935323332368359aac1085a908e03d66c8ec83517ce63f5c202f174ae2e802d6539a7cc7ae3fbfb30dc8e34b188aa87f439d721d4b80adbd61013c7b112c8a3a121e96269e90d82dfe9a700bc4129cfc0a4b257fc94ae0a389066d2747a1f4ff10c0bd9b6aaa9a0365c84b0e18e388f2b0b647616deed3fec91085c49daf96aa7393cc94d039133bfcdbe9ec7b823149b26fb257e929aab60";

TEST_CASE("ReadData", "[data1]") {
    BinaryReader reader(0,uint_array1,sizeof(uint_array1));

    REQUIRE(reader.read_uint8() == 1);//version
    REQUIRE(reader.read_uint8() == 1);//type
    REQUIRE(reader.read_uint8() == 3);//ttl
    REQUIRE(reader.read_uint64() == 0x1987183cdf9);//timestamp
    uint8_t flags = reader.read_uint8();
    REQUIRE(flags == 0);//flags
    auto payloadlength = reader.read_uint16();
    REQUIRE(payloadlength == 4);//payloadlength
    REQUIRE(reader.read_uint64() == 0x1d3d6a261523a828);//sender
    uint8_t payloadexpected[] = {'a', 'd', 'a', 'm'};
    uint8_t payload[payloadlength];
    memcpy (payload, reader.read_data(payloadlength),payloadlength);

    REQUIRE(payload[0] == payloadexpected[0]);
    REQUIRE(payload[1] == payloadexpected[1]);
    REQUIRE(payload[2] == payloadexpected[2]);
    REQUIRE(payload[3] == payloadexpected[3]);
    if (flags & 0b10) {
        uint8_t signature_length = 64;
        uint8_t signature[signature_length];
        memcpy (signature, reader.read_data(signature_length),signature_length);
        uint8_t signatureexpected[] = {'a'};
        REQUIRE(signature[0] == signatureexpected[0]);
    }
}

TEST_CASE("WriteData", "[data1]") {
    std::vector<uint8_t> uint8_vector;
    BinaryWriter writer(uint8_vector);

    writer.write_uint8(1); //version
    writer.write_uint8(1); //type
    writer.write_uint8(3);//ttl
    writer.write_uint64(0x1987183cdf9); //timestamp
    writer.write_uint8(0); //flags
    uint8_t payload[] = {'a', 'd', 'a', 'm'};
    writer.write_uint16(sizeof(payload));
    writer.write_uint64(0x1d3d6a261523a828); //sender
    writer.write_data(payload, sizeof(payload));
    writer.write_uint8(0); //no tail clipping

    REQUIRE(27 == writer.test_only_current_pos());
    auto eq = std::equal(std::begin(uint8_vector), std::end(uint8_vector), std::begin(uint_array1));
    REQUIRE(true == eq);
}

TEST_CASE("ReadDataUnString", "[data2]") {
    const uint8_t datalen=data2.length()/2;
    uint8_t uint_array[datalen];
    populate_array_from_string(uint_array, data2);

    BinaryReader reader(0,uint_array,sizeof(uint_array));

    REQUIRE(reader.read_uint8() == 1);//version
    REQUIRE(reader.read_uint8() == 1);//type
    REQUIRE(reader.read_uint8() == 3);//ttl
    REQUIRE(reader.read_uint64() == 0x198717c1114);//timestamp
    uint8_t flags = reader.read_uint8();
    REQUIRE(flags == 0);//flags
    auto payloadlength = reader.read_uint16();
    REQUIRE(payloadlength == 4);//payloadlength
    REQUIRE(reader.read_uint64() == 0x19077f0222faf5ce);//sender
    if (flags & 0b1) { // has recipient
        REQUIRE(reader.read_uint64() == 0x00f00f00f00f);//recipient
    }
    if (flags & 0b100) {
        //compressed gives original size before payload
        const auto originalSize = reader.read_uint16();
        LOG_DEBUG("originalSize: %d\n", originalSize);
    }
    uint8_t payloadexpected[] = {'a', 'd', 'a', 'm'};
    uint8_t payload[payloadlength];
    memcpy (payload, reader.read_data(payloadlength),payloadlength);

    REQUIRE(payload[0] == payloadexpected[0]);
    REQUIRE(payload[1] == payloadexpected[1]);
    REQUIRE(payload[2] == payloadexpected[2]);
    REQUIRE(payload[3] == payloadexpected[3]);
    if (flags & 0b10) { // has signature
        uint8_t signature_length = 64;
        uint8_t signature[signature_length];
        memcpy (signature, reader.read_data(signature_length),signature_length);
        uint8_t signatureexpected[] = {'n','o','n','e'};
        REQUIRE(signature[0] == signatureexpected[0]);
    }

    auto padding_to_remove = uint_array[datalen-1];
    REQUIRE(0x9a == padding_to_remove);

    // For some reason our test captured packets appear to be having data after the payload
    print_named_data("packet data", &uint_array[reader.test_only_current_pos()], datalen-padding_to_remove);

}

TEST_CASE("ReadDataUnString3", "[data3]") {
    const uint8_t datalen=data3.length()/2;
    uint8_t uint_array[datalen];
    populate_array_from_string(uint_array, data3);

    BinaryReader reader(0,uint_array,sizeof(uint_array));

    REQUIRE(reader.read_uint8() == 1);//version
    REQUIRE(reader.read_uint8() == 0x13);//type
    REQUIRE(reader.read_uint8() == 1);//ttl
    REQUIRE(reader.read_uint64() == 0x198717c1114);//timestamp
    uint8_t flags = reader.read_uint8();
    REQUIRE(flags == 0b101);//flags
    auto payloadlength = reader.read_uint16();
    REQUIRE(payloadlength == 75);//payloadlength
    REQUIRE(reader.read_uint64() == 0x19077f0222faf5ce);//sender
    if (flags & 0b1) { // has recipient
        REQUIRE(reader.read_uint64() == 0xffffffffffffffff);//recipient
    }
    if (flags & 0b100) {
        //compressed gives original size before payload
        const auto originalSize = reader.read_uint16();
        LOG_DEBUG("originalSize: %d\n", originalSize);
    }
    uint8_t payloadexpected[] = {'x', 0x1, 'c', 0x90, 'd'};
    uint8_t payload[payloadlength];
    memcpy (payload, reader.read_data(payloadlength),payloadlength);

    REQUIRE(payload[0] == payloadexpected[0]);
    REQUIRE(payload[1] == payloadexpected[1]);
    REQUIRE(payload[2] == payloadexpected[2]);
    REQUIRE(payload[3] == payloadexpected[3]);
    if (flags & 0b10) {
        uint8_t signature_length = 64;
        uint8_t signature[signature_length];
        memcpy (signature, reader.read_data(signature_length),signature_length);
        uint8_t signatureexpected[] = {'a'};
        REQUIRE(signature[0] == signatureexpected[0]);
    }

    auto padding_to_remove = uint_array[datalen-1];
    REQUIRE(114 == padding_to_remove);

    // For some reason our test captured packets appear to be having data after the payload
    print_named_data("packet data", &uint_array[reader.test_only_current_pos()], datalen-padding_to_remove);

}
