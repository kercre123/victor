/**
 * File: AIGoal
 *
 * Author: Raul
 * Created: 05/02/16
 *
 * Description: High level goal: a group of behaviors that make sense to run together towards a common objective
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#include "AIGoal.h"

#include "anki/cozmo/basestation/behaviorSystem/behaviorChooserFactory.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorChoosers/iBehaviorChooser.h"

#include "anki/common/basestation/jsonTools.h"

#include "util/logging/logging.h"
#include "util/helpers/templateHelpers.h"
#include "json/json.h"

namespace Anki {
namespace Cozmo {

const char* AIGoal::kBehaviorChooserConfigKey = "behaviorChooser";

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AIGoal::AIGoal()
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AIGoal::~AIGoal()
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AIGoal::Init(Robot& robot, const Json::Value& config)
{
  // read info from config

  // configure chooser and set in out pointer
  const Json::Value& chooserConfig = config[kBehaviorChooserConfigKey];
  IBehaviorChooser* newChooser = BehaviorChooserFactory::CreateBehaviorChooser(robot, chooserConfig);
  _behaviorChooserPtr.reset( newChooser );

  const bool success = (nullptr != _behaviorChooserPtr);
  return success;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior* AIGoal::ChooseNextBehavior(const Robot& robot) const
{
  // at the moment delegate on chooser. At some point we'll have intro/outro and other reactions
  IBehavior* ret = _behaviorChooserPtr->ChooseNextBehavior(robot);
  return ret;
}


} // namespace Cozmo
} // namespace Anki
