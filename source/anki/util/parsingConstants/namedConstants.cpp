/***********************************************************************************************************************
 *
 *  NamedConstants
 *  BaseStation
 *
 *  Created by Jarrod Hatfield on 6/26/14.
 *  Copyright (c) 2014 Anki. All rights reserved.
 *
 ***********************************************************************************************************************/

#include "namedConstants.h"

#ifdef CONST_STRING_ID
  #error "CONST_STRING_ID already defined. Please fix."
#endif

namespace Anki{ namespace Util
{

#define CONST_STRING_ID( name ) const StringID STRID_##name( #name );
  #include "namedConstants.def"
#undef CONST_STRING_ID
  
}
}