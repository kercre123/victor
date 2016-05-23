/***********************************************************************************************************************
 *
 *  stringID.h
 *  Anki::Util
 *
 *  Created by Jarrod Hatfield on 4/10/14.
 *  Copyright (c) 2014 Anki. All rights reserved.
 *
 *  Description:
 *  - A unique ID to represent a string
 *  - Fast conversion between string and id
 *
 ***********************************************************************************************************************/

#ifndef UTIL_STRINGID
#define UTIL_STRINGID

#ifdef __cplusplus
#include "util/stringTable/stringTable.h"
#endif


//======================================================================================================================
// C Interface

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
  
  unsigned int StringID_Create( const char* name );
  const char* StringID_ToString( unsigned int id );
  
#ifdef __cplusplus
}
#endif // __cplusplus



#ifndef __cplusplus

  // C Compiler
  typedef unsigned int StringID;
  #define STRINGID( name ) const StringID STRID_##name = StringID_Create( #name )

  #define STRID_None 0

#else

  // C++ Compiler
  #define STRINGID( name ) static const Anki::Util::StringID STRID_##name( #name )

namespace Anki{
namespace Util{
  
//**********************************************************************************************************************
class StringID
{
  friend const char* ::StringID_ToString( unsigned int stringID );
  
  //--------------------------------------------------------------------------------------------------------------------
public:
  StringID();
  StringID( const StringID& other );
  explicit StringID( const std::string& name );
  ~StringID();
  
  void Set( const std::string& name );
  StringTable::STRID GetID() const { return id_; }
  const std::string& ToString() const;
  const char* c_str() const;
  
  bool operator==( const StringID& rhs ) const { return ( id_ == rhs.id_ ); }
  bool operator!=( const StringID& rhs ) const { return ( id_ != rhs.id_ ); }
  bool operator<( const StringID& rhs ) const { return ( id_ < rhs.id_ ); }
  bool operator>( const StringID& rhs ) const { return ( id_ > rhs.id_ ); }
  StringID& operator=( const StringID& rhs ) { id_ = rhs.id_; return *this; }
  StringID& operator=( const std::string& rhs ) { Set( rhs ); return *this; }
  
  //--------------------------------------------------------------------------------------------------------------------
private:
  StringTable::STRID id_;
  
  static StringTable& GetStringTable();
};

extern StringID STRID_None;
  
} // namespace Anki
} //namespace Util

//**********************************************************************************************************************
// Define a hash function for StringID so that we can use it with std containers
namespace std
{
  template<>
  struct hash<Anki::Util::StringID>
  {
    typedef Anki::Util::StringID argument_type;
    typedef std::size_t result_type;

    result_type operator()( argument_type const& id ) const
    {
      return std::hash<Anki::Util::StringTable::STRID>()( id.GetID() );
    }
  };
} // namespace std


#endif // __cplusplus


#endif // UTIL_STRINGID
