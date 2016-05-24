/**
 * File: parsingConstants.h
 *
 * Author: Damjan Stulic
 * Created: 1/Aug/2012
 * 
 * Description: data needed for parsing config files
 *
 * Copyright: Anki, Inc. 2012
 *
 **/

#ifndef UTIL_PARSING_CONSTANTS_H_
#define UTIL_PARSING_CONSTANTS_H_

namespace Anki {
namespace Util {

#ifdef STRING_NAMED_CONST
  #error "STRING_NAMED_CONST already defined. Please fix."
  #endif

#define STR_VALUE(arg)      #arg
#define NAME_FROM_VALUE(prefix, arg)   prefix ## arg

//#define STRING_NAMED_CONST(name, value) extern const std::string name;
#define STRING_NAMED_CONST(name, value) extern char const * const name;

#include "util/parsingConstants/parsingConstants.def"
#undef STRING_NAMED_CONST

#define STRING_CONST(name) extern char const * const NAME_FROM_VALUE(kP_, name);

#include "util/parsingConstants/soundNames.def"
#undef STRING_CONST

}
}

#endif