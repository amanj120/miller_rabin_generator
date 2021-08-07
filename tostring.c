#include <stdlib.h>
#include <stdio.h>
#include "number.h"

static char * to_binary(number * n) {
    int16 num_digits = n -> length;
    char * ret = malloc(num_digits + 1);
    if (ret == NULL) {
        return NULL;
    }
    for (int i = n -> length - 1; i >= 0; i--) {
        ret[i] = (n -> data)[i] == 0 ? '0' : '1';
    }
    ret[num_digits] = '\0';
    return ret;
}

static char * to_octal(number * n) {
    char charmap[8] = {'0', '1', '2', '3', '4', '5', '6', '7'};
    int16 remainder = (n -> length) % 3;
    int16 num_digits = (n -> length)/3;
    if (remainder != 0) {
        num_digits += 1;
    }

    char * ret = malloc(num_digits + 1);
    if (ret == NULL) {
        return NULL;
    }

    for (int i = 0; i < n -> length; i += 3) {
        int16 digit = ((n -> data)[i] << 0) + ((n -> data)[i + 1] << 1) + ((n -> data)[i + 2] << 2); 
        ret[num_digits - i/3 - 1] = charmap[digit];
    }

    int16 msd = 0; //most significant digit
    for (int i = 0; i < remainder; i++) {
        msd += (n -> data)[num_digits * 3 + i - 3] << i;
    }
    if (remainder != 0) {
        ret[0] = charmap[msd];
    }
    ret[num_digits] = '\0';
    return ret;
}

static char * to_hex(number * n) {
    char charmap[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    int16 remainder = (n -> length) % 4;
    int16 num_digits = (n -> length)/4;
    if (remainder != 0) {
        num_digits += 1;
    }

    // printf("remiander: %d, num_digits: %d\n", remainder, num_digits);

    char * ret = malloc(num_digits + 1);
    if (ret == NULL) {
        return NULL;
    }

    for (int i = 0; i < n -> length; i += 4) {
        int16 digit = ((n -> data)[i] << 0) + ((n -> data)[i + 1] << 1) + ((n -> data)[i + 2] << 2) + ((n -> data)[i + 3] << 3); 
        ret[num_digits - i/4 - 1] = charmap[digit];
    }

    int16 msd = 0; //most significant digit
    for (int i = 0; i < remainder; i++) {
        msd += (n -> data)[num_digits * 4 + i - 4] << i;
    }

    if (remainder != 0) {
        ret[0] = charmap[msd];
    }
    ret[num_digits] = '\0';
    return ret;
}

char * to_string(number * n, base p) {
    switch(p) {
        case binary:
            return to_binary(n);
        case octal:
            return to_octal(n);
        case hexadecimal:
            return to_hex(n);
        case decimal:
        default:
            return "";
    }
}