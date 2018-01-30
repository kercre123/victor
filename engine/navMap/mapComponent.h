/**
 * File: mapComponent.h
 *
 * Author: Michael Willett
 * Created: 2017-09-11
 *
 * Description: Component for consuming new sensor data and processing the information into
 *              the appropriate map objects
 *
 * Copyright: Anki, Inc. 2017
 *
 **/
 
 #ifndef __Anki_Cozmo_MapComponent_H__
 #define __Anki_Cozmo_MapComponent_H__

#include "coretech/common/shared/types.h"

#include "engine/aiComponent/behaviorComponent/behaviorComponents_fwd.h"
#include "engine/ankiEventUtil.h"
#include "engine/dependencyManagedComponent.h"
#include "engine/overheadEdge.h"
#include "engine/navMap/iNavMap.h"
#include "engine/robotComponents_fwd.h"

#include "coretech/vision/engine/observableObjectLibrary.h"

#include "util/helpers/noncopyable.h"

#include <assert.h>
#include <string>

namespace Anki {
namespace Cozmo {

// Forward declarations
class Robot;
class ObservableObject;
  
class MapComponent : public IDependencyManagedComponent<RobotComponentID>, private Util::noncopyable
{
public: 
  explicit MapComponent();
  ~MapComponent();

  //////
  // IDependencyManagedComponent functions
  //////
  virtual void InitDependent(Cozmo::Robot* robot, const RobotCompMap& dependentComponents) override;
  // Maintain the chain of initializations currently in robot - it might be possible to
  // change the order of initialization down the line, but be sure to check for ripple effects
  // when changing this function
  virtual void GetInitDependencies(RobotCompIDSet& dependencies) const override {
    dependencies.insert(RobotComponentID::Vision);
  };
  virtual void GetUpdateDependencies(RobotCompIDSet& dependencies) const override {};
  //////
  // end IDependencyManagedComponent functions
  //////
  
  ////////////////////////////////////////////////////////////////////////////////
  // Update and init
  ////////////////////////////////////////////////////////////////////////////////

  Result Update();
  
  void UpdateMapOrigins(PoseOriginID_t oldOriginID, PoseOriginID_t newOriginID);
  
  void UpdateRobotPose();

  void UpdateObjectPose(const ObservableObject& object, const Pose3d* oldPose, PoseState oldPoseState);
  
  // Processes the edges found in the given frame
  Result ProcessVisionOverheadEdges(const OverheadEdgeFrame& frameInfo);

  ////////////////////////////////////////////////////////////////////////////////
  // Message handling / dispatch
  ////////////////////////////////////////////////////////////////////////////////
  
  // flag all interesting edges in front of the robot (using ground plane ROI) as uncertain, meaning we want
  // the robot to grab new edges since we don't trust the ones we currently have in front of us
  void FlagGroundPlaneROIInterestingEdgesAsUncertain();
  
  // flags any interesting edges in the given quad as not interesting anymore. Quad should be passed wrt current origin
  void FlagQuadAsNotInterestingEdges(const Quad2f& quadWRTOrigin);
  
  // flags all current interesting edges as too small to give useful information
  void FlagInterestingEdgesAsUseless();

  // create a new memory map from current robot frame of reference.
  void CreateLocalizedMemoryMap(PoseOriginID_t worldOriginID);
  
  // Visualize the navigation memory information
  void DrawMap() const;
  
  // Send navigation memory map (e.g. so SDK can get the data)
  void BroadcastMap();
  
  // clear the space in the memory map between the robot and observed markers for the given object,
  // because if we saw the marker, it means there's nothing between us and the marker.
  // The observed markers are obtained querying the current marker observation time
  void ClearRobotToMarkers(const ObservableObject* object);
    
  
  ////////////////////////////////////////////////////////////////////////////////
  // Accessors
  ////////////////////////////////////////////////////////////////////////////////
  
  const INavMap* GetCurrentMemoryMap() const {return GetCurrentMemoryMapHelper(); }  
        INavMap* GetCurrentMemoryMap()       {return GetCurrentMemoryMapHelper(); }  


  // template for all events we subscribe to
  template<typename T>
  void HandleMessage(const T& msg);
  
private:
  INavMap* GetCurrentMemoryMapHelper() const;
  
  // remove current renders for all maps if any
  void ClearRender() const;

  // enable/disable rendering of the memory maps
  void SetRenderEnabled(bool enabled);

  // updates the objects reported in curOrigin that are moving to the relocalizedOrigin by virtue of rejiggering
  void UpdateOriginsOfObjects(PoseOriginID_t curOriginID, PoseOriginID_t relocalizedOriginID);

  // add/remove the given object to/from the memory map
  void AddObservableObject(const ObservableObject& object, const Pose3d& newPose);
  void RemoveObservableObject(const ObservableObject& object, PoseOriginID_t originID);
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Vision border detection
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // adds edges from the given frame to the world info
  Result AddVisionOverheadEdges(const OverheadEdgeFrame& frameInfo);
  
  // review interesting edges within the given quad and decide whether they are still interesting
  void ReviewInterestingEdges(const Quad2f& withinQuad, INavMap* map);
  
  // poses we have sent to the memory map for objects we know, in each origin
  struct PoseInMapInfo {
    PoseInMapInfo(const Pose3d& p, bool inMap) : pose(p), isInMap(inMap) {}
    PoseInMapInfo() : pose(), isInMap(false) {}
    Pose3d pose;
    bool isInMap; // if true the pose was sent to the map, if false the pose was removed from the map
  };
  
  using MapTable                  = std::map<PoseOriginID_t, std::unique_ptr<INavMap>>;
  using OriginToPoseInMapInfo     = std::map<PoseOriginID_t, PoseInMapInfo>;
  using ObjectIdToPosesPerOrigin  = std::map<int, OriginToPoseInMapInfo>;
  using EventHandles              = std::vector<Signal::SmartHandle>;
  
  Robot*                          _robot;
  EventHandles                    _eventHandles;
  MapTable                        _navMaps;
  PoseOriginID_t                  _currentMapOriginID;
  ObjectIdToPosesPerOrigin        _reportedPoses;
  Pose3d                          _reportedRobotPose;
  
  bool                            _isRenderEnabled;
  float                           _broadcastRate_sec = -1.0f;      // (Negative means don't send)
  float                           _nextBroadcastTimeStamp = 0.0f;  // The next time we should broadcast
};

}
}

#endif
