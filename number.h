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

number * new(int16 length) {
    number * ret = malloc(sizeof(number));
    if (ret == NULL) {
        return NULL;
    }
    bit * data = malloc(length * sizeof(bit));
    if (data == NULL) {
        free(ret);
        return NULL;
    }
    ret -> data = data;
    return ret;
}

void delete(number * n) {
    free(n -> data);
    free(n);
    return;
}