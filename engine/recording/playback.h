/**
 * File: playback
 *
 * Author: damjan
 * Created: 2/6/13
 * 
 * Description: controls playback of ui messages and car comms
 * 
 *
 * Copyright: Anki, Inc. 2012
 *
 **/

#ifdef COZMO_RECORDING_PLAYBACK

#ifndef BASESTATION_RECORDING_REPLAY_H_
#define BASESTATION_RECORDING_REPLAY_H_
#include "recording.h"
#include <string>
#include "json/json.h"

using namespace std;

namespace Anki {
namespace Vector {


// internal definition of wrapper that contains message meta data and message itself
struct MessageWrapper {
  MessageWrapper(){};
  MessageWrapper(BaseStationTime_t time, UiMessage* message):_time(time), _message(message) {};
  BaseStationTime_t _time;
  UiMessage* _message;
};

/**
 *  controls playback of ui messages and car comms
 *
 * @author   damjan
 */
class Playback :public IRecordingPlaybackModule {
public:

  // Constructor
  Playback();

  // Destructor
  ~Playback();

  // initializeReplaymodule and sets up comms and config pointers if needed
  RecordingPlaybackStatus Init(Comms::IComms *realComms, Comms::IComms ** replacementComms, Json::Value *config, string commsLogFile);

  // planner is created, initialized and set up for recording and playback all in one call..
  //void InitializePlanner(IPathPlanner *plannerManager);

  // processes messages sent to basestation
  RecordingPlaybackStatus PreTickMessageProcess();

  // processes messages sent by basestation
  RecordingPlaybackStatus PostTickMessageProcess();

protected:
  // keep pointer so that we can free up memory
  Comms::IComms *replacementComms_;

  vector<MessageWrapper> _preTick;
  vector<MessageWrapper> _postTick;
  unsigned int _preTickNextMessage;
  unsigned int _postTickNextMessage;
  BaseStationTime_t _timeLastMessageRecorded;
  //ptree _originalMetaGameConfig;
  bool _replayEndedOnce;

  string workPath_;

  //PlannerManager *plannerManager_;
};

} // namespace Vector
} // namespace Anki

#endif //BASESTATION_RECORDING_RECORDING_H_

#endif // #ifdef COZMO_RECORDING_PLAYBACK
