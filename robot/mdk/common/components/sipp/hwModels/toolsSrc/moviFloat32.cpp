// ***************************************************************************
// Copyright(C)2011 Movidius Ltd. All rights reserved
// ---------------------------------------------------------------------------
// File       : mvfloat32.cpp
// Description: IEEE 754 floating point implementation
// ---------------------------------------------------------------------------
// HISTORY
// Version        | Date       | Owner         | Purpose
// ---------------------------------------------------------------------------
// 0.1            | 12.04.2011 | Alin Dobre    | Initial version
// ***************************************************************************
#include "moviFloat32.h"
#include <stdio.h>

unsigned int detect_tiny_mode = F32_DETECT_TINY_AFTER_RND;
static const unsigned int leading_zeros[] = {4, 3, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0};

inline T_F64 PACK_F64(unsigned int sign, int exp, unsigned int high, unsigned int low)
{
    T_F64 result;
    result.low = low;
    result.high = ((sign << 31) + ((unsigned int)exp << 20) + high);
    return result;
}

inline void f32_normalize_components(int* exp, unsigned int* frac)
{
    int count;
    unsigned int temp = *frac;
    // find out the number of leading zeros in the fraction
    count = 0;
    if (temp < 0x10000)
    {
        count += 16;
        temp <<= 16;
    }
    if (temp < 0x1000000)
    {
        count += 8;
        temp <<= 8;
    }
    if (temp < 0x10000000)
    {
        count += 4;
        temp <<= 4;
    }
    count += leading_zeros[temp >> 28];
    // normalize number
    count--;
    if (count < 0)
    {
        //AlDo: do nothing?
        *frac = 0;
    }
    else
    {
        *frac = *frac << count;
    }
    //is count greater than exponent?
    if (count <= *exp)
    {
        *exp = (*exp - count) & 0xFF;
    }
    else
    {
        //AlDo: Do we need to zero out the fraction as well?
        *exp = 0;
        *frac = 0;
    }
}

inline void f32_normalize_subnormal(unsigned int* frac, int* exp)
{
    int count = 0;
    unsigned int temp = *frac;
    // find out the number of leading zeros in the fraction
    if (temp < 0x10000)
    {
        count += 16;
        temp <<= 16;
    }
    if (temp < 0x1000000)
    {
        count += 8;
        temp <<= 8;
    }
    if (temp < 0x10000000)
    {
        count += 4;
        temp <<= 4;
    }
    count += leading_zeros[temp >> 28];
    // normalize number - ignore first 8 zeros (1-sign, 8-exponent, but add 1 bit to mantissa, so 1+8-1 = 8 to ignore)
    count = count - 8;
    if ((count & 0xFFFFFFE0) != 0)
    {
        // AlDo: Can this ever happen?
        *frac = 0;
    }
    else
    {
        *frac = *frac << count;
    }
    *exp = 1 - count; // exp = (0 - BIAS + 1 - leading zeros)
}

inline void f16_normalize_subnormal(unsigned int* frac, int* exp)
{
    int count = 0;
    unsigned int temp = *frac << 16;
    // find out the number of leading zeros in the fraction
    if (temp < 0x1000000)
    {
        count += 8;
        temp <<= 8;
    }
    if (temp < 0x10000000)
    {
        count += 4;
        temp <<= 4;
    }
    count += leading_zeros[temp >> 28];
    // normalize number - ignore first 5 zeros, as the exponent size is 5
    count = count - 5;
    if ((count & 0xFFFFFFF0) != 0)
    {
        // AlDo: Do nothing?
        *frac = 0;
    }
    else
    {
        *frac = *frac << count;
    }
    *exp = 1 - count; // exp = (0 - BIAS + 1 - leading zeros)
}

unsigned int f32_shift_right_loss_detect(unsigned int op, unsigned int cnt)
{
    unsigned int ret_val;
    if (cnt == 0)
    {
        ret_val = op;
    }
    else if (cnt < 32) 
    {
        ret_val = op >> cnt;
        // mark LSB as 1 if we shifted out some ones 
        if ((op & ((0x1 << cnt) - 1)) != 0)
        {
            ret_val |= 0x1;
        }
    }
    else 
    {
        // mark LSB as 1 if we shifted out some ones
        ret_val = (op != 0) ? 1 : 0;
    }
    return ret_val;
}

unsigned int f16_shift_right_loss_detect(unsigned int op, unsigned int cnt)
{
    unsigned int ret_val;
    if (cnt == 0)
    {
        ret_val = op;
    }
    else if (cnt < 16) 
    {
        ret_val = op >> cnt;
        // mark LSB as 1 if we shifted out some ones 
        if ((op & ((0x1 << cnt) - 1)) != 0)
        {
            ret_val |= 0x1;
        }
    }
    else 
    {
        // mark LSB as 1 if we shifted out some ones
        ret_val = (op != 0) ? 1 : 0;
    }
    return ret_val;
}

unsigned int f32_shift_right(unsigned int op, unsigned int cnt)
{
    unsigned int result;
    if (cnt == 0)
    {
        result = op;
    }
    else if (cnt < 32)
    {
        result = (op >> cnt);
    }
    else 
    {
        result = 0;
    }
    return result;
}

unsigned int f16_shift_left(unsigned int op, unsigned int cnt)
{
    unsigned int result;
    if (cnt == 0)
    {
        result = op;
    }
    else if (cnt < 32)
    {
        result = (op << cnt);
    }
    else 
    {
        result = 0;
    }
    return result;
}

inline void f64_shift_right_loss_detect(unsigned int high, unsigned int low, unsigned int cnt, unsigned int* out_high, unsigned int* out_low)
{
    unsigned int res_0, res_1;
    unsigned int negative_cnt = (((~cnt) + 1) & 31);

    if (cnt == 0)
    {
        res_1 = high;
        res_0 = low;
    }
    else if (cnt < 32)
    {
        res_0 = (high << negative_cnt) | (low >> cnt);
        if ((high << negative_cnt) != 0)
        {
            res_0 |= 0x1;
        }
        res_1 = high >> cnt;
    }
    else 
    {
        if (cnt == 32)
        {
            res_0 = high | (low != 0);
        }
        else if (cnt < 64)
        {
            res_0 = (high >> (cnt & 31));
            if (((high << negative_cnt) | low) != 0)
            {
                res_0 |= 0x1;
            }
        }
        else 
        {
            if ((low | high) != 0)
            {
                res_0 = 0x1;
            }
            else
            {
                res_0 = 0;
            }
        }
        res_1 = 0;
    }
    *out_high = res_1;
    *out_low = res_0;
}

inline void f64_shift_right(unsigned int high, unsigned int low, int cnt, unsigned int* out_high, unsigned int* out_low)
{
    int neg_cnt = ((~cnt) + 1) & 31;
    if (cnt == 0)
    {
        *out_low = low;
        *out_high = high;
    }
    else if (cnt < 32)
    {
        *out_low = ((high << neg_cnt) | (low >> cnt));
        *out_high = (high >> cnt);
    }
    else 
    {
        if (cnt < 64)
        {
            *out_low = (high >> (cnt & 31));
        }
        else
        {
            *out_low = 0;
        }
        *out_high = 0;
    }
}

unsigned int f32_pack_round(unsigned int sign, int exp, unsigned int frac, unsigned int rnd_mode, unsigned int* exceptions)
{
    unsigned int ret_val;
    int ret_val_exp = exp;
    unsigned int ret_val_frac = frac;
    bool rnd_nearest_even;
    int rnd_inc, rnd_bits;
    bool is_tiny;

    rnd_nearest_even = (rnd_mode == F32_RND_NEAREST_EVEN);
    // prepare rounding increment
    rnd_inc = 0x40;
    if (!rnd_nearest_even) 
    {
        if (rnd_mode == F32_RND_TO_ZERO) 
        {
            rnd_inc = 0;
        }
        else 
        {
            rnd_inc = 0x7F;
            if (sign) 
            {
                if (rnd_mode == F32_RND_PLUS_INF)
                {
                    rnd_inc = 0;
                }
            }
            else 
            {
                if (rnd_mode == F32_RND_MINUS_INF) 
                {
                    rnd_inc = 0;
                }
            }
        }
    }
    // do rounding and set exceptions
    rnd_bits = frac & 0x7F;
    if ((exp > 0xFD) || ((exp == 0xFD) && ((int)(frac + rnd_inc) < 0))) 
    {
        *exceptions |= (F32_EX_OVERFLOW | F32_EX_INEXACT);
        if (rnd_inc == 0)
        {
            ret_val = PACK_F32(sign, 0xFF, 0) - 1;
        }
        else
        {
            //infinity
            ret_val = PACK_F32(sign, 0xFF, 0);
        }
    }
    else
    {
        if (0xFD <= (unsigned short int)exp)
        {
            if (exp < 0)
            {
                is_tiny = (detect_tiny_mode == F32_DETECT_TINY_BEFORE_RND) || 
                    (exp < -1) || ((frac + rnd_inc) < 0x80000000);
                ret_val_frac = f32_shift_right_loss_detect (frac, (~exp) + 1);
                ret_val_exp = 0;
                rnd_bits = ret_val_frac & 0x7F;
                if (is_tiny && rnd_bits)
                {
                    *exceptions |= F32_EX_UNDERFLOW;
                }
            }
        }
        if (rnd_bits)
        {
            *exceptions |= F32_EX_INEXACT;
        }
        ret_val_frac = (ret_val_frac + rnd_inc) >> 7;
        if ((rnd_bits ^ 0x40) == 0)
        {
            if (rnd_nearest_even)
            {
                ret_val_frac &= (~0x1);
            }
        }
        if (ret_val_frac == 0) 
        {
            ret_val_exp = 0;
        }
        // pack result
        ret_val = PACK_F32(sign, ret_val_exp, ret_val_frac);
    }
    return ret_val;
}

unsigned int f16_pack_round(unsigned int sign, int exp, unsigned int frac, unsigned int rnd_mode, unsigned int* exceptions)
{
    unsigned int ret_val;
    int ret_val_exp = exp;
    unsigned int ret_val_frac = frac;
    bool rnd_nearest_even;
    int rnd_inc, rnd_bits;
    bool is_tiny;

    rnd_nearest_even = (rnd_mode == F32_RND_NEAREST_EVEN);
    // prepare rounding increment
    rnd_inc = 0x08;
    if (!rnd_nearest_even) 
    {
        if (rnd_mode == F32_RND_TO_ZERO) 
        {
            rnd_inc = 0;
        }
        else 
        {
            rnd_inc = 0xF;
            if (sign) 
            {
                if (rnd_mode == F32_RND_PLUS_INF)
                {
                    rnd_inc = 0;
                }
            }
            else 
            {
                if (rnd_mode == F32_RND_MINUS_INF) 
                {
                    rnd_inc = 0;
                }
            }
        }
    }
    // do rounding and set exceptions
    rnd_bits = frac & 0xF;
    if ((exp > 0x1D) || ((exp == 0x1D) && ((int)(frac + rnd_inc) < 0))) 
    {
        *exceptions |= (F32_EX_OVERFLOW | F32_EX_INEXACT);
        if (rnd_inc == 0)
        {
            ret_val = PACK_F16(sign, 0x1F, 0) - 1;
        }
        else
        {
            ret_val = PACK_F16(sign, 0x1F, 0);
        }
    }
    else
    {
        if (0x1D <= (unsigned short int)exp) 
        {
            if (exp < 0) 
            {
                is_tiny = (detect_tiny_mode == F32_DETECT_TINY_BEFORE_RND) || (exp < -1) || ((frac + rnd_inc) < 0x8000);
                ret_val_frac = f32_shift_right_loss_detect (frac, (~exp) + 1);
                ret_val_exp = 0;
                rnd_bits = ret_val_frac & 0x1F;
                if (is_tiny && rnd_bits)
                {
                    *exceptions |= F32_EX_UNDERFLOW;
                }
            }
        }
        if (rnd_bits)
        {
            *exceptions |= F32_EX_INEXACT;
        }
        ret_val_frac = (ret_val_frac + rnd_inc) >> 4;
        if ((rnd_bits ^ 0x10) == 0)
        {
            if (rnd_nearest_even)
            {
                ret_val_frac &= (~0x1);
            }
        }
        if (ret_val_frac == 0) 
        {
            ret_val_exp = 0;
        }
        // pack result
        ret_val = PACK_F16(sign, ret_val_exp, ret_val_frac);
    }
    return ret_val;
}

inline void unsigned64_shift_left(unsigned int op1_l, unsigned int op1_h, unsigned int cnt, unsigned int* res_l, unsigned int* res_h)
{
    if (cnt > 32)
    {
        cnt = 32;
    }
    *res_l = op1_l << cnt;
    if (cnt == 0)
    {
        *res_h = op1_h;
    }
    else
    {
        *res_h = (op1_h << cnt) | (op1_l >> (32 - cnt));
    }
}

unsigned int f32_propagate_NaNs(unsigned int op1, unsigned int op2, unsigned int* exceptions)
{
    unsigned int ret_val;
    // find out which operand is the NaN and what kind of NaN
    bool op1_NaN = F32_IS_NAN(op1);
    bool op2_NaN = F32_IS_NAN(op2);
    bool op1_SNaN = F32_IS_SNAN(op1);
    bool op2_SNaN = F32_IS_SNAN(op2);

    // reset the signalling bit
    RESET_SNAN_BIT(op1);
    RESET_SNAN_BIT(op2);
    // raise exception for signalling NaNs
    if (op1_SNaN || op2_SNaN)
    {
        *exceptions |= F32_EX_INVALID;
    }
    if (op1_SNaN)
    {
        // op1 is a signalling NaN
        if (op2_SNaN)
        {
            // op2 is a sNaN
            if ((op1 & 0x3FFFFF) > (op2 & 0x3FFFFF))
            {
                // op1 fraction is larger
                ret_val = op1; 
            }
            else
            {
                // op2 fraction is larger or equal
                ret_val = op2;
            }
        }
        else if (op2_NaN)
        {
            // op2 is a NaN
            ret_val = op2;
        }
        else
        {
            // op2 is not a NaN
            ret_val = op1;
        }
    }
    else if (op1_NaN)
    {
        // op1 is a NaN
        if (op2_SNaN || (!op2_NaN))
        {
            // op2 is a NaN
            ret_val = op1;
        }
        else 
        {
            // op2 is a NaN
            if ((op1 & 0x3FFFFF) > (op2 & 0x3FFFFF))
            {
                // op1 fraction is larger
                ret_val = op1; 
            }
            else
            {
                // op2 fraction is larger or equal
                ret_val = op2;
            }
        }  
    }
    else
    {
        // op1 is not a NaN or sNaN so op2 must be one
        ret_val = op2;
    }
    return ret_val;
}

unsigned int round_f32_to_i32(unsigned int x, unsigned int rnd_mode, unsigned int* exceptions)
{
    unsigned int sign = EXTRACT_F32_SIGN(x);
    int exp = EXTRACT_F32_EXP(x);
    unsigned int frac = EXTRACT_F32_FRAC(x);
    unsigned int last_bit_mask = 0x1;
    unsigned int rnd_bits_mask;
    unsigned int result;
    if (exp >= 0x96)
    {
        if ((exp == 0xFF) && (frac != 0))
        {
            result = f32_propagate_NaNs(x, x, exceptions);
        }
        else
        {
            result  = x;
        }
    }
    else if (exp <= 0x7E)
    {
        if ((x << 1) == 0)
        {
            result = x;
        }
        else
        {
            *exceptions |= F32_EX_INEXACT;
            switch (rnd_mode)
            {
                case F32_RND_NEAREST_EVEN:
                    if ((exp == 0x7E) && (frac != 0))
                    {
                        result = PACK_F32(sign, 0x7F, 0);
                    }
                    else
                    {
                        result = PACK_F32(sign, 0, 0);
                    }
                    break;
                case F32_RND_MINUS_INF:
                    result = (sign != 0) ? 0xBF800000 : 0;
                    break;
                case F32_RND_PLUS_INF:
                    result = (sign != 0) ? 0x80000000 : 0x3F800000;
                    break;
                default:
                    result = PACK_F32(sign, 0, 0);
                    break;
            }
        }
    }
    else
    {
        last_bit_mask <<= (0x96 - exp);
        rnd_bits_mask = last_bit_mask - 1;
        result = x;
        if (rnd_mode == F32_RND_NEAREST_EVEN)
        {
            result += (last_bit_mask >> 1);
            if ((result & rnd_bits_mask) == 0)
            {
                result &= (~last_bit_mask);
            }
        }
        else if (rnd_mode != F32_RND_TO_ZERO)
        {
            if (((sign == 0) && (rnd_mode == F32_RND_PLUS_INF)) || ((sign == 1) && (rnd_mode == F32_RND_MINUS_INF)))
            {
                result += rnd_bits_mask;
            }
        }
        result &= (~rnd_bits_mask);
        if (result != x)
        {
            *exceptions |= F32_EX_INEXACT;
        }
    }
    return result;
}

//Conversions
unsigned int u32_to_f32_conv(int x, unsigned int rnd_mode, unsigned int* exceptions)
{
    unsigned int result;
    int exp = 0x9C;
    unsigned int frac = (unsigned int)x;
    if (x == 0)
    {
        result = 0;
    }
    else
    {
        f32_normalize_components(&exp, &frac);
        result = f32_pack_round(0, exp, frac, rnd_mode, exceptions);
    }
    return result;
}

unsigned int i32_to_f32_conv(int x, unsigned int rnd_mode, unsigned int* exceptions)
{
    unsigned int sign;
    int exp = 0x9C;
    unsigned int frac;
    unsigned int result;
    if (x == 0)
    {
        result = 0;
    }
    else if (x == (int) 0x80000000)
    {
        result = PACK_F32((unsigned int)1, (unsigned int)0x9E, (unsigned int)0);
    }
    else
    {
        sign = (x < 0) ? 1 : 0;
        frac = (sign == 0) ? x : (-x);
        f32_normalize_components(&exp, &frac);
        result = f32_pack_round(sign, exp, frac, rnd_mode, exceptions);
    }
    return result;
}

int f32_to_i32_conv(unsigned int x, unsigned int rnd_mode, unsigned int* exceptions)
{
    unsigned int sign;
    int exp;
    int cnt;
    unsigned int frac, f;
    int result;

    sign = EXTRACT_F32_SIGN(x);
    exp = EXTRACT_F32_EXP(x);
    frac = EXTRACT_F32_FRAC(x);
    cnt = exp - 0x96;
    if (cnt >= 0)
    {
        if (exp >= 0x9E)
        {
            if (x != 0xCF000000)
            {
                *exceptions |= F32_EX_INVALID;
                result = (sign==0) ? (int)0x7FFFFFFF : (int)0x80000000;
            }
            else
            {
                result = (int) 0x80000000;
            }
        }
        else
        {
            result = (frac | 0x00800000) << cnt;
            if (sign != 0)
            {
                result = (-result);
            }
        }
    }
    else 
    {
        if (exp < 0x7E)
        {
            f = exp | frac;
            result = 0;
        }
        else 
        {
            frac |= 0x00800000;
            f = frac << (cnt & 0x1F);
            result = frac >> (- cnt);
        }
        if (f != 0)
        {
            *exceptions |= F32_EX_INEXACT;
        }
        if (rnd_mode == F32_RND_NEAREST_EVEN) 
        {
            if ((int)f < 0)
            {
                result++;
                if ((f << 1) == 0)
                {
                    result &= (~1);
                }
            }
            if (sign)
            {
                result = (-result);
            }
        }
        else 
        {
            if (f != 0)
            {
                f = 1;
            }
            if (sign)
            {
                if (rnd_mode == F32_RND_MINUS_INF)
                {
                    result += f;
                }
                result = (-result);
            }
            else 
            {
                if (rnd_mode == F32_RND_PLUS_INF)
                {
                    result += f;
                }
            }
        }
    }
    return result;
}

unsigned int f64_to_f32_conv(unsigned int high, unsigned int low, unsigned int rnd_mode, unsigned int* exceptions)
{
    unsigned int result;
    unsigned int sign;
    int exp;
    unsigned int frac_0, frac_1, res_frac;
    unsigned int temp;

    frac_1 = EXTRACT_F64_FRAC_H(high);
    frac_0 = low;
    exp = EXTRACT_F64_EXP(high);
    sign = EXTRACT_F64_SIGN(high);

    if (exp == 0x7FF)
    {
        // it's either a NaN or infinite
        if ((frac_0 | frac_1) != 0)
        {
            //NaN
            if (((frac_1 >> 19) & 0x1) == 0x0)
            {
                // signalling NaN
                *exceptions |= F32_EX_INVALID;
            }
            unsigned int h;
            h = (high << 12) | (low >> 20);
            result = (sign << 31) | 0x7FC00000 | (h >> 9);
        }
        else
        {
            //infinity
            result = PACK_F32(sign, 0xFF, 0);
        }
    }
    else
    {
        // we need to shift 22 positions (normally we would shift 29 positions, but we'll keep the dot at bits 29-30 for precision)
        //printf("frac_1 = %8X frac_0 = %8X \n", frac_1, frac_0);
        f64_shift_right_loss_detect(frac_1, frac_0, 22, &temp, &res_frac);
        //If exponent is not 0, add the implicit 1
        if (exp)
        {
            res_frac |= 0x40000000;
        }
        // exp = exp - 1023 + 127 - 1 = exp - 897       
        // -1 -> exponent must be 1 unit less than real exponent for rounding and packing
        //printf("result = sign: %8X exp: %8X frac: %8X \n", sign, exp - 0x381, res_frac);
        result = f32_pack_round(sign, exp - 0x381, res_frac, rnd_mode, exceptions);
    }
    return result;
}

T_F64 f32_to_f64_conv(unsigned int x, unsigned int* exceptions)
{
    unsigned int sign;
    int exp;
    unsigned int frac;
    T_F64 result;

    frac = EXTRACT_F32_FRAC(x);
    exp = EXTRACT_F32_EXP(x);
    sign = EXTRACT_F32_SIGN(x);
    if (exp == 0xFF)
    {
        if (frac)
        {
            // NaN
            if (F32_IS_SNAN(x))
            {
                *exceptions |= F32_EX_INVALID;
            }
            result.low = 0;
            //Get rid of exponent and sign
            result.high = x << 9;
            f64_shift_right(result.high, result.low, 12, &result.high, &result.low);
            result.high |= ((sign << 31) | 0x7FF80000);
        }
        else
        {
            //infinity
            result = PACK_F64(sign, 0x7FF, 0, 0);
        }
    }
    else if (exp == 0)
    {
        //either denormal or zero
        if (frac == 0)
        {
            //zero
            result = PACK_F64(sign, 0, 0, 0);
        }
        else
        {
            //subnormal
            f32_normalize_subnormal(&frac, &exp);
            exp--;
            f64_shift_right(frac, 0, 3, &result.high, &result.low);
            result = PACK_F64(sign, (exp + 1024 -128), result.high, result.low);
        }
    }
    else
    {
        f64_shift_right(frac, 0, 3, &result.high, &result.low);
        result = PACK_F64(sign, (exp + 1024 -128), result.high, result.low);
    }
    return result;
}

unsigned int f32_to_f16_conv(unsigned int x, unsigned int rnd_mode, unsigned int* exceptions)
{
#ifdef MOVIDIUS_FP32
    unsigned int s;
    signed int   e;
    unsigned int f;
    // clear flags
    *exceptions = 0;
    unsigned int round;
    unsigned int sticky;
    unsigned int flsb;
    // Extract fields
    s = (x >> 31) & 0x00000001;
    e = (x >> 23) & 0x000000ff;
    f = x & 0x007fffff;
    if((e == 0) && (f == 0))
    {
        // fp32 number is zero 
        return(s<<15);
    }
    else if( e==255 )
    {
        if( f==0 )
        {
            // fp32 number is an infinity
            return((s<<15)|0x7c00);
        }
        else 
        {
            // fp32 number is a NaN - return QNaN, raise invalid if
            // SNaN. QNaN assumed to have MSB of significand set
            if( ~(f&0x00400000) ) *exceptions |= F32_EX_INVALID;
            return(0x7e00); // Sign of NaN ignored
        }
    }
    else
    {
        // fp32 number is normal or possibly denormal
        // Add hidden bit if normal
        if(e!=0)
        {
            f = f | 0x00800000;
        }
        // Unbias exponent
        e = e-127;
        // Check if not below fp16 normal
        if( e>=-14 ) {
            // Round significand according to specified mode
            // Extract round and sticky bits
            round = (f & 0x00001000) >> 12;
            //sticky = |(f & 0x00000fff);
            //replaced with:
            sticky = ((f & 0x00000fff) == 0) ? 0 : 1;
            // Truncate signficand
            f = f >> 13;
            flsb = f & 0x00000001; // LSB 
            // Increment if necessary 
            switch(rnd_mode)
            {
                // Use softfloat mappings (P_CFG will have been mapped before call to CMU
                case F32_RND_NEAREST_EVEN:
                    if((round && flsb) || (round && sticky))
                    {
                        f = f+1;
                    }
                    break;
                case F32_RND_TO_ZERO:
                    break;
                case F32_RND_PLUS_INF:
                    if((s == 0) && (round || sticky))
                    {
                        f = f+1;
                    }
                    break;
                case F32_RND_MINUS_INF:
                    if((s == 1) && (round || sticky))
                    {
                        f = f+1;
                    }
                    break;
            }
            // Inexact if either round or sticky bit set
            if(round || sticky)
            {
                *exceptions |= F32_EX_INEXACT;
            }
            // Check if significand overflow occurred
            if(f&0x00000800)
            {
                f = f >> 1;
                e = e + 1;
            }
            // Add fp16 bias to exponent
            e = e + 15;
            // Check for exponent overflow
            if(e > 30)
            {
                // Return according to rounding mode and set overflow and inexact flags
                *exceptions |=  F32_EX_OVERFLOW;
                *exceptions |=  F32_EX_INEXACT ;
                switch(rnd_mode)
                {
                case F32_RND_NEAREST_EVEN:
                    return ((s << 15) | 0x7c00);// Returns infinity
                case F32_RND_TO_ZERO:
                    return ((s << 15) | 0x7bff);// Largest finite #
                case F32_RND_PLUS_INF:
                    return ((s == 0) ? 0x7c00 : 0xfbff);
                case F32_RND_MINUS_INF:
                    return ((s == 1) ? 0xfc00 : 0x7bff);
                }
            }
            else
            {
                // Remove hidden bit, pack, and return
                f = f & 0x000003ff;
                return ((s << 15) | (e << 10) | f);
            }
        }
        else
        {
            // fp32 number may be representable as a fp16 denormal
            // flushing FP16 denormal outputs to zero
            return(s << 15);
        }
    }
    return 0;
#else
    unsigned int result;
    unsigned int sign;
    int exp;
    unsigned int frac, res_frac;

    frac = EXTRACT_F32_FRAC(x);
    exp = EXTRACT_F32_EXP(x);
    sign = EXTRACT_F32_SIGN(x);

    if (exp == 0xFF)
    {
        // it's either a NaN or infinite
        if (frac != 0)
        {
            //NaN
            if (((frac >> 22) & 0x1) == 0x0)
            {
                // signalling NaN
                *exceptions |= F32_EX_INVALID;
            }
            result = (sign << 15) | 0x7C00 | (frac >> 13);
        }
        else
        {
            //infinity
            result = PACK_F16(sign, 0x1F, 0);
        }
    }
    else
    {
        // we need to shift 13 positions but will shift only 9, to keep the point at bits 13-14
        res_frac = f32_shift_right_loss_detect(frac, 9);
        //If exponent is not 0, add the implicit 1
        if (exp)
        {
            res_frac |= 0x00004000;
        }
        // exp = exp - 127 + 15 - 1 = exp - 113       
        // -1 -> exponent must be 1 unit less than real exponent for rounding and packing
        result = f16_pack_round(sign, exp - 0x71, res_frac, rnd_mode, exceptions);
    }
    return result;
#endif
}

unsigned int f16_to_f32_conv(unsigned int x, unsigned int* exceptions)
{
    unsigned int sign;
    int exp;
    unsigned int frac;
    unsigned int result;

    frac = EXTRACT_F16_FRAC(x);
    exp = EXTRACT_F16_EXP(x);
    sign = EXTRACT_F16_SIGN(x);
    if (exp == 0x1F) 
    {
        if (frac != 0)
        {
            // NaN
            if (F16_IS_SNAN(x))
            {
                *exceptions |= F32_EX_INVALID;
            }
            result = 0;
            //Get rid of exponent and sign
#ifndef MOVIDIUS_FP32
            result = x << 22;
            result = f32_shift_right(result, 9);
            result |= ((sign << 31) | 0x7F800000);
#else
            result |= ((sign << 31) | 0x7FC00000);
#endif
        }
        else
        {
            //infinity
            result = PACK_F32(sign, 0xFF, 0);
        }
    }
    else if (exp == 0)
    {
        //either denormal or zero
        if (frac == 0)
        {
            //zero
            result = PACK_F32(sign, 0, 0);
        }
        else
        {
            //subnormal
#ifndef MOVIDIUS_FP32
            f16_normalize_subnormal(&frac, &exp);
            exp--;
            // ALDo: is the value 13 ok??
            result = f16_shift_left(frac, 13);
            // exp = exp + 127 - 15 = exp + 112
            result = PACK_F32(sign, (exp + 0x70), result);
#else
            result = PACK_F32(sign, 0, 0);
#endif
        }
    }
    else
    {
        // ALDo: is the value 13 ok??
        result = f16_shift_left(frac, 13);
        result = PACK_F32(sign, (exp + 0x70), result);
    }
    return result;
}

//Utility Functions
inline void unsigned64_sub(unsigned int op1_l,  unsigned int op1_h,  unsigned int op2_l,  unsigned int op2_h,  unsigned int *res_l,  unsigned int *res_h)
{
    *res_l = op1_l - op2_l;
    *res_h = op1_h - op2_h;
    if(op1_l < op2_l)
    {
        *res_h = *res_h - 1;
    }
}

inline void unsigned64_add(unsigned int op1_l,  unsigned int op1_h,  unsigned int op2_l,  unsigned int op2_h,  unsigned int *res_l,  unsigned int *res_h)
{
    *res_l = op1_l + op2_l;
    *res_h = op1_h + op2_h;
    if(*res_l < op1_l)
    {
        *res_h = *res_h + 1;
    }
}

unsigned int f32_add_operands(unsigned int op1, unsigned int op2, unsigned int rnd_mode, unsigned int* exceptions)
{
    unsigned int ret_val = 0;
    unsigned int ret_val_sign;
    int ret_val_exp;
    unsigned int ret_val_frac;
    bool op_done = false;
    // extract all the parts of the operands
    unsigned int op1_sign = EXTRACT_F32_SIGN(op1);
    //unsigned int op2_sign = EXTRACT_F32_SIGN(op2);
    int op1_exp = EXTRACT_F32_EXP(op1);
    int op2_exp = EXTRACT_F32_EXP(op2);
    unsigned int op1_frac = EXTRACT_F32_FRAC(op1);
    unsigned int op2_frac = EXTRACT_F32_FRAC(op2);
    int exp_diff = op1_exp - op2_exp;
    // shift the fraction to the left in order to get maximum possible precision
    op1_frac <<= 6;
    op2_frac <<= 6;
    ret_val_sign = op1_sign;
    // the same sign - we need to add the numbers
    if (exp_diff > 0)
    {
        // first exponent is bigger
        if (op1_exp == 0xFF)
        {
            ret_val_exp = op1_exp; 
            if (op1_frac > 0)
            {
                // op1 is a NaN
                ret_val = f32_propagate_NaNs(op1, op2, exceptions);
                op_done = true;
            }
            else
            {
                // op1 is infinity
                ret_val = op1;
                op_done = true;
            }
        }
        else
        {
            if (op2_exp == 0)
            {
                exp_diff--;
            }
            else
            {
                op2_frac |= 0x20000000;
            }
            op2_frac = f32_shift_right_loss_detect(op2_frac, exp_diff);
            ret_val_exp = op1_exp;
        }
    }
    else if (exp_diff < 0)
    {
        // second exponent is bigger
        if (op2_exp == 0xFF)
        {
            ret_val_exp = op2_exp;
            if (op2_frac > 0)
            {
                // op2 is a NaN
                ret_val = f32_propagate_NaNs(op1, op2, exceptions);
                op_done = true;
            }
            else
            {
                // op2 is infinity
                ret_val = op2;
                op_done = true;
            }
        }
        else
        {
            if (op1_exp == 0)
            {
                exp_diff++;
            }
            else
            {
                op1_frac |= 0x20000000;
            }
            // use the positive difference
            op1_frac = f32_shift_right_loss_detect(op1_frac, (-exp_diff));
            ret_val_exp = op2_exp;
        }
    }
    else
    {
        // exponents are equal
        ret_val_exp = op1_exp;
        if (op1_exp == 0xFF)
        {
            if ((op1_frac > 0) || (op2_frac > 0))
            {
                // one operand is a NaN
                ret_val = f32_propagate_NaNs(op1, op2, exceptions);
                op_done = true;
            }
            else
            {
                // One operand is infinity
                ret_val = op1;
                op_done = true;
            }
        }
        else
        {
            if (op1_exp == 0)
            {
                ret_val = PACK_F32 (ret_val_sign, 0, ((op1_frac + op2_frac) >> 6));
                op_done = true;
            }
            else
            {
                ret_val_frac = 0x40000000 + op1_frac + op2_frac;
                ret_val_exp = op1_exp;
                ret_val = f32_pack_round(ret_val_sign, ret_val_exp, ret_val_frac, rnd_mode, exceptions);
                op_done = true;
            }
        }
    }
    if (op_done == false)
    {
        op1_frac |= 0x20000000;
        ret_val_frac = ((op1_frac + op2_frac) << 1);
        ret_val_exp--;
        if ((ret_val_frac & 0x80000000) != 0)
        {
            ret_val_frac = (op1_frac + op2_frac);
            ret_val_exp++;
        }
        ret_val = f32_pack_round(ret_val_sign, ret_val_exp, ret_val_frac, rnd_mode, exceptions);
    }
    return ret_val;
}

unsigned int f32_sub_operands(unsigned int op1, unsigned int op2, unsigned int rnd_mode, unsigned int* exceptions)
{
    unsigned int ret_val;
    unsigned int ret_val_sign;
    int ret_val_exp;
    unsigned int ret_val_frac;
    // extract all the parts of the operands
    unsigned int op1_sign = EXTRACT_F32_SIGN(op1);
    //unsigned int op2_sign = EXTRACT_F32_SIGN(op2);
    int op1_exp = EXTRACT_F32_EXP(op1);
    int op2_exp = EXTRACT_F32_EXP(op2);
    unsigned int op1_frac = EXTRACT_F32_FRAC(op1);
    unsigned int op2_frac = EXTRACT_F32_FRAC(op2);
    int exp_diff = op1_exp - op2_exp;
    // shift the fraction to the left in order to get maximum possible precision
    op1_frac <<= 7;
    op2_frac <<= 7;
    ret_val_sign = op1_sign;
    if (exp_diff > 0)
    {
        // exponent of op1 is bigger
        if (op1_exp == 0xFF) 
        {
            if (op1_frac != 0)
            {
                // op1 is a NaN
                ret_val = f32_propagate_NaNs(op1, op2, exceptions);
            }
            else
            {
                // infinity
                ret_val = op1;
            }
        }
        if (op2_exp == 0) 
        {
            exp_diff--;
        }
        else 
        {
            op2_frac |= 0x40000000;
        }
        op2_frac = f32_shift_right_loss_detect(op2_frac, exp_diff);
        op1_frac |= 0x40000000;
        ret_val_frac = op1_frac - op2_frac;
        ret_val_exp = op1_exp - 1;
        f32_normalize_components(&ret_val_exp, &ret_val_frac);
        ret_val = f32_pack_round(ret_val_sign, ret_val_exp, ret_val_frac, rnd_mode, exceptions);
    }
    else if (exp_diff < 0) 
    {
        if (op2_exp == 0xFF) 
        {
            if (op2_frac)
            {
                // op2 is a NaN
                ret_val = f32_propagate_NaNs(op1, op2, exceptions);
            }
            else
            {
                // answer is infinity
                ret_val = PACK_F32 (ret_val_sign^1, 0xFF, 0);
            }
        }
        if (op1_exp == 0) 
        {
            exp_diff++;
        }
        else 
        {
            op1_frac |= 0x40000000;
        }
        op1_frac = f32_shift_right_loss_detect(op1_frac, 0 - exp_diff);
        op2_frac |= 0x40000000;
        ret_val_frac = op2_frac - op1_frac;
        ret_val_exp = op2_exp - 1;
        ret_val_sign ^= 1;
        f32_normalize_components(&ret_val_exp, &ret_val_frac);
        ret_val = f32_pack_round(ret_val_sign, ret_val_exp, ret_val_frac, rnd_mode, exceptions);
    }
    else
    {
        if (op1_exp == 0xFF) 
        {
            // numbers are NaNs or Infinity
            if ((op1_frac != 0) || (op2_frac != 0))
            {
                // NaNs 
                ret_val = f32_propagate_NaNs(op1, op2, exceptions);
            }
            else
            {
                // infinity
                *exceptions |= F32_EX_INVALID;
                ret_val =  F32_NAN_DEFAULT;
            }
        }
        else 
        {
            // exponents are equal
            if (op1_exp == 0) 
            {
                op1_exp = 1;
                op2_exp = 1;
            }
            if (op1_frac > op2_frac)
            {
                // op1 fraction is bigger
                ret_val_frac = op1_frac - op2_frac;
                ret_val_exp = op1_exp - 1;
                f32_normalize_components(&ret_val_exp, &ret_val_frac);
                ret_val = f32_pack_round(ret_val_sign, ret_val_exp, ret_val_frac, rnd_mode, exceptions);
            }
            else if (op1_frac < op2_frac)
            {
                // op1 fraction is smaller
                ret_val_frac = op2_frac - op1_frac;
                ret_val_exp = op2_exp - 1;
                ret_val_sign ^= 1;
                f32_normalize_components(&ret_val_exp, &ret_val_frac);
                ret_val = f32_pack_round(ret_val_sign, ret_val_exp, ret_val_frac, rnd_mode, exceptions);
            }
            else
            {
                // answer is + or - zero
                ret_val_sign = (rnd_mode == F32_RND_MINUS_INF) ? 1 : 0;
                ret_val = PACK_F32 (ret_val_sign, 0, 0);
            }
        }
    }
    return ret_val;
}

unsigned int f32_add(unsigned int op1, unsigned int op2, unsigned int rnd_mode, unsigned int* exceptions)
{
    unsigned int ret_val;
    // extract all the parts of the operands
    unsigned int op1_sign = EXTRACT_F32_SIGN(op1);
    unsigned int op2_sign = EXTRACT_F32_SIGN(op2);
    if (op1_sign == op2_sign)
    {
        ret_val = f32_add_operands(op1, op2, rnd_mode, exceptions);
    }
    else
    {
        // different signs - we need to substract
        ret_val = f32_sub_operands(op1, op2, rnd_mode, exceptions);
    }
    return ret_val;
}

unsigned int f32_sub(unsigned int op1, unsigned int op2, unsigned int rnd_mode, unsigned int* exceptions)
{
    unsigned int ret_val;
    // extract all the parts of the operands
    unsigned int op1_sign = EXTRACT_F32_SIGN(op1);
    unsigned int op2_sign = EXTRACT_F32_SIGN(op2);
    if (op1_sign == op2_sign)
    {
        ret_val = f32_sub_operands(op1, op2, rnd_mode, exceptions);
    }
    else
    {
        // different signs - we need to add
        ret_val = f32_add_operands(op1, op2, rnd_mode, exceptions);
    }
    return ret_val;
}

void f32_multiply_operands(unsigned int op1_frac, unsigned int op2_frac, unsigned int* res_l, unsigned int* res_h)
{
    unsigned int op1_frac_h, op1_frac_l, op2_frac_h, op2_frac_l;
    unsigned int res_mid_a, res_mid_b;
    // use the formula: A*B = 2^32 * Ah*Bh + 2^16 * (Ah*Bl + Al*Bh) + AlBl
    op1_frac_l = op1_frac & 0xFFFF;
    op1_frac_h = (op1_frac >> 16) & 0xFFFF;
    op2_frac_l = op2_frac & 0xFFFF;
    op2_frac_h = (op2_frac >> 16) & 0xFFFF;
    // calculate partial products
    *res_l = op1_frac_l * op2_frac_l;
    res_mid_a = op1_frac_l * op2_frac_h;
    res_mid_b = op1_frac_h * op2_frac_l;
    *res_h = op1_frac_h * op2_frac_h;
    // store middle result into res_mid_a
    res_mid_a += res_mid_b;
    // add the upper part of the middle result
    if (res_mid_a < res_mid_b)
    {
        // add the carry bit from the middle addition
        *res_h += ((0x1 << 16) + (res_mid_a >> 16));
    }
    else
    {
        *res_h += (res_mid_a >> 16);
    }
    res_mid_a <<= 16;
    // store the lower unsigned int of the result
    *res_l += res_mid_a;
    // store the carry bit in the upper unsigned int of the result
    if (*res_l < res_mid_a)
    {
        *res_h += 0x1;
    }
}

unsigned int f32_mul(unsigned int op1, unsigned int op2, unsigned int rnd_mode, unsigned int* exceptions)
{
    unsigned int result;
    unsigned int result_sign;
    int result_exp;
    unsigned int result_frac;
    // low and high 32 bit parts of the multiplication result
    unsigned int res_0, res_1;

    unsigned int op1_sign = EXTRACT_F32_SIGN(op1);
    unsigned int op2_sign = EXTRACT_F32_SIGN(op2);
    int op1_exp = EXTRACT_F32_EXP(op1);
    int op2_exp = EXTRACT_F32_EXP(op2);
    unsigned int op1_frac = EXTRACT_F32_FRAC(op1);
    unsigned int op2_frac = EXTRACT_F32_FRAC(op2);
    // calculate sign
    result_sign = op1_sign ^ op2_sign;
    // check for NaNs
    if (op1_exp == 0xFF)
    {
        // op1 is NaN or infinite
        if ((op1_frac != 0) || ((op2_exp == 0xFF) && (op2_frac != 0)))
        {
            result = f32_propagate_NaNs(op1, op2, exceptions);
        }
        // op1 is infinite and op2 is + or - zero
        else if ((op2_exp == 0) && (op2_frac == 0))
        {
            *exceptions |= F32_EX_INVALID;
            result = F32_NAN_DEFAULT;
        }
        else
        {
            // result is infinity
            result = PACK_F32(result_sign, 0xFF, 0);
        }
    }
    else if (op2_exp == 0xFF)
    {
        // op2 is NaN or infinite
        if (op2_frac != 0)
        {
            // op2 is NaN
            result = f32_propagate_NaNs(op1, op2, exceptions);
        }
        else if ((op1_exp == 0) && (op1_frac == 0))
        {
            //op2 is infinite and op1 is + or - zero
            *exceptions |= F32_EX_INVALID;
            result = F32_NAN_DEFAULT;
        }
        else
        {
            // result is infinity
            result = PACK_F32(result_sign, 0xFF, 0);
        }
    }
    // check for zero and subnormals
    else if (op1_exp == 0)
    {
        if (op1_frac == 0)
        {
            // op1 is 0, so result is 0
            result = PACK_F32(result_sign, 0, 0);
        }
        else
        {
            // op1 is subnormal
            f32_normalize_subnormal(&op1_frac, &op1_exp);
            // do multiplication
            result_exp = op1_exp + op2_exp - 0x7F;
            // shift values for higher precision
            op1_frac = (op1_frac | 0x00800000) << 7;
            op2_frac = (op2_frac | 0x00800000) << 8;
            f32_multiply_operands(op1_frac, op2_frac, &res_0, &res_1);
            // res_0 will be ignored - check if we need to round the LSB
            if (res_0 != 0)
            {
                res_1 |= 0x1;
            }
            if ((int)(res_1 << 1) >= 0)
            {
                res_1 <<= 1;
                result_exp--;
            }
            // prepare the result
            result_frac = res_1;
            result = f32_pack_round(result_sign, result_exp, result_frac, rnd_mode, exceptions);
        }
    }
    else if (op2_exp == 0)
    {
        if (op2_frac == 0)
        {
            result =  PACK_F32(result_sign, 0, 0);
        }
        else
        {
            // normalize op2
            f32_normalize_subnormal(&op2_frac, &op2_exp);
            // do multiplication
            result_exp = op1_exp + op2_exp - 0x7F;
            op1_frac = (op1_frac | 0x00800000) << 7;
            op2_frac = (op2_frac | 0x00800000) << 8;
            f32_multiply_operands(op1_frac, op2_frac, &res_0, &res_1);
            // res_0 will be ignored - check if we need to round the LSB
            if (res_0 != 0)
            {
                res_1 |= 0x1;
            }
            if ((int)(res_1 << 1) >= 0)
            {
                res_1 <<= 1;
                result_exp--;
            }
            // prepare the result
            result_frac = res_1;
            result = f32_pack_round(result_sign, result_exp, result_frac, rnd_mode, exceptions);
        }
    }
    else
    {
        result_exp = op1_exp + op2_exp - 0x7F;
        op1_frac = (op1_frac | 0x00800000) << 7;
        op2_frac = (op2_frac | 0x00800000) << 8;
        f32_multiply_operands(op1_frac, op2_frac, &res_0, &res_1);
        // res_0 will be ignored - check if we need to round the LSB
        if (res_0 != 0)
        {
            res_1 |= 0x1;
        }
        if ((int)(res_1 << 1) >= 0)
        {
            res_1 <<= 1;
            result_exp--;
        }
        // prepare the result
        result_frac = res_1;
        result = f32_pack_round(result_sign, result_exp, result_frac, rnd_mode, exceptions);
    }
    return result;
}

unsigned int f32_divide_frac (unsigned int frac_1, unsigned int frac_2)
{
    unsigned int f0, f1 = 1;
    // remainder fractions
    unsigned int rem_0, rem_1;
    // result fractions
    unsigned int res_0, res_1;
    unsigned int result;
    //consider frac_1 as the high part of a 64 bit fraction, for higher precision
    if (frac_2 <= frac_1)
    {
        result = 0xFFFFFFFF;
    }
    else
    {
        f1 = frac_2 >> 16;
        result = ((f1 << 16) <= frac_1) ? 0xFFFF0000 : (frac_1 / f1) << 16;
        // obtain the multiply result in res_0 (low) and res_1 (high)       
        f32_multiply_operands(frac_2, result, &res_0, &res_1);
        // calculate remainder
        unsigned64_sub(0, frac_1, res_0, res_1, &rem_0, &rem_1); 
        while ((int)rem_1 < 0)
        {
            result -= 0x10000;
            f0 = frac_2 << 16;
            unsigned64_add(rem_0, rem_1, f0, f1, &rem_0, &rem_1);
        }
        rem_1 = (rem_1 << 16) | (rem_0 >> 16);
        if ((f1 << 16) <= rem_1)
        {
            result |= 0xFFFF;
        }
        else
        {
            result |= (rem_1 / f1);
        }
    }
    return result;
}

unsigned int f32_div(unsigned int op1, unsigned int op2, unsigned int rnd_mode, unsigned int* exceptions)
{
    unsigned int result = 0;
    unsigned int result_sign;
    int result_exp;
    unsigned int result_frac;
    unsigned int rest_0, rest_1, t_0, t_1;

    unsigned int op1_sign = EXTRACT_F32_SIGN(op1);
    unsigned int op2_sign = EXTRACT_F32_SIGN(op2);
    int op1_exp = EXTRACT_F32_EXP(op1);
    int op2_exp = EXTRACT_F32_EXP(op2);
    unsigned int op1_frac = EXTRACT_F32_FRAC(op1);
    unsigned int op2_frac = EXTRACT_F32_FRAC(op2);    

    result_sign = op1_sign ^ op2_sign;
    if (op1_exp == 0xFF)
    {
        // op1 is NaN
        if (op1_frac != 0)
        {
            result = f32_propagate_NaNs(op1, op2, exceptions);
        }
        // op1 is infinite
        else if (op2_exp == 0xFF)
        {
            // op2 is NaN
            if (op2_frac != 0)
            {
                result = f32_propagate_NaNs(op1, op2, exceptions);
            }
            // op2 is infinite
            else
            {
                *exceptions |= F32_EX_INVALID;
                result = F32_NAN_DEFAULT;           
            }
        }
    }
    else if (op2_exp == 0xFF)
    {
        if (op2_frac != 0)
        {
            // op2 is NaN
            result = f32_propagate_NaNs(op1, op2, exceptions);
        }
        else
        {
            // op2 is infinite and op1 is not, so result is 0
            result = PACK_F32(result_sign, 0, 0);
        }
    }
    else if (op2_exp == 0)
    {
        if (op2_frac == 0)
        {
            // op2 is 0
            if ((op1_exp | op1_frac) != 0)
            {
                // division by 0 - return infinity
                *exceptions |= F32_EX_DIV_BY_ZERO;
                result = PACK_F32(result_sign, 0xFF, 0);
            }
            else
            {
                // 0/0 invalid operation - return NaN
                *exceptions |= F32_EX_INVALID;
                result = F32_NAN_DEFAULT;
            }
        }
        else
        {
            // op2 is subnormal
            f32_normalize_subnormal(&op2_frac, &op2_exp);
            // do division operation
            result_exp = op1_exp - op2_exp + 0x7D;
            op1_frac = (op1_frac | 0x00800000) << 7;
            op2_frac = (op2_frac | 0x00800000) << 8;
            if ((op1_frac + op1_frac) >= op2_frac)
            {
                op1_frac >>= 1;
                result_exp++;
            }
            result_frac = f32_divide_frac (op1_frac, op2_frac);
            if ((result_frac & 0x3F) <= 2)
            {
                f32_multiply_operands(op2_frac, result_frac, &t_0, &t_1);
                unsigned64_sub(0, op1_frac, t_0, t_1, &rest_0, &rest_1);
                while (rest_1 < 0)
                {
                    result_frac--;
                    unsigned64_add(rest_0, rest_1, op2_frac, 0, &rest_0, &rest_1);
                }
                if (rest_0 != 0)
                {
                    result_frac |= 0x1;
                }
            }
            result = f32_pack_round(result_sign, result_exp, result_frac, rnd_mode, exceptions);
        }
    }
    else if (op1_exp == 0)
    {
        if (op1_frac == 0)
        {
            // op1 is 0 - result is 0
            result = PACK_F32(result_sign, 0, 0);
        }
        else
        {
            // op1 is subnormal
            f32_normalize_subnormal(&op1_frac, &op1_exp);
            // do division operation
            result_exp = op1_exp - op2_exp + 0x7D;
            op1_frac = (op1_frac | 0x00800000) << 7;
            op2_frac = (op2_frac | 0x00800000) << 8;
            if ((op1_frac + op1_frac) >= op2_frac)
            {
                op1_frac >>= 1;
                result_exp++;
            }
            result_frac = f32_divide_frac (op1_frac, op2_frac);
            if ((result_frac & 0x3F) <= 2)
            {
                f32_multiply_operands(op2_frac, result_frac, &t_0, &t_1);
                unsigned64_sub(0, op1_frac, t_0, t_1, &rest_0, &rest_1);
                while (rest_1 < 0)
                {
                    result_frac--;
                    unsigned64_add(rest_0, rest_1, op2_frac, 0, &rest_0, &rest_1);
                }
                if (rest_0 != 0)
                {
                    result_frac |= 0x1;
                }
            }
            result = f32_pack_round(result_sign, result_exp, result_frac, rnd_mode, exceptions);
        }
    }
    else
    {
        // do division operation
        result_exp = op1_exp - op2_exp + 0x7D;
        op1_frac = (op1_frac | 0x00800000) << 7;
        op2_frac = (op2_frac | 0x00800000) << 8;
        if ((op1_frac + op1_frac) >= op2_frac)
        {
            op1_frac >>= 1;
            result_exp++;
        }
        result_frac = f32_divide_frac (op1_frac, op2_frac);
        if ((result_frac & 0x3F) <= 2)
        {
            f32_multiply_operands(op2_frac, result_frac, &t_0, &t_1);
            unsigned64_sub(0, op1_frac, t_0, t_1, &rest_0, &rest_1);
            while (rest_1 < 0)
            {
                result_frac--;
                unsigned64_add(rest_0, rest_1, op2_frac, 0, &rest_0, &rest_1);
            }
            if (rest_0 != 0)
            {
                result_frac |= 0x1;
            }
        }
        result = f32_pack_round(result_sign, result_exp, result_frac, rnd_mode, exceptions);
    }
    return result;
}

unsigned int f32_sqrt_frac(int exp, unsigned int frac)
{
    static const unsigned int sqrt_Odd_Errors[] = {0x0004, 0x0022, 0x005D, 0x00B1, 0x011D, 0x019F, 0x0236, 0x02E0,
                                                   0x039C, 0x0468, 0x0545, 0x0631, 0x072B, 0x0832, 0x0946, 0x0A67};
    static const unsigned int sqrt_Even_Errors[] = {0x0A2D, 0x08AF, 0x075A, 0x0629, 0x051A, 0x0429, 0x0356, 0x029E,
                                                    0x0200, 0x0179, 0x0109, 0x00AF, 0x0068, 0x0034, 0x0012, 0x0002};
    unsigned int result;
    bool op_done = false;
    unsigned int i;
    i = (frac >> 27) & 15;
    if (exp & 1)
    {
        result = 0x4000 + (frac >> 17) - sqrt_Odd_Errors[i];
        result = ((frac / result) << 14) + (result << 15);
        frac >>= 1;
    }
    else 
    {
        result = 0x8000 + (frac >> 17) - sqrt_Even_Errors[i];
        result = frac / result + result;
        result = (0x20000 <= result) ? 0xFFFF8000 : (result << 15);
        if (result <= frac)
        {
            result = (unsigned int)(((int)frac) >> 1);
            op_done = true;
        }
    }
    if (!op_done)
    {
        result = ((f32_divide_frac(frac, result)) >> 1) + (result >> 1);
    }
    return result;
}

unsigned int f32_sqrt(unsigned int x, unsigned int rnd_mode, unsigned int* exceptions)
{
    unsigned int result = 0;
    bool op_done = false;
    int res_exp;
    unsigned int res_frac, r_l, r_h, t_l, t_h;

    unsigned int sign = EXTRACT_F32_SIGN(x);
    int exp = EXTRACT_F32_EXP(x);
    unsigned int frac = EXTRACT_F32_FRAC(x);

    if (exp == 0xFF)
    {
        if (frac)
        {
            result = f32_propagate_NaNs(x, 0, exceptions);
        }
        else if (sign == 0)
        {
            result = x;
        }
        else
        {
            *exceptions |= F32_EX_INVALID;
            result =  F32_NAN_DEFAULT;
        }
        op_done = true;
    }
    else if (sign)
    {
        if ((exp | frac) == 0)
        {
            result = x;
        }
        else
        {
            *exceptions |= F32_EX_INVALID;
            result =  F32_NAN_DEFAULT;
        }
        op_done = true;
    }
    else if (exp == 0)
    {
        if (frac == 0)
        {
            result = 0;
            op_done = true;
        }
        else
        {
            f32_normalize_subnormal(&frac, &exp);
        }
    }
    if (!op_done)
    {
        // no special case - calculate the root
        res_exp = ((exp - 0x7F) >> 1) + 0x7E;
        frac = (frac | 0x00800000) << 8;
        res_frac = f32_sqrt_frac(exp, frac) + 2;
        if ((res_frac & 0x7F) <= 5)
        {
            if (res_frac < 2)
            {
                res_frac = 0x7FFFFFFF;
                result = f32_pack_round(0, res_exp, res_frac, rnd_mode, exceptions);
            }
            else
            {
                frac >>= (exp & 1);
                f32_multiply_operands(res_frac, res_frac, &t_l, &t_h);
                unsigned64_sub(0, frac, t_l, t_h, &r_l, &r_h);
                while ((int)r_h < 0)
                {
                    res_frac--;
                    unsigned64_shift_left(res_frac, 0, 1, &t_l, &t_h);
                    t_l |= 1;
                    unsigned64_add(r_l, r_h, t_l, t_h, &r_l, &r_h);
                }
                if ((r_l | r_h) != 0)
                {
                    res_frac |= 0x1;
                }
                res_frac = f32_shift_right_loss_detect(res_frac, 1);
                result = f32_pack_round(0, res_exp, res_frac, rnd_mode, exceptions);
            }
        }
        else
        {
            res_frac = f32_shift_right_loss_detect(res_frac, 1);
            result = f32_pack_round(0, res_exp, res_frac, rnd_mode, exceptions);
        }
    }
    return result;
}

//Float 64 add and sub - needed for SUMX
inline void f64_shift_left(unsigned int high, unsigned int low, unsigned int cnt, unsigned int* out_high, unsigned int* out_low)
{
    if (cnt == 0)
    {
        *out_low = low;
        *out_high = high;
    }
    else
    {
        if (cnt < 32)
        {
            *out_low = low << cnt;
            *out_high = (high << cnt) | (low >> ((32 - cnt)));
        }
        else if (cnt < 64)
        {
            *out_low = 0;
            *out_high = (low >> (64 - cnt));
        }
        else
        {
            *out_low = 0;
            *out_high = 0;
        }
    }
}

inline void f64_shift_right_96_bits(unsigned int high, unsigned int low, unsigned int loss, unsigned int cnt, unsigned int *out_high, unsigned int *out_low, unsigned int *out_loss)
{
    unsigned int temp;
    int neg_cnt = (((~cnt) + 1) & 0x1F);

    if (cnt == 0)
    {
        *out_loss = loss;
        *out_low = low;
        *out_high = high;
    }
    else 
    {
        if (cnt < 32)
        {
            *out_loss = (low << neg_cnt);
            *out_low = ((high << neg_cnt) | (low >> cnt));
            *out_high = (high >> cnt);
        }
        else 
        {
            if (cnt == 32) 
            {
                *out_loss = low;
                *out_low = high;
            }
            else 
            {
                temp = (loss | low);
                if (cnt < 64)
                {
                    *out_loss = (high << neg_cnt);
                    *out_low = (high >> (cnt & 31));
                }
                else 
                {
                    if (cnt == 64)
                    {
                        *out_loss = high;
                    }
                    else
                    {
                        *out_loss = (high != 0)? 1 : 0;
                    }
                    *out_low = 0;
                }
                *out_loss |= (temp != 0) ? 1 : 0;
            }
            *out_high = 0;
        }
    }
}

T_F64 f64_propagate_NaNs(T_F64 op1, T_F64 op2, unsigned int* exceptions)
{
    bool op1_NaN, op1_sNaN, op2_NaN, op2_sNaN;
    T_F64 result;
    op1_NaN = F64_IS_NAN(op1);
    op1_sNaN = F64_IS_SNAN(op1);
    op2_NaN = F64_IS_NAN(op2);
    op2_sNaN = F64_IS_SNAN(op2);
    op1.high |= 0x00080000;
    op2.high |= 0x00080000;
    if (op1_sNaN | op2_sNaN)
    {
        *exceptions |= F32_EX_INVALID;
    }
    if (op1_sNaN) 
    {
        if (op2_sNaN)
        {
            unsigned int tmp1 = (op1.high << 1);
            unsigned int tmp2 = (op2.high << 1); 
            if ((tmp1 < tmp2) || ((tmp1 == tmp2) && (op1.low < op2.low)))
            {
                result = op2;
            }
            else if ((tmp2 < tmp1) || ((tmp1 == tmp2) && (op2.low < op1.low)))
            {
                result = op1;
            }
            else
            {
                result = (op1.high < op2.high) ? op1 : op2;
            }
        }
        else
        {
            result = op2_NaN ? op2 : op1;
        }
    }
    else if (op1_NaN) 
    {
        if (op2_sNaN | (!op2_NaN))
        {
            result = op1;
        }
        else
        {
            unsigned int tmp1 = (op1.high << 1);
            unsigned int tmp2 = (op2.high << 1); 
            if ((tmp1 < tmp2) || ((tmp1 == tmp2) && (op1.low < op2.low)))
            {
                result = op2;
            }
            else if ((tmp2 < tmp1) || ((tmp1 == tmp2) && (op2.low < op1.low)))
            {
                result = op1;
            }
            else
            {
                result = (op1.high < op2.high) ? op1 : op2;
            }
        }
    }
    else 
    {
        result = op2;
    }
    return result;
}

inline void f64_normalize_components(int *exp, unsigned int* frac_h, unsigned int* frac_l, unsigned int* loss)
{
    int cnt;

    if (*frac_h == 0)
    {
        *frac_h = *frac_l;
        *frac_l = 0;
        *exp -= 32;
    }
    // find out the number of leading zeros in the fraction
    unsigned int temp = *frac_h;
    cnt = 0;
    if (temp < 0x10000)
    {
        cnt += 16;
        temp <<= 16;
    }
    if (temp < 0x1000000)
    {
        cnt += 8;
        temp <<= 8;
    }
    if (temp < 0x10000000)
    {
        cnt += 4;
        temp <<= 4;
    }
    cnt += leading_zeros[temp >> 28];
    //remove bits reserved for sign and exponent
    cnt -= 11;
    if (cnt >= 0)
    {
        *loss = 0;
        f64_shift_left(*frac_h, *frac_l, cnt, frac_h, frac_l);
    }
    else
    {
        f64_shift_right_96_bits(*frac_h, *frac_l, 0, ((~cnt) + 1), frac_h, frac_l, loss);
    }
    if (*exp >= cnt)
    {
        *exp -= cnt;
    }
    else
    {
        //AlDo: Do we need to zero out the fraction as well?
        *exp = 0;
        *frac_h = 0;
        *frac_l = 0;
    }
}

T_F64 f64_pack_round(unsigned int sign, int exp, unsigned int frac_h, unsigned int frac_l, unsigned int loss, unsigned int rnd_mode, unsigned int* exceptions)
{
    T_F64 result;
    bool inc, is_tiny;
    inc = ((int)loss < 0);
    if (rnd_mode != F32_RND_NEAREST_EVEN)
    {
        if (rnd_mode == F32_RND_TO_ZERO) 
        {
            inc = 0;
        }
        else 
        {
            if (sign) 
            {
                inc = ((rnd_mode == F32_RND_MINUS_INF) && (loss != 0));
            }
            else 
            {
                inc = ((rnd_mode == F32_RND_PLUS_INF) && (loss != 0));
            }
        }
    }
    if ((unsigned int) exp >= 0x7FD) 
    {
        if ((exp > 0x7FD) || ((exp == 0x7FD) && (frac_h == 0x001FFFFF) && (frac_l == 0xFFFFFFFF) && inc))
        {
            *exceptions |= (F32_EX_OVERFLOW | F32_EX_INEXACT);
            if ((rnd_mode == F32_RND_TO_ZERO) || 
               ((sign != 0) && (rnd_mode == F32_RND_PLUS_INF)) || 
               ((sign == 0) && (rnd_mode == F32_RND_MINUS_INF)))
            {
                result = PACK_F64(sign, 0x7FE, 0x000FFFFF, 0xFFFFFFFF);
            }
            else
            {
                result = PACK_F64(sign, 0x7FF, 0, 0);
            }
        }
        else 
        {
            if (exp < 0) 
            {
                is_tiny = ((detect_tiny_mode == F32_DETECT_TINY_BEFORE_RND) || (exp < (-1)) || (!inc) || 
                          ((frac_h < 0x001FFFFF) || ((frac_h == 0x001FFFFF) && (frac_l < 0xFFFFFFFF))));
                f64_shift_right_96_bits(frac_h, frac_l, loss, (-exp), &frac_h, &frac_l, &loss);
                exp = 0;
                if (is_tiny && (loss != 0))
                {
                    *exceptions |= F32_EX_UNDERFLOW;
                }
                if (rnd_mode == F32_RND_NEAREST_EVEN) 
                {
                    inc = ((int)loss < 0);
                }
                else 
                {
                    if (sign) 
                    {
                        inc = (rnd_mode == F32_RND_MINUS_INF) && (loss != 0);
                    }
                    else 
                    {
                        inc = (rnd_mode == F32_RND_PLUS_INF) && (loss != 0);
                    }
                }
            }
            if (loss != 0)
            {
                *exceptions |= F32_EX_INEXACT;
            }
            if (inc)
            {
                unsigned64_add(frac_l, frac_h, 1, 0, &frac_l, &frac_h);
                if ((loss + loss == 0) && (rnd_mode == F32_RND_NEAREST_EVEN))
                {
                    frac_l &= (~0x1);
                }
                else
                {
                    frac_l &= 0xFFFFFFFF;
                }
            }
            else 
            {
                if ((frac_h | frac_l) == 0)
                {
                    exp = 0;
                }
            }
            result = PACK_F64(sign, exp, frac_h, frac_l);
        }
    }
    else 
    {
       if (loss != 0)
        {
            *exceptions |= F32_EX_INEXACT;
        }
        if (inc)
        {
            unsigned64_add(frac_l, frac_h, 1, 0, &frac_l, &frac_h);
            if ((loss + loss == 0) && (rnd_mode == F32_RND_NEAREST_EVEN))
            {
                frac_l &= (~0x1);
            }
            else
            {
                frac_l &= 0xFFFFFFFF;
            }
        }
        else 
        {
            if ((frac_h | frac_l) == 0)
            {
                exp = 0;
            }
        }
        result = PACK_F64(sign, exp, frac_h, frac_l);
    }
    return result;
}

T_F64 f64_add_operands(T_F64 op1, T_F64 op2, unsigned int rnd_mode, unsigned int* exceptions)
{
    T_F64 result;
    unsigned int sign = EXTRACT_F64_SIGN(op1.high);
    int op1_exp, op2_exp, res_exp;
    unsigned int op1_frac_l, op1_frac_h, op2_frac_l, op2_frac_h, res_frac_h, res_frac_l, frac_rem;
    int exp_diff;
    op1_frac_l = op1.low;
    op1_frac_h = EXTRACT_F64_FRAC_H(op1.high);
    op1_exp = EXTRACT_F64_EXP(op1.high);
    op2_frac_l = op2.low;
    op2_frac_h = EXTRACT_F64_FRAC_H(op2.high);
    op2_exp = EXTRACT_F64_EXP(op2.high);
    exp_diff = op1_exp - op2_exp;
    if (exp_diff > 0) 
    {
        if (op1_exp == 0x7FF)
        {
            if (op1_frac_h | op1_frac_l) 
            {
                result = f64_propagate_NaNs(op1, op2, exceptions);
            }
            else
            {
                result = op1;
            }
        }
        else if (op2_exp == 0) 
        {
            exp_diff--;
        }
        else 
        {
            op2_frac_h |= 0x00100000;
        }
        f64_shift_right_96_bits(op2_frac_h, op2_frac_l, 0, exp_diff, &op2_frac_h, &op2_frac_l, &frac_rem);
        res_exp = op1_exp;
        op1_frac_h |= 0x00100000;
        unsigned64_add(op1_frac_h, op1_frac_l, op2_frac_h, op2_frac_l, &res_frac_h, &res_frac_l);
        res_exp--;
        if (res_frac_h < 0x00200000)
        {
            result = f64_pack_round(sign, res_exp, res_frac_h, res_frac_l, frac_rem, rnd_mode, exceptions);
        }
        else
        {
            res_exp++;
            f64_shift_right_96_bits(res_frac_h, res_frac_l, frac_rem, 1, &res_frac_h, &res_frac_l, &frac_rem);
            result = f64_pack_round(sign, res_exp, res_frac_h, res_frac_l, frac_rem, rnd_mode, exceptions);
        }        
    }
    else if (exp_diff < 0) 
    {
        if (op2_exp == 0x7FF)
        {
            if (op2_frac_h | op2_frac_l)
            {
                result = f64_propagate_NaNs(op1, op2, exceptions);
            }
            else
            {
                result = PACK_F64(sign, 0x7FF, 0, 0);
            }
        }
        else 
        {
            if (op1_exp == 0) 
            {
                exp_diff++;
            }
            else 
            {
                op1_frac_h |= 0x00100000;
            }
            f64_shift_right_96_bits(op1_frac_h, op1_frac_l, 0, ((~exp_diff) + 1), &op1_frac_h, &op1_frac_l, &frac_rem);
            res_exp = op2_exp;
            op1_frac_h |= 0x00100000;
            unsigned64_add(op1_frac_l, op1_frac_h, op2_frac_l, op2_frac_h, &res_frac_l, &res_frac_h);
            res_exp--;
            if (res_frac_h < 0x00200000)
            {
                result = f64_pack_round(sign, res_exp, res_frac_h, res_frac_l, frac_rem, rnd_mode, exceptions);
            }
            else
            {
                res_exp++;
                f64_shift_right_96_bits(res_frac_h, res_frac_l, frac_rem, 1, &res_frac_h, &res_frac_l, &frac_rem);
                result = f64_pack_round(sign, res_exp, res_frac_h, res_frac_l, frac_rem, rnd_mode, exceptions);
            }
        }
    }
    else 
    {
        if (op1_exp == 0x7FF) 
        {
            if (op1_frac_h | op1_frac_l | op2_frac_h | op2_frac_l) 
            {
                result = f64_propagate_NaNs(op1, op2, exceptions);
            }
            else
            {
                result = op1;
            }
        }
        else
        {
            unsigned64_add(op1_frac_l, op1_frac_h, op2_frac_l, op2_frac_h, &res_frac_l, &res_frac_h);
            if (op1_exp == 0) 
            {
                result = PACK_F64(sign, 0, res_frac_h, res_frac_l);
            }
            else
            {
                frac_rem = 0;
                res_frac_h |= 0x00200000;
                res_exp = op1_exp;
                f64_shift_right_96_bits(res_frac_h, res_frac_l, frac_rem, 1, &res_frac_h, &res_frac_l, &frac_rem);
                result = f64_pack_round(sign, res_exp, res_frac_h, res_frac_l, frac_rem, rnd_mode, exceptions);
            }
        }
    }
    return result;
}

T_F64 f64_sub_operands(T_F64 op1, T_F64 op2, unsigned int rnd_mode, unsigned int* exceptions)
{
    int op1_exp, op2_exp, res_exp;
    unsigned int sign = EXTRACT_F64_SIGN(op1.high);
    unsigned int op1_frac_h, op1_frac_l, op2_frac_h, op2_frac_l, res_frac_h, res_frac_l, frac_rem;
    int exp_diff;
    T_F64 result;

    op1_frac_l = op1.low;
    op1_frac_h = EXTRACT_F64_FRAC_H(op1.high);
    op1_exp = EXTRACT_F64_EXP(op1.high);
    op2_frac_l = op2.low;
    op2_frac_h = EXTRACT_F64_FRAC_H(op2.high);
    op2_exp = EXTRACT_F64_EXP(op2.high);
    exp_diff = op1_exp - op2_exp;
    f64_shift_left(op1_frac_h, op1_frac_l, 10, &op1_frac_h, &op1_frac_l);
    f64_shift_left(op2_frac_h, op2_frac_l, 10, &op2_frac_h, &op2_frac_l);
    if (exp_diff > 0)
    {
        if (op1_exp == 0x7FF) 
        {
            if (op1_frac_h | op1_frac_l)
            {
                result = f64_propagate_NaNs(op1, op2, exceptions);
            }
            else
            {
                result = op1;
            }
        }
        else
        {
            if (op2_exp == 0) 
            {
                exp_diff--;
            }
            else 
            {
                op2_frac_h |= 0x40000000;
            }
            f64_shift_right_loss_detect(op2_frac_h, op2_frac_l, exp_diff, &op2_frac_h, &op2_frac_l);
            op1_frac_h |= 0x40000000;
            unsigned64_sub(op1_frac_h, op1_frac_l, op2_frac_h, op2_frac_l, &res_frac_h, &res_frac_l);
            res_exp = op1_exp;
            res_exp -= 11;
            f64_normalize_components(&res_exp, &res_frac_h, &res_frac_l, &frac_rem);
            result = f64_pack_round(sign, res_exp, res_frac_h, res_frac_l, frac_rem, rnd_mode, exceptions);
        }
    }
    else if (exp_diff < 0)
    {
        if (op2_exp == 0x7FF)
        {
            if (op2_frac_h | op2_frac_l)
            {
                result = f64_propagate_NaNs(op1, op2, exceptions);
            }
            else
            {
                result = PACK_F64(sign ^ 1, 0x7FF, 0, 0);
            }
        }
        else
        {
            if (op1_exp == 0)
            {
                exp_diff++;
            }
            else 
            {
                op1_frac_h |= 0x40000000;
            }
            f64_shift_right_loss_detect(op1_frac_h, op1_frac_l, (-exp_diff), &op1_frac_h, &op1_frac_l);
            op2_frac_h |= 0x40000000;
            unsigned64_sub(op2_frac_h, op2_frac_l, op1_frac_h, op1_frac_l, &res_frac_h, &res_frac_l);
            res_exp = op2_exp;
            sign ^= 1;
            res_exp -= 11;
            f64_normalize_components(&res_exp, &res_frac_h, &res_frac_l, &frac_rem);
            result = f64_pack_round(sign, res_exp, res_frac_h, res_frac_l, frac_rem, rnd_mode, exceptions);
        }
    }
    else if (op1_exp == 0x7FF) 
    {
        if (op1_frac_h | op1_frac_l | op2_frac_h | op2_frac_l) 
        {
            result = f64_propagate_NaNs(op1, op2, exceptions);
        }
        else
        {
            *exceptions |= F32_EX_INVALID;
            result.low = F64_NAN_DEFAULT_L;
            result.high = F64_NAN_DEFAULT_H;
        }
    }
    else
    {
        if (op1_exp == 0) 
        {
            op1_exp = 1;
            op2_exp = 1;
        }
        if (op2_frac_h < op1_frac_h)
        {
            unsigned64_sub(op1_frac_h, op1_frac_l, op2_frac_h, op2_frac_l, &res_frac_h, &res_frac_l);
            res_exp = op1_exp;
            res_exp -= 11;
            f64_normalize_components(&res_exp, &res_frac_h, &res_frac_l, &frac_rem);
            result = f64_pack_round(sign, res_exp, res_frac_h, res_frac_l, frac_rem, rnd_mode, exceptions);
        }
        else if (op1_frac_h < op2_frac_h)
        {
            unsigned64_sub(op2_frac_h, op2_frac_l, op1_frac_h, op1_frac_l, &res_frac_h, &res_frac_l);
            res_exp = op2_exp;
            sign ^= 1;
            res_exp -= 11;
            f64_normalize_components(&res_exp, &res_frac_h, &res_frac_l, &frac_rem);
            result = f64_pack_round(sign, res_exp, res_frac_h, res_frac_l, frac_rem, rnd_mode, exceptions);
        }
        else if (op2_frac_l < op1_frac_l)
        {
            unsigned64_sub(op1_frac_h, op1_frac_l, op2_frac_h, op2_frac_l, &res_frac_h, &res_frac_l);
            res_exp = op1_exp;
            res_exp -= 11;
            f64_normalize_components(&res_exp, &res_frac_h, &res_frac_l, &frac_rem);
            result = f64_pack_round(sign, res_exp, res_frac_h, res_frac_l, frac_rem, rnd_mode, exceptions);
        }
        else if (op1_frac_l < op2_frac_l)
        {
            unsigned64_sub(op2_frac_h, op2_frac_l, op1_frac_h, op1_frac_l, &res_frac_h, &res_frac_l);
            res_exp = op2_exp;
            sign ^= 1;
            res_exp -= 11;
            f64_normalize_components(&res_exp, &res_frac_h, &res_frac_l, &frac_rem);
            result = f64_pack_round(sign, res_exp, res_frac_h, res_frac_l, frac_rem, rnd_mode, exceptions);
        }
        else
        {
            if (rnd_mode == F32_RND_MINUS_INF)
            {
                result = PACK_F64(1, 0, 0, 0);
            }
            else
            {
                result = PACK_F64(0, 0, 0, 0);
            }
        }
    }
    return result;
}

T_F64 f64_add(T_F64 op1, T_F64 op2, unsigned int rnd_mode, unsigned int* exceptions)
{
    unsigned int op1_sign = EXTRACT_F64_SIGN(op1.high);
    unsigned int op2_sign = EXTRACT_F64_SIGN(op2.high);
    if (op1_sign == op2_sign) 
    {
        return f64_add_operands(op1, op2, rnd_mode, exceptions);
    }
    else 
    {
        return f64_sub_operands(op1, op2, rnd_mode, exceptions);
    }
}

T_F64 f64_sub(T_F64 op1, T_F64 op2, unsigned int rnd_mode, unsigned int* exceptions)
{
    unsigned int op1_sign = EXTRACT_F64_SIGN(op1.high);
    unsigned int op2_sign = EXTRACT_F64_SIGN(op2.high);
    if (op1_sign == op2_sign) 
    {
        return f64_sub_operands(op1, op2, rnd_mode, exceptions);
    }
    else
    {
        return f64_add_operands(op1, op2, rnd_mode, exceptions);
    }
}

