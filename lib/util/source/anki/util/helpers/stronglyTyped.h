/**
 * File: stronglyTyped.h
 *
 * Author: ross
 * Created: Jun 9 2018
 *
 * Description: Helper to generate (mostly) strongly-typed wrappers around POD types.
 *              Example:
 *                typedef StronglyTyped<uint32_t, struct TypeIdentifierA> TypeA_t;
 *                typedef StronglyTyped<uint32_t, struct TypeIdentifierB> TypeB_t;
 *              This names TypeA_t and TypeB_t as strongly-typed uint32_t's. A TypeA_t
 *              can operate on other TypeA_t's, and can even operate on a uint32_t (hence
 *              being only "mostly" strongly-typed). But it cannot operate on a TypeB_t.
 *
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Util_Helpers_StronglyTyped_H__
#define __Util_Helpers_StronglyTyped_H__
#pragma once

#include <ostream>
#include <type_traits>

namespace Anki{
namespace Util {

// T: underlying type
// Identifier: a unique, strongly-typed identifier
template <typename T, typename Identifier>
class StronglyTyped
{
public:
  using Underlying_t = T;
  
  // default construction
  StronglyTyped() {}
  
  // implicit construction from underlying type
  StronglyTyped( const T& value )
    : _value( value )
  {
    static_assert( std::is_pod<T>::value, "This class is not built for non-POD types" );
  }
  
  // implicit copy construction
  StronglyTyped( const StronglyTyped& other )
    : StronglyTyped( other._value )
  { }
  
  // casting is explicit to avoid the implicit conversion of StronglyTyped<T,A> ==> T ==> StronglyTyped<T,B>
  explicit operator const T&() const { return _value; }
  explicit operator float() const { return (float)_value; }
  
  // assignment
  StronglyTyped& operator =( const T& value ) { _value = value; return *this; }
  StronglyTyped& operator =( const StronglyTyped& other ) { _value = other._value; return *this; }
  
  // boolean operators
  
  bool operator ==( const T& value ) const { return _value == value; }
  bool operator !=( const T& value ) const { return _value != value; }
  bool operator  >( const T& value ) const { return _value  > value; }
  bool operator  <( const T& value ) const { return _value  < value; }
  bool operator >=( const T& value ) const { return _value >= value; }
  bool operator <=( const T& value ) const { return _value <= value; }
  
  bool operator ==( const StronglyTyped& other ) const { return _value == other._value; }
  bool operator !=( const StronglyTyped& other ) const { return _value != other._value; }
  bool operator  >( const StronglyTyped& other ) const { return _value  > other._value; }
  bool operator  <( const StronglyTyped& other ) const { return _value  < other._value; }
  bool operator >=( const StronglyTyped& other ) const { return _value >= other._value; }
  bool operator <=( const StronglyTyped& other ) const { return _value <= other._value; }
  
  friend bool operator ==( const T& value, const StronglyTyped& other ) { return value == other._value; }
  friend bool operator !=( const T& value, const StronglyTyped& other ) { return value != other._value; }
  friend bool operator  >( const T& value, const StronglyTyped& other ) { return value  > other._value; }
  friend bool operator  <( const T& value, const StronglyTyped& other ) { return value  < other._value; }
  friend bool operator >=( const T& value, const StronglyTyped& other ) { return value >= other._value; }
  friend bool operator <=( const T& value, const StronglyTyped& other ) { return value <= other._value; }
  
  // arithmetic operators
  
  StronglyTyped operator -() const { return StronglyTyped{ -_value }; }
  StronglyTyped operator +( const T& value ) const { return StronglyTyped{ _value + value }; }
  StronglyTyped operator -( const T& value ) const { return StronglyTyped{ _value - value }; }
  StronglyTyped operator *( const T& value ) const { return StronglyTyped{ _value * value }; }
  StronglyTyped operator /( const T& value ) const { return StronglyTyped{ _value / value }; }
  StronglyTyped operator %( const T& value ) const { return StronglyTyped{ _value % value }; }
  
  StronglyTyped operator +( const StronglyTyped& other ) const { return StronglyTyped{ _value + other._value }; }
  StronglyTyped operator -( const StronglyTyped& other ) const { return StronglyTyped{ _value - other._value }; }
  StronglyTyped operator *( const StronglyTyped& other ) const { return StronglyTyped{ _value * other._value }; }
  StronglyTyped operator /( const StronglyTyped& other ) const { return StronglyTyped{ _value / other._value }; }
  StronglyTyped operator %( const StronglyTyped& other ) const { return StronglyTyped{ _value % other._value }; }
  
  friend StronglyTyped operator +( const T& value, const StronglyTyped& other ) { return StronglyTyped{ value + other._value }; }
  friend StronglyTyped operator -( const T& value, const StronglyTyped& other ) { return StronglyTyped{ value - other._value }; }
  friend StronglyTyped operator *( const T& value, const StronglyTyped& other ) { return StronglyTyped{ value * other._value }; }
  friend StronglyTyped operator /( const T& value, const StronglyTyped& other ) { return StronglyTyped{ value / other._value }; }
  friend StronglyTyped operator %( const T& value, const StronglyTyped& other ) { return StronglyTyped{ value % other._value }; }
  
  StronglyTyped& operator ++() { ++_value; return *this; }
  StronglyTyped& operator --() { --_value; return *this; }
  
  StronglyTyped& operator +=( const T& value ) { _value += value; return *this; }
  StronglyTyped& operator -=( const T& value ) { _value -= value; return *this; }
  StronglyTyped& operator *=( const T& value ) { _value *= value; return *this; }
  StronglyTyped& operator /=( const T& value ) { _value /= value; return *this; }
  
  StronglyTyped& operator +=( const StronglyTyped& other ) { _value += other._value; return *this; }
  StronglyTyped& operator -=( const StronglyTyped& other ) { _value -= other._value; return *this; }
  StronglyTyped& operator *=( const StronglyTyped& other ) { _value *= other._value; return *this; }
  StronglyTyped& operator /=( const StronglyTyped& other ) { _value /= other._value; return *this; }
  
  
  friend std::ostream& operator <<(std::ostream& os, const StronglyTyped& other) { return os << other._value; }
  
private:
  T _value;
};
  
} // namespace
} // namespace

#endif // __Util_Helpers_StronglyTyped_H__
