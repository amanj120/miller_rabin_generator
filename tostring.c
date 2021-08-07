#include "number.h"

char * to_binary(number * n) {
    int16 num_digits = n -> length;
    char * ret = malloc(num_digits * sizeof(char));
    if (ret == NULL) {
        return NULL;
    }
    for (int i = n -> length - 1; i >= 0; i--) {
        ret[i] = (n -> data)[i] == 0 ? '0' : '1';
    }
    return ret;
}

char * to_octal(number * n) {

}


char * string(number * n, base p) {
    switch(p) {
        case binary:
            return to_binary(n);
        case octal:
        case hexadecimal:
        case decimal:
        default:
    }
    printf("\n");
}