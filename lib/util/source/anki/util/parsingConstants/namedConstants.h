/***********************************************************************************************************************
 *
 *  NamedConstants
 *  Anki::Util
 *
 *  Created by Jarrod Hatfield on 6/26/14.
 *  Copyright (c) 2014 Anki. All rights reserved.
 *
 ***********************************************************************************************************************/

#ifndef __Anki_Util_NamedConstants_H__
#define __Anki_Util_NamedConstants_H__

#include "util/stringTable/stringID.h"

#ifdef CONST_STRING_ID
  #error "CONST_STRING_ID already defined. Please fix."
#endif

namespace Anki {
namespace Util {

#define CONST_STRING_ID(name) extern const StringID STRID_##name;

#include "util/parsingConstants/namedConstants.def"

#undef CONST_STRING_ID

}
}

#endif // __Anki_Util_NamedConstants_H__