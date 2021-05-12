#include "util.h"

int tarsutils::get_bits(uint64_t number)
{
    int i = 0;

    while (number)
    {
        number = number >> 1;
        i++;
    }

    return i;
}

std::vector<Byte> tarsutils::int_to_little_endian_byte_array(uint64_t value)
{
    std::vector<Byte> bytes;

    while (value != 0)
    {
        Byte byte = value & 0xFF;
        bytes.push_back(byte);

        value >>= 8;
    }

    return bytes;
}

bool tarsutils::valid_hex_string(std::string hex)
{
    return hex.compare(0, 2, "0x") == 0
        && hex.size() > 2
        && hex.find_first_not_of("0123456789abcdefABCDEF", 2) == std::string::npos;
}

bool tarsutils::compare_register_size(Byte encoding, int minSize)
{
    if (minSize <= 8) return true;
    else if (minSize <= 16) return encoding >= 0x08;
    else if (minSize <= 64) return encoding >= 0x10;
}

int tarsutils::get_register_size(Byte encoding)
{
    if (encoding <= 0x07) return 8;
    else if (encoding <= 0x0F) return 16;
    else return 64;
}
