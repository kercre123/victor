/**
 * File: basestation.h
 *
 * Author: Kevin Yoon
 * Created: 7/15/2014
 *
 * Description: Main function and class for basestation.
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#ifndef BASESTATION_H_
#define BASESTATION_H_


#include "anki/common/types.h"
#include "json/json.h"

#include "anki/vision/basestation/image.h"

#include "anki/common/basestation/objectTypesAndIDs.h"
#include "anki/common/basestation/math/rect.h"

namespace Anki {

  // Forward declarations
  namespace Comms {
    class IComms;
  }
  
  class Radians;
  class Pose3d;
  
namespace Cozmo {

  // Return values for basestation interface
  typedef enum {
    BS_OK = 0,
    BS_JUST_PAUSED,
    BS_END_INIT_ERROR,
    BS_END_RUN_EXCEPTION_CAUGHT,
    BS_END_UNINIT_EXCEPTION_CAUGHT,
    BS_END_CLEAN_EXIT,
    BS_PLAYBACK_ERROR,
    BS_PLAYBACK_VERSION_MISMATCH,
    BS_PLAYBACK_ENDED,
    // last value, used for bounds checking
    BS_LAST_VALUE
  } BasestationStatus;

  // Forward declarations in Cozmo namespace
  class Robot;
  class BasestationMainImpl;
    
  // Main base station class that maintains the state of the system, calls update
  // functions, processes results, communicates with roadViz, etc.
  class BasestationMain { //: public IPausable {

  public:

    // Constructor
    BasestationMain();

    // Destructor
    virtual ~BasestationMain();

    // Initializes components of basestation
    // RETURNS: BS_OK, or BS_END_INIT_ERROR
    //BasestationStatus Init(const MetaGame::GameParameters& params, IComms* comms, boost::property_tree::ptree &config, BasestationMode mode);
    BasestationStatus Init(Comms::IComms* robot_comms, Json::Value& config);

    // unintializes basestation. returns BS_END_CLEAN_EXIT if all ok
    BasestationStatus UnInit();

    // add a robot with the specified ID
    Result AddRobot(RobotID_t robotID);
    
    // Runs an iteration of the base-station.  Takes an argument for the current
    // system time.
    BasestationStatus Update(BaseStationTime_t currTime);
  /*
    // returns true if the basestation is loaded (including possibly planner table computation, etc)
    bool DoneLoading();

    // returns the loading progress while the basestation is loading (between 0 and 1)
    float GetLoadingProgress();
    

    // stops game
    static void StopGame();
  */
    
    //
    // API for a Game to query robot/world state
    //
    
    int GetNumRobots() const;
    Robot* GetRobotByID(const RobotID_t robotID);
    
    // *Copies* into the given image object. Returns true if successful.
    bool GetCurrentRobotImage(const RobotID_t robotID, Vision::Image& img, TimeStamp_t newerThan);
    
    // Return the animation ID for the given robot and animation name.
    // A negative result means failure: -1 means animation name was unknown. -2 means
    // the robotID was invalid.
    s32 GetAnimationID(const RobotID_t robotID,
                       const std::string& animationName);
    
    // Query a specific robot's belief about where it is in 3D space, its head angle
    // and it's lift position. Returns true on success.
    bool GetRobotPose(const RobotID_t robotID, Pose3d& pose, Radians& headAngle, f32& liftHeight);
    
  private:

    BasestationMainImpl* impl_;
    
  };


} // namespace Cozmo
} // namespace Anki

#endif // BASESTATION_H_

