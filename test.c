#include "number.h"
#include <stdio.h>
#include <stdlib.h>

void test_string_0x53() {
    bit data[] = {1, 1, 0, 0, 1, 0, 1};
    number * n = &(number){
        .data = data, 
        .length = 7
    };
    char * str = to_string(n, hexadecimal);
    printf("expected: 0x53: 0x%s\n", str);
    free(str);
}

void test_string_0xa3() {
    bit data[] = {1, 1, 0, 0, 0, 1, 0, 1};
    number * n = &(number){
        .data = data, 
        .length = 8
    };
    char * str = to_string(n, hexadecimal);
    printf("expected: 0xa3: 0x%s\n", str);
    free(str);
}

void run_tests() {
    test_string_0x53();
    test_string_0xa3();
}
