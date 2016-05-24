//
//  EnumToString.h
//  RushHour
//
//  Created by Mark Pauley on 1/4/13.
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//
//  With credit to http://stackoverflow.com/questions/3168306/print-text-instead-of-value-from-c-enum
//

#undef DECL_ENUM_ELEMENT
#undef DECL_ENUM_ELEMENT_VAL
#undef DECL_ENUM_ELEMENT_STR
#undef DECL_ENUM_ELEMENT_VAL_STR
#undef BEGIN_ENUM
#undef END_ENUM


////////////////////////////////////////////////////////////////////////////////
#ifdef GENERATE_ENUM_STRINGS

#define BEGIN_ENUM( ENUM_NAME) const char * GetString##ENUM_NAME( enum tag##ENUM_NAME index ) {\
switch( index ) {
#define DECL_ENUM_ELEMENT( element ) case element: return #element; break;
#define DECL_ENUM_ELEMENT_VAL( element, value ) DECL_ENUM_ELEMENT( element )
#define DECL_ENUM_ELEMENT_STR( element, descr ) case element: return descr; break;
#define DECL_ENUM_ELEMENT_VAL_STR( element, value, descr ) DECL_ENUM_ELEMENT_STR( element, descr )

#define END_ENUM( ENUM_NAME ) default: return "Unknown value"; } } ;

////////////////////////////////////////////////////////////////////////////////
#elif defined GENERATE_PTREE_TRANSLATOR_HEADER

#include "util/ptree/includePtree.h"
// #include <boost/optional.hpp>
#include <string>

#define BEGIN_ENUM( ENUM_NAME ) struct enum_translator_##ENUM_NAME {    \
  typedef std::string internal_type;                                    \
  typedef ENUM_NAME external_type;                                      \
  ENUM_NAME _ret;                                                       \
  bool _found;                                                          \
  void set_value_internal(const std::string& str);                      \
  boost::optional<ENUM_NAME> get_value(const std::string& str);         \
  boost::optional<std::string> put_value(const ENUM_NAME& enm);         \
  enum_translator_##ENUM_NAME() : _found(false)

#define END_ENUM( ENUM_NAME )   };                                      \
  namespace boost { namespace property_tree                             \
  {                                                                     \
    template<typename Ch, typename Traits, typename Alloc>              \
    struct translator_between<std::basic_string< Ch, Traits, Alloc >, ENUM_NAME> \
    {                                                                   \
      typedef enum_translator_##ENUM_NAME type;                         \
    }; } }


#define DECL_ENUM_ELEMENT( element ) 
#define DECL_ENUM_ELEMENT_VAL( element, value ) 
#define DECL_ENUM_ELEMENT_STR( element, descr ) 
#define DECL_ENUM_ELEMENT_VAL_STR( element, value, descr ) 

////////////////////////////////////////////////////////////////////////////////
#elif defined GENERATE_PTREE_TRANSLATOR_BODY

#define BEGIN_ENUM( ENUM_NAME ) void enum_translator_##ENUM_NAME::set_value_internal(const std::string& str)

#define DECL_ENUM_ELEMENT_STR( element, descr )  if(str == descr) {_ret = element; _found = true;}
#define DECL_ENUM_ELEMENT( element ) DECL_ENUM_ELEMENT_STR(element, element)
#define DECL_ENUM_ELEMENT_VAL( element, value ) DECL_ENUM_ELEMENT_STR(element, element)
#define DECL_ENUM_ELEMENT_VAL_STR( element, value, descr ) DECL_ENUM_ELEMENT_STR(element, descr)

#define END_ENUM( ENUM_NAME ) boost::optional<ENUM_NAME> enum_translator_##ENUM_NAME::get_value(const std::string& str) { \
    set_value_internal(str);                                            \
    if(_found) return boost::optional<ENUM_NAME>(_ret);                 \
    else return boost::optional<ENUM_NAME>(boost::none);                \
  }                                                                     \
  boost::optional<std::string> enum_translator_##ENUM_NAME::put_value(const ENUM_NAME& enm) { \
    return boost::optional<std::string>(GetString##ENUM_NAME(enm));     \
  }

////////////////////////////////////////////////////////////////////////////////
#else

// generate the actual enum

#define DECL_ENUM_ELEMENT( element ) element,
#define DECL_ENUM_ELEMENT_VAL( element, value ) element = value,
#define DECL_ENUM_ELEMENT_STR( element, descr ) DECL_ENUM_ELEMENT( element )
#define DECL_ENUM_ELEMENT_VAL_STR( element, value, descr ) DECL_ENUM_ELEMENT_VAL( element, value )
#define BEGIN_ENUM( ENUM_NAME ) typedef enum tag##ENUM_NAME
#define END_ENUM( ENUM_NAME ) ENUM_NAME; \
const char* GetString##ENUM_NAME(enum tag##ENUM_NAME index);

#endif
