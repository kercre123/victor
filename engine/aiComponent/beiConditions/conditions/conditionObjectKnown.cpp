/**
 * File: conditionObjectKnown.h
 *
 * Author: ross
 * Created: April 17 2018
 *
 * Description: Condition that is true when an object has been seen recently
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/beiConditions/conditions/conditionObjectKnown.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/blockWorld/blockWorldFilter.h"
#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/objectIDs.h"
#include "coretech/common/engine/utils/timer.h"
#include "util/logging/logging.h"


namespace Anki {
namespace Vector {

namespace{
  const char* kObjectTypesKey = "objectTypes";
  const char* kMaxAgeKey = "maxAge_ms";
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionObjectKnown::ConditionObjectKnown(const Json::Value& config)
  : IBEICondition(config)
{
  
  const auto& typeArray = config[kObjectTypesKey];
  if( ANKI_VERIFY( typeArray.isArray(),
                   "ConditionObjectKnown.Ctor.InvalidObjectTypes",
                   "Missing or invalid object types" ) )
  {
    ANKI_VERIFY( !typeArray.empty(),
                 "ConditionObjectKnown.Ctor.EmptyTypeArray",
                 "Empty type array" );
    for( const auto& type : typeArray ) {
      const auto& objectTypeStr = type.asString();
      ObjectType targetType =  ObjectType::InvalidObject;
      ANKI_VERIFY( ObjectTypeFromString( objectTypeStr, targetType ),
                   "ConditionObjectKnown.Ctor.UnknownObjectType",
                   "Object type '%s' is not valid",
                   objectTypeStr.c_str() );
      _targetTypes.insert( targetType );
    }
  }
  
  if( config[kMaxAgeKey].isUInt() ) {
    _maxAge_ms = config[kMaxAgeKey].asUInt();
    _setMaxAge = true;
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionObjectKnown::ConditionObjectKnown(const Json::Value& config, TimeStamp_t maxAge_ms)
  : ConditionObjectKnown(config)
{
  ANKI_VERIFY( !_setMaxAge,
               "ConditionObjectKnown.Ctor.MaxAgeSet",
               "Json set a max age of [%d], overriding with ctor as [%d]",
               _maxAge_ms, maxAge_ms );
  
  _maxAge_ms = maxAge_ms;
  _setMaxAge = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionObjectKnown::~ConditionObjectKnown()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ConditionObjectKnown::SetActiveInternal(BehaviorExternalInterface& behaviorExternalInterface, bool setActive)
{
  _lastObjects.clear();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ConditionObjectKnown::AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const
{
  bool ret = false;
  
  BlockWorldFilter filter;
  std::set<ObjectType> setCopy( _targetTypes );
  filter.SetAllowedTypes( std::move(setCopy) );
  
  std::vector<const ObservableObject*> matches;
  behaviorExternalInterface.GetBlockWorld().FindLocatedMatchingObjects( filter, matches );
  
  std::vector<ObjectInfo> newInfo;
  
  const auto currTimeStamp = behaviorExternalInterface.GetRobotInfo().GetLastMsgTimestamp();
  const bool thisTickOnly = _setMaxAge && (_maxAge_ms == 0);
  for( const auto& match : matches ) {
    ObjectID matchID = match->GetID();
    const RobotTimeStamp_t matchTime = match->GetLastObservedTime();
    if( thisTickOnly ) {
      // this is different than filter.OnlyConsiderLatestUpdate because that checks against the
      // last marker sighting time. If we were to add our own filter lambda to compare the object
      // timestamp against the current timestamp, that still won't tell us if it was sighted this
      // tick. so we rely on _lastObjects. the object should either be new (not in _lastObjects)
      // or the timestamp should be newer than in _lastObjects. (_lastObjects can't be a map because
      // of ObjectID)
      auto it = std::find_if( _lastObjects.begin(), _lastObjects.end(), [&matchID](const auto& obj) {
        return obj.objectID == matchID;
      });
      // always add this one so there's a timestamp to compare against, but only mark it as
      // matchedThisTickOnly if the object is new or newer
      newInfo.emplace_back( matchTime, matchID );
      if( (matchTime > 0) && ((it == _lastObjects.end()) || (it->observedTime < matchTime)) ) {
        ret = true;
        newInfo.back().matchedThisTickOnly = true;
      }
    } else if( !_setMaxAge || (matchTime + _maxAge_ms >= currTimeStamp) ) {
      newInfo.emplace_back( matchTime, matchID );
      ret = true;
    }
  }
  
  _lastObjects = std::move(newInfo);
  
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const std::vector<const ObservableObject*> ConditionObjectKnown::GetObjects(BehaviorExternalInterface& bei) const
{
  const bool thisTickOnly = _setMaxAge && (_maxAge_ms == 0);
  std::vector<const ObservableObject*> ret;
  ret.reserve( _lastObjects.size() );
  for( const auto& observed : _lastObjects ) {
    if( thisTickOnly && !observed.matchedThisTickOnly ) {
      continue;
    }
    const auto* object = bei.GetBlockWorld().GetLocatedObjectByID( observed.objectID );
    if( object != nullptr ) {
      ret.push_back( object );
    }
  }
  return ret;
}

} // namespace Vector
} // namespace Anki
