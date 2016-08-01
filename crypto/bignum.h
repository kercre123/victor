#ifndef __BIGNUM_H
#define __BIGNUM_H

#include <stdint.h>

typedef uint16_t  big_num_cell_t;
typedef int16_t   big_num_signed_cell_t;
typedef uint32_t  big_num_double_cell_t;

// 2048-bit safe (RSA Pub)
static const int CELL_SIZE = 257;
static const int CELL_BITS = (sizeof(big_num_cell_t) * 8);

struct big_num_t {
  bool            negative;
  int             used;
  big_num_cell_t  digits[CELL_SIZE];
};

struct big_mont_t {
  int shift;

  big_num_t one;
  big_num_t modulo;
  big_num_t r2;
  big_num_t rinv;
  big_num_t minv;
};

struct big_mont_pow_t {
  big_num_t exp;
  big_num_t base;
  big_num_t result;

  int msb, bit;
  bool negative;
  bool scale;
};

struct big_modulo_t {
  big_num_t modulo;
  big_num_t divisor;
  int shift;
};

// This is used for public decryption
struct big_rsa_t {
  big_num_t modulo;
  big_num_t exp;
};

union big_overflow_t {
  struct {
    big_num_cell_t lower;
    big_num_cell_t upper;
  };

  big_num_double_cell_t word;
};

static const big_num_t BIG_ZERO = {false, 0};
static const big_num_t BIG_ONE = {false, 1, {1}};

// These are in-place safe
void big_init(big_num_t& num);

bool big_zero(const big_num_t& a);
bool big_bit_get(const big_num_t& a, int bit);
bool big_bit_set(const big_num_t& a, int bit);

int big_msb(const big_num_t& a);
int big_lsb(const big_num_t& a);

bool big_shr(big_num_t& out, const big_num_t& a, const int bits);
bool big_shl(big_num_t& out, const big_num_t& a, const int bits);

int big_compare(const big_num_t& a, const big_num_t& b);
bool big_unsigned_add(big_num_t& out, const big_num_t& a, const big_num_t& b);
bool big_unsigned_subtract(big_num_t& out, const big_num_t& a, const big_num_t& b);
bool big_add(big_num_t& out, const big_num_t& a, const big_num_t& b);
bool big_subtract(big_num_t& out, const big_num_t& a, const big_num_t& b);

// Partially inplace, first term may be self modifying
bool big_power(big_num_t& out, const big_num_t& base, const big_num_t& exp);
bool big_modulo(big_num_t& modulo, const big_num_t& a, const big_num_t& b);

// modulo and a may be the same, remaining values are unsafe
bool big_multiply(big_num_t& out, const big_num_t& a, const big_num_t& b);
bool big_divide(big_num_t& result, big_num_t& modulo, const big_num_t& a, const big_num_t& b);

// These may not be safe
bool big_shl(big_num_t& out, const big_num_t& a, int bits);

// Montgomery domain numbers (not in-place)
bool mont_init(big_mont_t& mont, const big_num_t& modulo);
bool mont_to(const big_mont_t& mont, big_num_t& out, const big_num_t& in);
bool mont_from(const big_mont_t& mont, big_num_t& out, const big_num_t& in);
bool mont_multiply(const big_mont_t& mont, big_num_t& out, const big_num_t& a, const big_num_t& b);
bool mont_power(const big_mont_t& mont, big_num_t& out, const big_num_t& base_in, const big_num_t& exp);

// Async montgomery power
void mont_power_async_init(const big_mont_t& mont, big_mont_pow_t& state, const big_num_t& base, const big_num_t& exp);
bool mont_power_async(const big_mont_t& mont, big_mont_pow_t& state);
void big_modulo_async_init(big_modulo_t& mod, const big_num_t& a, const big_num_t& b);
bool big_modulo_async(big_modulo_t& modulo);

#endif
