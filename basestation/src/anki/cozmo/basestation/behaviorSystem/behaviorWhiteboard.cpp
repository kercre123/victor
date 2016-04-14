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

#include "anki/cozmo/basestation/ankiEventUtil.h"
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
    auto helper = MakeAnkiEventUtil(*robot.GetExternalInterface(), *this, _signalHandles);
    helper.SubscribeEngineToGame<MessageEngineToGameTag::RobotObservedPossibleObject>();
  }
  else {
    PRINT_NAMED_WARNING("BehaviorWhiteboard.Init", "Initialized whiteboard with no external interface. Will miss events.");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorWhiteboard::DisableCliffReaction(void* id)
{
  _disableCliffIds.insert(id);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorWhiteboard::RequestEnableCliffReaction(void* id)
{
  size_t numErased = _disableCliffIds.erase(id);

  if( numErased == 0 ){
    PRINT_NAMED_WARNING("BehaviorWhiteboard.RequestEnableCliffReaction.InvalidId",
                        "tried to request enabling cliff reaction with id %p, but no id found. %zu in set",
                        id,
                        _disableCliffIds.size());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorWhiteboard::IsCliffReactionEnabled() const
{
  return _disableCliffIds.empty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void BehaviorWhiteboard::HandleMessage(const ExternalInterface::RobotObservedPossibleObject& msg)
{
  // TODO Add the information that ExplorePossibleCube requires
}


} // namespace Cozmo
} // namespace Anki
