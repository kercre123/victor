/**
 * File: conditionCubeTapped.h
 *
 * Author:  Jarrod Hatfield
 * Created: 3/22/18
 *
 * Description: Strategy which listens for cube taps
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

// #include "engine/aiComponent/beiConditions/conditions/conditionCubeTapped.h"
#include "conditionCubeTapped.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "coretech/common/engine/utils/timer.h"
#include "engine/aiComponent/beiConditions/beiConditionMessageHelper.h"
#include "util/logging/logging.h"

namespace Anki {
namespace Vector {
  
namespace
{
  const char* kKeyMaxResponseTime             = "maxTapResponseTimeSec";
  const float kCubeTapDefaultResponseTime     = 1.0f;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionCubeTapped::ConditionCubeTapped( const Json::Value& config ) :
  IBEICondition( config ),
  _tapInfo( { ObjectID(), 0.0f } )
{
  _params.maxTapResponseTime = config.get( kKeyMaxResponseTime, kCubeTapDefaultResponseTime ).asFloat();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionCubeTapped::~ConditionCubeTapped()
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ConditionCubeTapped::InitInternal( BehaviorExternalInterface& behaviorExternalInterface )
{
  _messageHelper.reset( new BEIConditionMessageHelper( this, behaviorExternalInterface ) );
  _messageHelper->SubscribeToTags({ EngineToGameTag::ObjectTapped });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ConditionCubeTapped::AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const
{
  bool wasTappedRecently = false;

  if ( _tapInfo.id.IsSet() )
  {
    // if a cube has been tapped within our specified response time window ...
    const float currentTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    const float minTapTime = ( currentTime - _params.maxTapResponseTime );

    wasTappedRecently = ( _tapInfo.lastTappedTime >= minTapTime );
  }

  return wasTappedRecently;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ConditionCubeTapped::HandleEvent( const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface )
{
  // we've only subscribed to the one event, so make sure that holds
  DEV_ASSERT_MSG( ( event.GetData().GetTag() == EngineToGameTag::ObjectTapped ),
                  "ConditionCubeTapped", "Received event from non-subscribed event type %s",
                  MessageEngineToGameTagToString( event.GetData().GetTag() ) );

  HandleObjectTapped( behaviorExternalInterface, event.GetData().Get_ObjectTapped() );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ConditionCubeTapped::HandleObjectTapped( BehaviorExternalInterface& bei, const ExternalInterface::ObjectTapped& msg )
{
  _tapInfo.id = msg.objectID;
  _tapInfo.lastTappedTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
}

} // namespace Vector
} // namespace Anki
