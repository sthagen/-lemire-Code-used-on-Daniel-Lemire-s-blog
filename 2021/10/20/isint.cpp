#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <sstream>

uint64_t nano() {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

bool is_integer(double x) {
  uint64_t bits = 0;
  ::memcpy(&bits, &x, sizeof(x));
  if ((bits & 0x7fffffffffffffff) == 0) { // We have a zero or -zero.
    return true;
  }
  uint64_t mantissa = (bits & 0xfffffffffffff) | 0x10000000000000;
  int raw_exp = (bits >> 52) & 0x7ff;
  if (raw_exp == 0) {
    return false; // subnormal
  }
  if (raw_exp == 0x7ff) {
    return false; // NaN or infinity
  }
  int exp = raw_exp - 1023 - 52;
  // except for the sign, we
  // have that x = mantissa * 2**exp.
  if (exp >= 0) {
    return true;
  }
  if (exp < -64) {
    return false;
  }
  return (mantissa << (64 + exp)) == 0;
}

/**
 * Returns true if double x can be exactly converted
 * to an uint64_t number.
 * If so, writes the result to out.
 */
bool to_int64_simple(double x, int64_t *out) {
  int64_t tmp = int64_t(x);
  *out = tmp;
  return tmp == x;
}
bool to_int64(double x, int64_t *out) {
  uint64_t bits = 0;
  ::memcpy(&bits, &x, sizeof(x));
  bool negative = (bits >> 63);
  if (bits == 0) { // We have a zero.
    *out = 0;
    return true;
  }
  uint64_t mantissa = (bits & 0xfffffffffffff) | 0x10000000000000;
  int raw_exp = (bits >> 52) & 0x7ff;
  int exp = raw_exp - 1023 - 52;
  if ((exp >= 0) && (exp < 11)) {
    int64_t tmp = int64_t(mantissa << exp);
    if (negative) {
      tmp = -tmp;
    }
    *out = tmp;
    return true;
  }
  if ((exp > -64) && (exp < 0) && ((mantissa << (64 + exp)) == 0)) {
    int64_t tmp = int64_t(mantissa >> -exp);
    if (negative) {
      tmp = -tmp;
    }
    *out = tmp;
    return true;
  }
  return false;
}

bool fast_to_int64(double x, int64_t *out) {
  uint64_t bits = 0;
  ::memcpy(&bits, &x, sizeof(x));
  bool negative = (bits >> 63);
  if (bits == 0) { // We have a zero.
    *out = 0;
    return true;
  }
  uint64_t mantissa = (bits & 0xfffffffffffff) | 0x10000000000000;
  int tz = __builtin_ctzll(mantissa);
  int raw_exp = (bits >> 52) & 0x7ff;
  int exp = raw_exp - 1023 - 52;
  if ((exp >= 11) || (exp < -tz)) {
    return false;
  }
  int64_t tmp;
  if (exp >= 0) {
    tmp = int64_t(mantissa << exp);
  } else {
    tmp = int64_t(mantissa >> -exp);
  }
  if (negative) {
    tmp = -tmp;
  }
  *out = tmp;
  return true;
}

typedef union {
  struct {
    uint64_t fraction : 52;
    uint32_t exp_bias : 11;
    uint32_t sign : 1;
  } fields;
  double value;
} binary64_float;

bool to_int64_mu(double x, int64_t *out) {
  binary64_float number = {.value = x};
  const int shift = number.fields.exp_bias - 1023 - 52;
  if (shift > 0) {
    return false;
  }
  if (shift <= -64) {
    if (x == 0) {
      *out = 0;
      return true;
    }
    return false;
  }
  uint64_t integer = number.fields.fraction | 0x10000000000000;
  if (integer << (64 + shift)) {
    return false;
  }
  integer >>= -shift;
  *out = (number.fields.sign) ? -integer : integer;
  return true;
}

bool is_integer(float x) {
  uint32_t bits = 0;
  ::memcpy(&bits, &x, sizeof(x));
  if ((bits & 0x7fffffff) == 0) { // We have a zero or -zero.
    return true;
  }
  uint32_t mantissa = (bits & 0x7fffff) | 0x800000;
  int raw_exp = (bits >> 23) & 0xff;
  if (raw_exp == 0) {
    return false; // subnormal
  }
  if (raw_exp == 0xff) {
    return false; // NaN or infinity
  }
  int exp = raw_exp - 127 - 23;
  // except for the sign, we
  // have that x = mantissa * 2**exp.
  if (exp >= 0) {
    return true;
  }
  if (exp < -32) {
    return false;
  }
  return (mantissa << (32 + exp)) == 0;
}

/**
 * Returns true if double x can be exactly converted
 * to an uint64_t number.
 * If so, writes the result to out.
 */
bool to_int32(float x, int32_t *out) {
  uint64_t bits = 0;
  ::memcpy(&bits, &x, sizeof(x));
  bool negative = (bits >> 31);
  if (bits == 0) { // We have a zero.
    *out = 0;
    return true;
  }
  uint32_t mantissa = (bits & 0x7fffff) | 0x800000;
  int raw_exp = (bits >> 23) & 0xff;
  int exp = raw_exp - 127 - 23;
  if ((exp >= 0) && (exp < 9)) {
    int32_t tmp = int32_t(mantissa << exp);
    if (negative) {
      tmp = -tmp;
    }
    *out = tmp;
    return true;
  }
  if ((exp > -32) && (exp < 0) && ((mantissa << (32 + exp)) == 0)) {
    int32_t tmp = int32_t(mantissa >> -exp);
    if (negative) {
      tmp = -tmp;
    }
    *out = tmp;
    return true;
  }
  return false;
}
typedef union {
  struct {
    uint32_t fraction : 23;
    uint32_t exp_bias : 8;
    uint32_t sign : 1;
  } fields;
  float value;
} binary32_float;

// credit :
// https://github.com/WojciechMula/toys/blob/master/float2int/float2int.c
bool to_int32_mu(float x, int64_t *out) {
  binary32_float number = {.value = x};
  const int shift = number.fields.exp_bias - 127 - 23;
  if (shift > 0) {
    return false;
  }
  if (shift <= -32) {
    if (x == 0) {
      *out = 0;
      return true;
    }
    return false;
  }
  uint32_t integer = number.fields.fraction | 0x800000;
  if (integer << (32 + shift)) {
    return false;
  }
  integer >>= -shift;
  *out = (number.fields.sign) ? -integer : integer;
  return true;
}

void check(bool x) {
  if (!x) {
    throw std::runtime_error("bug");
  }
}

void test() {
  int64_t tmp;
  check(to_int64(0.0, &tmp) && (tmp == 0));
  check(to_int64(1.0, &tmp) && (tmp == 1));
  check(to_int64(-1.0, &tmp) && (tmp == -1));
  check(to_int64(9007199254740991.0, &tmp) && (tmp == 9007199254740991));
  check(to_int64(-9007199254740991.0, &tmp) && (tmp == -9007199254740991));
  check(to_int64_mu(0.0, &tmp) && (tmp == 0));
  check(to_int64_mu(1.0, &tmp) && (tmp == 1));
  check(to_int64_mu(-1.0, &tmp) && (tmp == -1));
  check(to_int64_mu(9007199254740991.0, &tmp) && (tmp == 9007199254740991));
  check(to_int64_mu(-9007199254740991.0, &tmp) && (tmp == -9007199254740991));

  int32_t tmp32;
  check(to_int32(0.0f, &tmp32) && (tmp32 == 0));
  check(to_int32(1.0f, &tmp32) && (tmp32 == 1));
  check(to_int32(-1.0f, &tmp32) && (tmp32 == -1));
  check(to_int32(16777215.0f, &tmp32) && (tmp32 == 16777215));
  check(to_int32(-16777215.0f, &tmp32) && (tmp32 == -16777215));

  check(is_integer(0.0));
  check(is_integer(-0.0));
  check(is_integer(0.0f));
  check(is_integer(-0.0f));

  check(!is_integer(0.5));
  check(!is_integer(0.5f));
  check(!is_integer(-0.5));
  check(!is_integer(-0.5f));

  for (uint64_t x = 1; x < 9007199254740992; x *= 2) {
    check(is_integer(double(x)));
    check(is_integer(float(x)));

    if (x < 4503599627370496) {
      check(!is_integer(double(x) + 0.5));
    }
    if (x < 8388608) {
      check(!is_integer(float(x) + 0.5f));
    }
  }
}

int main() {
  test();

  for (size_t trial = 0; trial < 3; trial++) {
    std::cout << std::endl;
    std::cout << "to_int64        ";
    std::cout.flush();
    uint64_t start = nano();
    uint64_t sum = 0;
    uint64_t counter = 0;
    for (double x = 1; x < 1000000000; x += 0.5) {
      counter++;
      int64_t tmp;
      if (to_int64(x, &tmp)) {
        sum += tmp;
      }
    }
    uint64_t finish = nano();
    check(499999999500000000 == sum);
    std::cout << double(finish - start) / counter << std::endl;

    std::cout << "fast_to_int64mu ";

    std::cout.flush();
    start = nano();
    sum = 0;
    counter = 0;
    for (double x = 1; x < 1000000000; x += 0.5) {
      counter++;
      int64_t tmp;
      if (to_int64_mu(x, &tmp)) {
        sum += tmp;
      }
    }
    finish = nano();
    check(499999999500000000 == sum);
    std::cout << double(finish - start) / counter << std::endl;

    std::cout << "fast_to_int64   ";

    std::cout.flush();
    start = nano();
    sum = 0;
    counter = 0;
    for (double x = 1; x < 1000000000; x += 0.5) {
      counter++;
      int64_t tmp;
      if (fast_to_int64(x, &tmp)) {
        sum += tmp;
      }
    }
    finish = nano();
    check(499999999500000000 == sum);
    std::cout << double(finish - start) / counter << std::endl;

    std::cout << "to_int64_simple ";
    std::cout.flush();

    start = nano();
    sum = 0;
    counter = 0;
    for (double x = 1; x < 1000000000; x += 0.5) {
      counter++;
      int64_t tmp;
      if (to_int64_simple(x, &tmp)) {
        sum += tmp;
      }
    }
    finish = nano();
    check(499999999500000000 == sum);
    std::cout << double(finish - start) / counter << std::endl;
  }
  return EXIT_SUCCESS;
}