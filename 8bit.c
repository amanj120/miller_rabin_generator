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

void print(uint8_t * a, uint8_t size) {
    for (int i = (1<<size) - 1; i >= 0; i--) {
        printf("%02x", a[i]);
    }
    // printf("\n");
}

uint8_t * create(uint8_t constant, uint8_t size) {
    uint8_t * c = calloc((1<<size), sizeof(uint8_t));
    c[0] = constant;
    return c;
}

// shift < 8
void rshift(uint8_t * a, uint8_t shift, uint8_t size) {
    for (int i = 0; i < (1 << size) - 1; i++) {
        a[i] = a[i] >> shift | a[i+1] << (8 - shift);
    }
    a[(1 << size) -1] >>= shift;
}

// typedef uint8_t uint8_t;  
// return a < b
uint8_t altb(uint8_t * a, uint8_t * b, uint8_t size) {
    for (int i = (1 << size) - 1; i >= 0; i--) {
        if (a[i] < b[i]) {
            return 1;
        } else if (a[i] > b[i]) {
            return 0;
        }
    }
    return 0;
}

// a -= b, undefined behavior is a < b
uint8_t * asubb(uint8_t * a, uint8_t * b, uint8_t size) {
    uint8_t * diff = calloc((1 << size), sizeof(uint8_t));
    uint8_t carry = 1;
    for (int i = 0; i < (1 << size); i++) {
        uint8_t t = a[i] + ~b[i] + carry;
        carry = (t < a[i]) ? 1 : 0;
        diff[i] = t;
    }
    return diff;
}

// a += b, overflows back to 0
uint8_t * aaddb(uint8_t * a, uint8_t * b, uint8_t size) {
    uint8_t carry = 0;
    uint8_t * sum = calloc((1<<size), sizeof(uint8_t));
    for (int i = 0; i < (1 << size); i++) {
        uint8_t t = a[i] + b[i] + carry;
        carry = (t < a[i]) ? 1 : 0;
        sum[i] = t;
    }
    return sum;
}

// size = log_2(length of a, b)
uint8_t * axb(uint8_t * a, uint8_t * b, uint8_t size) {
    uint8_t * product = calloc(1<<(size + 1), sizeof(uint8_t));
    uint16_t idx;
    uint8_t carry, temp;
    for (int i = 0; i < (1 << size); i++) {
        for (int j = 0; j < (1 << size); j++) {
            temp = product[i + j];
            uint16_t p = a[i] * b[j]; // TODO: make this 8 bit
            carry = p >> 8;
            idx = 1;
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

// a = qn + r. Assume a < n^2 (because every multiplication we mod n)
// size = log_2(size of n)
// algorithm: binary search for q
uint8_t * amodn(uint8_t * a, uint8_t * n, uint8_t size) {
    uint8_t * q_hi = calloc(1<<size, sizeof(uint8_t));
    uint8_t * q_lo = calloc(1<<size, sizeof(uint8_t));
    for (int i = 0; i < (1<< size); i++) {
        q_hi[i] = 0xff;
    }
    uint8_t * q_mid, * prod;
    int iter = 0;
    while (altb(q_lo, q_hi, size)) {
        q_mid = aaddb(q_hi, q_lo, size);
        rshift(q_mid, 1, size);
        
        print(q_lo, size);
        printf(", ");
        print(q_hi, size);
        printf(", ");
        print(q_mid, size);
        printf(", prod: ");

        prod = axb(q_mid, n, size);
        print(prod, size+1);
        printf("\n");

        if (altb(prod, a, size+1) == 1) {
            printf("prod < n\n");
            free(q_lo);
            q_lo = aaddb(q_mid, create(1, size), size);
        } else  {
            printf("prod > n\n");
            free(q_hi);
            q_hi = aaddb(q_mid, create(0, size), size);  
        } 
        if (iter++ == 32) {
            break;
        }
    }
    // 
    // (0, 15) -> (0,7) (8,15) -> (0,3), (4,7), (8,11), (12, 15) -> ... (4, 5) (5, 6) ... -> ... (4, 4) (5,5)  
    // 

    // q_lo = asubb(q_lo, create(1, size), size);
    // uint8_t * below = axb(q_lo, n, size);
    printf("a: ");
    print(a, size+1);
    printf(" below: ");
    print(prod, size+1);
    printf("\n");
    return asubb(a, prod, size+1);
}

int main(int argc, char * argv[]) {

    uint8_t a[4] = {0xcd, 0xab, 0x89, 0x67};
    uint8_t b[4] = {0x78, 0x56, 0x34, 0x12};

    uint8_t * prod = axb(&a[0], &b[0], 2);

    for (int i = 7; i >= 0; i--) {
        printf("%02x", prod[i]);
    } 
    printf("\n");

    printf("%d, %d\n", altb(a, b, 2), altb(b, a, 2));

    uint8_t * aa = aaddb(a, b, 2);
    for (int i = 3; i >= 0; i--) {
        printf("%02x", aa[i]);
    } 
    printf("\n");

    aa = asubb(aa, b, 2);
    for (int i = 3; i >= 0; i--) {
        printf("%02x", aa[i]);
    } 
    printf("\n");
    
    aa = asubb(aa, b, 2);
    for (int i = 3; i >= 0; i--) {
        printf("%02x", aa[i]);
    } 
    printf("\n");

    aa = asubb(aa, b, 2);
    for (int i = 3; i >= 0; i--) {
        printf("%02x", aa[i]);
    } 
    printf("\n");

    rshift(aa, 3, 2);
    for (int i = 3; i >= 0; i--) {
        printf("%02x", aa[i]);
    } 
    printf("\n");

    aa = axb(aa, aa, 2);
    for (int i = 7; i >= 0; i--) {
        printf("%02x", aa[i]);
    } 
    printf("\n"); // so far so good: 
    // 00466926970ec559

    uint8_t * mod = amodn(aa, b, 2);
    for (int i = 7; i >= 0; i--) {
        printf("%02x", mod[i]);
    } 
    printf("\n");
    // should be: e962fc9



    printf("Hello World\n");
    return 0;
}