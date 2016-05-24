/**
 * File: enumOperators
 *
 * Author: Mark Wesley
 * Created: 11/09/15
 *
 * Description: Macros to automatically create pre/post increment operators for an enum
 *
 * Copyright: Anki, Inc. 2015
 *
 **/


#ifndef __Util_Enums_EnumOperators_H__
#define __Util_Enums_EnumOperators_H__


namespace Anki {
namespace Util {


// ================================================================================
//
// Macros to simplify creation of pre and post increment operators on an enum
//
// Put "DECLARE_ENUM_INCREMENT_OPERATORS(MyEnum);"   in your .h   file
// Put "IMPLEMENT_ENUM_INCREMENT_OPERATORS(MyEnum);" in your .cpp file
//
// ================================================================================


#define DECLARE_ENUM_INCREMENT_OPERATORS(EnumType) \
    EnumType& operator++(EnumType& inEnum);        \
    EnumType  operator++(EnumType& inEnum, int);

  
#define IMPLEMENT_ENUM_INCREMENT_OPERATORS(EnumType)  \
    EnumType& operator++(EnumType& inEnum)            \
    {                                                 \
      inEnum = (EnumType)( int(inEnum) + 1 );         \
      return inEnum;                                  \
    }                                                 \
                                                      \
    EnumType  operator++(EnumType& inEnum, int)       \
    {                                                 \
      EnumType oldValue(inEnum);                      \
      ++inEnum;                                       \
      return oldValue;                                \
    }
  

} // namespace Util
} // namespace Anki


#endif // __Util_Enums_EnumOperators_H__

