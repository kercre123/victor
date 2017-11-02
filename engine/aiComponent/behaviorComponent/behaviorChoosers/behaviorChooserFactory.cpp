/**
 * File: BehaviorChooserFactory
 *
 * Author: Raul
 * Created: 05/02/16
 *
 * Description: Exactly what it sounds like, a factory for behavior choosers.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#include "engine/aiComponent/behaviorComponent/behaviorChoosers/behaviorChooserFactory.h"

// behavior choosers
#include "engine/aiComponent/behaviorComponent/activities/activities/activityFreeplay.h"
#include "engine/aiComponent/behaviorComponent/behaviorChoosers/selectionBehaviorChooser.h"
#include "engine/aiComponent/behaviorComponent/behaviorChoosers/scoringBehaviorChooser.h"
#include "engine/aiComponent/behaviorComponent/behaviorChoosers/strictPriorityBehaviorChooser.h"

#include "anki/common/basestation/jsonTools.h"

#include "clad/types/behaviorComponent/behaviorChooserTypes.h"

#include "util/logging/logging.h"
#include "util/helpers/templateHelpers.h"
#include "json/json.h"

#include <algorithm>

namespace Anki {
namespace Cozmo {
namespace BehaviorChooserFactory {
  
namespace{
const char* const kChooserTypeConfigKey = "type";
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::unique_ptr<IBehaviorChooser> CreateBehaviorChooser(BehaviorExternalInterface& behaviorExternalInterface, const Json::Value& config)
{
  std::unique_ptr<IBehaviorChooser> newChooser;
  
  // extract type
  BehaviorChooserType type = BehaviorChooserTypeFromString(
                               JsonTools::ParseString(config, kChooserTypeConfigKey,
                                                      "BehaviorChooserFactory.CreateBehaviorChooser.NoTypeSpecified"));
  
  switch(type){
    case BehaviorChooserType::Scoring:
    {
      newChooser.reset(new ScoringBehaviorChooser(behaviorExternalInterface, config));
      break;
    }
    case BehaviorChooserType::Selection:
    {
      newChooser.reset(new SelectionBehaviorChooser(behaviorExternalInterface, config));
      break;
    }
    case BehaviorChooserType::StrictPriority:
    {
      newChooser.reset(new StrictPriorityBehaviorChooser(behaviorExternalInterface, config));
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
