/*******************************************************************************************************************************
 *
 *  stringTable.cpp
 *  Anki::Util
 *
 *  Created by Jarrod Hatfield on 4/10/14.
 *  Copyright (c) 2014 Anki. All rights reserved.
 *
 *******************************************************************************************************************************/

#include "util/stringTable/stringTable.h"

using namespace std;

// This will cause us to keep a separate list of all of our strings in order to convert back from STRID to std::string.
// Although helpful for debug, we may not want this for release builds since it will take up a lot of memory.
#define ALLOW_STRID_TO_SRING_MAPPING ( 1 )

// This is how many strings we will pre-allocate for.  We can still grow after this, but this will allow us to limit how
// many re-hashing we need to do during the lifecycle of our game.
#define INITIAL_NUM_STRINGS ( 1024 )

// JARROD-TODO: Add debug and tracking information for the string table

namespace Anki{ namespace Util
{
  
//------------------------------------------------------------------------------------------------------------------------------
StringTable::StringTable()
{
  stringtable_.reserve( INITIAL_NUM_STRINGS );

  // The first element should always be the empty string.
  AddStringID( "" );
}

//------------------------------------------------------------------------------------------------------------------------------
const string& StringTable::GetString( STRID id ) const
{
  static const string emptystring;
  
#if ALLOW_STRID_TO_SRING_MAPPING

  return ( ( id < stringMap_.size() ) ? stringMap_[id] : emptystring );
  
#else

  return emptystring;
  
#endif // ALLOW_SRTID_TO_SRING_MAPPING
}

//------------------------------------------------------------------------------------------------------------------------------
StringTable::STRID StringTable::GetStringID( const string& name ) const
{
  StringTableType::const_iterator it = stringtable_.find( name );
  return ( ( it != stringtable_.end() ) ? (*it).second : STRID_INDEX_NONE );
}

//------------------------------------------------------------------------------------------------------------------------------
StringTable::STRID StringTable::AddStringID( const string& name )
{
  STRID id = static_cast<STRID>(stringtable_.size());
  
  pair<StringTableType::iterator, bool> result = stringtable_.emplace( name, id );
  if ( !result.second )
  {
    // This name already exists, simply return the id for that string.
    id = (*result.first).second;
  }
  else
  {
    #if ALLOW_STRID_TO_SRING_MAPPING
    {
      // This is a new name, store the string so we can convert back if needed.
      stringMap_.push_back( name );
    }
    #endif
  }
  
  return id;
}
  
} // namespace Anki
} //namespace Util