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
#include "anki/cozmo/basestation/components/unlockIdsHelpers.h"

#include "anki/common/basestation/jsonTools.h"

#include "util/logging/logging.h"
#include "util/helpers/templateHelpers.h"
#include "json/json.h"

namespace Anki {
namespace Cozmo {

const char* AIGoal::kBehaviorChooserConfigKey = "behaviorChooser";
static const char* kRequiresSparkKey      = "requireSpark";
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  AIGoal::AIGoal()
  : _requiresSpark(UnlockId::Count)
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

  // - - - - - - - - - -
  // Needs Sparks
  // - - - - - - - - - -
  
  std::string sparkString;
  if( JsonTools::GetValueOptional(config,kRequiresSparkKey,sparkString) )
  {
    _requiresSpark = UnlockIdsFromString(sparkString.c_str());
  }
  
  // configure chooser and set in out pointer
  const Json::Value& chooserConfig = config[kBehaviorChooserConfigKey];
  IBehaviorChooser* newChooser = BehaviorChooserFactory::CreateBehaviorChooser(robot, chooserConfig);
  _behaviorChooserPtr.reset( newChooser );
  
  _name = JsonTools::ParseString(config, "name", "AIGoal.Init");

  const bool success = (nullptr != _behaviorChooserPtr);
  return success;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior* AIGoal::ChooseNextBehavior(Robot& robot, bool didCurrentFinish) const
{
  // at the moment delegate on chooser. At some point we'll have intro/outro and other reactions
  // note we pass
  IBehavior* ret = _behaviorChooserPtr->ChooseNextBehavior(robot, didCurrentFinish);
  return ret;
}


} // namespace Cozmo
} // namespace Anki
