/**
File: vectorTypes.h
Author: Peter Barnum
Created: 2014-05-23

Vector types for SIMD processing.

The internal structure should be bit-compatible with most SIMD processors. Also, basic operations should work with most OpenCL-compatible compilers.

lo and hi are for the low and high halves.
v# are the elements, from least-significant to most-significant
bits is the raw bits, and is mainly useful for reinterpret casting the values to different types

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef ANKICORETECH_COMMON_VECTOR_TYPES_H_
#define ANKICORETECH_COMMON_VECTOR_TYPES_H_

#include "types.h"

struct u8x2;
struct u8x4;
struct u8x8;
struct u8x16;
struct s8x2;
struct s8x4;
struct s8x8;
struct s8x16;
struct u16x2;
struct u16x4;
struct u16x8;
struct s16x2;
struct s16x4;
struct s16x8;
struct u32x2;
struct u32x4;
struct s32x2;
struct s32x4;
struct u64x2;
struct s64x2;
struct f32x2;
struct f32x4;
struct f64x2;

struct register32;
struct register64;
struct register128;

typedef struct u8x2
{
  union
  {
    struct { u8 lo, hi; };
    struct { u8 v0, v1; };
    u8 v[2];
    u16 bits;
  };

  //u8x2() {}
  //u8x2(const u8 v0, const u8 v1) : v0(v0), v1(v1) {}
  //u8x2(const u32 bits) : bits(bits) {}

  inline operator u16&() { return bits; }
  inline operator const u16&() const { return bits; }
  inline u8x2& operator =(const u16 bits) { this->bits = bits; return *this; }
} u8x2;

typedef struct u8x4
{
  union
  {
    struct { u8x2 lo, hi; };
    struct { u8 v0, v1, v2, v3; };
    u8 v[4];
    u32 bits;
  };

  //u8x4() {}
  //u8x4(const u8 v0, const u8 v1, const u8 v2, const u8 v3) : v0(v0), v1(v1), v2(v2), v3(v3) {}
  //u8x4(const u32 bits) : bits(bits) {}

  inline operator u32&() { return bits; }
  inline operator const u32&() const { return bits; }
  inline u8x4& operator =(const u32 bits) { this->bits = bits; return *this; }
} u8x4;

typedef struct u8x8
{
  union
  {
    struct { u8x4 lo, hi; };
    struct { u8 v0, v1, v2, v3, v4, v5, v6, v7, v8; };
    u8 v[8];
    u64 bits;
  };

  //u8x8() {}
  //u8x8(const u8 v0, const u8 v1, const u8 v2, const u8 v3, const u8 v4, const u8 v5, const u8 v6, const u8 v7) : v0(v0), v1(v1), v2(v2), v3(v3), v4(v4), v5(v5), v6(v6), v7(v7) {}
  //u8x8(const u64 bits) : bits(bits) {}

  inline operator u64&() { return bits; }
  inline operator const u64&() const { return bits; }
  inline u8x8& operator =(const u64 bits) { this->bits = bits; return *this; }
} u8x8;

typedef struct u8x16
{
  union
  {
    struct { u8x8 lo, hi; };
    struct { u8 v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, va, vb, vc, vd, ve, vf; };
    u8 v[16];
    u64 bits[2];
  };

  //u8x16() {}
  //u8x16(const u8 v0, const u8 v1, const u8 v2, const u8 v3, const u8 v4, const u8 v5, const u8 v6, const u8 v7, const u8 v8, const u8 v9, const u8 va, const u8 vb, const u8 vc, const u8 vd, const u8 ve, const u8 vf) : v0(v0), v1(v1), v2(v2), v3(v3), v4(v4), v5(v5), v6(v6), v7(v7), v8(v8), v9(v9), va(va), vb(vb), vc(vc), vd(vd), ve(ve), vf(vf) {}
  //u8x16(const u64 bits[2]) { this->bits[0]=bits[0]; this->bits[1]=bits[1]; }

  inline u8x16& operator =(const u64 bits[2]) { this->bits[0]=bits[0]; this->bits[1]=bits[1]; return *this; }
} u8x16;

typedef struct s8x2
{
  union
  {
    struct { s8 lo, hi; };
    struct { s8 v0, v1; };
    s8 v[2];
    u16 bits;
  };

  //s8x2() {}
  //s8x2(const s8 v0, const s8 v1) : v0(v0), v1(v1) {}
  //s8x2(const u32 bits) : bits(bits) {}

  inline operator u16&() { return bits; }
  inline operator const u16&() const { return bits; }
  inline s8x2& operator =(const u16 bits) { this->bits = bits; return *this; }
} s8x2;

typedef struct s8x4
{
  union
  {
    struct { s8x2 lo, hi; };
    struct { s8 v0, v1, v2, v3; };
    s8 v[4];
    u32 bits;
  };

  //s8x4() {}
  //s8x4(const s8 v0, const s8 v1, const s8 v2, const s8 v3) : v0(v0), v1(v1), v2(v2), v3(v3) {}
  //s8x4(const u32 bits) : bits(bits) {}

  inline operator u32&() { return bits; }
  inline operator const u32&() const { return bits; }
  inline s8x4& operator =(const u32 bits) { this->bits = bits; return *this; }
} s8x4;

typedef struct s8x8
{
  union
  {
    struct { s8x4 lo, hi; };
    struct { s8 v0, v1, v2, v3, v4, v5, v6, v7, v8; };
    s8 v[8];
    u64 bits;
  };

  //s8x8() {}
  //s8x8(const s8 v0, const s8 v1, const s8 v2, const s8 v3, const s8 v4, const s8 v5, const s8 v6, const s8 v7) : v0(v0), v1(v1), v2(v2), v3(v3), v4(v4), v5(v5), v6(v6), v7(v7) {}
  //s8x8(const u64 bits) : bits(bits) {}

  inline operator u64&() { return bits; }
  inline operator const u64&() const { return bits; }
  inline s8x8& operator =(const u64 bits) { this->bits = bits; return *this; }
} s8x8;

typedef struct s8x16
{
  union
  {
    struct { s8x8 lo, hi; };
    struct { s8 v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, va, vb, vc, vd, ve, vf; };
    s8 v[16];
    u64 bits[2];
  };

  //s8x16() {}
  //s8x16(const s8 v0, const s8 v1, const s8 v2, const s8 v3, const s8 v4, const s8 v5, const s8 v6, const s8 v7, const s8 v8, const s8 v9, const s8 va, const s8 vb, const s8 vc, const s8 vd, const s8 ve, const s8 vf) : v0(v0), v1(v1), v2(v2), v3(v3), v4(v4), v5(v5), v6(v6), v7(v7), v8(v8), v9(v9), va(va), vb(vb), vc(vc), vd(vd), ve(ve), vf(vf) {}
  //s8x16(const u64 bits[2]) { this->bits[0]=bits[0]; this->bits[1]=bits[1]; }

  inline s8x16& operator =(const u64 bits[2]) { this->bits[0]=bits[0]; this->bits[1]=bits[1]; return *this; }
} s8x16;

typedef struct u16x2
{
  union
  {
    struct { u16 lo, hi; };
    struct { u16 v0, v1; };
    u16 v[2];
    u32 bits;
  };

  //u16x2() {}
  //u16x2(const u16 v0, const u16 v1) : v0(v0), v1(v1) {}
  //u16x2(const u32 bits) : bits(bits) {}

  inline operator u32&() { return bits; }
  inline operator const u32&() const { return bits; }
  inline u16x2& operator =(const u32 bits) { this->bits = bits; return *this; }
} u16x2;

typedef struct u16x4
{
  union
  {
    struct { u16x2 lo, hi; };
    struct { u16 v0, v1, v2, v3; };
    u16 v[4];
    u64 bits;
  };

  //u16x4() {}
  //u16x4(const u16 v0, const u16 v1, const u16 v2, const u16 v3) : v0(v0), v1(v1), v2(v2), v3(v3) {}
  //u16x4(const u64 bits) : bits(bits) {}

  inline operator u64&() { return bits; }
  inline operator const u64&() const { return bits; }
  inline u16x4& operator =(const u64 bits) { this->bits = bits; return *this; }
} u16x4;

typedef struct u16x8
{
  union
  {
    struct { u16x4 lo, hi; };
    struct { u16 v0, v1, v2, v3, v4, v5, v6, v7, v8; };
    u16 v[8];
    u64 bits[2];
  };

  //u16x8() {}
  //u16x8(const u16 v0, const u16 v1, const u16 v2, const u16 v3, const u16 v4, const u16 v5, const u16 v6, const u16 v7) : v0(v0), v1(v1), v2(v2), v3(v3), v4(v4), v5(v5), v6(v6), v7(v7) {}
  //u16x8(const u64 bits[2]) { this->bits[0]=bits[0]; this->bits[1]=bits[1]; }

  inline u16x8& operator =(const u64 bits[2]) { this->bits[0]=bits[0]; this->bits[1]=bits[1]; return *this; }
} u16x8;

typedef struct s16x2
{
  union
  {
    struct { s16 lo, hi; };
    struct { s16 v0, v1; };
    s16 v[2];
    u32 bits;
  };

  //s16x2() {}
  //s16x2(const s16 v0, const s16 v1) : v0(v0), v1(v1) {}
  //s16x2(const u32 bits) : bits(bits) {}

  inline operator u32&() { return bits; }
  inline operator const u32&() const { return bits; }
  inline s16x2& operator =(const u32 bits) { this->bits = bits; return *this; }
} s16x2;

typedef struct s16x4
{
  union
  {
    struct { s16x2 lo, hi; };
    struct { s16 v0, v1, v2, v3; };
    s16 v[4];
    u64 bits;
  };

  //s16x4() {}
  //s16x4(const s16 v0, const s16 v1, const s16 v2, const s16 v3) : v0(v0), v1(v1), v2(v2), v3(v3) {}
  //s16x4(const u64 bits) : bits(bits) {}

  inline operator u64&() { return bits; }
  inline operator const u64&() const { return bits; }
  inline s16x4& operator =(const u64 bits) { this->bits = bits; return *this; }
} s16x4;

typedef struct s16x8
{
  union
  {
    struct { s16x4 lo, hi; };
    struct { s16 v0, v1, v2, v3, v4, v5, v6, v7, v8; };
    s16 v[8];
    u64 bits[2];
  };

  //s16x8() {}
  //s16x8(const s16 v0, const s16 v1, const s16 v2, const s16 v3, const s16 v4, const s16 v5, const s16 v6, const s16 v7) : v0(v0), v1(v1), v2(v2), v3(v3), v4(v4), v5(v5), v6(v6), v7(v7) {}
  //s16x8(const u64 bits[2]) { this->bits[0]=bits[0]; this->bits[1]=bits[1]; }

  inline s16x8& operator =(const u64 bits[2]) { this->bits[0] = bits[0]; this->bits[1] = bits[1]; return *this; }
} s16x8;

typedef struct u32x2
{
  union
  {
    struct { u32 lo, hi; };
    struct { u32 v0, v1; };
    u32 v[2];
    u64 bits;
  };

  //u32x2() {}
  //u32x2(const u32 v0, const u32 v1) : v0(v0), v1(v1) {}
  //u32x2(const u64 bits) : bits(bits) {}

  inline operator u64&() { return bits; }
  inline operator const u64&() const { return bits; }
  inline u32x2& operator =(const u64 bits) { this->bits = bits; return *this; }
} u32x2;

typedef struct u32x4
{
  union
  {
    struct { u32x2 lo, hi; };
    struct { u32 v0, v1, v2, v3; };
    u32 v[4];
    u64 bits[2];
  };

  //u32x4() {}
  //u32x4(const u32 v0, const u32 v1, const u32 v2, const u32 v3) : v0(v0), v1(v1), v2(v2), v3(v3) {}
  //u32x4(const u64 bits[2]) { this->bits[0]=bits[0]; this->bits[1]=bits[1]; }

  inline u32x4& operator =(const u64 bits[2]) { this->bits[0] = bits[0]; this->bits[1] = bits[1]; return *this; }
} u32x4;

typedef struct s32x2
{
  union
  {
    struct { s32 lo, hi; };
    struct { s32 v0, v1; };
    s32 v[2];
    u64 bits;
  };

  //s32x2() {}
  //s32x2(const s32 v0, const s32 v1) : v0(v0), v1(v1) {}
  //s32x2(const u64 bits) : bits(bits) {}

  inline operator u64&() { return bits; }
  inline operator const u64&() const { return bits; }
  inline s32x2& operator =(const u64 bits) { this->bits = bits; return *this; }
} s32x2;

typedef struct s32x4
{
  union
  {
    struct { s32x2 lo, hi; };
    struct { s32 v0, v1, v2, v3; };
    s32 v[4];
    u64 bits[2];
  };

  //s32x4() {}
  //s32x4(const s32 v0, const s32 v1, const s32 v2, const s32 v3) : v0(v0), v1(v1), v2(v2), v3(v3) {}
  //s32x4(const u64 bits[2]) { this->bits[0]=bits[0]; this->bits[1]=bits[1]; }

  inline s32x4& operator =(const u64 bits[2]) { this->bits[0] = bits[0]; this->bits[1] = bits[1]; return *this; }
} s32x4;

typedef struct u64x2
{
  union
  {
    struct { u64 lo, hi; };
    struct { u64 v0, v1; };
    u64 v[2];
    u64 bits[2];
  };

  //u64x2() {}
  //u64x2(const u64 v0, const u64 v1) : v0(v0), v1(v1) {}
  //u64x2(const u64 bits[2]) { this->bits[0]=bits[0]; this->bits[1]=bits[1]; }

  inline u64x2& operator =(const u64 bits[2]) { this->bits[0] = bits[0]; this->bits[1] = bits[1]; return *this; }
} u64x2;

typedef struct s64x2
{
  union
  {
    struct { s64 lo, hi; };
    struct { s64 v0, v1; };
    s64 v[2];
    u64 bits[2];
  };

  //s64x2() {}
  //s64x2(const s64 v0, const s64 v1) : v0(v0), v1(v1) {}
  //s64x2(const u64 bits[2]) { this->bits[0]=bits[0]; this->bits[1]=bits[1]; }

  inline s64x2& operator =(const u64 bits[2]) { this->bits[0] = bits[0]; this->bits[1] = bits[1]; return *this; }
} s64x2;

typedef struct f32x2
{
  union
  {
    struct { f32 lo, hi; };
    struct { f32 v0, v1; };
    f32 v[2];
    u64 bits;
  };

  //f32x2() {}
  //f32x2(const f32 v0, const f32 v1) : v0(v0), v1(v1) {}
  //f32x2(const u64 bits) : bits(bits) {}

  inline operator u64&() { return bits; }
  inline operator const u64&() const { return bits; }
  inline f32x2& operator =(const u64 bits) { this->bits = bits; return *this; }
} f32x2;

typedef struct f32x4
{
  union
  {
    struct { f32x2 lo, hi; };
    struct { f32 v0, v1, v2, v3; };
    f32 v[4];
    u64 bits[2];
  };

  //f32x4() {}
  //f32x4(const f32 v0, const f32 v1, const f32 v2, const f32 v3) : v0(v0), v1(v1), v2(v2), v3(v3) {}
  //f32x4(const u64 bits[2]) { this->bits[0]=bits[0]; this->bits[1]=bits[1]; }

  inline f32x4& operator =(const u64 bits[2]) { this->bits[0] = bits[0]; this->bits[1] = bits[1]; return *this; }
} f32x4;

typedef struct f64x2
{
  union
  {
    struct { f64 lo, hi; };
    struct { f64 v0, v1; };
    f64 v[2];
    u64 bits[2];
  };

  //f64x2() {}
  //f64x2(const f64 v0, const f64 v1) : v0(v0), v1(v1) {}
  //f64x2(const u64 bits[2]) { this->bits[0]=bits[0]; this->bits[1]=bits[1]; }

  inline f64x2& operator =(const u64 bits[2]) { this->bits[0] = bits[0]; this->bits[1] = bits[1]; return *this; }
} f64x2;

typedef struct register32
{
  union
  {
    u8x4  _u8x4;
    u16x2 _u16x2;
    u32   _u32;

    s8x4  _s8x4;
    s16x2 _s16x2;
    s32   _s32;

    f32   _f32;
  };

  register32() {}
  register32(const u32 bits) : _u32(bits) {}

  inline operator u32&() { return _u32; }
  inline operator const u32&() const { return _u32; }
  inline register32& operator =(const u32 bits) { this->_u32 = bits; return *this; }
} register32;

typedef struct register64
{
  union
  {
    u8x8  _u8x8;
    u16x4 _u16x4;
    u32x2 _u32x2;
    u64   _u64;

    s8x8  _s8x8;
    s16x4 _s16x4;
    s32x2 _s32x2;
    s64   _s64;

    f32x2 _f32x2;
    f64   _f64;
  };

  register64() {}
  register64(const u64 bits) : _u64(bits) {}

  inline operator u64&() { return _u64; }
  inline operator const u64&() const { return _u64; }
  inline register64& operator =(const u64 bits) { this->_u64 = bits; return *this; }
} register64;

typedef struct register128
{
  union
  {
    u8x16 _u8x16;
    u16x8 _u16x8;
    u32x4 _u32x4;
    u64x2 _u64x2;

    s8x16 _s8x16;
    s16x8 _s16x8;
    s32x4 _s32x4;
    s64x2 _s64x2;

    f32x4 _f32x4;
    f64x2 _f64x2;
  };

  register128() {}
  register128(const u64 bits[2]) { this->_u64x2.bits[0] = bits[0]; this->_u64x2.bits[1] = bits[1]; }

  inline register128& operator =(const u64 bits[2]) { this->_u64x2.bits[0] = bits[0]; this->_u64x2.bits[1] = bits[1]; return *this; }
} register128;

#endif // ANKICORETECH_COMMON_VECTOR_TYPES_H_
