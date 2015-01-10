/*
 * File:          basestation.cpp
 * Date:
 * Description:   
 * Author:        
 * Modifications: 
 */

#include <fstream>
#include <exception>

#include "anki/common/basestation/math/pose.h"
#include "anki/common/basestation/math/rotatedRect_impl.h"
#include "anki/common/basestation/utils/timer.h"
#include "anki/common/basestation/utils/fileManagement.h"
#include "anki/common/basestation/utils/logging/logging.h"
#include "anki/common/basestation/math/quad_impl.h"
#include "anki/common/basestation/math/rect_impl.h"
#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/basestation/jsonTools.h"
#include "anki/common/basestation/platformPathManager.h"

#include "anki/cozmo/basestation/basestation.h"
#include "anki/cozmo/robot/cozmoConfig.h"
#include "anki/cozmo/basestation/blockWorld.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotManager.h"
#include "anki/cozmo/basestation/utils/exceptions.h"
#include "anki/cozmo/basestation/utils/parsingConstants/parsingConstants.h"

#include "multiClientComms.h"
#include "recording/playback.h"
#include "messageHandler.h"
#include "uiMessageHandler.h"
#include "pathPlanner.h"
#include "behaviorManager.h"
#include "vizManager.h"
#include "soundManager.h"


namespace Anki {
namespace Cozmo {


  class BasestationMainImpl
  {
  public:
    // Constructor
    BasestationMainImpl();
    
    // Destructor
    virtual ~BasestationMainImpl();
    
    // Initializes components of basestation
    // RETURNS: BS_OK, or BS_END_INIT_ERROR
    //BasestationStatus Init(const MetaGame::GameParameters& params, IComms* comms, boost::property_tree::ptree &config, BasestationMode mode);
    BasestationStatus Init(Comms::IComms* robot_comms, Comms::IComms* ui_comms, Json::Value& config);
    
    // unintializes basestation. returns BS_END_CLEAN_EXIT if all ok
    BasestationStatus UnInit();
     
    // Removes / cleans up all singletons
    static void RemoveSingletons();
    
    // Runs an iteration of the base-station.  Takes an argument for the current
    // system time.
    BasestationStatus Update(BaseStationTime_t currTime);
    
     // Converts recording / playback module status to basestation status.
     BasestationStatus ConvertStatus(RecordingPlaybackStatus status);
    /*
     // returns true if the basestation is loaded (including possibly planner table computation, etc)
     bool DoneLoading();
     
     // returns the loading progress while the basestation is loading (between 0 and 1)
     float GetLoadingProgress();
     
     
     // stops game
     static void StopGame();
     */

    bool GetCurrentRobotImage(const RobotID_t robotID, Vision::Image& img, TimeStamp_t newerThan);

    bool GetCurrentVisionMarkers(const RobotID_t robotID,
                                 std::vector<BasestationMain::ObservedObjectBoundingBox>& boundingQuad);
    s32 GetAnimationID(const RobotID_t robotID,
                       const std::string& animationName);
    
    bool GetRobotPose(const RobotID_t robotID, Pose3d& pose, Radians& headAngle, f32& liftHeight);
    
  private:
    // Instantiate all the modules we need
    //BlockWorld blockWorld_;
    RobotManager robotMgr_;
    MessageHandler msgHandler_;
    //BehaviorManager behaviorMgr_;
    UiMessageHandler uiMsgHandler_;
    
    //LatticePlanner *pathPlanner_;
    
    /*
     // process message queue
     void ProcessUiMessagesInPausedState();
     
     // process message queue
     void ProcessUiMessages();
     */

    Json::Value config_;
    IRecordingPlaybackModule *recordingPlaybackModule_;
    IRecordingPlaybackModule *uiRecordingPlaybackModule_;
    
    /*
     boost::property_tree::ptree config_;
     MetaGame::GameSettings* gameSettings_;
     */


    
  };
   
  BasestationMainImpl::BasestationMainImpl()
  {
    
  }

  BasestationMainImpl::~BasestationMainImpl()
  {

  }

  BasestationStatus BasestationMainImpl::Init(Comms::IComms* robot_comms, Comms::IComms* ui_comms, Json::Value& config)
  {
    BasestationStatus status = BS_OK;
    
    // Copy config
    config_ = config;

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
    
    // Initialize the modules by telling them about each other:
    msgHandler_.Init(robot_comms, &robotMgr_);
    robotMgr_.Init(&msgHandler_);
    uiMsgHandler_.Init(ui_comms, &robotMgr_);
    
    if(config.isMember(AnkiUtil::kP_VIZ_HOST_IP)) {
      VizManager::getInstance()->Connect(config[AnkiUtil::kP_VIZ_HOST_IP].asCString(), VIZ_SERVER_PORT);
      
      // Only send images if the viz host is the same as the robot advertisement service
      // (so we don't waste bandwidth sending (uncompressed) viz data over the network
      //  to be displayed on another machine)
      if(config[AnkiUtil::kP_VIZ_HOST_IP] == config[AnkiUtil::kP_ADVERTISING_HOST_IP]) {
        VizManager::getInstance()->EnableImageSend(true);
      }
      
    } else {
      PRINT_NAMED_WARNING("BasestationInit.NoVizHostIP", "No VizHostIP member in JSON config file. Not initializing VizManager.\n");
    }


    
    // Instantiate and init connected robots
    for (auto robotIDVal : config_[AnkiUtil::kP_CONNECTED_ROBOTS]) {
      RobotID_t robotID = (RobotID_t)robotIDVal.asInt();
      robotMgr_.AddRobot(robotID);
      
      Robot* robot = robotMgr_.GetRobotByID(robotID);
      
      assert(robot != nullptr);
      
      robot->SyncTime();
      
      // Have to do this until we have Events triggering inside the robot:
      robot->SetUiMessageHandler(&uiMsgHandler_);
      
      // Uncomment to Test "reactions":
      // TODO: Make a keypress for switching to this behavior mode
      // robotMgr_.GetRobotByID(robotID)->StartBehaviorMode(BehaviorManager::ReactToMarkers);
    }

    
    return status;
  }
    
  BasestationStatus BasestationMainImpl::UnInit()
  {
    try {
      
      // Uninitialization sequence
      VizManager::getInstance()->Disconnect();
      RemoveSingletons();
      
    }
    // catch Basestation exceptions
    catch (ExceptionType& bsException)
    {
      PRINT_NAMED_ERROR("Basestation.InternalException", "%s", GetExceptionDefinition(bsException));
      return BS_END_RUN_EXCEPTION_CAUGHT;
    }
    // catch c++ exceptions
    catch (std::exception& e) {
      PRINT_NAMED_ERROR("Basestation.UnInit", "%s\n", e.what());
      return BS_END_UNINIT_EXCEPTION_CAUGHT;
    }
    // catch all other issues
    catch (...)
    {
      PRINT_NAMED_ERROR("Basestation.UnInit", "Unknown error\n");
      return BS_END_UNINIT_EXCEPTION_CAUGHT;
    }
    
    return BS_END_CLEAN_EXIT;
  }
    
  void BasestationMainImpl::RemoveSingletons()
  {
    VizManager::removeInstance();
    SoundManager::removeInstance();
  }

    
    
  BasestationStatus BasestationMainImpl::Update(BaseStationTime_t currTime)
  {
    BasestationStatus status = BS_OK;
    
    // Update time
    BaseStationTimer::getInstance()->UpdateTime(currTime);
    
    // Read UI messages
    uiMsgHandler_.ProcessMessages();
    
    msgHandler_.ProcessMessages();
      
    // Let the robot manager do whatever it's gotta do to update the
    // robots in the world.
    robotMgr_.UpdateAllRobots();

      return status;
  }

  // Converts recording / playback module status to basestation status.
  BasestationStatus BasestationMainImpl::ConvertStatus(RecordingPlaybackStatus status)
  {
    switch (status)
    {
      case RPMS_OK:
        return BS_OK;
        
      case RPMS_INIT_ERROR:
        return BS_END_INIT_ERROR;
        
      case RPMS_ERROR:
        return BS_PLAYBACK_ERROR;
        
      case RPMS_PLAYBACK_ENDED:
        return BS_PLAYBACK_ENDED;
        
      case RPMS_VERSION_MISMATCH:
        return BS_PLAYBACK_VERSION_MISMATCH;
    }
  }

  bool BasestationMainImpl::GetCurrentRobotImage(const RobotID_t robotID, Vision::Image& img, TimeStamp_t newerThan)
  {
    Robot* robot = robotMgr_.GetRobotByID(robotID);
    
    if(robot != nullptr) {
      return robot->GetCurrentImage(img, newerThan);
    } else {
      PRINT_NAMED_ERROR("BasestationMainImpl.GetCurrentRobotImage.InvalidRobotID",
                        "Image requested for invalid robot ID = %d.\n", robotID);
      return false;
    }
  }
    
    
  bool BasestationMainImpl::GetCurrentVisionMarkers(const RobotID_t robotID,
                                                    std::vector<BasestationMain::ObservedObjectBoundingBox>& boundingQuads)
  {
    Robot* robot = robotMgr_.GetRobotByID(robotID);
    if(robot != nullptr) {
      const BlockWorld::ObservedObjectBoundingBoxes& obsObjects = robot->GetBlockWorld().GetProjectedObservedObjects();
      for(auto obsObject : obsObjects) {
        boundingQuads.emplace_back(obsObject.first.GetValue(), obsObject.second);
        
        // Display
        Quad2f quad;
        obsObject.second.GetQuad(quad);
        VizManager::getInstance()->DrawCameraQuad(0, quad, NamedColors::GREEN);
      }
      return true;
    } else {
      PRINT_NAMED_ERROR("BasestationMainImpl.GetCurrentVisionMarkers.InvalidRobotID",
                        "Image requested for invalid robot ID = %d.\n", robotID);
      return false;
    }
  }
    
  s32 BasestationMainImpl::GetAnimationID(const RobotID_t robotID,
                                          const std::string& animationName)
  {
    Robot* robot = robotMgr_.GetRobotByID(robotID);
    if(robot != nullptr) {
      return robot->GetAnimationID(animationName);
    } else {
      PRINT_NAMED_ERROR("BasestationMainImpl.GetAnimationID.InvalidRobotID",
                        "Animation ID requested for invalid robot ID = %d.\n", robotID);
      return -2;
    }
  }
  
  bool BasestationMainImpl::GetRobotPose(const RobotID_t robotID, Pose3d& pose, Radians& headAngle, f32& liftHeight)
  {
    Robot* robot = robotMgr_.GetRobotByID(robotID);
    if(robot != nullptr) {
      pose       = robot->GetPose();
      headAngle  = robot->GetHeadAngle();
      liftHeight = robot->GetLiftHeight();
      return true;
    } else {
      PRINT_NAMED_ERROR("BasestationMainImpl.GetRobotPose.InvalidRobotID",
                        "Pose requested for invalid robot ID = %d.\n", robotID);
      return false;
    }
  }


  // =========== Start BasestationMain forwarding functions =======
    
    
  BasestationMain::BasestationMain()
  {
    impl_ = new BasestationMainImpl();
  }

  BasestationMain::~BasestationMain()
  {
    delete impl_;
  }

  BasestationStatus BasestationMain::Init(Comms::IComms* robot_comms, Comms::IComms* ui_comms, Json::Value& config)
  {
    return impl_->Init(robot_comms, ui_comms, config);
  }

  BasestationStatus BasestationMain::UnInit()
  {
    return impl_->UnInit();
  }
    
  BasestationStatus BasestationMain::Update(BaseStationTime_t currTime)
  {
    return impl_->Update(currTime);
  }
    
  bool BasestationMain::GetCurrentRobotImage(const RobotID_t robotID, Vision::Image& img, TimeStamp_t newerThan)
  {
    return impl_->GetCurrentRobotImage(robotID, img, newerThan);
  }
    
  bool BasestationMain::GetCurrentVisionMarkers(const RobotID_t robotID,
                                                std::vector<ObservedObjectBoundingBox>& boundingQuads)
  {
    return impl_->GetCurrentVisionMarkers(robotID, boundingQuads);
  }

  s32 BasestationMain::GetAnimationID(const RobotID_t robotID,
                     const std::string& animationName)
  {
    return impl_->GetAnimationID(robotID, animationName);
  }
    
  bool BasestationMain::GetRobotPose(const RobotID_t robotID, Pose3d& pose, Radians& headAngle, f32& liftHeight)
  {
    return impl_->GetRobotPose(robotID, pose, headAngle, liftHeight);
  }

  
} // namespace Cozmo
} // namespace Anki
