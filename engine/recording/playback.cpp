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

//#include "basestation/version.h"
#include "anki/messaging/basestation/IComms.h"
#include "engine/comms/commsPlayback.h"
#include "playback.h"
//#include "engine/cozmoAPI/comms/messaging/uiMessages.h"
//#include "basestation/ui/messaging/messageQueue.h"
//#include "basestation/ui/messaging/messages/inputControllerMessage.h"
//#include "basestation/ui/messaging/messages/reproducibilityMessage.h"
//#include "basestation/vehicle/vehicleManager.h"
//#include "metagame/loading/metaGameController.h"
#include "engine/utils/parsingConstants/parsingConstants.h"
//#include "util/logging/logging.h"
#include "json/json.h"

using namespace AnkiUtil;
//using namespace MetaGame;

namespace Anki {
namespace Vector {

#define SOME_VERY_LOW_NUMBER_THAT_IS_TOO_LOW_TO_DESCRIBE_MESSAGE 3

// Constructor
Playback::Playback()
{
  replacementComms_ = NULL;
  _replayEndedOnce = false;
  //plannerManager_ = NULL;
}

// Destructor
Playback::~Playback()
{
  if (replacementComms_ != NULL)
    delete replacementComms_;

  //for (unsigned int i = 0; i < _postTick.size(); ++i)
  //  delete _postTick[i]._message;

  _postTick.clear();
  _preTick.clear();

  //MetaGameController::ReloadFromPtree(_originalMetaGameConfig);
}

// initializes the module and sets up comms and config pointers if needed
RecordingPlaybackStatus Playback::Init(Comms::IComms *realComms, Comms::IComms ** replacementComms, Json::Value *config, string commsLogFile)
{
/*
  // prepare for recording comms messages
  boost::optional<string> optionalWorkPath = config->get_optional<string>(AnkiUtil::kP_WORK_PATH);
  if (!optionalWorkPath)
    return RPMS_INIT_ERROR;
  workPath_ = *optionalWorkPath;
  PRINT_NAMED_INFO("Playback.Init", "obtained log path at %s", workPath_.c_str());
  string commsLogFile = workPath_ + AnkiUtil::kP_COMMS_LOG_PATH;
*/
  //string commsLogFile = "robot_comms.log";
  *replacementComms = new CommsPlayback(commsLogFile, realComms);
  replacementComms_ = *replacementComms;
  ((CommsRecorder*)replacementComms_)->PrepareLogFile();
  
/*
  BaseStationTime_t commsLastMessageTime = ((CommsPlayback*)replacementComms_)->GetTimeLastMessageRecorded();

  // save some params from current config
  boost::optional<string> baseConfigPath = config->get_optional<string>(AnkiUtil::kP_BASE_CONFIG_PATH);
  if (!baseConfigPath)
    return RPMS_INIT_ERROR;

  boost::optional<string> roadVizIp = config->get_optional<string>(AnkiUtil::kP_ROAD_VIZ_IP);
  bool useRoadViz = config->get(AnkiUtil::kP_USE_ROAD_VIZ, false);

  // now read from config file
  config->clear();
  string fileName;
  try {
    fileName = workPath_ + AnkiUtil::kP_CONFIG_JSON_PATH;
    boost::property_tree::json_parser::read_json(fileName, *config, std::locale());
    PRINT_NAMED_INFO("Playback.ReadConfig", "Read config from '%s'", fileName.c_str());
  } catch (exception &e) {
    PRINT_NAMED_ERROR("Playback.LoadingFromFile", "read config from fileName = %s, error = %s", fileName.c_str(), e.what());
    return RPMS_INIT_ERROR;
  }
*/

/*
  try {
    _originalMetaGameConfig = ptree(MetaGameController::GetCurrentConfig());
    ptree neMetaGameConfig;
    fileName = workPath_ + AnkiUtil::kP_UPGRADE_CONFIG_JSON_PATH;
    boost::property_tree::json_parser::read_json(fileName, neMetaGameConfig, std::locale());
    MetaGameController::ReloadFromPtree(neMetaGameConfig);
  } catch (exception &e) {
    PRINT_NAMED_ERROR("Playback.LoadingFromFile", "read meta game config from fileName = %s, error = %s", fileName.c_str(), e.what());
    return RPMS_INIT_ERROR;
  }
*/
  
/*
  // overwrite dev params with correct data for our current setup
  config->put(AnkiUtil::kP_BASE_CONFIG_PATH, *baseConfigPath);
  if (roadVizIp)
    config->put(AnkiUtil::kP_ROAD_VIZ_IP, *roadVizIp);
  config->put(AnkiUtil::kP_USE_ROAD_VIZ, useRoadViz);
*/
  
  
/*
  // prepare joystick log
  fileName = workPath_ + AnkiUtil::kP_UI_MESSAGE_LOG_PATH;
  // stream for dumping recorded data out to a file
  //read from file
  ifstream logFileStream (fileName.c_str(), ifstream::binary);
  logFileStream.seekg(0, ifstream::end);
  unsigned int size = (unsigned int)logFileStream.tellg();
  logFileStream.seekg(0);
  unsigned char buffer[size];
  logFileStream.read((char*)&buffer, size);
  logFileStream.close();

  // tell the log
#ifdef DEBUG_RECORDING_PLAYBACK
  PRINT_NAMED_INFO("Playback.ParsingDone", "bytes read %ld", size);
#endif

  // populate multimap
  unsigned int offset = 0;
  unsigned int numProcessed = 0;
  _timeLastMessageRecorded = 0;
  while (offset < size)
  {
    BaseStationTime_t messageTime = 0;
    unsigned int timeSize = sizeof(messageTime);
    memcpy(&messageTime, &buffer[offset], timeSize);
    offset += timeSize;
    UiMessageCommonType type = UiMessage::GetTypeFromBuffer(buffer, size, offset);
    UiMessage* message;
    switch (type)
    {
      case UMCT_INPUT_CONTROLLER_MESSAGE:
        message = new InputControllerMessage(ICMT_NONE, SEND_TO_ALL_ID, SEND_TO_ALL_ID);
        break;

      case UMCT_REPRODUCIBILITY_MESSAGE:
        message = new ReproducibilityMessage(RT_NONE, 0, SEND_TO_ALL_ID);
        break;

      default:
        message = new UiMessage(type);
        break;

    }
    unsigned  int numRead = message->ReadFromBuffer(buffer, size, offset);
    offset += numRead;
    if (numRead < SOME_VERY_LOW_NUMBER_THAT_IS_TOO_LOW_TO_DESCRIBE_MESSAGE) {
      PRINT_NAMED_ERROR("Playback.ParsingBuffer", "message not read from bufer, aborting file read process!");
      offset = size;
    } else {
      if (message->type_ == UMCT_REPRODUCIBILITY_MESSAGE &&
              static_cast<ReproducibilityMessage*>(message)->reproducibilityType_ != RT_PLANNER_LOADED)
        _postTick.push_back(MessageWrapper(messageTime, message));
      else
        _preTick.push_back(MessageWrapper(messageTime, message));
      numProcessed++;
      if (_timeLastMessageRecorded < messageTime)
        _timeLastMessageRecorded = messageTime;
    }
  }

  // tell the log
#ifdef DEBUG_RECORDING_PLAYBACK
  PRINT_NAMED_INFO("Playback.ParsingDone", "messages read %d", numProcessed);
#endif

  // remember the last message recorded time
  if (_timeLastMessageRecorded < commsLastMessageTime)
    _timeLastMessageRecorded = commsLastMessageTime;
#ifdef DEBUG_RECORDING_PLAYBACK
  PRINT_NAMED_INFO("Playback.ParsingDone", "timeLastMessageRecorded %lld", _timeLastMessageRecorded);
#endif
  _preTickNextMessage = 0;
  _postTickNextMessage = 0;

  // verify versions match
  // verify that these params are already provided by the system
  boost::optional<string> optionalRecordedAppVersion = config->get_optional<string>(AnkiUtil::kP_APPVERSION);
  if (! optionalRecordedAppVersion )
  {
    PRINT_NAMED_ERROR("Playback.ParsingError", "hgnode and appversion not provided in the recorded config");
    return RPMS_VERSION_MISMATCH;
  }

  if (optionalRecordedAppVersion.get().compare(BASESTATION_VERSION) != 0)
  {
    PRINT_NAMED_ERROR("Playback.ParsingError", "detected version mismatch %s != %s", optionalRecordedAppVersion.get().c_str(), BASESTATION_VERSION);
    return RPMS_VERSION_MISMATCH;
  }
*/
  return RPMS_OK;
}
/*
// planner is created, initialized and set up for recording and playback all in one call..
void Playback::InitializePlanner(PlannerManager *plannerManager)
{
  // Load planner data from file
  BaseStationTime_t lastPlannerTime = plannerManager->SetPlaybackAndRecording(workPath_, BaseStation::PR_MODE_PLAYBACK);
  if(lastPlannerTime > _timeLastMessageRecorded) {
    _timeLastMessageRecorded = lastPlannerTime;
  }

  plannerManager_ = plannerManager;
}
*/
// processes messages sent to basestation
RecordingPlaybackStatus Playback::PreTickMessageProcess()
{
/*
  if (_replayEndedOnce)
    return RPMS_OK;


  BaseStationTime_t currentTickCount = BaseStationTimer::getInstance()->GetCurrentTimeInNanoSeconds();
  // test that we did not exceed recording time
  if (currentTickCount > _timeLastMessageRecorded)
  {
    PRINT_NAMED_WARNING("Playback.End", "no more messages to playback");
    // tell the planner manager to enter NONE mode (instead of playback)
    //if(plannerManager_)
    //  plannerManager_->SetPlaybackAndRecording("", BaseStation::PR_MODE_NONE);

    // tell someone (one time only) that game is done
    _replayEndedOnce = true;
    return RPMS_PLAYBACK_ENDED;
  }

  // find all messages for this tick
  while (_preTickNextMessage < _preTick.size() && _preTick[_preTickNextMessage]._time <= currentTickCount)
  {
    UiMessage* message = _preTick[_preTickNextMessage++]._message;

    if(message->type_ == UMCT_REPRODUCIBILITY_MESSAGE) {
      ReproducibilityMessage* rMessage = dynamic_cast<ReproducibilityMessage*>(message);
      if(rMessage && rMessage->reproducibilityType_ == RT_PLANNER_LOADED) {
        PRINT_NAMED_INFO("Playback.FakingPlannerDoneLoading", "found message in preTick");
        //VehicleManager::getInstance()->GetPlannerManagerP()->SetFakeDoneLoading();
      }
      else {
        PRINT_NAMED_ERROR("Playback.InvalidLog",
                              "logged reproducibility message, but message was not of type RT_PLANNER_LOADED");
      }
    }
    else {
      // put joystick messages in the queue
      MessageQueue::getInstance()->AddMessageForGame(message);
#ifdef DEBUG_RECORDING_PLAYBACK
      PRINT_NAMED_INFO("Playback.FoundEvent", "time %llu - type %d - src %d - target %d", currentTickCount, message->type_, message->sourceId_, message->targetId_);
#endif
    }
  }
*/
  return RPMS_OK;
}

// processes messages sent by basestation
RecordingPlaybackStatus Playback::PostTickMessageProcess()
{
/*
  // If we are done replaying, skip this
  if (_replayEndedOnce)
    return RPMS_OK;


  BaseStationTime_t currentTickCount = BaseStationTimer::getInstance()->GetCurrentTimeInNanoSeconds();
  // find all messages for this tick
  vector<ReproducibilityMessage *> messagesFromLog;
  while (_postTickNextMessage < _postTick.size() && _postTick[_postTickNextMessage]._time <= currentTickCount)
  {
    UiMessage* message = _postTick[_postTickNextMessage++]._message;
    messagesFromLog.push_back((ReproducibilityMessage *)message);
  }

  // look for reproducibility messages in the queue
  deque<UiMessage*> queue = MessageQueue::getInstance()->GetGameToUiQueue();
  size_t size = queue.size();
  vector<bool> removeMessage;
  for (unsigned int i = 0; i < size; ++i)
  {
    UiMessage* message = queue[i];
    removeMessage.push_back(false);
    if (message->type_ == UMCT_REPRODUCIBILITY_MESSAGE )
    {
      ReproducibilityMessage *messageFromTheGame = (ReproducibilityMessage *)message;
#ifdef DEBUG_RECORDING_PLAYBACK
      PRINT_NAMED_INFO("Playback.FoundReproMessage", "%llu -> %d - %u", currentTickCount, messageFromTheGame->sourceId_, messageFromTheGame->hashValue_);
#endif

      int found = -1;
      for (unsigned int j = 0; j < messagesFromLog.size(); ++j)
      {
        if (messagesFromLog[j]->sourceId_ == messageFromTheGame->sourceId_ &&
          messagesFromLog[j]->reproducibilityType_ == messageFromTheGame->reproducibilityType_ )
        {
          if (messagesFromLog[j]->hashValue_ != messageFromTheGame->hashValue_)
          {
            PRINT_NAMED_ERROR("Playback.MessageError", "log message does not match game message -> %u - %u; tick count %llu, target %d",
              messagesFromLog[j]->hashValue_, messageFromTheGame->hashValue_, currentTickCount, messageFromTheGame->sourceId_);
            // tell someone that game is not reproducing correctly
            return RPMS_ERROR;
          }
          found = j;
          break;
        }
      }

      if (found < 0)
      {
        PRINT_NAMED_ERROR("Playback.MessageError", "message not found in log; tick count %llu", currentTickCount);
        // tell someone that game is not reproducing correctly
        return RPMS_ERROR;
      }
      else
        messagesFromLog.erase(messagesFromLog.begin() + found);
    }

    // block the message that pops up the "press ok" box during playback
    if (message->type_ == UMCT_PROMPT_USER_START_GAME) {
      removeMessage[i] = true;
    }
  }

  // Prune any messages that need to be pruned
  MessageQueue::getInstance()->PruneGameToUiMessages(removeMessage);

  if (messagesFromLog.size() != 0)
  {
    PRINT_NAMED_ERROR("Playback.MessageError", "message found in log but not generated by game; tick count %llu", currentTickCount);
    // tell someone that game is not reproducing correctly
    return RPMS_ERROR;
  }
*/
  return RPMS_OK;
}


} // namespace Vector
} // namespace Anki

#endif // #ifdef COZMO_RECORDING_PLAYBACK
