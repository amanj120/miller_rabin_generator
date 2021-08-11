#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

// 8 bit so it works on AVR
typedef unsigned char byte;

byte primes[54] = {2,	3,	 5,	  7,   11,	13,	 17,  19,  23,	29,	 31,
				   37,	41,	 43,  47,  53,	59,	 61,  67,  71,	73,	 79,
				   83,	89,	 97,  101, 103, 107, 109, 113, 127, 131, 137,
				   139, 149, 151, 157, 163, 167, 173, 179, 181, 191, 193,
				   197, 199, 211, 223, 227, 229, 233, 239, 241, 251};

// size = size of src in bytes
void clear(byte *src, int size) {
	for (int i = 0; i < size; i++) {
		src[i] = 0;
	}
}

void print(byte *src, int size) {
	for (int i = size - 1; i >= 0; i--) {
		printf("%02x", src[i]);
	}
}

// set the first byte in dest to constant, clear the rest
void set(byte *dest, byte constant, int size) {
	clear(dest, size);
	dest[0] = constant;
}

void copy(byte *src, byte *dest, int size) {
	for (int i = 0; i < size; i++) {
		dest[i] = src[i];
	}
}

void rand_int(byte *dest, int size) {
	for (int i = 0; i < size; i++) {
		dest[i] = rand() & 0xff;
	}
	dest[0] |= 1;
	dest[size - 1] |= 0x80;
	// set another random bit in the least significant byte
	// because right shift only works for shift < 8;
	if (dest[0] == 1) {
		dest[0] |= (1 << ((rand() % 7) + 1));
	}
}

// 50 % of the run time is taken up by rshift
// shift must be less than 8
void rshift(byte *src, byte shift, int size) {
	byte lshift = (8 - shift);
	for (int i = 0; i < size - 1; i++) {
		src[i] = src[i] >> shift | src[i + 1] << lshift;
	}
	src[size - 1] >>= shift;
}

byte find_lsb_set(byte src) {
	for (byte i = 0; i < 8; i++) {
		if ((src >> i) & 1 == 1) {
			return i;
		}
	}
}

// return <0  if src1 < src2, > 0 if src1 > src2, else 0
int compare(byte *src1, byte *src2, int size) {
	for (int i = size - 1; i >= 0; i--) {
		int t = src1[i] - src2[i];
		if (t != 0) {
			return t;
		}
	}
	return 0;
}

// dest = src1 + src2, returns carry
byte add(byte *src1, byte *src2, byte *dest, int size) {
	byte carry = 0;
	for (int i = 0; i < size; i++) {
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

void add_const(byte *src, byte value, byte *dest, int size) {
	for (int i = 0; i < size; i++) {
		byte t = src[i] + value;
		value = (t < src[i]) ? 1 : 0;
		dest[i] = t;
	}
}

void avg(byte *src1, byte *src2, byte *dest, int size) {
	byte carry = add(src1, src2, dest, size);
	rshift(dest, 1, size);
	if (carry) {
		dest[size - 1] |= 0x8f; // set the msb
	}
}

byte is_const(byte *src, byte constant, int size) {
	if (src[0] != constant) {
		return 0;
	}
	for (int i = 1; i < size; i++) {
		if (src[i] != 0) {
			return 0;
		}
	}
	return 1;
}

// dest = src1 - src2, undefined behavior is src1 < src2
void sub(byte *src1, byte *src2, byte *dest, int size) {
	byte t = 0;
	for (int i = 0; i < size; i++) {
		if (src2[i] > src1[i]) { // No out of bounds errors bc src1 > src2
			for (int j = i + 1; j < size; j++) {
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
void mult(byte *src1, byte *src2, byte *dest, int size) {
	clear(dest, size + 1);
	int idx;
	byte carry, temp;
	for (int i = 0; i < size; i++) {
		for (int j = 0; j < size; j++) {
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

void mod(byte *a, byte *n, byte *dest, int size) {
	byte minus[size << 1];
	clear(minus, size + 1);
	copy(n, &minus[size], size); // minus = n << (1 << size);

	for (int i = 0; i <= (size << 3); i++) {
		if (compare(minus, a, size + 1) < 0) {
			sub(a, minus, a, size + 1); // a -= (n << i)
		}
		rshift(minus, 1, size + 1);
	}
	copy(a, dest, size);
}

// compute a^2 mod n and store in dest
void asqmodn(byte *a, byte *n, byte *dest, int size) {
	byte temp[size << 1];
	clear(temp, size + 1);
	mult(a, a, temp, size);
	mod(temp, n, dest, size);
}

// compute a^d mod n and store in dest
// a, d, and n are all 2^size bytes wide
void admodn(byte *a, byte *d, byte *n, byte *dest, int size) {
	byte cur_a[size];
	byte prod[size];
	byte temp[size << 1];

	clear(cur_a, size);
	clear(prod, size);
	clear(temp, size << 1);

	set(prod, 1, size);

	copy(a, cur_a, size); // current = a

	for (int i = 0; i < size; i++) {
		for (int j = 0; j < 8; j++) {
			// if exponent set, prod = (prod * cur_a) % n
			if ((d[i] >> j) & 1 == 1) {
				mult(cur_a, prod, temp, size);
				mod(temp, n, prod, size);
			}
			// regardless of whether exponent set:
			mult(cur_a, cur_a, temp, size);
			mod(temp, n, cur_a, size);
		}
	}
	copy(prod, dest, size);
}

// p is the int we are testing for primality
int miller_rabin(byte *p, int size) {
	int eqpm1;
	byte a[size];
	byte d[size];
	copy(p, d, size);

	byte lsb = (p[0] & 0xfe);
	byte shift = find_lsb_set(lsb);
	rshift(d, shift, size);

	for (int k = 0; k < 54; k++) {
		set(a, primes[k], size);
		admodn(a, d, p, a, size);

		printf("Testing witness: %d\n", primes[k]);

		p[0] &= 0xfe;
		eqpm1 = compare(a, p, size);
		p[0] |= 1;

		if (is_const(a, 1, size) == 1 || eqpm1 == 0) {
			continue;
		}

		int is_witness = 0;
		for (int s = 0; s < shift; s++) {
			asqmodn(a, p, a, size);

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

// somewhat exhaustive test of the system
void test() {
	// for valgrind, run between 0x30001 and 0x30401
	// for (long n = 0x30001; n < 0x30401; n += 2) {
	// about ~0.5 million values: 0x101 - 0x100001
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
		int idx = is_prime + miller_rabin(test, 4);

		if (idx == 0) {
			printf("%ld is composite\n", n);
		} else if (idx == 1) {
			printf("%ld: prime: %d miller rabin: %d\n", n, is_prime,
				   idx - is_prime);
			break;
		} else {
			printf("%ld is prime\n", n);
		}
	}
}

void find(int size, int stats) {
	byte *test = calloc(size, 1);

	int runs = 0;

	struct timeval start, end;

	gettimeofday(&start, NULL);
	while (1) {
		runs++;
		rand_int(test, size);
		printf("Testing: ");
		print(test, size);
		printf("\n");
		if (miller_rabin(test, size) == 1) {
			break;
		}
	}
	gettimeofday(&end, NULL);

	print(test, size);
	printf(" is probably prime!\n");

	long diff = (end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec);

	if (stats) {
		printf("took %d runs and %ld us\n", runs, diff);
		printf("%d %ld\n", runs, diff); // for easier parsing

	}
}

void print_help() {
	printf("Use this program to generate random probable primes using the miller-rabin algorithm:");
	// generated because 1 : 4 ^ 54 chance that the number is not prime 
	printf("\"probable\": less than a 1 in 300,000,000,000,000,000,000,000,000,000,000 (3.2 * 10^32) chance that the output is not prime)\n");
	// (2 ^ 108) / (2,500,000 L/pool 1000g/1L * 1 mol/ 18 g * 6.022*10^23 molecules/mol) = 3.88
	printf("to put that into perspective, that's about how many MOLECULES of water there are in 4 olympic sized pools combined.\n");
	
	// or '--size=<valid size value>' 
	printf("\nuse the flag '-s=<valid size value>' to pass in the size of the prime you would like to generate \n");
	printf("valid values are: \n");
	printf("\t'1': 16 bit prime\n");
	printf("\t'2': 32 bit prime\n");
	printf("\t'3': 64 bit prime\n");
	printf("\t'4': 128 bit prime\n");
	printf("\t'5': 256 bit prime\n");
	printf("\t'6': 512 bit prime\n");
	printf("\t'7': 1024 bit prime\n");
	printf("\t'8': 2048 bit prime\n");
	printf("The default value is 3 (64 bit prime)");
	// or '--stats'
	printf("use the flag '-t' to print stats about the run\n");
}

// stack usage -> about 20x the size of the prime, need to cut down
// cut down to 15x, still too much
// cut down to 9x, getting better; 7 probably the sweet spot
// cut down to 8x by removing dest in miller rabin
int main(int argc, char *argv[]) {

	test();

	// struct timeval seed;
	// gettimeofday(&seed, NULL);
	// long s_val = seed.tv_sec * 1000000 + seed.tv_usec;
	// int s_val_int =  s_val & (((long)1 << 32) - 1);
	// srand(s_val_int);

	// int size = 16;
	// int stats = 1;

	// if (argc == 2) {
	// 	if (argv[1][0] == '-' && argv[1][1] == 's' && argv[1][2] == '=') {
	// 		size = (int)argv[1][3] - 48; // (char)48 = '0';
	// 		stats = 0;
	// 	} else if (argv[1][0] == '-' && argv[1][1] == 't') {
	// 		size = 5;
	// 		stats = 1;
	// 	} 
	// } else if (argc == 3) {
	// 	if (argv[1][0] == '-' && argv[1][1] == 's' && argv[1][2] == '=') {
	// 		size = (int)argv[1][3] - 48; // (char)48 = '0';
	// 	}
	// 	if (argv[1][0] == '-' && argv[1][1] == 't') {
	// 		stats = 1;
	// 	} 
	// 	if (argv[2][0] == '-' && argv[2][1] == 's' && argv[2][2] == '=') {
	// 		size = (int)argv[1][3] - 48; // (char)48 = '0';
	// 	}
	// 	if (argv[2][0] == '-' && argv[2][1] == 't') {
	// 		stats = 1;
	// 	} 
	// } else if (argc >= 4) {
	// 	printf("invalid arguments, continuing with '-s=5 -t'\n");
	// }

	// find(size, stats);
}