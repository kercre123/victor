/**
 * File: recordingPlaybackStatus.h
 *
 * Author: damjan
 * Created: 2/3/14
 *
 *
 * Copyright: Anki, Inc. 2014
 *
 **/
#ifndef BASESTATION_RECORDING_RECORDINGPLAYBACKSTATUS_H_
#define BASESTATION_RECORDING_RECORDINGPLAYBACKSTATUS_H_

namespace Anki {
namespace Cozmo {


// Return values for basestation recording/playback interface
typedef enum {
  RPMS_OK = 0,
  RPMS_INIT_ERROR,
  RPMS_ERROR,
  RPMS_VERSION_MISMATCH,
  RPMS_PLAYBACK_ENDED
} RecordingPlaybackStatus;

} // namespace Cozmo
} // namespace Anki

#endif //BASESTATION_RECORDING_RECORDINGPLAYBACKSTATUS_H_
