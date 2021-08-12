#define NP 53

static const byte primes[NP] = {
  2,   3,   5,   7, 11,  13,  17,  19,  23,  29,  31,  37,  41,  43,
  47,  53,  59,  61,  67,  71,  73,  79,  83,  89,  97,  101, 103, 107,
  109, 113, 127, 131, 137, 139, 149, 151, 157, 163, 167, 173, 179, 181,
  191, 193, 197, 199, 211, 223, 227, 229, 233, 239, 241};

static const char *help_message =
  "\nThis program uses the Miller-Rabin algorithm to generate random "
  "probable primes.\nThere is about a 1 in 2^106 chance that the output "
  "is not prime.\nThat's how many molecules of water there are in an "
  "Olympic sized swimming pool.\n\n\t-s <size> to set the size of the "
  "generated prime in bits\n\n\t-d to turn off debug info\n\n";

// size = size of src in bytes
void clear(byte *src, int size) {
  for (int i = 0; i < size; i++) {
    src[i] = 0;
  }
}

const char hexarr[16] = {'0', '1', '2', '3', '4', '5', '6',
                         '7', '8', '9', 'a', 'b', 'c', 'd',
                         'e', 'f'};
void print(byte *src, int size) {
  for (int i = size - 1; i >= 0; i--) {
    byte l = (src[i] >> 4) & 0xf;
    byte r = src[i] & 0xf;
    Serial.print(hexarr[l]);
    Serial.print(hexarr[r]);
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
    dest[i] = random(0, 256) & 0xff;
  }
  dest[0] |= 1;
  dest[size - 1] |= 0x80;
  // set another random bit in the least significant byte
  // because right shift only works for shift < 8;
  if (dest[0] == 1) {
    dest[0] |= (1 << random(1, 8));
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
      t = 256 - (src2[i] - src1[i]);
    } else {
      t = src1[i] - src2[i];
    }
    dest[i] = t;
  }
  return;
}

// size of dest must be twice that of src1 and src2
// src1 and src2 must be the same size
void mult(byte *src1, byte *src2, byte *dest, int size) {
  clear(dest, size << 1);
  int idx;
  byte carry, temp;
  for (int i = 0; i < size; i++) {
    for (int j = 0; j < size; j++) {
      temp = dest[i + j];
      word p = src1[i] * src2[j];
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

// assume a is twice as big as n
// a = qn + r for some integer q and some r in [0, n); return r
void mod(byte *a, byte *n, byte *dest, int size) {
  if (compare(a, n, size) < 0 && is_const(&a[size], 0, size)) {
    copy(a, dest, size);
    return;
  }
  byte minus[size << 1];
  clear(minus, size << 1);
  copy(n, &minus[size], size); // minus = n << (1 << size);

  for (int i = 0; i <= (size << 3); i++) {
    if (compare(minus, a, size << 1) < 0) {
      sub(a, minus, a, size << 1); // a -= (n << i)
    }
    rshift(minus, 1, size << 1);
  }
  copy(a, dest, size);
}

// compute a^2 mod n and store in dest
void asqmodn(byte *a, byte *n, byte *dest, int size) {
  byte temp[size << 1];
  clear(temp, size << 1);
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
  copy(a, cur_a, size);

  // binary exponentiation
  for (int i = 0; i < size; i++) {
    for (int j = 0; j < 8; j++) {
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

// p is the int we are testing for primality
int miller_rabin(byte *p, int size) {
  int eqpm1;
  byte a[size];
  byte d[size];
  copy(p, d, size);

  byte lsb = (p[0] & 0xfe);
  byte shift = find_lsb_set(lsb);
  rshift(d, shift, size);

  for (int k = 0; k < NP; k++) {
    set(a, primes[k], size);
    admodn(a, d, p, a, size);

    p[0] &= 0xfe; // little trick to see if a = p - 1 in place
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

void find(byte *dest, int size, int print_stuff) {
  int runs = 0;

  unsigned long start = millis();
  while (++runs) {
    rand_int(dest, size);
    if (print_stuff) {
      Serial.print("Testing: ");
      print(dest, size);
      Serial.print("\n");
    }
    if (miller_rabin(dest, size) == 1) {
      break;
    }
  }
  unsigned long finish = millis();

  if (print_stuff) {
    Serial.print("\n");
  }

  Serial.print("Found probable prime:\n");
  print(dest, size);
  Serial.print("\n");


  if (print_stuff) {
    Serial.print("\ntook ");
    Serial.print(runs, DEC);
    Serial.print("runs in");
    Serial.print(finish - start, DEC);
    Serial.print("ms\n");
  }
}

int help() {
  Serial.print(help_message);
  return 0;
}

// somewhat exhaustive test of the system
void test() {
  // for valgrind, run between 0x30001 and 0x30401
  for (long n = 0x30001; n < 0x30401; n += 2) {
  // about ~0.5 million values: 0x101 - 0x100001
  // for (long n = 0x101; n < 0x100001; n += 2) {
    if ((n & 0xff) == 0x1) { // we can't handle these cases
      Serial.print("skipping ");
      Serial.println(n, dec);
      continue;
    }

    int is_prime = 1;
    for (int d = 3; d * d <= n; d += 2) {
      if (n % d == 0) {
        is_prime = 0;
        break;
      }
    }
    byte test[4] = {(byte)(n & 0xff), (byte)((n >> 8) & 0xff), 
                    (byte)((n >> 16) & 0xff), (byte)((n >> 24) & 0xff)};
    int idx = is_prime + miller_rabin(test, 4);

    if (idx == 0) {
      Serial.print(n);
      Serial.println("composite");
    } else if (idx == 1) {
      Serial.print(n);
      Serial.println("broken");
      break;
    } else {
      Serial.print(n);
      Serial.println("prime");
    }
  }
}

void setup() {
  Serial.begin(9600);
  test();
  randomSeed(micros());
}

void loop() {
  int size = 256; // in bits
  byte test[size >> 3];
  find(test, size, 1);
}