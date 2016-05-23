/**
 * File: hashStringTable
 *
 * Author: raul
 * Created: 02/27/15
 * 
 * Description:
 * 
 * Copyright: Anki, Inc. 2015
 *
 **/
#include "hashStringTable.h"

#include "util/logging/logging.h"
#include "util/global/globalDefinitions.h"
#include <cassert>
#include <functional>

namespace Anki {
namespace Util {

const std::string HashStringTable::failStr("null");

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HashStringTable
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HashStringTable::IdType HashStringTable::GetIdFromString(const std::string& str)
{
  if ( str.empty() ) {
    return 0;
  }

  // ids are hashes
  size_t strHash = std::hash<std::string>()( str );

  // in dev, verify that there are no hash conflicts in data for different roles. Otherwise we can't use it as Id
  //#if ANKI_DEVELOPER_CODE
  {
    const IdToStringTable::const_iterator match = _idToString.find( strHash );
    if ( match != _idToString.end() )
    {
      if ( match->second != str )
      {
        PRINT_NAMED_ERROR("HashStringTable.DuplicatedHash",
          "Entries '%s' and '%s' have same hash '%zu', they can't be addressed uniquely by hash.",
          match->second.c_str(), str.c_str(), strHash );
        assert( false );
      }
    } else {
      _idToString[strHash] = str;
    }
  }
  //#endif

  return strHash;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

const std::string& HashStringTable::GetStrFromId(const IdType myId) const
{
  //#if ANKI_DEVELOPER_CODE
  {
    const IdToStringTable::const_iterator match = _idToString.find( myId );
    if ( match != _idToString.end() )
    {
      return match->second;
    }
  }
  //#else

  return failStr;
}

} // namespace
} // namespace
