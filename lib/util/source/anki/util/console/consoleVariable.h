/*******************************************************************************************************************************
 *
 *  consoleVariable.h
 *  Anki::Util
 *
 *  Created by Jarrod Hatfield on 4/7/14.
 *  Copyright (c) 2014 Anki. All rights reserved.
 *
 *  Description:
 *  - A Console Variable is a variable the can be registered with the ConsoleInterface and be edited at runtime via a remote
 *    console/terminal.
 *  - Console Variables will be const in shipping builds (if desired)
 *
 *******************************************************************************************************************************/

#ifndef ANKIUTIL_CONSOLE_VARIABLE
#define ANKIUTIL_CONSOLE_VARIABLE

#include "util/helpers/includeSstream.h"
#include "util/math/math.h"
#include "util/math/numericCast.h"
#include <string>
#include <assert.h>

namespace Anki{ namespace Util {


//******************************************************************************************************************************
class IConsoleVariable
{
public:
  IConsoleVariable( const char* id, const char* category );
  virtual ~IConsoleVariable() {};
  
  const std::string& GetID() const { return id_; }
  const std::string& GetCategory() const { return category_; }
  const std::string GetFullPathString() const { return ( category_ + "." + id_ ); }
  
  virtual bool ParseText( const char* text ) = 0;
  
  virtual std::string ToString() const = 0;
  virtual std::string GetDefaultAsString() const = 0;
  
  virtual double    GetAsDouble() const = 0;
  virtual int64_t   GetAsInt64()  const = 0;
  virtual uint64_t  GetAsUInt64() const = 0;
  
  virtual double    GetMinAsDouble()  const = 0;
  virtual double    GetMaxAsDouble()  const = 0;
  
  virtual void ToggleValue() = 0;
  virtual void ResetToDefault() = 0;
  virtual bool IsDefaultValue() const = 0;
  
  virtual bool IsToggleable() const = 0;
  virtual bool IsIntegerType() const = 0; // i.e. not a floating point type
  virtual bool IsSignedType() const = 0;
  
protected:
  std::string id_;
  std::string category_;
  
  IConsoleVariable();
  IConsoleVariable( const IConsoleVariable& );
};


//==============================================================================================================================
// Our templated wrapper/decorator for our console vars.
template <typename T>
class ConsoleVar : public IConsoleVariable
{
public:
  ConsoleVar( T& value, const char* id, const char* category )
    : IConsoleVariable( id, category )
    , _value( value )
    , _minValue(std::numeric_limits<T>::lowest())
    , _maxValue(std::numeric_limits<T>::max())
    , _defaultValue( value )
  { }
  
  ConsoleVar( T& value, const char* id, const char* category, const T& minValue, const T& maxValue )
    : IConsoleVariable( id, category )
    , _value( value )
    , _minValue( minValue )
    , _maxValue( maxValue )
    , _defaultValue( value )
  {
  }
  
  //----------------------------------------------------------------------------------------------------------------------------
  // These should implemented by each specialization;
  virtual bool ParseText( const char* text ) override;
  virtual std::string ToString() const override { return ToString( _value ); }
  virtual std::string GetDefaultAsString() const override { return ToString( _defaultValue ); }

  virtual double    GetAsDouble() const override { return numeric_cast_clamped<double>(_value);  }
  virtual int64_t   GetAsInt64()  const override { return numeric_cast_clamped<int64_t>(_value); }
  virtual uint64_t  GetAsUInt64() const override { return numeric_cast_clamped<uint64_t>(_value); }
  
  virtual double    GetMinAsDouble()  const override { return numeric_cast_clamped<double>(_minValue);  }
  virtual double    GetMaxAsDouble()  const override { return numeric_cast_clamped<double>(_maxValue);  }
  
  virtual void ToggleValue() override { _value = !((bool)_value); }
  virtual void ResetToDefault() override { _value = _defaultValue; }
  virtual bool IsDefaultValue() const override { return (_value == _defaultValue); }
  
  virtual bool IsToggleable()  const override { return (std::numeric_limits<T>::digits == 1); }
  virtual bool IsIntegerType() const override { return (std::numeric_limits<T>::is_integer);  }
  virtual bool IsSignedType()  const override { return (std::numeric_limits<T>::is_signed);   }

protected:
  std::string ToString( T value ) const;
  
  //----------------------------------------------------------------------------------------------------------------------------
protected:
  T& _value;
  T  _minValue;
  T  _maxValue;
  T  _defaultValue;
  
  ConsoleVar();
  ConsoleVar( const ConsoleVar& );
};


//******************************************************************************************************************************
// Generic conversions.  Add any type-specific conversions below these.
template<typename T>
inline std::string ConsoleVar<T>::ToString( T value ) const
{
  std::ostringstream ss;
  ss.setf( std::ios::boolalpha | std::ios::showpoint );
  ss.setf( std::ios::fixed, std::ios::floatfield );
  
  ss << value;
  return ss.str();
}

//==============================================================================================================================
template<typename T>
inline bool ConsoleVar<T>::ParseText( const char* text )
{
  std::istringstream is( text );
  
  T temp;
  is >> temp;
  
  const bool parsedOK = !is.fail();
  
  if (parsedOK)
  {
    _value = temp;
  }
  
  return parsedOK;
}
  
//==============================================================================================================================
// Declarations for typedef specializations
// must be declared specializations here (implemented in cpp) otherwise compiler will just use the defaults
  
//==============================================================================================================================
// Specailizations for signed/unsigned char as stringstream treats them as ASCII otherwise
  
template<>
std::string ConsoleVar<signed char>::ToString(signed char value) const;

template<>
bool ConsoleVar<signed char>::ParseText(const char* text);

template<>
std::string ConsoleVar<unsigned char>::ToString(unsigned char value) const;

template<>
bool ConsoleVar<unsigned char>::ParseText(const char* text);
  
//==============================================================================================================================
// Specializations for bool to handle string based true/false values

template<>
bool ConsoleVar<bool>::ParseText(const char* text);


} // namespace Anki
} //namespace Util


#endif // ANKIUTIL_CONSOLE_VARIABLE
