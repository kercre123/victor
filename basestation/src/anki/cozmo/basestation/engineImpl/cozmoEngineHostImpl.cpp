/*
 * File:          cozmoEngine.cpp
 * Date:          12/23/2014
 *
 * Description:   (See header file.)
 *
 * Author: Andrew Stein / Kevin Yoon
 *
 * Modifications:
 */

#include "anki/cozmo/basestation/engineImpl/cozmoEngineHostImpl.h"

namespace Anki {
namespace Cozmo {
  

CozmoEngineHostImpl::CozmoEngineHostImpl(IExternalInterface* externalInterface)
: CozmoEngineImpl(externalInterface)
, _isListeningForRobots(false)
, _robotAdvertisementService("RobotAdvertisementService")
, _robotMgr(externalInterface)
, _lastAnimationFolderScan(0)
, _animationReloadActive(false)
{

  PRINT_NAMED_INFO("CozmoEngineHostImpl.Constructor",
                   "Starting RobotAdvertisementService, reg port %d, ad port %d\n",
                   ROBOT_ADVERTISEMENT_REGISTRATION_PORT, ROBOT_ADVERTISING_PORT);

  _robotAdvertisementService.StartService(ROBOT_ADVERTISEMENT_REGISTRATION_PORT,
                                          ROBOT_ADVERTISING_PORT);

  // Handle robot disconnection:
  _signalHandles.emplace_back( _robotMgr.OnRobotDisconnected().ScopedSubscribe(
    std::bind(&CozmoEngineHostImpl::DisconnectFromRobot, this, std::placeholders::_1)
  ));
}

Result CozmoEngineHostImpl::InitInternal()
{
  Result lastResult = _robotMsgHandler.Init(&_robotChannel, &_robotMgr);

  return lastResult;
}

void CozmoEngineHostImpl::ForceAddRobot(AdvertisingRobot robotID,
                                        const char*      robotIP,
                                        bool             robotIsSimulated)
{
  if(_isInitialized) {
    PRINT_NAMED_INFO("CozmoEngineHostImpl.ForceAddRobot", "Force-adding %s robot with ID %d and IP %s\n",
                     (robotIsSimulated ? "simulated" : "real"), robotID, robotIP);

    // Force add physical robot since it's not registering by itself yet.
    Anki::Comms::AdvertisementRegistrationMsg forcedRegistrationMsg;
    forcedRegistrationMsg.id = robotID;
    forcedRegistrationMsg.port = Anki::Cozmo::ROBOT_RADIO_BASE_PORT + (robotIsSimulated ? robotID : 0);
    forcedRegistrationMsg.protocol = USE_UDP_ROBOT_COMMS == 1 ? Anki::Comms::UDP : Anki::Comms::TCP;
    forcedRegistrationMsg.enableAdvertisement = 1;
    snprintf((char*)forcedRegistrationMsg.ip, sizeof(forcedRegistrationMsg.ip), "%s", robotIP);

    _robotAdvertisementService.ProcessRegistrationMsg(forcedRegistrationMsg);

    // Mark this robot as force-added so we can deregister it from the advertising
    // service manually once we connect to it.
    _forceAddedRobots[robotID] = true;
  } else {
    PRINT_NAMED_ERROR("CozmoEngineHostImpl.ForceAddRobot",
                      "You cannot force-add a robot until the engine is initialized.\n");
  }
}

void CozmoEngineHostImpl::InitPlaybackAndRecording()
{
  // TODO: get playback/recording working again

  /*
   // Get basestation mode out of the config
   int modeInt;
   if(JsonTools::GetValueOptional(config, AnkiUtil::kP_BASESTATION_MODE, modeInt)) {
   mode_ = static_cast<BasestationMode>(modeInt);
   assert(mode_ <= BM_PLAYBACK_SESSION);
   } else {
   mode_ = BM_DEFAULT;
   }

   PRINT_INFO("Starting basestation mode %d\n", mode_);
   switch(mode_)
   {
   case BM_RECORD_SESSION:
   {
   // Create folder for all recorded logs
   std::string rootLogFolderName = AnkiUtil::kP_GAME_LOG_ROOT_DIR;
   if (!DirExists(rootLogFolderName.c_str())) {
   if(!MakeDir(rootLogFolderName.c_str())) {
   PRINT_NAMED_WARNING("Basestation.Init.RootLogDirCreateFailed", "Failed to create folder %s\n", rootLogFolderName.c_str());
   return BS_END_INIT_ERROR;
   }

   }

   // Create folder for log
   std::string logFolderName = rootLogFolderName + "/" + GetCurrentDateTime() + "/";
   if(!MakeDir(logFolderName.c_str())) {
   PRINT_NAMED_WARNING("Basestation.Init.LogDirCreateFailed", "Failed to create folder %s\n", logFolderName.c_str());
   return BS_END_INIT_ERROR;
   }

   // Save config to log folder
   Json::StyledStreamWriter writer;
   std::ofstream jsonFile(logFolderName + AnkiUtil::kP_CONFIG_JSON_FILE);
   writer.write(jsonFile, config);
   jsonFile.close();


   // Setup recording modules
   Comms::IComms *replacementComms = NULL;
   recordingPlaybackModule_ = new Recording();
   status = ConvertStatus(recordingPlaybackModule_->Init(robot_comms, &replacementComms, &config_, logFolderName + AnkiUtil::kP_ROBOT_COMMS_LOG_FILE));
   robot_comms = replacementComms;

   uiRecordingPlaybackModule_ = new Recording();
   status = ConvertStatus(uiRecordingPlaybackModule_->Init(ui_comms, &replacementComms, &config_, logFolderName + AnkiUtil::kP_UI_COMMS_LOG_FILE));
   ui_comms = replacementComms;
   break;
   }

   case BM_PLAYBACK_SESSION:
   {
   // Get log folder from config
   std::string logFolderName;
   if (!JsonTools::GetValueOptional(config, AnkiUtil::kP_PLAYBACK_LOG_FOLDER, logFolderName)) {
   PRINT_NAMED_ERROR("Basestation.Init.PlaybackDirNotSpecified", "\n");
   return BS_END_INIT_ERROR;
   }
   logFolderName = AnkiUtil::kP_GAME_LOG_ROOT_DIR + string("/") + logFolderName + "/";


   // Check if folder exists
   if (!DirExists(logFolderName.c_str())) {
   PRINT_NAMED_ERROR("Basestation.Init.PlaybackDirNotFound", "%s\n", logFolderName.c_str());
   return BS_END_INIT_ERROR;
   }

   // Load configuration json from playback log folder
   Json::Reader reader;
   std::ifstream jsonFile(logFolderName + AnkiUtil::kP_CONFIG_JSON_FILE);
   reader.parse(jsonFile, config_);
   jsonFile.close();


   // Setup playback modules
   Comms::IComms *replacementComms = NULL;
   recordingPlaybackModule_ = new Playback();
   status = ConvertStatus(recordingPlaybackModule_->Init(robot_comms, &replacementComms, &config_, logFolderName + AnkiUtil::kP_ROBOT_COMMS_LOG_FILE));
   robot_comms = replacementComms;

   uiRecordingPlaybackModule_ = new Playback();
   status = ConvertStatus(uiRecordingPlaybackModule_->Init(ui_comms, &replacementComms, &config_, logFolderName + AnkiUtil::kP_UI_COMMS_LOG_FILE));
   ui_comms = replacementComms;
   break;
   }

   case BM_DEFAULT:
   break;
   }
   */
}

Result CozmoEngineHostImpl::AddRobot(RobotID_t robotID)
{
  Result lastResult = RESULT_OK;

  _robotMgr.AddRobot(robotID, &_robotMsgHandler);
  Robot* robot = _robotMgr.GetRobotByID(robotID);
  if(nullptr == robot) {
    PRINT_NAMED_ERROR("CozmoEngineHostImpl.AddRobot", "Failed to add robot ID=%d (nullptr returned).\n", robotID);
    lastResult = RESULT_FAIL;
  } else {
    lastResult = robot->SyncTime();
  }

  return lastResult;
}

Robot* CozmoEngineHostImpl::GetFirstRobot()
{
  return _robotMgr.GetFirstRobot();
}

int CozmoEngineHostImpl::GetNumRobots() const
{
  const size_t N = _robotMgr.GetNumRobots();
  assert(N < INT_MAX);
  return static_cast<int>(N);
}

Robot* CozmoEngineHostImpl::GetRobotByID(const RobotID_t robotID)
{
  return _robotMgr.GetRobotByID(robotID);
}

std::vector<RobotID_t> const& CozmoEngineHostImpl::GetRobotIDList() const
{
  return _robotMgr.GetRobotIDList();
}

bool CozmoEngineHostImpl::ConnectToRobot(AdvertisingRobot whichRobot)
{
  // Check if already connected
  Robot* robot = CozmoEngineHostImpl::GetRobotByID(whichRobot);
  if (robot != nullptr) {
    PRINT_NAMED_INFO("CozmoEngineHost.ConnectToRobot.AlreadyConnected", "Robot %d already connected", whichRobot);
    return true;
  }

  // Connection is the same as normal except that we have to remove forcefully-added
  // robots from the advertising service manually (if they could do this, they also
  // could have registered itself)
  bool result = CozmoEngineImpl::ConnectToRobot(whichRobot);
  if(_forceAddedRobots.count(whichRobot) > 0) {
    PRINT_NAMED_INFO("CozmoEngineHostImpl.ConnectToRobot",
                     "Manually deregistering force-added robot %d from advertising service.\n", whichRobot);
    _robotAdvertisementService.DeregisterAllAdvertisers();
  }

  // Another exception for hosts: have to tell the basestation to add the robot as well
  AddRobot(whichRobot);

  return result;
}

void CozmoEngineHostImpl::ListenForRobotConnections(bool listen)
{
  _isListeningForRobots = listen;
}

Result CozmoEngineHostImpl::UpdateInternal(const BaseStationTime_t currTime_ns)
{
  ReloadAnimations(currTime_ns);

  // Update robot comms
  _robotChannel.Update();

  if(_isListeningForRobots) {
    _robotAdvertisementService.Update();
  }

  // Update time
  BaseStationTimer::getInstance()->UpdateTime(currTime_ns);


  _robotMsgHandler.ProcessMessages();

  // Let the robot manager do whatever it's gotta do to update the
  // robots in the world.
  _robotMgr.UpdateAllRobots();

  return RESULT_OK;
} // UpdateInternal()

void CozmoEngineHostImpl::ReloadAnimations(const BaseStationTime_t currTime_ns)
{
  if (!_animationReloadActive) {
    return;
  }
  const BaseStationTime_t reloadFrequency = static_cast<BaseStationTime_t>SEC_TO_NANOS(0.5f);
  if (_lastAnimationFolderScan + reloadFrequency < currTime_ns) {
    _lastAnimationFolderScan = currTime_ns;
    Robot* robot = _robotMgr.GetFirstRobot();
    if (robot != nullptr) {
      robot->ReadAnimationDir(true);
    }
  }
}

bool CozmoEngineHostImpl::GetCurrentRobotImage(RobotID_t robotID, Vision::Image& img, TimeStamp_t newerThanTime)
{
  Robot* robot = _robotMgr.GetRobotByID(robotID);

  if(robot != nullptr) {
    return robot->GetCurrentImage(img, newerThanTime);
  } else {
    PRINT_NAMED_ERROR("BasestationMainImpl.GetCurrentRobotImage.InvalidRobotID",
                      "Image requested for invalid robot ID = %d.\n", robotID);
    return false;
  }
}


} // namespace Cozmo
} // namespace Anki