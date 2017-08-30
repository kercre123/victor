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
#ifndef __Cozmo_Basestation_BehaviorSystem_BehaviorChooserFactory_H__
#define __Cozmo_Basestation_BehaviorSystem_BehaviorChooserFactory_H__

#include "json/json-forwards.h"
#include "anki/common/basestation/jsonTools.h"

#include "engine/behaviorSystem/behaviors/iBehavior.h"
#include "engine/behaviorSystem/bsRunnableChoosers/selectionBSRunnableChooser.h"
#include "engine/behaviorSystem/bsRunnableChoosers/scoringBSRunnableChooser.h"
#include "engine/behaviorSystem/bsRunnableChoosers/strictPriorityBSRunnableChooser.h"
#include "engine/behaviorSystem/iBSRunnable.h"

#include "clad/types/behaviorSystem/behaviorChooserTypes.h"

#include <type_traits>

namespace Anki {
namespace Cozmo {

class Robot;
class IBSRunnableChooser;

namespace BSRunnableChooserFactory {

// creates and initializes the proper behavior chooser from the given config. Returns a new instance if
// the config is valid, or nullptr if the configuration was not valid. Freeing the memory is responsibility
// of the caller. This factory does not store the created pointers, however it provides a function for destruction
std::unique_ptr<IBSRunnableChooser> CreateBSRunnableChooser(Robot& robot, const Json::Value& config);

}; // namespace BehaviorChooserFactory


} // namespace Cozmo
} // namespace Anki

#endif //
