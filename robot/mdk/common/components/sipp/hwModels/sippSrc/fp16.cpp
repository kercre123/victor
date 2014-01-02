// -----------------------------------------------------------------------------
// Copyright (C) 2012 Movidius Ltd. All rights reserved
//
// Company          : Movidius
// Author           : Razvan Delibasa (razvan.delibasa@movidius.com)
// Description      : FP16 implementation
// -----------------------------------------------------------------------------

#include <iostream>
#include <math.h>
#include <stdlib.h>
#include "fp16.h"


using namespace std;

unsigned int excep;

// ############
// Constructors
// ############

fp16::fp16()
{
  _h = 0;
}

fp16::fp16(float v)
{
  unsigned int f_val = *(unsigned int *)&v;
  _h = (unsigned short)f32_to_f16_conv(f_val, F32_RND_TO_ZERO, &excep);
}

fp16::fp16(double v)
{
  float f = (float)v;
  unsigned int f_val = *(unsigned int *)&f;
  _h = (unsigned short)f32_to_f16_conv(f_val, F32_RND_TO_ZERO, &excep);
}

// ############
// Destructor
// ############

fp16::~fp16()
{
}

// #####################################
// General purpose set and get functions
// #####################################

void
fp16::setUnpackedValue(float v)
{
  unsigned int f_val = *(unsigned int *)&v;
  _h = (unsigned short)f32_to_f16_conv(f_val, F32_RND_TO_ZERO, &excep);
}

void
fp16::setPackedValue(unsigned short v)
{
  _h = v;
}

float
fp16::getUnpackedValue()
{
  float f;
  unsigned int f_val = f16_to_f32_conv(_h, &excep);
  f = *(float *)&f_val;
  return f;
}

unsigned short
fp16::getPackedValue()
{
  return _h;
}

// #######################################
// Auxiliary swap function for assignments
// #######################################

void
swap(fp16& first, fp16& second)
{
  using std::swap;

  swap(first._h, second._h);
}

// ####################
// Assignment operators
// ####################

fp16 &
fp16::operator=(fp16 rhs)
{
  HANDLE_NEGZERO(rhs);
  swap(*this, rhs);
  return *this;
}

fp16 &
fp16::operator=(float rhs)
{
  fp16 temp(rhs);
  HANDLE_NEGZERO(temp);
  swap(*this, temp);
  return *this;
}

//fp16 &
//fp16::operator=(double rhs)
//{
//  fp16 temp(rhs);
//  swap(*this, temp);
//  return *this;
//}

// ###########################
// Binary comparison operators
// ###########################

bool
fp16::operator == (const fp16 rhs)
{
  unsigned int op1, op2;
  float f1, f2;
  op1 = f16_to_f32_conv(this->_h, &excep);
  op2 = f16_to_f32_conv(rhs._h, &excep);
  f1 = *(float *)&op1;
  f2 = *(float *)&op2;
  return f1 == f2;
}

bool
fp16::operator != (const fp16 rhs)
{
  return !(*this == rhs);
}

bool
fp16::operator >  (const fp16 rhs)
{
  unsigned int op1, op2;
  float f1, f2;
  op1 = f16_to_f32_conv(this->_h, &excep);
  op2 = f16_to_f32_conv(rhs._h, &excep);
  f1 = *(float *)&op1;
  f2 = *(float *)&op2;
  return f1 > f2;
}

bool
fp16::operator <  (const fp16 rhs)
{
  unsigned int op1, op2;
  float f1, f2;
  op1 = f16_to_f32_conv(this->_h, &excep);
  op2 = f16_to_f32_conv(rhs._h, &excep);
  f1 = *(float *)&op1;
  f2 = *(float *)&op2;
  return f1 < f2;
}

bool
fp16::operator >= (const fp16 rhs)
{
  return !(*this < rhs);
}

bool
fp16::operator <= (const fp16 rhs)
{
  return !(*this > rhs);
}

bool
fp16::operator == (const float rhs)
{
  unsigned int op1;
  float f1;
  op1 = f16_to_f32_conv(this->_h, &excep);
  f1 = *(float *)&op1;
  return f1 == rhs;
}

bool
fp16::operator != (const float rhs)
{
  return !(*this == rhs);
}

bool
fp16::operator >  (const float rhs)
{
  unsigned int op1;
  float f1;
  op1 = f16_to_f32_conv(this->_h, &excep);
  f1 = *(float *)&op1;
  return f1 > rhs;
}

bool
fp16::operator <  (const float rhs)
{
  unsigned int op1;
  float f1;
  op1 = f16_to_f32_conv(this->_h, &excep);
  f1 = *(float *)&op1;
  return f1 < rhs;
}

bool
fp16::operator >= (const float rhs)
{
  return !(*this < rhs);
}

bool
fp16::operator <= (const float rhs)
{
  return !(*this > rhs);
}

// #############################
// Compound assignment operators
// #############################

fp16 &
fp16::operator += (const fp16 rhs)
{
  unsigned int op1, op2, res;
  op1 = f16_to_f32_conv(this->_h, &excep);
  op2 = f16_to_f32_conv(rhs._h, &excep);
  res = f32_add(op1, op2, F32_RND_TO_ZERO, &excep);
  this->_h = (unsigned short)f32_to_f16_conv(res, F32_RND_TO_ZERO, &excep);
  return *this;
}

fp16 &
fp16::operator -= (const fp16 rhs)
{
  unsigned int op1, op2, res;
  op1 = f16_to_f32_conv(this->_h, &excep);
  op2 = f16_to_f32_conv(rhs._h, &excep);
  res = f32_sub(op1, op2, F32_RND_TO_ZERO, &excep);
  this->_h = (unsigned short)f32_to_f16_conv(res, F32_RND_TO_ZERO, &excep);
  return *this;
}

fp16 &
fp16::operator *= (const fp16 rhs)
{
  unsigned int op1, op2, res;
  op1 = f16_to_f32_conv(this->_h, &excep);
  op2 = f16_to_f32_conv(rhs._h, &excep);
  res = f32_mul(op1, op2, F32_RND_TO_ZERO, &excep);
  this->_h = (unsigned short)f32_to_f16_conv(res, F32_RND_TO_ZERO, &excep);
  return *this;
}

fp16 &
fp16::operator /= (const fp16 rhs)
{
  unsigned int op1, op2, res;
  op1 = f16_to_f32_conv(this->_h, &excep);
  op2 = f16_to_f32_conv(rhs._h, &excep);
  res = f32_div(op1, op2, F32_RND_TO_ZERO, &excep);
  this->_h = (unsigned short)f32_to_f16_conv(res, F32_RND_TO_ZERO, &excep);
  return *this;
}

fp16 &
fp16::operator += (const float rhs)
{
  fp16 op2(rhs);
  *this += op2;
  return *this;
}

fp16 &
fp16::operator -= (const float rhs)
{
  fp16 op2(rhs);
  *this -= op2;
  return *this;
}

fp16 &
fp16::operator *= (const float rhs)
{
  fp16 op2(rhs);
  *this *= op2;
  return *this;
}

fp16 &
fp16::operator /= (const float rhs)
{
  fp16 op2(rhs);
  *this /= op2;
  return *this;
}

// ###########################
// Binary Arithmetic Operators
// ###########################

/*fp16 fp16::operator+ (const fp16 rhs)
{
  fp16 res = *this;
  res += rhs;
  return res;
}

fp16 fp16::operator- (const fp16 rhs)
{
  fp16 res = *this;
  res -= rhs;
  return res;
}

fp16 fp16::operator* (const fp16 rhs)
{
  fp16 res = *this;
  res *= rhs;
  return res;
}

fp16 fp16::operator/ (const fp16 rhs)
{
  fp16 res = *this;
  res /= rhs;
  return res;
}

fp16 fp16::operator+ (const float rhs)
{
  fp16 res = *this;
  res += rhs;
  return res;
}

fp16 fp16::operator- (const float rhs)
{
  fp16 res = *this;
  res -= rhs;
  return res;
}

fp16 fp16::operator* (const float rhs)
{
  fp16 res = *this;
  res *= rhs;
  return res;
}

fp16 fp16::operator/ (const float rhs)
{
  fp16 res = *this;
  res /= rhs;
  return res;
}*/

// #############
// I/O operators
// #############

ostream &
operator << (ostream &os, fp16 h)
{
    os << float(h);
    return os;
}

istream &
operator >> (istream &is, fp16 &h)
{
    float f;
    is >> f;
    h = fp16(f);
    return is;
}

// #############
// Cast operator
// #############

fp16::operator float ()
{
  unsigned int f_val = f16_to_f32_conv(_h, &excep);
  float f = *(float *)&f_val;
  return f;
}

// ##############
// Unary operator
// ##############

fp16 fp16::operator - () const
{
  fp16 tmp;
  tmp._h = _h ^ 0x8000;
  return tmp;
}

// #######################
// Absolute value operator
// #######################

float fp16abs(fp16 no)
{
  unsigned int f_val = f16_to_f32_conv(no._h, &excep);
  float f = *(float *)&f_val;
  return fabs(f);
}
