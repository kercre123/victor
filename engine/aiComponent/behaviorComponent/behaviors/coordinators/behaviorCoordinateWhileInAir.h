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
#include "engine/engineTimeStamp.h"

namespace Anki {
namespace Vector {

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

  // For tracking initial pickup reaction
  bool _hasPickupReactionPlayed = false;
  EngineTimeStamp_t _lastTimeWasOnTreads_ms = 0;

  ICozmoBehaviorPtr _initialPickupReaction;

  std::unique_ptr<AreBehaviorsActivatedHelper> _inAirDispatcherBehaviorSet;
  std::unique_ptr<AreBehaviorsActivatedHelper> _suppressInAirBehaviorSet;
  std::unique_ptr<AreBehaviorsActivatedHelper> _lockTracksBehaviorSet;

  bool IsInAirDispatcherActive() const;

  void LockTracksIfAppropriate();
  void SuppressInAirReactionIfAppropriate();
  void SuppressInitialPickupReactionIfAppropriate();

};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorCoordinateWhileInAir__
