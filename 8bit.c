#include <stdio.h>
#include <stdlib.h>

// 8 bit so it works on AVR
typedef unsigned char byte;

byte *new (byte size) { return calloc((1 << size), sizeof(byte)); }

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

void fill(byte *dest, byte constant, byte size) {
	clear(dest, size);
	dest[0] = constant;
}

void copy(byte *src, byte *dest, byte size) {
	for (int i = 0; i < (1 << size); i++) {
		dest[i] = src[i];
	}
}

// shift < 8
void rshift(byte *src, byte shift, byte size) {
	for (int i = 0; i < (1 << size) - 1; i++) {
		src[i] = src[i] >> shift | src[i + 1] << (8 - shift);
	}
	src[(1 << size) - 1] >>= shift;
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

// src1 -= src2, undefined behavior is src1 < src2
void sub(byte *src1, byte *src2, byte *dest, byte size) {
	byte carry = 1;
	for (int i = 0; i < (1 << size); i++) {
		byte t = src1[i] + ~src2[i] + carry;
		carry = (t < src1[i]) ? 1 : 0;
		dest[i] = t;
	}
	return;
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
	byte prod[1 << (size + 1)];

	clear(q_hi, size);
	clear(q_lo, size);
	clear(q_mid, size);
	clear(prod, size + 1);

	for (int i = 0; i < (1 << size); i++) {
		q_hi[i] = 0xff;
	}

	int iter = 0;

	while (compare(q_lo, q_hi, size) != 0) {
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
			add_const(q_mid, 0, q_lo, size);
		} else {
			add_const(q_mid, 0, q_hi, size);
		}

		if (iter++ == 16) {
			break;
		}
	}

	// byte *dest = new (size);
	byte zero[(1 << size)];
	clear(zero, size);

	if (compare(q_mid, zero, size) == 0) {
		copy(a, dest, size);
		return;
	}

	sub(a, prod, dest, size);

	// return dest;
}

// compute a^d mod n and store in dest
void admodn(byte *a, byte *d, byte *n, byte *dest, byte size) {
	byte cur_a[1 << size];
	byte prod[1 << size];
	byte temp[1 << (size + 1)];

	fill(prod, 1, size);

	copy(a, cur_a, size); // current = a

	for (int i = 0; i < 1 << (size); i++) {
		for (int j = 0; j < 8; j++) {

			printf("cur a: ");
			print(cur_a, size);
			printf(", exp: %d value: %d, new cur_a: ", (i * 8 + j),
				   (d[i] >> j) & 1);

			if ((d[i] >> j) & 1 == 1) {
				mult(cur_a, prod, temp, size);
				mod(temp, n, prod, size);
			}
			mult(cur_a, cur_a, temp, size);
			print(temp, size + 1);
			printf(" new cur a mod n: ");
			mod(temp, n, cur_a, size);
			print(cur_a, size);
			printf(" new prod: ");
			print(prod, size);
			printf("\n");
		}
	}

	copy(prod, dest, size);
}

int main(int argc, char *argv[]) {
	byte a[4] = {0xcd, 0xab, 0x89, 0x67};
	byte b[4] = {0x78, 0x56, 0x34, 0x12};

	byte *prod = calloc(8, sizeof(byte));
	mult(&a[0], &b[0], prod, 2);

	for (int i = 7; i >= 0; i--) {
		printf("%02x", prod[i]);
	}
	printf("\n");

	printf("%d, %d\n", compare(a, b, 2), compare(b, a, 2));

	byte *aa = calloc(4, 1);
	add(a, b, aa, 2);
	for (int i = 3; i >= 0; i--) {
		printf("%02x", aa[i]);
	}
	printf("\n");

	sub(aa, b, aa, 2);
	for (int i = 3; i >= 0; i--) {
		printf("%02x", aa[i]);
	}
	printf("\n");

	sub(aa, b, aa, 2);
	for (int i = 3; i >= 0; i--) {
		printf("%02x", aa[i]);
	}
	printf("\n");

	sub(aa, b, aa, 2);
	for (int i = 3; i >= 0; i--) {
		printf("%02x", aa[i]);
	}
	printf("\n");

	rshift(aa, 3, 2);
	for (int i = 3; i >= 0; i--) {
		printf("%02x", aa[i]);
	}
	printf("\n"); // 00466926970ec559

	mult(aa, aa, prod, 2);
	for (int i = 7; i >= 0; i--) {
		printf("%02x", prod[i]);
	}
	printf("\n"); // so far so good:
	// 00466926970ec559

	byte *modulo = new (2);
	mod(prod, b, modulo, 2);
	for (int i = 3; i >= 0; i--) {
		printf("%02x", modulo[i]);
	}
	printf("\n");
	// should be: 0e962fc9

	free(prod);
	free(aa);
	free(modulo);

	byte base[2] = {0x03, 0x00};
	byte n[2] = {0x23, 0x02};
	byte exp[2] = {0x22, 0x02};
	byte dest[2] = {0x00, 0x00};

	admodn(base, exp, n, dest, 1);

	print(dest, 1);
	printf("\n");

	return 0;
}