/**
 * File: behaviorWhiteboard
 *
 * Author: Raul
 * Created: 03/25/16
 *
 * Description: Whiteboard for behaviors to share information that is only relevant to them.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#include "anki/cozmo/basestation/behaviorSystem/behaviorWhiteboard.h"

#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"

//#include "util/console/consoleInterface.h"
//#include "util/logging/logging.h"
//#include "util/math/numericCast.h"
//#include "util/random/randomGenerator.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorWhiteboard::BehaviorWhiteboard()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorWhiteboard::Init(Robot& robot)
{

  // register to possible object events
  if ( robot.HasExternalInterface() )
  {
    using namespace ExternalInterface;
    MessageEngineToGameTag tag = MessageEngineToGameTag::RobotObservedPossibleObject;
    auto observedPossibleObjectCallback = std::bind(&BehaviorWhiteboard::OnPossibleObjectSpotted, this, std::placeholders::_1);
    _signalHandles.push_back(robot.GetExternalInterface()->Subscribe(tag, observedPossibleObjectCallback));
  }
  else {
    PRINT_NAMED_WARNING("BehaviorWhiteboard.Init", "Initialized whiteboard with no external interface. Will miss events.");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorWhiteboard::OnPossibleObjectSpotted(const AnkiEvent<ExternalInterface::MessageEngineToGame>& event)
{
  // TODO Add the information that ExplorePossibleCube requires
}

} // namespace Cozmo
} // namespace Anki
