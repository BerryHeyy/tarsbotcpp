#pragma once

#include <string>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <vector>

using Byte = uint8_t;
using Word = uint16_t;

namespace tarsutils
{
    int get_bits(uint64_t number);

    std::vector<Byte> int_to_little_endian_byte_array(uint64_t value);

    bool valid_hex_string(std::string hex);

    // TODO: Please do something better than this to get register sizes. This will
    // fail when the register map is edited in any way.
    bool compare_register_size(Byte encoding, int minSize);
    int get_register_size(Byte encoding);
}
