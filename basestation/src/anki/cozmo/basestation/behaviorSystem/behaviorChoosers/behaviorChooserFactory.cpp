/**
 * File: behaviorChooserFactory
 *
 * Author: Raul
 * Created: 05/02/16
 *
 * Description: Exactly what it sounds like, a factory for behavior choosers.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#include "anki/cozmo/basestation/behaviorSystem/behaviorChoosers/behaviorChooserFactory.h"

// behavior choosers
#include "anki/cozmo/basestation/behaviorSystem/activities/activities/activityFreeplay.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorChoosers/selectionBehaviorChooser.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorChoosers/scoringBehaviorChooser.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorChoosers/strictPriorityBehaviorChooser.h"

#include "anki/common/basestation/jsonTools.h"

#include "clad/types/behaviorChooserTypes.h"

#include "util/logging/logging.h"
#include "util/helpers/templateHelpers.h"
#include "json/json.h"

#include <algorithm>

namespace Anki {
namespace Cozmo {
namespace BehaviorChooserFactory {
  
namespace{
static const char* const kChooserTypeConfigKey = "type";

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehaviorChooser* CreateBehaviorChooser(Robot& robot, const Json::Value& config)
{
  IBehaviorChooser* newChooser = nullptr;

  // extract type
  BehaviorChooserType type = BehaviorChooserTypeFromString(
                                  JsonTools::ParseString(config, kChooserTypeConfigKey,
                                  "BehaviorChooserFactory.CreateBehaviorChooser.NoTypeSpecified"));
  
  switch(type){
    case BehaviorChooserType::Scoring:
    {
      newChooser = new ScoringBehaviorChooser(robot, config);
      break;
    }
    case BehaviorChooserType::Selection:
    {
      newChooser = new SelectionBehaviorChooser(robot, config);
      break;
    }
    case BehaviorChooserType::StrictPriority:
    {
      newChooser = new StrictPriorityBehaviorChooser(robot, config);
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
