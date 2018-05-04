/**
* File: dataAccessorComponent.h
*
* Author: Kevin Karol
* Created: 4/12/18
*
* Description: Component which provides access to the data stored in robotDataLoader
* directly instead of having to pass context around
*
* Copyright: Anki, Inc. 2018
*
**/

#ifndef __Cozmo_Basestation_Components_DataAccessorComponent_H__
#define __Cozmo_Basestation_Components_DataAccessorComponent_H__

#include "engine/robotComponents_fwd.h"
#include "engine/robotDataLoader.h"
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

class DataAccessorComponent : public IDependencyManagedComponent<RobotComponentID>, 
                             private Anki::Util::noncopyable
{
public:
  DataAccessorComponent();
  virtual ~DataAccessorComponent();

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

  const RobotDataLoader::CompImageMap* GetCompImgMap() { return _compImgMap; }
  const RobotDataLoader::CompLayoutMap* GetCompLayoutMap() { return _compLayoutMap; }

private:
  Vision::SpriteCache* _spriteCache = nullptr;
  Vision::SpriteSequenceContainer* _spriteSequenceContainer = nullptr;
  const RobotDataLoader::CompImageMap* _compImgMap = nullptr;
  const RobotDataLoader::CompLayoutMap* _compLayoutMap = nullptr;

}; // __Cozmo_Basestation_Components_DataAccessorComponent_H__


} // namespace Cozmo
} // namespace Anki

#endif
