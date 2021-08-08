#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// 8 bit so it works on AVR
/*
What we need (minimum):
x = ab (DONE)
x = a mod n
a == b
*/

// size = log_2(length of a, b)
uint8_t * axb(uint8_t * a, uint8_t * b, uint8_t size) {
    uint8_t * product = calloc(1<<(size + 1), sizeof(uint8_t));
    for (int i = 0; i < (1 << size); i++) {
        for (int j = 0; j < (1 << size); j++) {
            uint8_t temp = product[i + j];
            uint16_t p = a[i] * b[j];
            uint8_t carry = p >> 8;
            int idx = 1;
            product[i + j] += (uint8_t)(p & 0xff);
            carry += (product[i + j] < temp) ? 1 : 0;
            do {
                temp = product[i + j + idx];
                product[i + j + idx] += carry;
                carry = (product[i + j + idx] < temp) ? 1 : 0;
                idx++;
            } while (carry != 0);
        }
    }
    return product;
}

int main(int argc, char * argv[]) {

    uint8_t a[4] = {0xcd, 0xab, 0x89, 0x67};
    uint8_t b[4] = {0x78, 0x56, 0x34, 0x12};

    uint8_t * prod = axb(&a[0], &b[0], 2);

    for (int i = 7; i >= 0; i--) {
        printf("%02x", prod[i]);
    } 
    printf("\n");

    printf("Hello World\n");
    return 0;
}