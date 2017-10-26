/**
* File: ActivityGatherCubes.h
*
* Author: Kevin M. Karol
* Created: 04/27/17
*
* Description: Activity for cozmo to gather his cubes together
*
* Copyright: Anki, Inc. 2017
*
**/

#ifndef __Cozmo_Basestation_BehaviorSystem_Activities_Activities_ActivityGatherCubes_H__
#define __Cozmo_Basestation_BehaviorSystem_Activities_Activities_ActivityGatherCubes_H__

#include "anki/common/basestation/objectIDs.h"
#include "engine/activeObject.h"
#include "engine/aiComponent/behaviorComponent/activities/activities/iActivity.h"
#include <map>

namespace Json {
class Value;
}

namespace Anki {
namespace Cozmo {
  
class BehaviorExternalInterface;

class ActivityGatherCubes : public IActivity
{
public:
  ActivityGatherCubes(const Json::Value& config);
  virtual ~ActivityGatherCubes() {};
  
  virtual Result Update_Legacy(BehaviorExternalInterface& behaviorExternalInterface) override;
  
protected:
  virtual void OnActivatedActivity(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void OnDeactivatedActivity(BehaviorExternalInterface& behaviorExternalInterface) override;
  
private:
  std::map<ObjectID,bool> _isPlayingRingAnim;
  bool _gatherCubesFinished = false;
  
  void ClearBeacons(BehaviorExternalInterface& behaviorExternalInterface);
  void GetConnectedLightCubes(BehaviorExternalInterface& behaviorExternalInterface,
                              std::vector<const ActiveObject*>& result);
  void PlayFinishGatherCubeLight(BehaviorExternalInterface& behaviorExternalInterface,
                                 const ObjectID& cubeId);
  void PlayGatherCubeInProgressLight(BehaviorExternalInterface& behaviorExternalInterface,
                                     const ObjectID& cubeId);
  void PlayFreeplayLight(BehaviorExternalInterface& behaviorExternalInterface,
                         const ObjectID& cubeId);
  

  
};


} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_Activities_Activities_ActivityGatherCubes_H__
