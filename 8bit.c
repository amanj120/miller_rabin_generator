#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// 8 bit so it works on AVR
/*
What we need (minimum):
x = ab (DONE) 
x = a mod n (DONE)
a == b 
*/

void clear(uint8_t * a, uint8_t size) {
    for (int i = 0; i < (1 << size); i++) {
        a[i] = 0;
    }
    return;
} 

void print(uint8_t * a, uint8_t size) {
    for (int i = (1<<size) - 1; i >= 0; i--) {
        printf("%02x", a[i]);
    }
    // printf("\n");
}

uint8_t * new(uint8_t size) {
    return calloc((1<<size), sizeof(uint8_t));
}

void fill(uint8_t * dest, uint8_t constant, uint8_t size) {
    clear(dest, size);
    dest[0] = constant;
}

// shift < 8
void rshift(uint8_t * a, uint8_t shift, uint8_t size) {
    for (int i = 0; i < (1 << size) - 1; i++) {
        a[i] = a[i] >> shift | a[i+1] << (8 - shift);
    }
    a[(1 << size) -1] >>= shift;
}

// typedef uint8_t uint8_t;  
// return -1 if a < b, 1 if a > b, else 0
int compare(uint8_t * a, uint8_t * b, uint8_t size) {
    for (int i = (1 << size) - 1; i >= 0; i--) {
        if (a[i] < b[i]) {
            return -1;
        } else if (a[i] > b[i]) {
            return 1;
        }
    }
    return 0;
}

// a -= b, undefined behavior is a < b
void asubb(uint8_t * a, uint8_t * b, uint8_t * dest, uint8_t size) {
    uint8_t carry = 1;
    for (int i = 0; i < (1 << size); i++) {
        uint8_t t = a[i] + ~b[i] + carry;
        carry = (t < a[i]) ? 1 : 0;
        dest[i] = t;
    }
    return;
}

// a += b, overflows back to 0
void aaddb(uint8_t * a, uint8_t * b, uint8_t * dest, uint8_t size) {
    uint8_t carry = 0;
    clear(dest, size);
    for (int i = 0; i < (1 << size); i++) {
        uint8_t t = a[i] + b[i] + carry;
        carry = (t < a[i]) ? 1 : 0;
        dest[i] = t;
    }
}



// size = log_2(length of a, b)
// size of dest must be twice that of a and b
// a and b must be the same size
void axb(uint8_t * a, uint8_t * b, uint8_t * dest, uint8_t size) {
    clear(dest, size + 1);
    uint16_t idx;
    uint8_t carry, temp;
    for (int i = 0; i < (1 << size); i++) {
        for (int j = 0; j < (1 << size); j++) {
            temp = dest[i + j];
            uint16_t p = a[i] * b[j]; // TODO: make this 8 bit
            carry = p >> 8;
            idx = 1;
            dest[i + j] += (uint8_t)(p & 0xff);
            carry += (dest[i + j] < temp) ? 1 : 0;
            do {
                temp = dest[i + j + idx];
                dest[i + j + idx] += carry;
                carry = (dest[i + j + idx] < temp) ? 1 : 0;
                idx++;
            } while (carry != 0);
        }
    }
    return;
}

// a = qn + r. Assume a < n^2 (because every multiplication we mod n)
// size = log_2(size of n)
// algorithm: binary search for q
uint8_t * amodn(uint8_t * a, uint8_t * n, uint8_t size) {
    uint8_t q_hi[1 << size]; // = new(size);
    uint8_t q_lo[1 << size]; // = new(size);
    uint8_t q_mid[1 << size]; // = new(size);
    uint8_t temp[1 << size]; // = new(size);
    uint8_t prod[ 1 << (size + 1)]; // = new(size + 1);

    clear(q_hi, size);
    clear(q_lo, size);
    clear(q_mid, size);
    clear(temp, size);
    clear(prod, size + 1);

    for (int i = 0; i < (1<< size); i++) {
        q_hi[i] = 0xff;
    }

    int iter = 0;
    while (compare(q_lo, q_hi, size) == -1) {
        aaddb(q_hi, q_lo, q_mid, size);
        rshift(q_mid, 1, size);
        
        print(q_lo, size);
        printf(", ");
        print(q_hi, size);
        printf(", ");
        print(q_mid, size);
        printf(", prod: ");

        axb(q_mid, n, prod, size);
        print(prod, size+1);

        printf(" a: ");
        print(a, size + 1);
        printf("\n");

        if (compare(prod, a, size+1) == -1) {
            printf("prod < n\n");
            // free(q_lo);
            fill(temp, 1, size);
            aaddb(q_mid, temp, q_lo, size);
        } else  {
            printf("prod > n\n");
            // free(q_hi);
            clear(temp, size);
            aaddb(q_mid , temp, q_hi, size);  
        } 
        if (iter++ == 32) {
            break;
        }
    }

    printf("a: ");
    print(a, size+1);
    printf(" below: ");
    print(prod, size+1);
    printf("\n");
    uint8_t * dest = new(size);
    asubb(a, prod, dest, size);

    // free(prod);
    // free(temp);
    // free(q_lo);
    // free(q_hi);
    // free(q_mid);

    return dest;
}

int main(int argc, char * argv[]) {

    uint8_t a[4] = {0xcd, 0xab, 0x89, 0x67};
    uint8_t b[4] = {0x78, 0x56, 0x34, 0x12};

    uint8_t * prod = calloc(8, sizeof(uint8_t));
    axb(&a[0], &b[0], prod, 2);

    for (int i = 7; i >= 0; i--) {
        printf("%02x", prod[i]);
    } 
    printf("\n");

    printf("%d, %d\n", compare(a, b, 2), compare(b, a, 2));

    uint8_t * aa = calloc(4, 1);
    aaddb(a, b, aa, 2);
    for (int i = 3; i >= 0; i--) {
        printf("%02x", aa[i]);
    } 
    printf("\n");

    asubb(aa, b, aa, 2);
    for (int i = 3; i >= 0; i--) {
        printf("%02x", aa[i]);
    } 
    printf("\n");
    
    asubb(aa, b, aa, 2);
    for (int i = 3; i >= 0; i--) {
        printf("%02x", aa[i]);
    } 
    printf("\n");

    asubb(aa, b, aa, 2);
    for (int i = 3; i >= 0; i--) {
        printf("%02x", aa[i]);
    } 
    printf("\n");

    rshift(aa, 3, 2);
    for (int i = 3; i >= 0; i--) {
        printf("%02x", aa[i]);
    } 
    printf("\n"); //00466926970ec559

    axb(aa, aa, prod, 2);
    for (int i = 7; i >= 0; i--) {
        printf("%02x", prod[i]);
    } 
    printf("\n"); // so far so good: 
    // 00466926970ec559

    uint8_t * mod = amodn(prod, b, 2);
    for (int i = 3; i >= 0; i--) {
        printf("%02x", mod[i]);
    } 
    printf("\n");
    // should be: 0e962fc9

    free(prod);
    free(aa);
    free(mod);

    printf("Hello World\n");
    return 0;
}