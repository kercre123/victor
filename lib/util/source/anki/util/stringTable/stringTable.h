/*******************************************************************************************************************************
 *
 *  stringTable.h
 *  Anki::Util
 *
 *  Created by Jarrod Hatfield on 4/10/14.
 *  Copyright (c) 2014 Anki. All rights reserved.
 *
 *  Description:
 *  - Maintains a mapping of strings to ids for fast conversion between the two.
 *
 *******************************************************************************************************************************/


#ifndef UTIL_STRINGTABLE
#define UTIL_STRINGTABLE

#include <string>
#include <unordered_map>
#include <vector>

namespace Anki{
namespace Util{


//******************************************************************************************************************************
class StringTable
{
public:
  typedef unsigned int STRID;
  static const STRID STRID_INDEX_NONE = 0;

  //----------------------------------------------------------------------------------------------------------------------------
public:
  StringTable();
  
  const std::string& GetString( STRID id ) const;
  STRID GetStringID( const std::string& name ) const;
  STRID AddStringID( const std::string& name );

  //----------------------------------------------------------------------------------------------------------------------------
private:
  typedef std::unordered_map<std::string, STRID> StringTableType;
  StringTableType stringtable_;
  std::vector<std::string> stringMap_;

  StringTable( const StringTable& );
};


} // namespace Anki
} //namespace Util

#endif // UTIL_STRINGTABLE
