/**
* File: beiConditionDebugFactors.h
*
* Author: ross
* Created: May 18 2018
*
* Description: Helps with aggregating factors that influence the AreConditionsMet() method of IBEIConditions
*
* Copyright: Anki, Inc. 2018
*
**/


#include "engine/aiComponent/beiConditions/beiConditionDebugFactors.h"
#include "engine/aiComponent/beiConditions/iBEICondition.h"
#include "util/logging/logging.h"
#include "json/json.h"
#include <new>
#include <string>

namespace Anki {
namespace Vector {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BEIConditionDebugFactors::Data::Data(int val)
  : type( intType )
  , asInt( val )
{}
BEIConditionDebugFactors::Data::Data(bool val)
  : type( boolType )
  , asBool( val )
{}
BEIConditionDebugFactors::Data::Data(float val)
  : type( floatType )
  , asFloat( val )
{}
BEIConditionDebugFactors::Data::Data(const std::string& val)
  : type( stringType )
{
  new (&asString) std::string();
  asString = val;
}
BEIConditionDebugFactors::Data::Data(const char* val)
  : BEIConditionDebugFactors::Data::Data( std::string{val} )
{}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BEIConditionDebugFactors::Data::~Data()
{
  if( type == stringType ) {
    using std::string;
    asString.~string();
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BEIConditionDebugFactors::Data::Data(const BEIConditionDebugFactors::Data& other)
{
  if( other.type == stringType ) {
    new(&asString) std::string(other.asString);
  } else if( other.type == intType ) {
    asInt = other.asInt;
  } else if( other.type == boolType ) {
    asBool = other.asBool;
  } else if( other.type == floatType ) {
    asFloat = other.asFloat;
  }
  type = other.type;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BEIConditionDebugFactors::Data& BEIConditionDebugFactors::Data::operator=(const BEIConditionDebugFactors::Data& other)
{
  if( other.type == stringType ) {
    new(&asString) std::string(other.asString);
  } else if( other.type == intType ) {
    asInt = other.asInt;
  } else if( other.type == boolType ) {
    asBool = other.asBool;
  } else if( other.type == floatType ) {
    asFloat = other.asFloat;
  }
  type = other.type;
  return *this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BEIConditionDebugFactors::Data::SetJSON( Json::Value& value ) const
{
  if( type == stringType ) {
    value = asString;
  } else if( type == intType ) {
    value = asInt;
  } else if( type == boolType ) {
    value = asBool;
  } else if( type == floatType ) {
    value = asFloat;
  } else {
    DEV_ASSERT(false, "Data.SetJson.UnknownType");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BEIConditionDebugFactors::HaveChanged() const
{
  bool changed = (_hash != _previousHash);
  
  for( const auto& child : _children ) {
    const bool childChanged = child.second->GetDebugFactors().HaveChanged();
    if( childChanged ) {
      changed = true;
      break;
    }
  }
  return changed;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BEIConditionDebugFactors::Reset()
{
  // note clear retains capacity
  _factors.clear();
  for( auto& child : _children ) {
    child.second->GetDebugFactors().Reset();
  }
  _children.clear();
  
  _previousHash = _hash;
  _hash = 0;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BEIConditionDebugFactors::AddChild(const std::string& name, IBEIConditionPtr child)
{
  child->BuildDebugFactors( child->GetDebugFactors() );
  _children.emplace_back( name, child );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Json::Value BEIConditionDebugFactors::GetJSON() const
{
  Json::Value ret;
  for( const auto& factor : _factors ) {
    factor.second.SetJSON( ret[factor.first] );
  }
  
  for( const auto& child : _children ) {
    if( child.second != nullptr ) {
      ret[child.first] = child.second->GetDebugFactors().GetJSON();
    }
  }
  return ret;
}
  
} // namespace Vector
} // namespace Anki
