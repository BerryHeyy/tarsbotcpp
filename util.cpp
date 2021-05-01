#include "util.h"

int tarsutils::get_bits(int number)
{
    int i = 0;

    while (number)
    {
        number = number >> 1;
        i++;
    }

    return i;
}