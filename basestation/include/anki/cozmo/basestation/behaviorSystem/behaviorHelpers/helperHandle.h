/**
 * File: helperHandle.h
 *
 * Author: Kevin M. Karol
 * Created: 2/1/17
 *
 * Description: Handle interface - these can be stored by behaviors
 * to make repeat requests to the helper component using the same data
 * Used to model a planner async handle system
 *
 * Copyright: Anki, Inc. 2017
 *
 **/


#ifndef __Cozmo_Basestation_BehaviorSystem_BehaviorHelpers_HelperHandle_H__
#define __Cozmo_Basestation_BehaviorSystem_BehaviorHelpers_HelperHandle_H__

#include <memory>

namespace Anki {
namespace Cozmo {

class IHelper;

using HelperHandle = std::shared_ptr<IHelper>;
using WeakHelperHandle = std::weak_ptr<IHelper>;


} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorHelpers_HelperHandler_H__

