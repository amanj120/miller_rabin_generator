#include "number.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// use the miller rabin test to generate probabalistic primes
static const int16 primes[20] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71}; 

int16 find_r(number * n) {
    int16 ret = 0;
    while (n -> data[++ret] == 0);
    return ret;
}

// return a squared mod n:
number * sqmodn(number * a, number * n) {   
    return NULL;
}

// compute a^d mod n
number * a_d_mod_n(number * a, number * d, number * n) {
    number * ret = new(n -> length + 1);
    if (ret == NULL) {
        return NULL;
    }

}

// miller test with number n and base b
int miller_rabin_test(number * n, int16 b) {
    // write n as 2^r*d + 1
    int16 r = find_r(n);
    number * d = shift(n, -1 * r);
}   

number * generate_random_number(int16 length) {
    number * ret = new(length);
    if (ret == NULL) {
        return NULL;
    }
    (ret -> data)[length - 1] = 1; // set the most significant bit
    (ret -> data)[0] = 1; // set the least significant bit
    for (int i = 1; i < length - 1; i++) {
        (ret -> data)[i] = (bit)(rand() & 1);
    }
    return ret;
}

int main(int argc, char *argv[]) {
    srand((unsigned)time(NULL));
    run_tests();
}

/*
3^7 = 3^4 * 3^2 * 3^1

*/