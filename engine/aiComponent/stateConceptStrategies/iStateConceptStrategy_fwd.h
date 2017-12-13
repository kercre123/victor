/**
 * File: iStateConceptStrategy_fwd.h
 *
 * Author: Brad Neuman
 * Created: 2017-11-29
 *
 * Description: Forward declarations and common defines for state concept strategies
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Engine_AiComponent_StateConceptStrategies_IStateConceptStrategy_Fwd_H__
#define __Engine_AiComponent_StateConceptStrategies_IStateConceptStrategy_Fwd_H__

#include <memory>

namespace Anki {
namespace Cozmo {

class IStateConceptStrategy;

using IStateConceptStrategyPtr = std::shared_ptr<IStateConceptStrategy>;

}
}


#endif
