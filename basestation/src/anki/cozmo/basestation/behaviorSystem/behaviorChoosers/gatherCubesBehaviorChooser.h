/**
 * File: gatherCubesBehaviorChooser.h
 *
 * Author: Ivy Ngo
 * Created: 03/27/17
 *
 * Description: Class for managing light and beacon state during the gather cubes spark behavior
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_BehaviorSystem_BehaviorChoosers_GatherCubesBehaviorChooser_H__
#define __Cozmo_Basestation_BehaviorSystem_BehaviorChoosers_GatherCubesBehaviorChooser_H__

#include "anki/common/basestation/objectIDs.h"
#include "anki/cozmo/basestation/activeObject.h"
#include "anki/cozmo/basestation/behaviorSystem/AIWhiteboard.h"
#include "anki/cozmo/basestation/behaviorSystem/aiComponent.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorChoosers/simpleBehaviorChooser.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/blockWorld/blockWorldFilter.h"
#include "anki/cozmo/basestation/components/cubeLightComponent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robot.h"
#include "util/signals/simpleSignal_fwd.h"

#include "clad/externalInterface/messageEngineToGame.h"

namespace Anki {
namespace Cozmo {

class GatherCubesBehaviorChooser : public SimpleBehaviorChooser
{
using BaseClass = SimpleBehaviorChooser;
  
public:
  // constructor/destructor
  GatherCubesBehaviorChooser(Robot& robot, const Json::Value& config);
  virtual ~GatherCubesBehaviorChooser();
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // IBehaviorChooser API
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  virtual Result Update(Robot& robot) override;
  
  // name (for debug/identification)
  virtual const char* GetName() const override { return "GatherCubes"; }
  
  virtual void OnSelected() override;
  virtual void OnDeselected() override;
  
private:
  void ClearBeacons();
  void GetConnectedLightCubes(std::vector<const ActiveObject*>& result);
  void PlayFinishGatherCubeLight(const ObjectID& cubeId);
  void PlayGatherCubeInProgressLight(const ObjectID& cubeId);
  void PlayFreeplayLight(const ObjectID& cubeId);
  
  std::map<ObjectID,bool> _isPlayingRingAnim;
  bool _gatherCubesFinished = false;
};

}
}

#endif /* gatherCubesBehaviorChooser_hpp */
