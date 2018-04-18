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

#ifndef __Cozmo_Basestation_Components_SpriteCacheComponent_H__
#define __Cozmo_Basestation_Components_SpriteCacheComponent_H__

#include "engine/robotComponents_fwd.h"
#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/helpers/noncopyable.h"

#include <assert.h>

namespace Anki {

// forward declaration
namespace Vision{
class SpriteCache;
class SpriteSequenceContainer;
}

namespace Cozmo {

class SpriteCacheComponent : public IDependencyManagedComponent<RobotComponentID>, 
                             private Anki::Util::noncopyable
{
public:
  SpriteCacheComponent();
  virtual ~SpriteCacheComponent();

  //////
  // IDependencyManagedComponent functions
  //////
  virtual void InitDependent(Cozmo::Robot* robot, const RobotCompMap& dependentComponents) override;
  virtual void GetInitDependencies(RobotCompIDSet& dependencies) const override {
    dependencies.insert(RobotComponentID::CozmoContextWrapper);
  };
  //////
  // end IDependencyManagedComponent functions
  //////

  Vision::SpriteCache* GetSpriteCache() const { assert(_spriteCache != nullptr); return _spriteCache;  }

  Vision::SpriteSequenceContainer* GetSpriteSequenceContainer() const { assert(_spriteSequenceContainer != nullptr); return _spriteSequenceContainer;}

private:
  Vision::SpriteCache* _spriteCache = nullptr;
  Vision::SpriteSequenceContainer* _spriteSequenceContainer = nullptr;

}; // __Cozmo_Basestation_Components_SpriteCacheComponent_H__


} // namespace Cozmo
} // namespace Anki

#endif
