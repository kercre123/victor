/**
 * File: parsingConstants.cpp
 *
 * Author: Damjan Stulic
 * Created: 1/Aug/2012
 *
 * Description: data needed for parsing config files
 *
 * Copyright: Anki, Inc. 2012
 *
 **/
#include "parsingConstants.h"

#ifdef STRING_NAMED_CONST
#error "STRING_NAMED_CONST already defined. Please fix."
#endif

namespace Anki {
namespace Util {

#define STRING_NAMED_CONST(name, value) const char * const name = value;

#include "parsingConstants.def"
#undef STRING_NAMED_CONST

#define STRING_CONST(name) const char * const NAME_FROM_VALUE(kP_, name) = STR_VALUE(name);

#include "util/parsingConstants/soundNames.def"
#undef STRING_CONST

}
}