#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

typedef uint32_t unit;
typedef int64_t sunit;

#define NP 21

static const unit bases[NP] = {9, 31, 43, 61, 71, 89, 107, 131, 137, 149, 157, 163, 167, 173, 179, 181, 199, 211, 227, 233, 241};

static const char *help_message =
	"\nThis program uses the Miller-Rabin algorithm to generate random "
	"probable primes.\nThere is about a 1 in 2^42 chance that the output "
	"is not prime.\n\n\t-s <size> to set the size of the generated prime "
	"in bits\n\n\t-d to turn off debug info\n\n";

// size = size of src in units
void clear(unit *src, unit size) {
	for (unit i = 0; i < size; i++) {
		src[i] = 0;
	}
}

void print(unit *src, unit size) {
	for (sunit i = size - 1; i >= 0; i--) {
		printf("%08x", src[i]);
	}
}

// set the first unit in dest to constant, clear the rest
void set(unit *dest, unit constant, unit size) {
	clear(dest, size);
	dest[0] = constant;
}

void copy(unit *src, unit *dest, unit size) {
	for (unit i = 0; i < size; i++) {
		dest[i] = src[i];
	}
}
unit rand_unit() {
	unit l1 = rand() & 0xff;
	unit l2 = rand() & 0xff;
	unit l3 = rand() & 0xff;
	unit l4 = rand() & 0xff;

	return (unit)(l1 | (l2 << 8) | (l3 << 16) | (l4 << 24));
}


void rand_int(unit *dest, unit size) {
	for (unit i = 0; i < size; i++) {
		dest[i] = rand_unit();
	}
	dest[0] |= 3; // we're pinning the 2nd least bit to be 1 so that the miller rabin test is easier to do
	// dest[size - 1] |= 0x80000000; //pinning the msb to be 1
	printf("in rand int: ");
	print(dest, size);
	printf("\n");
}

// 50 % of the run time is taken up by rshift
// shift must be less than 32
void rshift(unit *src, unit shift, unit size) {
	unit lshift = (32 - shift);
	for (unit i = 0; i < size - 1; i++) {
		src[i] = src[i] >> shift | src[i + 1] << lshift;
	}
	src[size - 1] >>= shift;
}

// ecc2f6c3
// 3ecf2709

// unit find_lsb_set(unit src) {
// 	for (unit i = 0; i < 32; i++) {
// 		if ((src >> i) & 1 == 1) {
// 			return i;
// 		}
// 	}
// }

// return <0  if src1 < src2, > 0 if src1 > src2, else 0
unit compare(unit *src1, unit *src2, unit size) {
	for (sunit i = size - 1; i >= 0; i--) {
		sunit t = src1[i] - src2[i];
		if (t < 0) {
			return -1;
		} else if (t > 0) {
			return 1;
		}
	}
	return 0;
}

// dest = src1 + src2, returns carry
unit add(unit *src1, unit *src2, unit *dest, unit size) {
	unit carry = 0;
	for (unit i = 0; i < size; i++) {
		unit t = src1[i] + src2[i] + carry;
		if (carry == 1) {
			carry = (t <= src1[i]) ? 1 : 0;
		} else {
			carry = (t < src1[i]) ? 1 : 0;
		}
		dest[i] = t;
	}
	return carry;
}

void add_const(unit *src, unit value, unit *dest, unit size) {
	for (unit i = 0; i < size; i++) {
		unit t = src[i] + value;
		value = (t < src[i]) ? 1 : 0;
		dest[i] = t;
	}
}

unit is_const(unit *src, unit constant, unit size) {
	if (src[0] != constant) {
		return 0;
	}
	for (unit i = 1; i < size; i++) {
		if (src[i] != 0) {
			return 0;
		}
	}
	return 1;
}

// dest = src1 - src2, undefined behavior is src1 < src2
void sub(unit *src1, unit *src2, unit *dest, unit size) {
	unit t = 0;
	for (unit i = 0; i < size; i++) {
		if (src2[i] > src1[i]) { // No out of bounds errors bc src1 > src2
			for (unit j = i + 1; j < size; j++) {
				if (src1[j] == 0) {
					src1[j] = 0xffffffff;
				} else {
					src1[j] -= 1;
					break;
				}
			}
			t = (0x100000000L - (src2[i] - src1[i]));
		} else {
			t = src1[i] - src2[i];
		}
		dest[i] = t;
	}
	return;
}

// size of dest must be twice that of src1 and src2
// src1 and src2 must be the same size
void mult(unit *src1, unit *src2, unit *dest, unit size) {
	clear(dest, size << 1);
	unit idx;
	unit carry, temp;
	for (unit i = 0; i < size; i++) {
		for (unit j = 0; j < size; j++) {
			temp = dest[i + j];
			uint64_t p = src1[i] * src2[j];
			carry = (unit)((p >> 32) & 0xffffffff);
			idx = 1;
			dest[i + j] += (unit)(p & 0xffffffff);
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

// assume a is twice as big as n
// a = qn + r for some integer q and some r in [0, n); return r
void mod(unit *a, unit *n, unit *dest, unit size) {
	if (compare(a, n, size) < 0 && is_const(&a[size], 0, size)) {
		copy(a, dest, size); // a < n
		return;
	}
	unit bigsize = size << 1;
	unit minus[bigsize];
	clear(minus, bigsize);
	copy(n, &minus[size], size); // minus = n << (1 << size);

	for (unit i = 0; i <= (size << 5); i++) {
		if (compare(minus, a, bigsize) < 0) {
			sub(a, minus, a, bigsize); // a -= (n << i)
		}
		rshift(minus, 1, bigsize);
	}
	copy(a, dest, size);
}

// compute a^2 mod n and store in dest
// void asqmodn(unit *a, unit *n, unit *dest, unit size) {
// 	unit temp[size << 1];
// 	clear(temp, size << 1);
// 	mult(a, a, temp, size);
// 	mod(temp, n, dest, size);
// }

// compute a^d mod n and store in dest
// a, d, and n are all 2^size units wide
void admodn(unit *a, unit *d, unit *n, unit *dest, unit size) {
	unit cur_a[size];
	unit prod[size];
	unit temp[size << 1];

	clear(cur_a, size);
	clear(prod, size);
	clear(temp, size << 1);

	set(prod, 1, size);
	copy(a, cur_a, size);

	// binary exponentiation
	for (unit i = 0; i < size; i++) {
		for (unit j = 0; j < 32; j++) {
			if ((d[i] >> j) & 1 == 1) {
				mult(cur_a, prod, temp, size);
				mod(temp, n, prod, size);
			}
			mult(cur_a, cur_a, temp, size);
			mod(temp, n, cur_a, size);
		}
	}
	copy(prod, dest, size);
}

// p is the unit we are testing for primality
unit miller_rabin(unit *p, unit size) {
	unit eqpm1;
	unit a[size];
	unit d[size];
	copy(p, d, size);

	rshift(d, 1, size); // because we know the 2nd least bit is a 1

	for (unit k = 0; k < NP; k++) {
		set(a, bases[k], size);
		admodn(a, d, p, a, size);

		p[0] &= 0xfffffffe; // little trick to see if a = p - 1 in place
		eqpm1 = compare(a, p, size);
		p[0] |= 1;

		if (is_const(a, 1, size) != 1 && eqpm1 != 0) { // if a^d === 1 or -1 mod p, a is a witness; otherwise it is not
			print(a, size);
			printf(" no witness, a is 1 ? %d a is -1 ? %d\n", is_const(a, 1, size), eqpm1);
			return 0;
		}
	}
	return 1;
}

// void parallel_find(unit size) {
// 	unit found = 0;

// #pragma omp parallel for
// 	for (int i = 0; i < 1 << 10; ++i) {
// 		if (found == 0) {
// 			// unit *test = calloc(size, 1);
// 			unit test[size];
// 			rand_int(test, size);
// 			// printf("Testing: ");
// 			// print(test, size);
// 			// printf("\n");
// 			if (found == 0 && miller_rabin(test, size) == 1) {
// 				if (found == 0) {
// 					printf("Found probable prime:\n");
// 					print(test, size);
// 					printf("\n");
// 					found = 1;
// 				}
// 			}
// 			// free(test);
// 		}
// 	}
// }

void find(unit size, unit print_stuff) {
	unit *test = (unit *)calloc(size, sizeof(unit));
	unit runs = 0;
	struct timeval start, end;

	gettimeofday(&start, NULL);
	while (++runs) {
		rand_int(test, size);
		if (print_stuff) {
			printf("Testing w size %d: ", size);
			print(test, size);
			printf("\n");
		}
		if (miller_rabin(test, size) == 1) {
			break;
		}
	}
	gettimeofday(&end, NULL);

	if (print_stuff) {
		printf("\n");
	}

	printf("Found probable prime:\n");
	print(test, size);
	printf("\n");

	long diff = (end.tv_sec * 1000000 + end.tv_usec) -
				(start.tv_sec * 1000000 + start.tv_usec);

	if (print_stuff) {
		printf("\ntook %ld runs and %ld us\n", runs, diff);
	}

	free(test);
}

unit help() {
	printf("%s", help_message);
	return 0;
}

void seed() {
	struct timeval seed;
	gettimeofday(&seed, NULL);
	long s_val = seed.tv_sec * 1000000 + seed.tv_usec;
	unit s_val_unit = s_val & (((long)1 << 32) - 1);
	srand(s_val_unit);
}

// somewhat exhaustive test of the system
void test() {
	// for valgrind, run between 0x30001 and 0x30401
	// for (long n = 0x30001; n < 0x30401; n += 2) {
	// about ~0.5 million values: 0x101 - 0x100001
	for (long n = 0x101; n < 0x100001; n += 2) {
		if ((n & 0xffffffff) == 0x1) { // we can't handle these cases
			printf("skipping %ld \n", n);
			continue;
		}

		unit is_prime = 1;
		for (unit d = 3; d * d <= n; d += 2) {
			if (n % d == 0) {
				is_prime = 0;
				break;
			}
		}
		unit test[4] = {(n & 0xff), ((n >> 8) & 0xff), ((n >> 16) & 0xff),
						((n >> 24) & 0xff)};
		unit idx = is_prime + miller_rabin(test, 4);

		if (idx == 0) {
			// printf("%ld is composite\n", n);
		} else if (idx == 1) {
			printf("%ld: prime: %ld miller rabin: %ld\n", n, is_prime,
				   idx - is_prime);
			break;
		} else {
			// printf("%ld is prime\n", n);
		}
	}
}

// total stack usage -> about 8x size of prime
int main(int argc, char *argv[]) {
	seed();
	unit size = 0;
	unit print_stuff = 1;

	for (unit i = 1; i < argc; i++) {
		if (strncmp(argv[i], "-d", 2) == 0) {
			print_stuff = 0;
		} else if (strncmp(argv[i], "-s", 2) == 0) {
			size = atoi(argv[++i]) >> 5;
		} else {
			return help();
		}
	}
	if (size == 0) {
		return help();
	}

	find(size, print_stuff);
	// parallel_find(size);
}