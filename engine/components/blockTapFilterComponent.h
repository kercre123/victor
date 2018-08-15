/**
 * File: blockTapFilterComponent.h
 *
 * Author: Molly Jameson
 * Created: 2016-07-07
 *
 * Description: A component to manage time delays to only send the most intense taps
 *               from blocks sent close together, since the other taps were likely noise
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Anki_Cozmo_Basestation_Components_BlockTapFilterComponent_H__
#define __Anki_Cozmo_Basestation_Components_BlockTapFilterComponent_H__


#include "coretech/common/shared/types.h"
#include "util/helpers/noncopyable.h"
#include "util/signals/simpleSignal_fwd.h"
#include "util/global/globalDefinitions.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "util/entityComponent/iDependencyManagedComponent.h"
#include "engine/engineTimeStamp.h"
#include "engine/events/ankiEvent.h"
#include "engine/robotComponents_fwd.h"
#include "coretech/common/engine/objectIDs.h"

#include <list>

namespace Anki {
namespace Vector {

class Robot;
namespace ExternalInterface {
  struct ObjectTapped;
  struct ObjectMoved;
  struct ObjectStoppedMoving;
}

class BlockTapFilterComponent : public IDependencyManagedComponent<RobotComponentID>, private Util::noncopyable
{
public:
  explicit BlockTapFilterComponent();

  //////
  // IDependencyManagedComponent functions
  //////
  virtual void InitDependent(Vector::Robot* robot, const RobotCompMap& dependentComps) override;
  virtual void GetInitDependencies(RobotCompIDSet& dependencies) const override {
    dependencies.insert(RobotComponentID::CozmoContextWrapper);
  };
  virtual void GetUpdateDependencies(RobotCompIDSet& dependencies) const override {
    dependencies.insert(RobotComponentID::CubeComms);
    dependencies.insert(RobotComponentID::CubeAccel);
  };
  virtual void UpdateDependent(const RobotCompMap& dependentComps) override;
  //////
  // end IDependencyManagedComponent functions
  //////
  
  bool ShouldIgnoreMovementDueToDoubleTap(const ObjectID& objectID);
  
  void HandleObjectTapped(const ExternalInterface::ObjectTapped& message);
  
  template<typename T>
  void HandleMessage(const T& msg);

private:
  
  void HandleEnableTapFilter(const AnkiEvent<ExternalInterface::MessageGameToEngine>& message);
  
  void CheckForDoubleTap(const ObjectID& objectID);
  
  Robot* _robot = nullptr;

  Signal::SmartHandle _gameToEngineSignalHandle;
  
  std::list<Signal::SmartHandle> _eventHandles;
  
  bool _enabled;
  EngineTimeStamp_t _waitToTime;
  
  struct DoubleTapInfo {
    // The time we should stop waiting for a double tap
    EngineTimeStamp_t doubleTapTime = 0;
    
    // Whether or not the object is moving
    bool isMoving = false;
    
    // The time we should stop ignoring move messages for the objectID this DoubleTapInfo
    // maps to
    EngineTimeStamp_t ignoreNextMoveTime = 0;
    bool isIgnoringMoveMessages = false;
  };
  
  std::map<ObjectID, DoubleTapInfo> _doubleTapObjects;
  std::vector<ExternalInterface::ObjectTapped> _tapInfo;
  
#if ANKI_DEV_CHEATS
  void HandleSendTapFilterStatus(const AnkiEvent<ExternalInterface::MessageGameToEngine>& message);
  Signal::SmartHandle _debugGameToEngineSignalHandle;
#endif

};

}
}

#endif
