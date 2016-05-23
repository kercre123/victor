/**
 * File: arrayHelpers.h
 *
 * Author:  Mark Wesley
 * Created: 10/10/2014
 *
 * Description: Core defines, macros etc.
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#ifndef __Util_Helpers_ArrayHelpers_H__
#define __Util_Helpers_ArrayHelpers_H__


#ifdef __cplusplus
  namespace Anki{ namespace Util
  {
    template <typename T, size_t N>
    constexpr size_t array_size(const T (&)[N])
    {
      return N;
    }
  } }

  #define ARRAY_SIZE(inArray)   Anki::Util::array_size(inArray)
#else
  #define ARRAY_SIZE(inArray)   (sizeof(inArray) / sizeof((inArray)[0]))
#endif // def __cplusplus



#endif // #ifndef __Util_Helpers_ArrayHelpers_H__

