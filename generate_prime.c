#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

// use the miller rabin test to generate probabalistic primes

static const uint8_t primes[20] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71}; 

typedef struct number {
    uint16_t length;    //number of bytes in the number
    uint8_t *data;        //binary representation of the number
} number;

typedef enum base {
    binary,
    octal,
    decimal,
    hexadecimal,
} base;

// miller test with number n and base b
int miller_rabin_test(number n, uint8_t b) {
    // write n as 2^r*d + 1
    //
}

number * generate_random_number(uint16_t length) {
    number *ret = malloc(sizeof(number));
    if (ret == NULL) {
        return NULL;
    }
    uint8_t *data = malloc(length * sizeof(uint8_t));
    if (data == NULL) {
        free(ret);
        return NULL;
    }
    ret -> data = data;

    data[0] = rand() & 0xfe; // don't set the least significant bit
    for (int i = 1; i < length; i++) {
        data[i] = (uint8_t)(rand() & (uint8_t)0xff);
    }
    data[length - 1] |= (uint8_t)0x80; // set the most significant bit

}

char * string(number n, base p) {
    return "hello";
}


int main(int argc, char *argv[]) {
    srand((unsigned)time(NULL));
    printf("hello\n");
    printf("%d, %d, %d %d, %d\n", rand(), rand(), rand(), rand(), rand());
}