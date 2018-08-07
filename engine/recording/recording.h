/**
 * File: recording.h
 *
 * Author: damjan
 * Created: 2/5/13
 * 
 * Description: controls recording of ui messages and car comms
 * 
 *
 * Copyright: Anki, Inc. 2012
 *
 **/
#if COZMO_RECORDING_PLAYBACK

#ifndef BASESTATION_RECORDING_RECORDING_H_
#define BASESTATION_RECORDING_RECORDING_H_

#include "recordingPlaybackStatus.h"
#include "coretech/common/engine/utils/helpers/includeFstream.h"
#include "coretech/common/engine/utils/helpers/includeIostream.h"
#include "json/json.h"
#include <stdio.h>
#include <deque>
#include <string>

using namespace std;

namespace Anki {
  
// forward declaration
  namespace Comms{
    class IComms;
  }

namespace Vector {

// size of buffer for writing ui messages to file
#define TEMPORARY_BUFFER_SIZE 40

// forward declarations
class UiMessage;
class IPathPlanner;

/**
 *  interface definition
 *
 * @author   damjan
 */
class IRecordingPlaybackModule {
public:

  // Destructor
  virtual ~IRecordingPlaybackModule() {};

  // initializes the module and sets up comms and config pointers if needed
  virtual RecordingPlaybackStatus Init(Comms::IComms *realComms, Comms::IComms ** replacementComms, Json::Value *config, string commsLogFile) = 0;

  // planner is created, initialized and set up for recording and playback all in one call..
  //virtual void InitializePlanner(PlannerManager *plannerManager) = 0;

  // processes messages sent to basestation
  virtual RecordingPlaybackStatus PreTickMessageProcess() = 0;

  // processes messages sent by basestation
  virtual RecordingPlaybackStatus PostTickMessageProcess() = 0;
};



/**
 *  controls recording ui messages to file
 * 
 * @author   damjan
 */
class Recording :public IRecordingPlaybackModule {
  public:

  // Constructor
  Recording();

  // Destructor
  ~Recording();

  // initializes the module and sets up comms and config pointers if needed
  RecordingPlaybackStatus Init(Comms::IComms *realComms, Comms::IComms ** replacementComms, Json::Value *config, string commsLogFile);

  // planner is created, initialized and set up for recording and playback all in one call..
  void InitializePlanner(IPathPlanner *pathPlanner);

  // processes messages sent to basestation
  RecordingPlaybackStatus PreTickMessageProcess();

  // processes messages sent by basestation
  RecordingPlaybackStatus PostTickMessageProcess();

protected:
  // stream for dumping recorded data out to a file
  ofstream outStream_;

  // keep pointer so that we can free up memory
  Comms::IComms *replacementComms_;

  string workPath_;
};

} // namespace Vector
} // namespace Anki

#endif //BASESTATION_RECORDING_RECORDING_H_

#endif
