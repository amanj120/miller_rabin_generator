#include "number.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// use the miller rabin test to generate probabalistic primes

static const int16 primes[20] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71}; 

int16 find_r(number * n) {
    int16 ret = 0;
    while (n -> data[++ret] == 0);
    return ret - 1;
}

// miller test with number n and base b
int miller_rabin_test(number * n, int16 b) {
    // write n as 2^r*d + 1
    //
}

number * generate_random_number(int16 length) {
    number *ret = malloc(sizeof(number));
    if (ret == NULL) {
        return NULL;
    }
    bit * data = malloc(length * sizeof(bit));
    if (data == NULL) {
        free(ret);
        return NULL;
    }
    ret -> data = data;
    data[length - 1] = 1; // set the most significant bit
    data[0] = 1; // set the least significant bit
    for (int i = 1; i < length - 1; i++) {
        data[i] = (bit)(rand() & 1);
    }
    return ret;
}

void delete(number * n) {
    free(n -> data);
    free(n);
    return;
}

int main(int argc, char *argv[]) {
    run_tests();
    srand((unsigned)time(NULL));
}