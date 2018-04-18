/**
 * File: spriteCacheComponent.h
 *
 * Author: Kevin Karol
 * Created: 4/12/18
 *
 * Description: Interface for engine's sprite cache instance
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/components/spriteCacheComponent.h"

#include "engine/cozmoContext.h"
#include "engine/robotDataLoader.h"
#include "coretech/vision/shared/spriteCache/spriteCache.h"
#include "coretech/vision/shared/spriteSequence/spriteSequenceContainer.h"
#include "engine/robot.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpriteCacheComponent::SpriteCacheComponent()
: IDependencyManagedComponent<RobotComponentID>(this, RobotComponentID::SpriteCache)
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpriteCacheComponent::~SpriteCacheComponent()
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpriteCacheComponent::InitDependent(Cozmo::Robot* robot, const RobotCompMap& dependentComponents)
{
  auto* context = dependentComponents.GetValue<ContextWrapper>().context;
  _spriteCache = context->GetDataLoader()->GetSpriteCache();

  _spriteSequenceContainer = context->GetDataLoader()->GetSpriteSequenceContainer();
}

  
} // namespace Cozmo
} // namespace Anki
