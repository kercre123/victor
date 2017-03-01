/**
 * File: stringToEnumMapper
 *
 * Author: Mark Wesley
 * Created: 11/09/15
 *
 * Description: Template helper for doing relatively fast case insensitive string-to-enum lookups
 *
 *              NOTE: Should only be included in CPP files - only needs instantiating once per type
 *
 * Copyright: Anki, Inc. 2015
 *
 **/


#ifdef __Util_Enums_StringToEnumMapper_H__
  #error Multiple Includes Shouldnt Happen - Only include this from CPP files!
#else
#define __Util_Enums_StringToEnumMapper_H__


#include <map>
#include <string>
#include "util/global/globalDefinitions.h"
#include "util/logging/logging.h"
#include <assert.h>


namespace Anki {
namespace Util {

  
// ================================================================================
// Template Classes to simplify creation of string-to-enum lookups (case insensitive and case sensitive)
//
// Example usage:
//
// 1) In header: declare a string to enum function, e.g:
//
//      MyEnumType MyEnumTypeFromString(const char* inString)
//
// 2) In cpp: implement using StringToEnumMapperI<T> or StringToEnumMapper<T> (for case insensitive or not):
//
//      // One global instance, created at static initialization on app launch
//      static StringToEnumMapper<MyEnumType> gStringToMyEnumTypeMapper;
//
//      MyEnumType MyEnumTypeFromString(const char* inString)
//      {
//        return gStringToMyEnumTypeMapper.GetTypeFromString(inString);
//      }
//
// Notes:
//
// StringToEnumMapper<T> requires that enum T meets the following:
//
//   1) Have entries in range 0..Count-1 with no gaps, therefore:
//      a) 1st valid entry == T(0)
//      b) Has a T::Count entry, 1 higher than the last valid entry
//   2) Must support the preincrement operator (see enumOperators.h)
//   3) Must support "const char* EnumToString(MyEnumType)"
//
//  StringToEnumMapperI<T> also requires:
//
//   4) That no 2 enum entries have the same lower case representation
//      (the requirement for no 2 entries to have the exact same representation is already enforced by the compiler)
//
// ================================================================================

  
// Case insensitive version (forces strings and lookups to lowercase)
template <typename T>
class StringToEnumMapperI
{
public:
  
  StringToEnumMapperI()
  {
    std::string lowerCaseString;
    
    for (T i = T(0); i < T::Count; ++i)
    {
      // Add all entries to map in lower case - allows all lookups to be case insensitive
      
      lowerCaseString = EnumToString(i);
      std::transform(lowerCaseString.begin(), lowerCaseString.end(), lowerCaseString.begin(), ::tolower);
      
      ANKI_DEVELOPER_CODE_ONLY(auto newEntry =)
      _stringToEnumMap.insert( typename StringToEnumMap::value_type(lowerCaseString, i) );
      
      #if ANKI_DEVELOPER_CODE
      if (!newEntry.second)
      {
        const T existingMapping = newEntry.first->second;
        PRINT_NAMED_ERROR("StringToEnumMapperI.DuplicateNames", "Entries %d and %d have same lower-case representation '%s'", (int)existingMapping, (int)i, lowerCaseString.c_str());
        assert(0);
      }
      #endif
    }
  }
  
  T GetTypeFromString(const char* inString) const
  {
    // For case-insensitive lookup all strings are stored in lower case
    
    std::string lowerCaseString = inString;
    std::transform(lowerCaseString.begin(), lowerCaseString.end(), lowerCaseString.begin(), ::tolower);
    
    const auto& it = _stringToEnumMap.find(lowerCaseString);
    if (it != _stringToEnumMap.end())
    {
      return it->second;
    }
    
    PRINT_NAMED_WARNING("StringToEnumMapperI.GetTypeFromString.NotFound", "No match found for '%s'", lowerCaseString.c_str());
    
    return T::Count;
  }
  
private:
  
  using StringToEnumMap = std::map<std::string, T>;
  StringToEnumMap _stringToEnumMap;
};


// Case sensitive string mapper (avoids string->lower conversion on setup and lookup)
template <typename T>
class StringToEnumMapper
{
public:
  
  StringToEnumMapper()
  {
    for (T i = T(0); i < T::Count; ++i)
    {
      const char* s = EnumToString(i);
      if(s != nullptr)
      {
        _stringToEnumMap[s] = i;
      }
    }
  }
  
  bool HasType(const char* inString) const
  {
    const auto& it = _stringToEnumMap.find(inString);
    return it != _stringToEnumMap.end();
  }
  
  T GetTypeFromString(const char* inString, bool assertOnInvalidEnum = false) const
  {
    const auto& it = _stringToEnumMap.find(inString);
    if (it != _stringToEnumMap.end())
    {
      return it->second;
    }
    
    DEV_ASSERT_MSG(!assertOnInvalidEnum,
                   "StringToEnumMapper.GetTypeFromString.NotFound",
                   "No match found for '%s'",
                   inString);
    
    PRINT_NAMED_WARNING("StringToEnumMapper.GetTypeFromString.NotFound", "No match found for '%s'", inString);
    
    return T::Count;
  }
  
private:
  
  using StringToEnumMap = std::map<std::string, T>;
  StringToEnumMap _stringToEnumMap;
};


} // namespace Util
} // namespace Anki


#endif // __Util_Enums_StringToEnumMapper_H__

