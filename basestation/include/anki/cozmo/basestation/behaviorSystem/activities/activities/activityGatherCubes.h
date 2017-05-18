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
#include "anki/cozmo/basestation/activeObject.h"
#include "anki/cozmo/basestation/behaviorSystem/activities/activities/iActivity.h"
#include <map>

namespace Json {
class Value;
}

namespace Anki {
namespace Cozmo {

class ActivityGatherCubes : public IActivity
{
public:
  ActivityGatherCubes(Robot& robot, const Json::Value& config);
  ~ActivityGatherCubes() {};
  
  virtual Result Update(Robot& robot) override;
  
protected:
  virtual void OnSelectedInternal() override;
  virtual void OnDeselectedInternal() override;
  
private:
  Robot& _robot;
  std::map<ObjectID,bool> _isPlayingRingAnim;
  bool _gatherCubesFinished = false;
  
  void ClearBeacons();
  void GetConnectedLightCubes(std::vector<const ActiveObject*>& result);
  void PlayFinishGatherCubeLight(const ObjectID& cubeId);
  void PlayGatherCubeInProgressLight(const ObjectID& cubeId);
  void PlayFreeplayLight(const ObjectID& cubeId);
  

  
};


} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_Activities_Activities_ActivityGatherCubes_H__
