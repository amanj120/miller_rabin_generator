#include <stdio.h>
#include <stdlib.h>

// 8 bit so it works on AVR
#define DEBUG 0

typedef unsigned char byte;

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

// 50 % of the run time is taken up by rshift
// shift must be less than 8
void rshift(byte *src, byte shift, byte size) {
	byte lshift = (8 - shift);
	for (int i = 0; i < (1 << size) - 1; i++) {
		src[i] = src[i] >> shift | src[i + 1] << lshift;
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

// return <0  if src1 < src2, > 0 if src1 > src2, else 0
int compare(byte *src1, byte *src2, byte size) {
	for (int i = (1 << size) - 1; i >= 0; i--) {
		int t = src1[i] - src2[i];
		if (t != 0) {
			return t;
		}
	}
	return 0;
}

// src1 += src2, overflows back to 0
// returns carry
byte add(byte *src1, byte *src2, byte *dest, byte size) {
	byte carry = 0;
	// clear(dest, size);
	for (int i = 0; i < (1 << size); i++) {
		byte t = src1[i] + src2[i] + carry;
		if (carry == 1) {
			carry = (t <= src1[i]) ? 1 : 0;
		} else {
			carry = (t < src1[i]) ? 1 : 0;
		}
		dest[i] = t;
	}
	return carry;
}

void add_const(byte *src, byte value, byte *dest, byte size) {
	// clear(dest, size);
	for (int i = 0; i < (1 << size); i++) {
		byte t = src[i] + value;
		value = (t < src[i]) ? 1 : 0;
		dest[i] = t;
	}
}

void avg(byte *src1, byte *src2, byte *dest, byte size) {
	byte carry = add(src1, src2, dest, size);
	rshift(dest, 1, size);
	if (carry) {
		dest[(1 << size) - 1] |= 0x8f; // set the msb
	}
}

byte is_const(byte * src, byte constant, byte size) {
	if (src[0] != constant) {
		return 0;
	} 
	for (int i = 1; i < (1 << size); i++) {
		if (src[i] != 0) {
			return 0;
		}
	}
	return 1;
}

//dest = src1 - src2, undefined behavior is src1 < src2
void sub(byte *src1, byte *src2, byte *dest, byte size) {
	// printf("in sub\n");
	byte t = 0;
	for (int i = 0; i < (1 << size); i++) {
		if (src2[i] > src1[i]) {
			// No out of bounds errors bc src1 > src2
			for (int j = i + 1; j < (1 << size); j++) {
				if (src1[j] == 0) {
					src1[j] = 255;
				} else {
					src1[j] -= 1;
					break;
				}
			}
			t = (255 - (src2[i] - src1[i])) + 1;
		} else {
			t = src1[i] - src2[i];
		}
		dest[i] = t;
	}
	return;
}

// size = log_2(length of src1, src2)
// size of dest must be twice that of src1 and src2
// src1 and src2 must be the same size
void mult(byte *src1, byte *src2, byte *dest, byte size) {
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

void mod(byte * a, byte *n, byte * dest, byte size) {
	byte minus[1 << (size + 1)];
	clear(minus, size + 1);
	copy(n, &minus[(1 << size)], size); // shift n (1 << size);

	for (int i = 0; i <= (1 << (size + 3)); i++) {
		if (compare(minus, a, size + 1) < 0) {
			sub(a, minus, a, size + 1); // a -= (n << i)
		}
		rshift(minus, 1, size + 1);
	}
	copy(a, dest, size);
}


// void mod(byte * a, byte *n, byte * dest, byte size) {
// 	if (DEBUG) {
// 		printf("calculating ");
// 		print(a, size + 1);
// 		printf(" mod ");
// 		print(n, size);
// 		printf("\n");
// 	}

// 	// byte temp[1 << (size + 1)];
// 	byte minus[1 << (size + 1)];
// 	// clear(temp, size + 1);
// 	clear(minus, size + 1);
// 	copy(n, &minus[(1 << size)], size); // shift n (1 << size);

// 	for (int i = 0; i <= (1 << (size + 3)); i++) {

// 		if (DEBUG) {
// 			printf("minus: ");
// 			print(minus, size + 1);
// 			printf(" remainder = ");
// 			print(a, size + 1);
// 			printf("\n");
// 		}

// 		if (compare(minus, a, size + 1) < 0) {
// 			// if minus  < a:
// 			sub(a, minus, a, size + 1); // a -= (n << i)
// 			// copy(temp, a, size + 1);
// 		}
// 		rshift(minus, 1, size + 1);
// 	}
// 	copy(a, dest, size);

// }

// compute a^2 mod n and store in dest
void asqmodn(byte *a, byte *n, byte *dest, byte size) {
	if (DEBUG) {
		printf("calculating ");
		print(a, size);
		printf(" ^ 2 mod");
		print(n, size);
		printf("\n");
	}
	
	byte temp[1 << (size + 1)];
	clear(temp, size + 1);
	mult(a, a, temp, size);
	mod(temp, n, dest, size);

	if (DEBUG) {
		print(a, size);
		printf(" ^ 2 mod");
		print(n, size);
		printf(" = ");
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

	clear(cur_a, size);
	clear(prod, size);
	clear(temp, size + 1);

	set(prod, 1, size);

	copy(a, cur_a, size); // current = a

	for (int i = 0; i < 1 << (size); i++) {
		for (int j = 0; j < 8; j++) {
			if (DEBUG) {
				printf("on %d bit, it is%sset\n", i * 8 + j,
					   (((d[i] >> j) & 1 == 1) ? " " : " not "));
			}
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

	if (DEBUG) {
		print(a, size);
		printf(" ^ ");
		print(d, size);
		printf(" mod ");
		print(n, size);
		printf(" = ");
		print(dest, size);
		printf("\n");
	}
}

// p is the int we are testing for primality
int miller_rabin(byte *p, byte size) {
	int eqpm1;
	
	byte d[1 << size];
	copy(p, d, size);

	byte a[1 << size];
	// byte dest[1 << size];

	byte lsb = (p[0] & 0xfe);
	byte shift = find_lsb_set(lsb);
	rshift(d, shift, size);

	for (int k = 0; k < 54; k++) {
		if (DEBUG) {
			printf("checking witness: %d\n", primes[k]);
		}

		set(a, primes[k], size);
		admodn(a, d, p, a, size);
		// copy(dest, a, size);

		p[0] &= 0xfe;
		eqpm1 = compare(a, p, size);
		p[0] |= 1;
		if (is_const(a, 1, size) == 1 || eqpm1 == 0) {
			continue;
		}

		int is_witness = 0;
		for (int s = 0; s < shift; s++) {
			asqmodn(a, p, a, size);
			// copy(dest, a, size);

			p[0] &= 0xfe;
			int eqpm1 = compare(a, p, size);
			p[0] |= 1;

			if (eqpm1 == 0) {
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

// exhaustive test of the system
void test() {
	// about 1 million values
	// for valgrind, run between 0x30001 and 0x30401
	// for (long n = 0x30001; n < 0x30401; n += 2) {
	for (long n = 0x101; n < 0x40001; n += 2) {
		if ((n & 0xff) == 0x1) { // we can't handle these cases
			printf("skipping %ld \n", n);
			continue;
		}

		int is_prime = 1;
		for (int d = 3; d * d <= n; d += 2) {
			if (n % d == 0) {
				is_prime = 0;
				break;
			}
		}
		byte test[4] = {(n & 0xff), ((n >> 8) & 0xff), ((n >> 16) & 0xff),
						((n >> 24) & 0xff)};
		// print(test, 2);
		// printf(" ");
		int idx = is_prime + miller_rabin(test, 2);

		if (idx == 0) {
			// printf("%ld is composite\r", n);
		} else if (idx == 1) {
			printf("%ld: prime: %d miller rabin: %d\n", n, is_prime,
				   idx - is_prime);
			break;
		} else {
			// printf("%ld is prime    \r", n);
		}
	}
}

// stack usage -> about 20x the size of the prime, need to cut down
// cut down to 15x, still too much
// cut down to 9x, getting better; 7 probably the sweet spot
// cut down to 8x by removing dest in miller rabin
int main(int argc, char *argv[]) {
	// byte s1[4] = {0,0,0,0};
	// byte s2[4] = {0,1,0,0};
	// byte dest[4] = {0,0,0,0};
	// add(s1, s2, dest, 2);

	// failing on (0000 + 0100) >> 1 = 0080
	// byte n1[2] = {0x00, 0x00};
	// byte n2[2] = {0x00, 0x01};
	// byte dest[2] = {0x00, 0x00};
	// printf("average: ");
	// avg(n1, n2, dest, 1);
	// print(dest, 1);
	// printf("\n");

	// failing on calculating 40140190 mod 8015
	// byte a[4] = {0x90, 0x01, 0x14, 0x40};
	// byte n[2] = {0x15, 0x80};
	// byte dest[2] = {0x00, 0x00};
	// mod(a, n, dest, 1);
	// print(dest, 1);
	// printf("\n");

	// 17c7 ^ 0002 mod 8015
	// byte a[2] = {0xc7, 0x17};
	// byte d[2] = {0x02, 0x00};
	// byte n[2] = {0x15, 0x80};
	// byte dest[2] = {0x00, 0x00};
	// admodn(a, d, n, dest, 1);
	// print(dest, 1);
	// printf("\n");

	// byte p[4] = {0x07, 0x01, 0x00, 0x00};
	// int x = miller_rabin(p, 2);
	// printf("miller rabin 0x10d: %d\n", x);

	test();

	byte * test = calloc(8, 1);

	while (1) {
		rand_int(test, 3);
		printf("Testing: ");
		print(test, 3);
		printf("\n");
		if (miller_rabin(test, 3) == 1) {
			break;
		}
	}
	print(test, 3);
	printf(" is probably prime!\n");
}