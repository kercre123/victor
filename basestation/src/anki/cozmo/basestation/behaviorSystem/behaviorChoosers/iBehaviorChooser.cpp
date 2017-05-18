/**
 * File: iBehaviorChooser.cpp
 *
 * Author: Kevin M. Karol
 * Created: 06/24/16
 *
 * Description: Class for handling picking of behaviors.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/behaviorSystem/behaviorChoosers/iBehaviorChooser.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/ankiEventUtil.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorFactory.h"

#include "anki/messaging/basestation/IComms.h"

namespace Anki {
namespace Cozmo {

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehaviorChooser::IBehaviorChooser(Robot& robot, const Json::Value& config)
{
  
}

  
} // namespace Cozmo
} // namespace Anki
