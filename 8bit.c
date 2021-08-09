#include <stdio.h>
#include <stdlib.h>

// 8 bit so it works on AVR
typedef unsigned char byte;

byte *new (byte size) { return calloc((1 << size), sizeof(byte)); }

byte primes[53] = {2,	3,	 5,	  7,   11,	13,	 17,  23,  29,	31,	 37,
				   41,	43,	 47,  53,  59,	61,	 67,  71,  73,	79,	 83,
				   89,	97,	 101, 103, 107, 109, 113, 127, 131, 137, 139,
				   149, 151, 157, 163, 167, 173, 179, 181, 191, 193, 197,
				   199, 211, 223, 227, 229, 233, 239, 241, 251};

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
	clear(dest, size);
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
	// byte temp[1 << size];
	// for (int i = 0; i < (1 << size); i++) {
	// 	temp[i] = ~src2[i];
	// }
	// add_const(temp, 1, temp, size);
	printf("in sub:\n");
	int carry = 1;
	for (int i = 0; i < (1 << size); i++) {
		int t = (int)src1[i] + (int)(~src2[i]) + carry;
		carry = (t >> 8) & 1;
		printf("digit: %x, carry: %d ", t, carry);
		dest[i] = t;
	}
	printf("\n");
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

// a = qn + r. Assume a < n^2 (because every multiplication we mod n)
// size = log_2(size of n)
// algorithm: binary search for q
void mod(byte *a, byte *n, byte *dest, byte size) {
	printf("\ncalculating ");
	print(a, size + 1);
	printf(" mod ");
	print(n, size);
	printf("\n");

	clear(dest, size);

	byte q_hi[1 << size];
	byte q_lo[1 << size];
	byte q_mid[1 << size];
	byte prev[1 << size];
	byte prod[1 << (size + 1)];

	clear(q_hi, size);
	clear(q_lo, size);
	clear(q_mid, size);
	clear(prev, size);
	clear(prod, size + 1);

	for (int i = 0; i < (1 << size); i++) {
		q_hi[i] = 0xff;
	}

	do {
		copy(q_mid, prev, size);

		add(q_hi, q_lo, q_mid, size);
		rshift(q_mid, 1, size);
		mult(q_mid, n, prod, size);

		printf("lo: ");
		print(q_lo, size);
		printf(" high: ");
		print(q_hi, size);
		printf(" mid: ");
		print(q_mid, size);
		printf(" prod: ");
		print(prod, size + 1);
		printf(" n: ");
		print(n, size);
		printf(" a: ");
		print(a, size + 1);
		printf(" \n");

		if (compare(prod, a, size + 1) < 0) {
			copy(q_mid, q_lo, size);
		} else {
			copy(q_mid, q_hi, size);
		}
	} while (compare(prev, q_mid, size) != 0);

	// sub(a, prod, dest, size);

	// byte *dest = new (size);
	byte zero[(1 << size)];
	clear(zero, size);

	printf("prod: ");
	print(prod, size + 1);
	printf("\n");

	if (compare(q_mid, zero, size) == 0) {
		copy(a, dest, size);
	} else {
		printf("prod: ");
		print(prod, size + 1);
		printf("\n");
		printf("a: ");
		print(a, size + 1);
		printf("\n");
		byte temp2[1 << (size + 1)];
		printf("a compare to prod: %d \n", compare(a, prod, size + 1));
		sub(a, prod, temp2, size + 1);
		printf("temp: ");
		print(temp2, size + 1);
		printf("\n");
	}
}

// compute a^d mod n and store in dest
// a, d, and n are all 2^size bytes wide
void admodn(byte *a, byte *d, byte *n, byte *dest, byte size) {
	printf("\t\tcalculating ");
	print(a, size);
	printf(" ^ ");
	print(d, size);
	printf(" mod ");
	print(n, size);
	printf(": \n");

	byte cur_a[1 << size];
	byte prod[1 << size];
	byte temp[1 << (size + 1)];

	set(prod, 1, size);

	copy(a, cur_a, size); // current = a

	for (int i = 0; i < 1 << (size); i++) {
		for (int j = 0; j < 8; j++) {

			printf("cur a: ");
			print(cur_a, size);
			printf("\nexp: %d value: %d\n", (i * 8 + j), (d[i] >> j) & 1);

			// if exponent set, prod = (prod * cur_a) % n
			if ((d[i] >> j) & 1 == 1) {
				clear(temp, size);
				mult(cur_a, prod, temp, size);
				mod(temp, n, prod, size);
				printf("\tcur_prod: ");
				print(prod, size);
				printf("\n");
			}
			// regardless of whether exponent set:
			// cur_a = (cur_a * cur_a) % n
			printf(" here ");
			clear(temp, size);
			mult(cur_a, cur_a, temp, size);
			// print(temp, size + 1);
			// printf(" new cur a mod n: ");
			mod(temp, n, cur_a, size);
			printf("new cur_a: ");

			print(cur_a, size);
			// printf(" new prod: ");
			// print(prod, size);
			printf("\n");
		}
	}

	copy(prod, dest, size);
}

// p is the int we are testing for primality
int miller_rabin(byte *p, byte size) {
	// generate a prime
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

	for (int k = 0; k < 53; k++) {
		printf("checking witness: %d\n", primes[k]);

		byte a[1 << size];
		byte dest[1 << size];
		set(a, primes[k], size);
		admodn(a, d, p, dest, size);
		copy(dest, a, size);

		printf("\t");
		print(a, size);
		printf("\n");

		if (compare(a, one, size) == 0 || compare(a, pm1, size) == 0) {
			printf("\tfirst iteration passed\n");
			continue;
		}

		int is_witness = 0;
		for (int s = 0; s < shift; s++) {
			admodn(a, two, p, dest, size);
			copy(dest, a, size);

			printf("\t");
			print(a, size);
			printf("\n");

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

int main(int argc, char *argv[]) {

	byte x63d[2] = {0x3d, 0x06};
	byte x000c7a01[4] = {0x01, 0x7a, 0x0c, 0x00};
	byte x0001[2] = {0x00, 0x00};
	mod(x000c7a01, x63d, x0001, 1);
	print(x0001, 1);
	printf("\n");

	// calculating 000c7a01 mod 063d -> this is messing up

	// byte ft[2] = {0x2b, 0x00};
	// byte pow[2] = {0x8f, 0x01};
	// byte n[2] = {0x3d, 0x06};
	// byte dest[2] = {0x00, 0x00};
	// admodn(ft, pow, n, dest, 1);

	// print(dest, 1);
	// printf("\n");

	// byte a[4] = {0xcd, 0xab, 0x89, 0x67};
	// byte b[4] = {0x78, 0x56, 0x34, 0x12};

	// byte *prod = calloc(8, sizeof(byte));
	// mult(&a[0], &b[0], prod, 2);

	// for (int i = 7; i >= 0; i--) {
	// 	printf("%02x", prod[i]);
	// }
	// printf("\n");

	// printf("%d, %d\n", compare(a, b, 2), compare(b, a, 2));

	// byte *aa = calloc(4, 1);
	// add(a, b, aa, 2);
	// for (int i = 3; i >= 0; i--) {
	// 	printf("%02x", aa[i]);
	// }
	// printf("\n");

	// sub(aa, b, aa, 2);
	// for (int i = 3; i >= 0; i--) {
	// 	printf("%02x", aa[i]);
	// }
	// printf("\n");

	// sub(aa, b, aa, 2);
	// for (int i = 3; i >= 0; i--) {
	// 	printf("%02x", aa[i]);
	// }
	// printf("\n");

	// sub(aa, b, aa, 2);
	// for (int i = 3; i >= 0; i--) {
	// 	printf("%02x", aa[i]);
	// }
	// printf("\n");

	// rshift(aa, 3, 2);
	// for (int i = 3; i >= 0; i--) {
	// 	printf("%02x", aa[i]);
	// }
	// printf("\n"); // 00466926970ec559

	// mult(aa, aa, prod, 2);
	// for (int i = 7; i >= 0; i--) {
	// 	printf("%02x", prod[i]);
	// }
	// printf("\n"); // so far so good:
	// // 00466926970ec559

	// byte *modulo = new (2);
	// mod(prod, b, modulo, 2);
	// for (int i = 3; i >= 0; i--) {
	// 	printf("%02x", modulo[i]);
	// }
	// printf("\n");
	// // should be: 0e962fc9

	// free(prod);
	// free(aa);
	// free(modulo);

	// byte base[2] = {0x00, 0x00};
	// byte n[2] = {0x23, 0x02};
	// byte exp[2] = {0x22, 0x02};
	// byte dest[2] = {0x00, 0x00};

	// for (int i = 0; i < 53; i++) {
	// 	set(base, primes[i], 1);
	// 	admodn(base, exp, n, dest, 1);
	// 	print(dest, 1);
	// 	printf("\n");
	// }

	// printf("%d, %d\n", find_lsb_set(0xf0), find_lsb_set(0x80));

	// byte test[2] = {0x3d, 0x06};
	// printf("answer: %d\n", miller_rabin(test, 1));

	// return 0;
}