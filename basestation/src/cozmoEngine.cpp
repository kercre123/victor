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

#include "anki/cozmo/basestation/cozmoEngine.h"

#include "anki/cozmo/basestation/visionProcessingThread.h"
#include "anki/cozmo/basestation/basestation.h"
#include "anki/cozmo/basestation/multiClientComms.h"
#include "anki/cozmo/basestation/utils/parsingConstants/parsingConstants.h"

#include "anki/messaging/basestation/advertisementService.h"

#include "anki/common/basestation/jsonTools.h"


namespace Anki {
namespace Cozmo {
  
  // Helper macro to only free non-null pointers and then set them to nullptr:
#define FREE_HELPER(__ptr__) if(__ptr__ != nullptr) { delete __ptr__; __ptr__ = nullptr; }

  
#if 0
#pragma mark -
#pragma mark Base Class Implementations
#endif
  
  CozmoEngine::CozmoEngine()
  : _isInitialized(false)
  , _robotVisionThread(nullptr)
  , _deviceVisionThread(nullptr)
  , _robotComms(nullptr)
  , _gameComms(nullptr)
  , _robotVisionMsgHandler(nullptr)
  , _deviceVisionMsgHandler(nullptr)
  {
    
  }
  
  CozmoEngine::~CozmoEngine()
  {
    
    FREE_HELPER(_robotVisionThread)
    FREE_HELPER(_deviceVisionThread)
    
    FREE_HELPER(_robotComms)
    FREE_HELPER(_gameComms)
    
    FREE_HELPER(_robotVisionMsgHandler)
    FREE_HELPER(_deviceVisionMsgHandler)
    
  }
  
  Result CozmoEngine::Init(const Json::Value& config)
  {
    if(!_isInitialized) {
      // Check everything first, before we create anything, so that we can return
      // failures without needing to worry about leaks from later memory allocation
      if(!config.isMember(AnkiUtil::kP_ADVERTISING_HOST_IP)) {
        PRINT_NAMED_ERROR("CozmoEngine.Init", "No AdvertisingHostIP defined in Json config.\n");
        return RESULT_FAIL;
      }
      
      if(!config.isMember(AnkiUtil::kP_ROBOT_ADVERTISING_PORT)) {
        PRINT_NAMED_ERROR("CozmoEngine.Init", "No RobotAdvertisingPort defined in Json config.\n");
        return RESULT_FAIL;
      }
      
      if(!config.isMember(AnkiUtil::kP_UI_ADVERTISING_PORT)) {
        PRINT_NAMED_ERROR("CozmoEngine.Init", "No UiAdvertisingPort defined in Json config.\n");
        return RESULT_FAIL;
      }
      
      //
      // Actually do the allocations below here:
      //
      
      // TODO: Only create these if/when they are needed for the particular game/state?
      _robotVisionThread  = new VisionProcessingThread();
      _deviceVisionThread = new VisionProcessingThread();
      
      _robotComms = new MultiClientComms(config[AnkiUtil::kP_ADVERTISING_HOST_IP].asCString(),
                                         config[AnkiUtil::kP_ROBOT_ADVERTISING_PORT].asInt());
      
      _gameComms = new MultiClientComms(config[AnkiUtil::kP_ADVERTISING_HOST_IP].asCString(),
                                        config[AnkiUtil::kP_UI_ADVERTISING_PORT].asInt());
      
      _isInitialized = true;
    }
                                         
    return RESULT_OK;
  }
  
  Result CozmoEngine::Update()
  {
    if(!_isInitialized) {
      PRINT_NAMED_ERROR("CozmoEngine.Init", "Cannot update CozmoEngine before it is initialized.\n");
      return RESULT_FAIL;
    }
    
    // Update robot comms
    if(_robotComms != nullptr) {
      _robotComms->Update();
      
      // If not already connected to a robot, connect to the
      // first one that becomes available.
      // TODO: Once we have a UI, we can select the one we want to connect to in a more reasonable way.
      if (_robotComms->GetNumConnectedDevices() == 0) {
        std::vector<int> advertisingRobotIDs;
        if (_robotComms->GetAdvertisingDeviceIDs(advertisingRobotIDs) > 0) {
          for(auto robotID : advertisingRobotIDs) {
            printf("RobotComms connecting to robot %d.\n", robotID);
            if (_robotComms->ConnectToDeviceByID(robotID)) {
              printf("Connected to robot %d\n", robotID);
              
              // Add connected_robot ID to config
              config[AnkiUtil::kP_CONNECTED_ROBOTS].append(robotID);
              
              break;
            } else {
              printf("Failed to connect to robot %d\n", robotID);
              return BS_END_INIT_ERROR;
            }
          }
        }
      }
    } // if(_robotComms != nullptr)

  }
  
#if 0
#pragma mark -
#pragma mark Derived Host Class Implementations
#endif
  
  CozmoEngineHost::CozmoEngineHost()
  : _basestation(nullptr)
  , _robotAdvertisementService(nullptr)
  , _uiAdvertisementService(nullptr)
  {
    
  }
  
  CozmoEngineHost::~CozmoEngineHost()
  {
    FREE_HELPER(_basestation)
    FREE_HELPER(_robotAdvertisementService)
    FREE_HELPER(_uiAdvertisementService)
  }

  Result CozmoEngineHost::Update()
  {
    // Call base class update first
    CozmoEngine::Update();
    
    
  }
  
  
#if 0
#pragma mark -
#pragma mark Derived Client Class Implementations
#endif
  
  CozmoEngineClient::CozmoEngineClient()
  : CozmoEngine()
  {
    
  }
  
  
#undef FREE_HELPER
  
} // namespace Cozmo
} // namespace Anki