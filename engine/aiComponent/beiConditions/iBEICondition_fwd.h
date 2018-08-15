/**
 * File: iBEICondition_fwd.h
 *
 * Author: Brad Neuman
 * Created: 2017-11-29
 *
 * Description: Forward declarations and common defines for state concept strategies
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Engine_AiComponent_StateConceptStrategies_IBEICondition_Fwd_H__
#define __Engine_AiComponent_StateConceptStrategies_IBEICondition_Fwd_H__

#include <memory>

namespace Anki {
namespace Vector {

class IBEICondition;

using IBEIConditionPtr = std::shared_ptr<IBEICondition>;

class CustomBEIConditionHandleInternal;

using CustomBEIConditionHandle = std::shared_ptr<CustomBEIConditionHandleInternal>;
using CustomBEIConditionHandleList = std::vector<CustomBEIConditionHandle>;

}
}


#endif
