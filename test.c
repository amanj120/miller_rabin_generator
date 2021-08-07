#include "number.h"
#include <stdio.h>
#include <stdlib.h>

void testhex(bit data[], int16 length, char * expected) {
    number * n = &(number){
        .data = data,
        .length = length
    };
    char * str = to_string(n, hexadecimal);
    printf("expected: %s: 0x%s\n", expected, str);
    free(str);
}

void testoct(bit data[], int16 length, char * expected) {
    number * n = &(number){
        .data = data,
        .length = length
    };
    char * str = to_string(n, octal);
    printf("expected: %s: 0o%s\n", expected, str);
    free(str);
}

void test_string_0x53() {
    bit data[] = {1, 1, 0, 0, 1, 0, 1};
    testhex(data, 7, "0x53");
    testoct(data, 7, "0o123");
}

void test_string_0xa3() {
    bit data[] = {1, 1, 0, 0, 0, 1, 0, 1};
    testhex(data, 8, "0xa3");
    testoct(data, 8, "0o243");
}

void test_string_0x05() {
    bit data[] = {1, 0, 1, 0, 0};
    testhex(data, 5, "0x05");
    testoct(data, 5, "0o05");
}

void test_string_0x5() {
    bit data[] = {1, 0, 1};
    testhex(data, 3, "0x5");
    testoct(data, 3, "0o5");

}

void test_string_0x10b() {
    bit data[] = {1,1,0,1,0,0,0,0,1};
    testhex(data, 9, "0x10b");
    testoct(data, 9, "0o413");
}

void test_string_0x2() {
    bit data[] = {0,1};
    testhex(data, 2, "0x2");
    testoct(data, 2, "0o2");
}


void run_tests() {
    test_string_0x53();
    test_string_0xa3();
    test_string_0x05();
    test_string_0x5();
    test_string_0x10b();
    test_string_0x2();
}
