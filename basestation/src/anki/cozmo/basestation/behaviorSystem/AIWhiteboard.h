/**
 * File: AIWhiteboard
 *
 * Author: Raul
 * Created: 03/25/16
 *
 * Description: Whiteboard for behaviors to share information that is only relevant to them.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#ifndef __Cozmo_Basestation_BehaviorSystem_AIWhiteboard_H__
#define __Cozmo_Basestation_BehaviorSystem_AIWhiteboard_H__

#include "AIBeacon.h" 

#include "anki/common/basestation/math/pose.h"
#include "anki/common/basestation/objectIDs.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface_fwd.h"
#include "clad/types/objectTypes.h"
#include "util/signals/simpleSignal_fwd.h"

#include <vector>
#include <set>
#include <list>

namespace Anki {
namespace Cozmo {

class Robot;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// AIWhiteboard
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class AIWhiteboard
{
public:
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Types
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // info for every marker that is a possible cube but we don't trust (based on distance or how quickly we saw it)
  struct PossibleMarker {
    PossibleMarker( const Pose3d& p, ObjectType objType ) : pose(p), type(objType) {}
    Pose3d pose;
    ObjectType type;
  };
  using PossibleMarkerList = std::list<PossibleMarker>;
  
  using BeaconList = std::vector<AIBeacon>;
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Initialization/destruction
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // constructor
  AIWhiteboard(Robot& robot);
  
  // initializes the whiteboard, registers to events
  void Init();
  
  // what to do when the robot is delocalized
  void OnRobotDelocalized();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Markers
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // called when Cozmo can identify a clear quad (no borders, obstacles, etc)
  void ProcessClearQuad(const Quad2f& quad);

  // set to the top cube when cozmo builds a stack he wants to admire, cleared if the stack gets disrupted
  void SetHasStackToAdmire(ObjectID topBlockID, ObjectID bottomBlockID) { _topOfStackToAdmire = topBlockID; _bottomOfStackToAdmire = bottomBlockID; }
  void ClearHasStackToAdmire() { _topOfStackToAdmire.UnSet(); _bottomOfStackToAdmire.UnSet(); }
  
  bool HasStackToAdmire() const { return _topOfStackToAdmire.IsSet(); }
  ObjectID GetStackToAdmireTopBlockID() const { return _topOfStackToAdmire; }
  ObjectID GetStackToAdmireBottomBlockID() const { return _bottomOfStackToAdmire; }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Accessors
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // list of possible markers
  const PossibleMarkerList& GetPossibleMarkers() const { return _possibleMarkers; }

  // beacons
  void AddBeacon( const Pose3d& beaconPos );
  const BeaconList& GetBeacons() const { return _beacons; }

  // return current active beacon if any, or nullptr if none are active
  const AIBeacon* GetActiveBeacon() const;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Tracked values
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // enable or disable the request. The id can be anything, but is intended to be used as `this` from an
  // object which is calling this function. The reaction will be disabled immediately when disable is called,
  // and will only be re-enabled when the corresponding number of called to RequestEnableCliffReaction are
  // made with the correct ids
  void DisableCliffReaction(void* id);
  void RequestEnableCliffReaction(void* id);
  bool IsCliffReactionEnabled() const;
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Events
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // template for all events we subscribe to
  template<typename T>
  void HandleMessage(const T& msg);

private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Methods
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // remove possible markers currently stored that
  void RemovePossibleMarkersMatching(ObjectType objectType, const Pose3d& pose);
  
  // update render of possible markers since they may have changed
  void UpdatePossibleMarkerRender();
  // update render of beacons
  void UpdateBeaconRender();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // the robot this whiteboard belongs to
  Robot& _robot;
  
  // signal handles for events we register to. These are currently unsubscribed when destroyed
  std::vector<Signal::SmartHandle> _signalHandles;
  
  std::multiset<void*> _disableCliffIds;  
 
  // list of markers we have not checked out yet. Using list because we make assume possible markers of same type
  // can be found at different locations
  PossibleMarkerList _possibleMarkers;
  
  // container of beacons currently defined (high level AI concept)
  BeaconList _beacons;

  ObjectID _topOfStackToAdmire;
  ObjectID _bottomOfStackToAdmire;
};
  

} // namespace Cozmo
} // namespace Anki

#endif //
