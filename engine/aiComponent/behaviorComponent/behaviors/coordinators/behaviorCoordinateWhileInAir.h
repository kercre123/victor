/**
 * File: BehaviorCoordinateWhileInAir.h
 *
 * Author: Kevin M. Karol
 * Created: 2018-06-23
 *
 * Description: Behavior responsible for managing the robot's behaviors when picked up/in the air
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorCoordinateWhileInAir__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorCoordinateWhileInAir__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/dispatch/behaviorDispatcherPassThrough.h"
#include "engine/aiComponent/behaviorComponent/behaviorTreeStateHelpers.h"

namespace Anki {
namespace Cozmo {

class BehaviorCoordinateWhileInAir : public BehaviorDispatcherPassThrough
{
public: 
  virtual ~BehaviorCoordinateWhileInAir();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorCoordinateWhileInAir(const Json::Value& config);
  virtual void InitPassThrough() override;
  virtual void OnPassThroughActivated() override;
  virtual void PassThroughUpdate() override;

private:
  bool _areTreadsLocked = false;
  ICozmoBehaviorPtr _whileInAirDispatcher;

  std::unique_ptr<AreBehaviorsActivatedHelper> _inAirDispatcherBehaviorSet;
  std::unique_ptr<AreBehaviorsActivatedHelper> _suppressInAirBehaviorSet;
  std::unique_ptr<AreBehaviorsActivatedHelper> _lockTracksBehaviorSet;

  bool IsInAirDispatcherActive() const;

  void LockTracksIfAppropriate();
  void SuppressInAirReactionIfAppropriate();
};

} // namespace Cozmo
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorCoordinateWhileInAir__
