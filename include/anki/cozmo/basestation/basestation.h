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

namespace Anki {

// forward declarations
  namespace Comms {
    class IComms;
  }
  
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

// Basestation run modes
typedef enum {
  BM_DEFAULT = 0,
  BM_RECORD_SESSION,
  BM_PLAYBACK_SESSION
} BasestationMode;


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
  BasestationStatus Init(Comms::IComms* robot_comms, Comms::IComms* ui_comms, Json::Value& config, BasestationMode mode);

  BasestationMode GetMode();

  // unintializes basestation. returns BS_END_CLEAN_EXIT if all ok
  BasestationStatus UnInit();

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
private:

  BasestationMainImpl* impl_;
  
};


} // namespace Cozmo
} // namespace Anki

#endif // BASESTATION_H_

