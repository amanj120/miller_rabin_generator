#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// 8 bit so it works on AVR
#define DEBUG 1

typedef uint8_t byte;

byte *new (byte size) { return calloc((1 << size), sizeof(byte)); }

byte primes[54] = {2,	3,	 5,	  7,   11,	13,	 17,  19,  23,	29,	 31,
				   37,	41,	 43,  47,  53,	59,	 61,  67,  71,	73,	 79,
				   83,	89,	 97,  101, 103, 107, 109, 113, 127, 131, 137,
				   139, 149, 151, 157, 163, 167, 173, 179, 181, 191, 193,
				   197, 199, 211, 223, 227, 229, 233, 239, 241, 251};

void clear(byte *src, byte size) {
	for (int i = 0; i < (1 << size); i++) {
		src[i] = 0;
	}
}

void print(byte *src, byte size) {
	for (int i = (1 << size) - 1; i >= 0; i--) {
		printf("%02x", src[i]);
	}
}

// set the first byte in dest to constant, clear the rest
void set(byte *dest, byte constant, byte size) {
	clear(dest, size);
	dest[0] = constant;
}

void copy(byte *src, byte *dest, byte size) {
	for (int i = 0; i < (1 << size); i++) {
		dest[i] = src[i];
	}
}

void rand_int(byte *dest, byte size) {
	for (int i = 0; i < (1 << size); i++) {
		dest[i] = rand() & 0xff;
	}
	dest[0] |= 1;
	dest[(1 << size) - 1] |= 0x80;

	if (dest[0] == 1) {
		// set another random bit in the least significant byte
		// because right shift only works for shift < 8;
		dest[0] |= (1 << ((rand() % 7) + 1));
	}
}

// shift must be less than 8
void rshift(byte *src, byte shift, byte size) {
	for (int i = 0; i < (1 << size) - 1; i++) {
		src[i] = src[i] >> shift | src[i + 1] << (8 - shift);
	}
	src[(1 << size) - 1] >>= shift;
}

byte find_lsb_set(byte src) {
	for (byte i = 0; i < 8; i++) {
		if ((src >> i) & 1 == 1) {
			return i;
		}
	}
}

// return -1 if src1 < src2, 1 if src1 > src2, else 0
int compare(byte *src1, byte *src2, byte size) {
	for (int i = (1 << size) - 1; i >= 0; i--) {
		if (src1[i] < src2[i]) {
			return -1;
		} else if (src1[i] > src2[i]) {
			return 1;
		}
	}
	return 0;
}

// src1 += src2, overflows back to 0
void add(byte *src1, byte *src2, byte *dest, byte size) {
	byte carry = 0;
	clear(dest, size);
	for (int i = 0; i < (1 << size); i++) {
		byte t = src1[i] + src2[i] + carry;
		carry = (t < src1[i]) ? 1 : 0;
		dest[i] = t;
	}
}

void add_const(byte *src, byte value, byte *dest, byte size) {
	clear(dest, size);
	for (int i = 0; i < (1 << size); i++) {
		byte t = src[i] + value;
		value = (t < src[i]) ? 1 : 0;
		dest[i] = t;
	}
}

// src1 -= src2, undefined behavior is src1 < src2
void sub(byte *src1, byte *src2, byte *dest, byte size) {
	// printf("in sub\n");
	for (int i = 0; i < (1 << size); i++) {
		if (src2[i] > src1[i]) {
			// No out of bounds errors bc src1 > src2
			if (src1[i + 1] != 0) {
				src1[i + 1] -= 1;
			} else {
				for (int j = i + 1; j < (1 << size); j++) {
					if (src1[j] == 0) {
						src1[j] = 255;
					} else {
						src1[j] -= 1;
						break;
					}
				}
			}
			dest[i] = 256 - (src2[i] - src1[i]);
		} else {
			dest[i] = src1[i] - src2[i];
		}
	}
	return;
}

// size = log_2(length of src1, src2)
// size of dest must be twice that of src1 and src2
// src1 and src2 must be the same size
void mult(byte *src1, byte *src2, byte *dest, byte size) {
	// if (DEBUG) {
	// 	printf("in mult\n");
	// }
	clear(dest, size + 1);
	int idx;
	byte carry, temp;
	for (int i = 0; i < (1 << size); i++) {
		for (int j = 0; j < (1 << size); j++) {
			temp = dest[i + j];
			unsigned int p = src1[i] * src2[j]; // TODO: make this 8 bit
			carry = (byte)((p >> 8) & 0xff);
			idx = 1;
			dest[i + j] += (byte)(p & 0xff);
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
void mod(byte *a, byte *n, byte *dest, byte size) {
	if (DEBUG) {
		printf("in mod\n");
	}
	byte prev[1 << size];
	byte zero[(1 << size)];
	byte q_hi[1 << size];
	byte q_lo[1 << size];
	byte q_mid[1 << size];
	byte prod[1 << (size + 1)];

	clear(prev, size);
	clear(zero, size);
	clear(q_hi, size);
	clear(q_lo, size);
	clear(q_mid, size);
	clear(prod, size + 1);

	for (int i = 0; i < (1 << size); i++) {
		q_hi[i] = 0xff;
	}

	do {
		copy(q_mid, prev, size);

		add(q_hi, q_lo, q_mid, size);
		rshift(q_mid, 1, size);
		mult(q_mid, n, prod, size);

		if (DEBUG) {
			printf("low: ");
			print(q_lo, size);
			printf(" high: ");
			print(q_hi, size);
			printf(" mid: ");
			print(q_mid, size);
			printf(" a: ");
			print(a, size + 1);
			printf(" prod: ");
			print(prod, size + 1);
			printf(" n: ");
			print(n, size);
			printf("\n");
		}

		if (compare(prod, a, size + 1) < 0) {
			copy(q_mid, q_lo, size);
		} else {
			copy(q_mid, q_hi, size);
		}
	} while (compare(prev, q_mid, size) != 0);

	if (compare(q_mid, zero, size) == 0) {
		copy(a, dest, size);
	} else {
		byte temp[(1 << (size + 1))];
		sub(a, prod, temp, size + 1);
		copy(temp, dest, size);
	}

	if (DEBUG) {
		printf("answer: ");
		print(dest, size);
		printf("\n");
	}
}

// compute a^d mod n and store in dest
// a, d, and n are all 2^size bytes wide
void admodn(byte *a, byte *d, byte *n, byte *dest, byte size) {
	if (DEBUG) {
		printf("calculating ");
		print(a, size);
		printf(" ^ ");
		print(d, size);
		printf(" mod ");
		print(n, size);
		printf("\n");
	}
	byte cur_a[1 << size];
	byte prod[1 << size];
	byte temp[1 << (size + 1)];

	set(prod, 1, size);

	copy(a, cur_a, size); // current = a

	for (int i = 0; i < 1 << (size); i++) {
		for (int j = 0; j < 8; j++) {
			// if exponent set, prod = (prod * cur_a) % n
			if ((d[i] >> j) & 1 == 1) {
				clear(temp, size);
				mult(cur_a, prod, temp, size);
				mod(temp, n, prod, size);
			}
			// regardless of whether exponent set:
			clear(temp, size);
			mult(cur_a, cur_a, temp, size);
			mod(temp, n, cur_a, size);
		}
	}
	copy(prod, dest, size);
}

// p is the int we are testing for primality
int miller_rabin(byte *p, byte size) {
	byte pm1[1 << size];
	byte one[1 << size];
	byte d[1 << size];
	copy(p, d, size);

	set(one, 1, size);
	sub(p, one, pm1, size);
	byte shift = find_lsb_set(pm1[0]);
	rshift(d, shift, size);

	byte two[1 << size];
	set(two, 2, size);

	for (int k = 0; k < 54; k++) {
		if (DEBUG) {
			printf("checking witness: %d\n", primes[k]);
		}

		byte a[1 << size];
		byte dest[1 << size];
		set(a, primes[k], size);
		admodn(a, d, p, dest, size);
		copy(dest, a, size);

		if (compare(a, one, size) == 0 || compare(a, pm1, size) == 0) {
			continue;
		}

		int is_witness = 0;
		for (int s = 0; s < shift; s++) {
			admodn(a, two, p, dest, size);
			copy(dest, a, size);
			if (compare(a, pm1, size) == 0) {
				is_witness = 1;
				break;
			}
		}
		if (is_witness == 1) {
			continue;
		} else {
			return 0;
		}
	}
	return 1;
}

// exhaustive test of the system for all numbers [256, 65535]
void test() {
	for (int n = 257; n < 65536; n += 2) {
		int is_prime = 1;
		for (int p = 0; p < 54; p++) {
			if (n % primes[p] == 0) {
				is_prime = 0;
				break;
			}
		}
		byte test[2] = {(n & 0xff), ((n >> 8) & 0xff)};

		int idx = is_prime + miller_rabin(test, 1);

		if (idx == 0) {
			printf("%d is composite\n", n);
		} else if (idx == 1) {
			printf("prime: %d miller rabin: %d\n", is_prime, idx - is_prime);
			break;
		} else {
			printf("%d is prime\n", n);
		}
	}
}

int main(int argc, char *argv[]) {
	byte p[2] = {0x15, 0x80};
	int x = miller_rabin(p, 1);
	printf("miller rabin 0x8015: %d\n", x);
	// test();
}