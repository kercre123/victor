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

#include "engine/components/dataAccessorComponent.h"

#include "engine/cozmoContext.h"
#include "engine/robotDataLoader.h"
#include "coretech/vision/shared/spriteCache/spriteCache.h"
#include "coretech/vision/shared/spriteSequence/spriteSequenceContainer.h"
#include "engine/robot.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DataAccessorComponent::DataAccessorComponent()
: IDependencyManagedComponent<RobotComponentID>(this, RobotComponentID::DataAccessor)
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DataAccessorComponent::~DataAccessorComponent()
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DataAccessorComponent::InitDependent(Cozmo::Robot* robot, const RobotCompMap& dependentComps)
{
  auto* context = dependentComps.GetComponent<ContextWrapper>().context;
  auto& dataLoader = *context->GetDataLoader();
  _spritePaths = dataLoader.GetSpritePaths();
  _spriteCache = dataLoader.GetSpriteCache();
  _spriteSequenceContainer = dataLoader.GetSpriteSequenceContainer();
  _compImgMap = dataLoader.GetCompImageMap();
  _compLayoutMap = dataLoader.GetCompLayoutMap();
  _cannedAnimationContainer = dataLoader.GetCannedAnimationContainer();
  _weatherResponseMap = dataLoader.GetWeatherResponseMap();
  _variableSnapshotJsonMap = dataLoader.GetVariableSnapshotJsonMap();
  // Copy, but it's fine
  _cupeSpinnerConfig = dataLoader.GetCubeSpinnerConfig();
}

  
} // namespace Cozmo
} // namespace Anki
