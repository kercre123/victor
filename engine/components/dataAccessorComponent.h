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
  virtual void InitDependent(Cozmo::Robot* robot, const RobotCompMap& dependentComps) override;
  virtual void GetInitDependencies(RobotCompIDSet& dependencies) const override {
    dependencies.insert(RobotComponentID::CozmoContextWrapper);
  };
  //////
  // end IDependencyManagedComponent functions
  //////

  const Vision::SpritePathMap* GetSpritePaths() const { assert(_spritePaths != nullptr); return _spritePaths; }

  Vision::SpriteCache* GetSpriteCache() const { assert(_spriteCache != nullptr); return _spriteCache;  }
  Vision::SpriteSequenceContainer* GetSpriteSequenceContainer() const { assert(_spriteSequenceContainer != nullptr); return _spriteSequenceContainer;}

  const RobotDataLoader::CompImageMap* GetCompImgMap() { assert(_compImgMap); return _compImgMap; }
  const RobotDataLoader::CompLayoutMap* GetCompLayoutMap() { assert(_compLayoutMap); return _compLayoutMap; }
  const CannedAnimationContainer* GetCannedAnimationContainer() { assert(_cannedAnimationContainer); return _cannedAnimationContainer; }
  const RobotDataLoader::WeatherResponseMap* GetWeatherResponseMap() const { assert(_weatherResponseMap); return _weatherResponseMap; }

private:
  const Vision::SpritePathMap* _spritePaths = nullptr;
  Vision::SpriteCache* _spriteCache = nullptr;
  Vision::SpriteSequenceContainer* _spriteSequenceContainer = nullptr;
  const RobotDataLoader::CompImageMap* _compImgMap = nullptr;
  const RobotDataLoader::CompLayoutMap* _compLayoutMap = nullptr;
  const CannedAnimationContainer* _cannedAnimationContainer = nullptr;
  const RobotDataLoader::WeatherResponseMap* _weatherResponseMap = nullptr;

}; // __Cozmo_Basestation_Components_DataAccessorComponent_H__


} // namespace Cozmo
} // namespace Anki

#endif
