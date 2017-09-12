#include <stdint.h>
#include <string.h>

#include "bignum.h"
#include "stm32f0xx.h"

void big_init(big_num_t& num) {
  num.negative = false;
  num.used = 0;
}

// Cheapo function for reducing used count
static void bit_reduce(big_num_t& num) {
  while (num.used > 0 && num.digits[num.used-1] == 0) {
    num.used--;
  }
}

// POS : A < B
// NEG : A > B
//  0  : A = B
int big_unsigned_compare(const big_num_t& a, const big_num_t& b) {
  // Zero values
  if (a.used == 0 && b.used == 0) {
    return 0;
  }

  // Check if one is significantly larger
  int diff = a.used - b.used;
  if (diff) {
    return diff;
  }

  // Find word that is different
  for (int i = a.used - 1; i >= 0; i--) {
    int diff = a.digits[i] - b.digits[i];

    if (diff) {
      return diff;
    }
  }

  return 0;
}

int big_compare(const big_num_t& a, const big_num_t& b) {
  // Zero values
  if (a.used == 0 && b.used == 0) {
    return 0;
  }

  // Signs are different
  if (a.negative != b.negative) {
    return a.negative ? -1 : 1;
  }

  int diff = big_unsigned_compare(a, b);
  return a.negative ? -diff : diff;
}


bool big_zero(const big_num_t& a) {
  return a.used == 0;
}

static bool big_odd(const big_num_t& a) {
  return big_bit_get(a, 0);
}

static bool big_mask(big_num_t& t, int bits) {
  int index = bits / CELL_BITS;
  big_num_cell_t mask = (1 << (bits % CELL_BITS)) - 1;

  if (t.used > index) {
    t.used = index + 1;
  }

  t.digits[index] &= mask;

  return false;
}

bool big_bit_get(const big_num_t& a, int bit) {
  int offset = bit / CELL_BITS;
  int shift = bit % CELL_BITS;

  if (offset >= a.used) return false;

  return (a.digits[offset] & (1 << shift)) != 0;
}

bool big_bit_set(big_num_t& a, int bit) {
  int offset = bit / CELL_BITS;
  int shift = bit % CELL_BITS;

  if (offset >= CELL_SIZE) {
    return true;
  }

  while (a.used <= offset) {
    a.digits[a.used++] = 0;
  }

  a.digits[offset] |= (1 << shift);
  return false;
}

int big_msb(const big_num_t& a) {
  if (a.used == 0) {
    return -1;
  }
  
  const big_num_cell_t data = a.digits[a.used - 1];
  
  for (int i = CELL_BITS - 1; i >= 0; i--) {
    if (data & (1 << i)) {
      return (a.used - 1) * CELL_BITS + i;
    }
  }

  return -1;
}

int big_lsb(const big_num_t& a) {
  for (int idx = 0; idx < a.used; idx++) {
    const big_num_cell_t data = a.digits[idx];

    if (data == 0) {
      continue ;
    }

    for (int i = 0; i < CELL_BITS; i++) {
      if (data & (1 << i)) {
        return idx * CELL_BITS + i;
      }
    }
  }

  return -1;
}

bool big_shr(big_num_t& out, const big_num_t& a, const int bits) {
  big_overflow_t carry;

  int cells = a.used;
  int offset = bits / CELL_BITS;
  int shift = bits % CELL_BITS;
  int index = 0;

  out.used = a.used - offset;

  while (offset < cells) {
    carry.lower = a.digits[offset++];
    carry.upper = (offset >= cells) ? 0 : a.digits[offset];
    out.digits[index++] = carry.word >> shift;
  }

  bit_reduce(out);
  out.negative = a.negative;

  return false;
}

bool big_shl(big_num_t& out, const big_num_t& a, const int bits) {
  big_overflow_t carry;

  // Bail on life
  if (bits == 0) {
    memcpy(&out, &a, sizeof(big_num_t));
    return false;
  }

  int offset = bits / CELL_BITS;
  int shift = bits % CELL_BITS;
  int write = a.used + offset;
  int index = a.used;

  out.used = write + 1;

  // Prevent overflow
  if (out.used > CELL_SIZE) {
    return true;
  }

  memset(&out.digits, 0, offset * sizeof(big_num_cell_t));

  carry.word = 0;
  while (index >= 0) {
    carry.upper = carry.lower;
    carry.lower = (index-- >= 0) ? a.digits[index] : 0;

    out.digits[write--] = carry.word >> (CELL_BITS - shift);
  }

  bit_reduce(out);
  out.negative = a.negative;

  return false;
}

bool big_unsigned_add(big_num_t& out, const big_num_t& a, const big_num_t& b) {
  big_overflow_t carry;

  const big_num_t* bottom;
  const big_num_t* top;
  int idx;

  if (a.used < b.used) {
    bottom = &a;
    top = &b;
  } else {
    bottom = &b;
    top = &a;
  }
  
  carry.word = 0;

  // Lower section summation
  for (idx = 0; idx < bottom->used; idx++) {
    carry.word = bottom->digits[idx] + top->digits[idx] + carry.upper;
    out.digits[idx] = carry.lower;
  }
  
  // Carry through
  for (; idx < top->used; idx++) {
    carry.word = top->digits[idx] + carry.upper;
    out.digits[idx] = carry.lower;
  }

  // Bit expansion
  if (carry.upper) {
    // Overflow error
    if (idx+1 >= CELL_SIZE) {
      return true;
    }

    out.digits[idx++] = carry.upper;
  }

  out.used = idx;

  return false;
}

bool big_unsigned_subtract(big_num_t& out, const big_num_t& a, const big_num_t& b) {
  union {
    struct {
      big_num_cell_t lower;
      big_num_signed_cell_t upper;
    } parts;

    big_num_double_cell_t word;
  };

  int idx;

  // Underflow certain
  if (a.used < b.used) {
    return true;
  }
  
  word = 0;

  // Lower section summation
  for (idx = 0; idx < b.used; idx++) {
    word = a.digits[idx] - b.digits[idx] + parts.upper;
    out.digits[idx] = parts.lower;
  }
  
  // Carry through
  for (; idx < a.used; idx++) {
    word = a.digits[idx] + parts.upper;
    out.digits[idx] = parts.lower;
  }

  // underflow
  if (parts.upper) {
    return true;
  }

  out.used = idx;
  bit_reduce(out);

  return false;
}

static bool big_add_sub(big_num_t& out, const big_num_t& a, const big_num_t& b, bool b_sign) {
  bool result;

  if (a.negative == b_sign) {
    result = big_unsigned_add(out, a, b);
    out.negative = a.negative;
  }
  else if (big_unsigned_compare(a, b) > 0) {
    result = big_unsigned_subtract(out, a, b);
    out.negative = a.negative;
  }
  else {
    result = big_unsigned_subtract(out, b, a);
    out.negative = !a.negative;
  }

  return result;
}

bool big_add(big_num_t& out, const big_num_t& a, const big_num_t& b) {
  return big_add_sub(out, a, b, b.negative);
}

bool big_subtract(big_num_t& out, const big_num_t& a, const big_num_t& b) {
  return big_add_sub(out, a, b, !b.negative);
}

bool big_multiply(big_num_t& out, const big_num_t& a, const big_num_t& b) {
  big_overflow_t carry;

  int amax = a.used - 1;
  int bmax = b.used - 1;
  bool clipped = false;

  // Clear our the section of the output that might be uninitalized
  for (int i = a.used; i < CELL_SIZE; i++) {
    out.digits[i]  = 0;
  }

  out.negative = a.negative != b.negative;
  out.used = 0;

  for (int ai = amax; ai >= 0; ai--) {
    int a_value = a.digits[ai];
    out.digits[ai] = 0; // This is done here so we can use inplace multiplication

    for (int bi = bmax; bi >= 0; bi--) {
      int write_index = ai + bi;

      // Multiply base terms and hope it's only 16-bit
      carry.word = a_value * b.digits[bi] + out.digits[write_index];

      // Zero result early abort
      if (carry.word == 0) {
        continue ;
      }

      // Cannot write outside of boundary
      if (write_index >= CELL_SIZE) {
        clipped = true;
      }

      out.digits[write_index++] = carry.lower;

      // Start carry through chain
      while (carry.upper) {
        // Overflow check
        if (write_index >= CELL_SIZE) {
          clipped = true;
        }

        carry.word = carry.upper + out.digits[write_index];
        out.digits[write_index++] = carry.lower;
      }

      // Set the new used count
      if (write_index > out.used) {
        out.used = write_index;
      }
    }
  }

  return clipped;
}

bool big_power(big_num_t& result, const big_num_t& base_in, const big_num_t& exp) {
  big_num_t working[2];
  big_num_t *base = &working[0];
  big_num_t *temp = &working[1];

  int msb = big_msb(exp);

  result = BIG_ONE;

  *base = base_in;

  for (int bit = 0; bit <= msb; bit++) {    
    if (big_bit_get(exp, bit)) {
      if (big_multiply(result, result, *base)) {
        return true;
      }
    }

    if (big_multiply(*temp, *base, *base)) {
      return true;
    }

    {
      big_num_t* swap = base;
      base = temp;
      temp = swap;
    }
  }

  result.negative = big_odd(base_in) ? base_in.negative : false;

  return false;
}

bool big_divide(big_num_t& result, big_num_t& modulo, const big_num_t& a, const big_num_t& b) {
  big_num_t divisor;

  // Zero out result, but say it's size is that of our divisor (maximum size)
  memset(&result, 0, sizeof(big_num_t));
  result.used = a.used;
  result.negative = a.negative != b.negative;

  modulo = a;

  int shift = big_msb(a) - big_msb(b);

  big_shl(divisor, b, shift);

  for (; shift >= 0; shift--) {
    if (big_unsigned_compare(modulo, divisor) >= 0) {
      big_unsigned_subtract(modulo, modulo, divisor);
      big_bit_set(result, shift);
    }
    
    big_shr(divisor, divisor, 1);
  }

  bit_reduce(result);
  bit_reduce(modulo);

  return false;
}

bool big_modulo(big_num_t& modulo, const big_num_t& a, const big_num_t& b) {
  big_num_t divisor;

  modulo = a;

  int shift = big_msb(a) - big_msb(b);

  if (shift < 0) {
    return false;
  }

  big_shl(divisor, b, shift);

  while (shift-- >= 0) {
    if (big_unsigned_compare(modulo, divisor) >= 0) {
      big_unsigned_subtract(modulo, modulo, divisor);
    }

    big_shr(divisor, divisor, 1);
  }

  bit_reduce(modulo);

  return false;
}

// Binary EEA
bool big_invm(big_num_t& out, const big_num_t& a_, const big_num_t& b_) {
  big_num_t a;
  big_num_t b = b_;
  big_num_t x1 = BIG_ONE;
  big_num_t x2 = BIG_ZERO;

  if (a_.negative) {
    big_modulo(a, a_, b_);
  } else {
    a = a_;
  }

  while (big_compare(a, BIG_ONE) > 0 && big_compare(b, BIG_ONE) > 0) {
    int a_lsb = big_lsb(a);

    if (a_lsb > 0) {
      big_shr(a, a, a_lsb);

      while (a_lsb-- > 0) {
        if (big_odd(x1)) {
          if (big_add(x1, x1, b_)) {
            return true;
          }
        }
        big_shr(x1, x1, 1);
      }
    }

    int b_lsb = big_lsb(b);

    if (b_lsb > 0) {
      big_shr(b, b, b_lsb);

      while (b_lsb-- > 0) {
        if (big_odd(x2)) {
          if (big_add(x2, x2, b_)) {
            return true;
          }
        }

        big_shr(x2, x2, 1);
      }
    }

    if (big_compare(a, b) >= 0) {
      if (big_subtract(a, a, b)) {
        return true;
      }
      if (big_subtract(x1, x1, x2)) {
        return true;
      }
    }
    else {
      if (big_subtract(b, b, a)) {
        return true;
      }
      if (big_subtract(x2, x2, x1)) {
        return true;
      }
    }
  }

  if (big_compare(a, BIG_ONE) == 0) {
    out = x1;
  } else {
    out = x2;
  }

  if (big_compare(out, BIG_ZERO) < 0) {
    if (big_add(out, out, b)) {
      return true;
    }
  }

  return false;
}

/*
 MONTGOMERY DOMAIN FUNCTIONS
*/

// Convert to and from montgomery domain
bool mont_to(const big_mont_t& mont, big_num_t& out, const big_num_t& in) {
  if (big_shl(out, in, mont.shift)) {
    return true;
  }

  big_modulo(out, out, AS_BN(mont.modulo));

  return false;
}

bool mont_from(const big_mont_t& mont, big_num_t& out, const big_num_t& in) {
  if (big_multiply(out, in, AS_BN(mont.rinv))) {
    return true;
  }

  big_modulo(out, out, AS_BN(mont.modulo));

  return false;
}

// TODO: overflow protection
bool mont_multiply(const big_mont_t& mont, big_num_t& out, const big_num_t& a, const big_num_t& b) {
  big_num_t temp;

  big_multiply(out, a, b);
  temp = out;
  big_mask(temp, mont.shift);
  big_multiply(temp, temp, AS_BN(mont.minv));
  big_mask(temp, mont.shift);
  big_multiply(temp, temp, AS_BN(mont.modulo));

  big_subtract(out, out, temp);
  big_shr(out, out, mont.shift);

  if (big_compare(out, AS_BN(mont.modulo)) >= 0) {
    big_subtract(out, out, AS_BN(mont.modulo));
  } else if (big_compare(out, BIG_ZERO) < 0) {
    big_add(out, out, AS_BN(mont.modulo));
  }

  return false;
}

bool mont_power(const big_mont_t& mont, big_num_t& out, const big_num_t& base_in, const big_num_t& exp) {
  big_num_t working[3];
  big_num_t *base = &working[0];
  big_num_t *temp = &working[1];
  big_num_t *result = &working[2];

  int msb = big_msb(exp);
  
  *base = base_in;
  *result = AS_BN(mont.one);

  for (int bit = 0; bit <= msb; bit++) {
    if (big_bit_get(exp, bit)) {
      if (mont_multiply(mont, *temp, *result, *base)) {
        return true;
      }

      {
        big_num_t *x = result;
        result = temp;
        temp = x;
      }
    }


    if (mont_multiply(mont, *temp, *base, *base)) {
      return true;
    }

    {
      big_num_t *x = base;
      base = temp;
      temp = x;
    }
  }

  memcpy(&out, result, sizeof(big_num_t));
  out.negative = big_odd(base_in) ? base_in.negative : false;

  return false;
}
