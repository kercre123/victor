/**
 * File: recording
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

#ifdef COZMO_RECORDING_PLAYBACK

#include "coretech/common/engine/utils/timer.h"
#include "recording.h"
#include "anki/messaging/basestation/IComms.h"
#include "../comms/commsRecorder.h"
#include "engine/utils/parsingConstants/parsingConstants.h"

//#include "basestation/version.h"
//#include "basestation/ui/messaging/messageQueue.h"
//#include "basestation/ui/messaging/messages/reproducibilityMessage.h"
//#include "basestation/planning/plannerManager.h"
//#include "metagame/loading/metaGameController.h"
#include "coretech/common/engine/exceptions.h"
//#include "util/logging/logging.h"
#include "json/json.h"

using namespace AnkiUtil;
//using namespace MetaGame;

namespace Anki {
namespace Vector {

// Constructor
Recording::Recording()
{
  replacementComms_ = NULL;
}

// Destructor
Recording::~Recording()
{
  if (outStream_.is_open())
  {
    outStream_.flush();
    outStream_.close();
  }

  if (replacementComms_ != NULL)
    delete replacementComms_;
}

// initializes the module and sets up comms and config pointers if needed
RecordingPlaybackStatus Recording::Init(Comms::IComms *realComms, Comms::IComms ** replacementComms, Json::Value *config, string commsLogFile)
{
  /*
  // prepare for recording comms messages
  boost::optional<string> optionalWorkPath = config->get_optional<string>(AnkiUtil::kP_WORK_PATH);
  if (!optionalWorkPath)
    return RPMS_INIT_ERROR;
  workPath_ = *optionalWorkPath;
  PRINT_NAMED_INFO("Recording.Parsing", "obtained log path at %s", (workPath_).c_str());
  string commsLogFile = workPath_ + AnkiUtil::kP_COMMS_LOG_PATH;
   */
  //string commsLogFile = "robot_comms.log";
  *replacementComms = new CommsRecorder(commsLogFile, realComms);
  replacementComms_ = *replacementComms;
  ((CommsRecorder*)replacementComms_)->PrepareLogFile();
  
/*
  // prepare joystick log
  //string fileName = workPath_ + AnkiUtil::kP_UI_MESSAGE_LOG_PATH;
  string fileName = "ui_comms.log";
  outStream_.open(fileName.c_str(), ofstream::binary);
  if (!outStream_.is_open())
  {
    PRINT_NAMED_ERROR("Recording.WriteToFile", "could not create file fileName=%s, errno=%s", fileName.c_str(), strerror(errno));
    return RPMS_INIT_ERROR;
  }
*/
  
  /*
  // verify that these params are already provided by the system
  config->put(AnkiUtil::kP_APPVERSION, BASESTATION_VERSION);

  // now write to config file
  try {
    fileName = workPath_ + AnkiUtil::kP_CONFIG_JSON_PATH;
    write_json(fileName, *config, std::locale(), true);
    PRINT_NAMED_INFO("Recording.WriteConfig", "Wrote config to '%s'", fileName.c_str());
  } catch (exception &e) {
    PRINT_NAMED_ERROR("Recording.WriteToFile", "could not write config to json fileName = %s, error = %s", fileName.c_str(), e.what());
    return RPMS_INIT_ERROR;
  }

  try {
    fileName = workPath_ + AnkiUtil::kP_UPGRADE_CONFIG_JSON_PATH;
    ptree upgradeConf = MetaGameController::GetCurrentConfig();
    write_json(fileName, upgradeConf, std::locale(), true);
  } catch (exception &e) {
    PRINT_NAMED_ERROR("Recording.WriteToFile", "could not write upgrade config to json fileName = %s, error = %s", fileName.c_str(), e.what());
    return RPMS_INIT_ERROR;
  }
   */

  return RPMS_OK;
}
  

// planner is created, initialized and set up for recording and playback all in one call..
void Recording::InitializePlanner(IPathPlanner *pathPlanner)
{
  //start planner recording
  //pathPlanner->SetPlaybackAndRecording(workPath_, BaseStation::PR_MODE_RECORD);
}


// processes messages sent to basestation
RecordingPlaybackStatus Recording::PreTickMessageProcess()
{
  /*
  ASSERT_NAMED(outStream_.is_open(), "Recording.PreTickMessageProcess.InvalidOutputStream");

  BaseStationTime_t currentTickCount = BaseStationTimer::getInstance()->GetCurrentTimeInNanoSeconds();
  unsigned int timeSize = sizeof(currentTickCount);

  deque<UiMessage*> queue = MessageQueue::getInstance()->GetUiToGameQueue();
  size_t size = queue.size();
  for (unsigned int i = 0; i < size; ++i)
  {
    UiMessage* message = queue[i];
#ifdef DEBUG_RECORDING_PLAYBACK
    PRINT_NAMED_INFO("Recording.SavingMessage", "tick %llu - type %d - src %d - target %d", currentTickCount, message->type_, message->sourceId_, message->targetId_);
#endif
    unsigned char buffer[TEMPORARY_BUFFER_SIZE];
    memset(&buffer[0], 0, TEMPORARY_BUFFER_SIZE);
    memcpy(&buffer[0], &currentTickCount, timeSize);
    unsigned int numWritten = message->WriteToBuffer(buffer, TEMPORARY_BUFFER_SIZE, timeSize);
    outStream_.write((const char*)buffer, numWritten + timeSize);
  }
   */
  return RPMS_OK;
}

// processes messages sent by basestation
RecordingPlaybackStatus Recording::PostTickMessageProcess()
{
  /*
  ASSERT_NAMED(outStream_.is_open(), "Recording.PostTickMessageProcess.InvalidOutputStream");

  BaseStationTime_t currentTickCount = BaseStationTimer::getInstance()->GetCurrentTimeInNanoSeconds();
  unsigned int timeSize = sizeof(currentTickCount);

  deque<UiMessage*> queue = MessageQueue::getInstance()->GetGameToUiQueue();
  size_t size = queue.size();
  for (unsigned int i = 0; i < size; ++i)
  {
    UiMessage* message = queue[i];
    if (message->type_ == UMCT_REPRODUCIBILITY_MESSAGE)
    {
#ifdef DEBUG_RECORDING_PLAYBACK
      ReproducibilityMessage *messageFromTheGame = (ReproducibilityMessage *)message;
      PRINT_NAMED_INFO("Recording.SavingReproducibilityMessage", "tick %llu - src %d - value %u", currentTickCount, messageFromTheGame->sourceId_, messageFromTheGame->hashValue_);
#endif
      unsigned char buffer[TEMPORARY_BUFFER_SIZE];
      memset(&buffer[0], 0, TEMPORARY_BUFFER_SIZE);
      memcpy(&buffer[0], &currentTickCount, timeSize);
      unsigned int numWritten = message->WriteToBuffer(buffer, TEMPORARY_BUFFER_SIZE, timeSize);
      outStream_.write((const char*)buffer, numWritten + timeSize);
    }
  }
   */
  return RPMS_OK;
}

} // namespace Vector
} // namespace Anki

#endif // #ifdef COZMO_RECORDING_PLAYBACK
