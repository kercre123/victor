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
#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/basestation/jsonTools.h"
#include "anki/common/basestation/platformPathManager.h"

#include "anki/cozmo/basestation/basestation.h"
#include "anki/cozmo/robot/cozmoConfig.h"
#include "anki/cozmo/basestation/blockWorld.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/tcpComms.h"
#include "anki/cozmo/basestation/uiTcpComms.h"
#include "anki/cozmo/basestation/utils/exceptions.h"
#include "anki/cozmo/basestation/utils/parsingConstants/parsingConstants.h"

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
  BasestationStatus Init(Comms::IComms* robot_comms, Comms::IComms* ui_comms, Json::Value& config, BasestationMode mode);
  
  BasestationMode GetMode();
  
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

  
private:
  // Instantiate all the modules we need
  BlockWorld blockWorld_;
  RobotManager robotMgr_;
  MessageHandler msgHandler_;
  BehaviorManager behaviorMgr_;
  UiMessageHandler uiMsgHandler_;
  
  LatticePlanner *pathPlanner_;
  
  /*
   // process message queue
   void ProcessUiMessagesInPausedState();
   
   // process message queue
   void ProcessUiMessages();
   */
  BasestationMode mode_;
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
  delete pathPlanner_;
}

  
BasestationMode BasestationMainImpl::GetMode()
{
  return mode_;
}


BasestationStatus BasestationMainImpl::Init(Comms::IComms* robot_comms, Comms::IComms* ui_comms, Json::Value& config, BasestationMode mode)
{
  BasestationStatus status = BS_OK;
  
  // Copy config
  config_ = config;
  
  PRINT_INFO("Starting basestation mode %d\n", mode);
  mode_ = mode;
  switch(mode)
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
  
  
  // read planner motion primitives
  Json::Value mprims;
  const std::string subPath("coretech/planning/matlab/cozmo_mprim.json");
  const std::string jsonFilename = PREPEND_SCOPED_PATH(Config, subPath);

  Json::Reader reader;
  std::ifstream jsonFile(jsonFilename);
  reader.parse(jsonFile, mprims);
  jsonFile.close();

  pathPlanner_ = new LatticePlanner(&blockWorld_, mprims);
  
  // Initialize the modules by telling them about each other:
  msgHandler_.Init(robot_comms, &robotMgr_, &blockWorld_);
  robotMgr_.Init(&msgHandler_, &blockWorld_, pathPlanner_);
  blockWorld_.Init(&robotMgr_);
  behaviorMgr_.Init(&robotMgr_, &blockWorld_);
  uiMsgHandler_.Init(ui_comms, &robotMgr_, &blockWorld_, &behaviorMgr_);
  
  VizManager::getInstance()->Connect(ROBOT_SIM_WORLD_HOST, VIZ_SERVER_PORT);

  
  // Instantiate and init connected robots
  for (auto robotIDVal : config_[AnkiUtil::kP_CONNECTED_ROBOTS]) {
    RobotID_t robotID = (RobotID_t)robotIDVal.asInt();
    robotMgr_.AddRobot(robotID);
    robotMgr_.GetRobotByID(robotID)->SendInit();
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
    
  // Draw observed markers, but only if images are being streamed
  blockWorld_.DrawObsMarkers();
  
  // Update the world (force robots to process their messages)
  uint32_t numBlocksObserved = 0;
  blockWorld_.Update(numBlocksObserved);
  
  // Update the behavior manager.
  // TODO: This object encompasses, for the time-being, what some higher level
  // module(s) would do.  e.g. Some combination of game state, build planner,
  // personality planner, etc.
  behaviorMgr_.Update();
  
  
  

  /////////// Update visualization ////////////
  { // Update Block-of-Interest display
    
    // Get selected block of interest from Behavior manager
    static ObjectID prev_boi;      // Previous block of interest
    static size_t prevNumPreDockPoses = 0;  // Previous number of predock poses
    // TODO: store previous block's color and restore it when unselecting
    
    // Draw current block of interest
    const ObjectID boi = behaviorMgr_.GetObjectOfInterest();
    
    DockableObject* block = dynamic_cast<DockableObject*>(blockWorld_.GetObjectByID(boi));
    if(block != nullptr) {

      // Get predock poses
      std::vector<Block::PoseMarkerPair_t> poses;
      block->GetPreDockPoses(PREDOCK_DISTANCE_MM, poses);
      
      // Erase previous predock pose marker for previous block of interest
      if (prev_boi != boi || poses.size() != prevNumPreDockPoses) {
        PRINT_INFO("BOI %d (prev %d), numPoses %d (prev %zu)\n",
                   boi.GetValue(), prev_boi.GetValue(), (u32)poses.size(), prevNumPreDockPoses);
        VizManager::getInstance()->EraseVizObjectType(VIZ_OBJECT_PREDOCKPOSE);
        
        // Return previous selected block to original color (necessary in the
        // case that this block isn't currently being observed, meaning its
        // visualization won't have updated))
        DockableObject* prevBlock = dynamic_cast<DockableObject*>(blockWorld_.GetObjectByID(prev_boi));
        if(prevBlock != nullptr && prevBlock->GetLastObservedTime() < BaseStationTimer::getInstance()->GetCurrentTimeStamp()) {
          prevBlock->Visualize(VIZ_COLOR_DEFAULT);
        }
        
        prev_boi = boi;
        prevNumPreDockPoses = poses.size();
      }

      // Draw cuboid for current selection, with predock poses
      block->Visualize(VIZ_COLOR_SELECTED_OBJECT, PREDOCK_DISTANCE_MM);
      
    } else {
      // block == nullptr (no longer exists, delete its predock poses)
      VizManager::getInstance()->EraseVizObjectType(VIZ_OBJECT_PREDOCKPOSE);
    }
    
  } // if blocks were updated
  
  // Draw all robot poses
  // TODO: Only send when pose has changed?
  for(auto robotID : robotMgr_.GetRobotIDList())
  {
    Robot* robot = robotMgr_.GetRobotByID(robotID);
    
    // Triangle pose marker
    VizManager::getInstance()->DrawRobot(robotID, robot->GetPose());
    
    // Full Webots CozmoBot model
    VizManager::getInstance()->DrawRobot(robotID, robot->GetPose(), robot->GetHeadAngle(), robot->GetLiftAngle());
    
    // Robot bounding box
    using namespace Quad;
    Quad2f quadOnGround2d = robot->GetBoundingQuadXY();
    Quad3f quadOnGround3d(Point3f(quadOnGround2d[TopLeft].x(),     quadOnGround2d[TopLeft].y(),     0.5f),
                          Point3f(quadOnGround2d[BottomLeft].x(),  quadOnGround2d[BottomLeft].y(),  0.5f),
                          Point3f(quadOnGround2d[TopRight].x(),    quadOnGround2d[TopRight].y(),    0.5f),
                          Point3f(quadOnGround2d[BottomRight].x(), quadOnGround2d[BottomRight].y(), 0.5f));

    VizManager::getInstance()->DrawRobotBoundingBox(robot->GetID(), quadOnGround3d, VIZ_COLOR_ROBOT_BOUNDING_QUAD);
    
    if(robot->IsCarryingObject()) {
      DockableObject* carryBlock = dynamic_cast<DockableObject*>(blockWorld_.GetObjectByID(robot->GetCarryingObject()));
      if(carryBlock == nullptr) {
        PRINT_NAMED_ERROR("BlockWorldController.CarryBlockDoesNotExist", "Robot %d is marked as carrying block %d but that block no longer exists.\n", robot->GetID(), robot->GetCarryingObject().GetValue());
        robot->UnSetCarryingObject();
      } else {
        carryBlock->Visualize(VIZ_COLOR_DEFAULT);
      }
    }
  }

  /////////// End visualization update ////////////

  
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
  
  

// =========== Start BasestationMain forwarding functions =======
  
  
BasestationMain::BasestationMain()
{
  impl_ = new BasestationMainImpl();
}

BasestationMain::~BasestationMain()
{
  delete impl_;
}

BasestationStatus BasestationMain::Init(Comms::IComms* robot_comms, Comms::IComms* ui_comms, Json::Value& config, BasestationMode mode)
{
  return impl_->Init(robot_comms, ui_comms, config, mode);
}

BasestationMode BasestationMain::GetMode()
{
  return impl_->GetMode();
}

BasestationStatus BasestationMain::UnInit()
{
  return impl_->UnInit();
}
  
BasestationStatus BasestationMain::Update(BaseStationTime_t currTime)
{
  return impl_->Update(currTime);
}
  
} // namespace Cozmo
} // namespace Anki
