/**
 * File: bsRunnableChooserFactory
 *
 * Author: Raul
 * Created: 05/02/16
 *
 * Description: Exactly what it sounds like, a factory for bsRunnable choosers.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#include "engine/behaviorSystem/bsRunnableChoosers/bsRunnableChooserFactory.h"

// behavior choosers
#include "engine/behaviorSystem/activities/activities/activityFreeplay.h"
#include "engine/behaviorSystem/bsRunnableChoosers/selectionBSRunnableChooser.h"
#include "engine/behaviorSystem/bsRunnableChoosers/scoringBSRunnableChooser.h"
#include "engine/behaviorSystem/bsRunnableChoosers/strictPriorityBSRunnableChooser.h"

#include "anki/common/basestation/jsonTools.h"

#include "clad/types/behaviorSystem/behaviorChooserTypes.h"

#include "util/logging/logging.h"
#include "util/helpers/templateHelpers.h"
#include "json/json.h"

#include <algorithm>

namespace Anki {
namespace Cozmo {
namespace BSRunnableChooserFactory {
  
namespace{
const char* const kChooserTypeConfigKey = "type";
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::unique_ptr<IBSRunnableChooser> CreateBSRunnableChooser(Robot& robot, const Json::Value& config)
{
  std::unique_ptr<IBSRunnableChooser> newChooser;
  
  // extract type
  BehaviorChooserType type = BehaviorChooserTypeFromString(
                               JsonTools::ParseString(config, kChooserTypeConfigKey,
                                                      "BehaviorChooserFactory.CreateBehaviorChooser.NoTypeSpecified"));
  
  switch(type){
    case BehaviorChooserType::Scoring:
    {
      newChooser.reset(new ScoringBSRunnableChooser(robot, config));
      break;
    }
    case BehaviorChooserType::Selection:
    {
      newChooser.reset(new SelectionBSRunnableChooser(robot, config));
      break;
    }
    case BehaviorChooserType::StrictPriority:
    {
      newChooser.reset(new StrictPriorityBSRunnableChooser(robot, config));
      break;
    }
  }
  
  // if failed print information to debug
  const bool failed = (nullptr == newChooser);
  if ( failed )
  {
    PRINT_NAMED_ERROR("BehaviorChooserFactory.CreateBehaviorChooser.Fail",
                      "Failed to create behavior chooser '%s'. Check log for config.",
                      BehaviorChooserTypeToString(type));
    JsonTools::PrintJsonError(config, "BehaviorChooserFactory.CreateBehaviorChooser.Fail");
  }
  
  return newChooser;
}


}; // namespace BehaviorChooserFactory
} // namespace Cozmo
} // namespace Anki
