/**
 * File: stringHelpers
 *
 * Author: Mark Wesley
 * Created: 05/11/16
 *
 * Description: String helpers functions beyond those provided by standard ANSI C
 *
 * Copyright: Anki, Inc. 2016
 *
 **/


#include "util/string/stringHelpers.h"
#include <assert.h>
#include <ctype.h>
#include <stdio.h>


namespace Anki {
namespace Util {

  
int stricmp(const char* string1, const char* string2)
{
  // Like strcmp, requires valid pointers to null-terminated strings
  assert(string1);
  assert(string2);
  
  while (true)
  {
    const char char1 = tolower(*string1);
    const char char2 = tolower(*string2);
    const int charDiff = char1 - char2;
    if ((char1 == 0) || (charDiff != 0))
    {
      // end of string1, or 1st difference found
      // Note: We don't need to explicitely check for end of string2 - that will show up as a difference
      return charDiff;
    }
    else
    {
      ++string1;
      ++string2;
    }
  }
}


} // namespace Util
} // namespace Anki



