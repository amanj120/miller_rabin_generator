#include "number.h"

// negative indicates right shift, positive indicates left shift
number * shift(number * n, int16 s) { 
    int16 length = n -> length + s;
    number * ret = new(length);
    if (ret == NULL) {
        return NULL;
    }
    for (int i = 0; i < length; i++) {
        if (s >=0) {
            (ret -> data)[i + s] = (n -> data)[i];
        } else {
            (ret -> data)[i] = (n -> data)[i + s];
        }
    }
    return ret;
}

number * add(number * a, number * b) {
    int16 length = MAX(a -> length, b -> length) + 1;
    number * ret = new(length);
    if (ret == NULL) {
        return NULL;
    }

    if (b -> length > a -> length) {
        bit * temp_data = a -> data;
        int16 temp_length = a -> length;
        a -> data = b -> data;
        a -> length = b -> length;
        b -> data = temp_data;
        b -> length = temp_length;
    }

    bit carry = 0;
    for (int16 i = 0; i < b -> length; i++) {
        if ((a -> data)[i] == 1 && (b -> data)[i] == 1) {
            (ret -> data)[i] = carry;
            carry = 1;
        } else {
            (ret -> data)[i] = (a -> data)[i] + (b -> data)[i];
            carry = 0;
        }
    }
    for (int i = b -> length; i < a -> length; i++) {
        if (carry == 1 && (a -> data)[i] == 1) {
            (ret -> data)[i] = 0;
            carry = 1;
        } else {
            (ret -> data)[i] = (a -> data)[i];
            carry = 0;
        }
    }
    (ret -> data)[a -> length] = carry;

    return ret;   
}