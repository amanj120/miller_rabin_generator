#include <stdint.h>

typedef uint8_t bit;
typedef uint16_t int16;

typedef struct number {
    int16 length;    //number of bits in the number
    bit *data;      //binary representation of the number
} number;

typedef enum base {
    binary,
    octal,
    decimal,
    hexadecimal,
} base;