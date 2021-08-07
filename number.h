#include <stdint.h>
#include <stdlib.h>

typedef uint8_t bit;
typedef int16_t int16;

typedef struct number {
    int16 length;    //number of bits in the number
    bit * data;      //binary representation of the number
} number;

typedef enum base {
    binary,
    octal,
    decimal,
    hexadecimal,
} base;

char * to_string(number *, base);
int16 find_r(number * n);
void run_tests();