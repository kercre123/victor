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

#include "anki/cozmo/game/cozmoGame.h"

#include "anki/cozmo/basestation/cozmoEngine.h"
#include "anki/cozmo/basestation/events/BaseStationEvent.h"
#include "anki/cozmo/basestation/utils/parsingConstants/parsingConstants.h"

#include "anki/common/basestation/utils/logging/logging.h"

#include "anki/cozmo/basestation/uiMessageHandler.h"
#include "anki/cozmo/basestation/multiClientComms.h"

namespace Anki {
namespace Cozmo {

#pragma mark - CozmoGameHost Implementation
  
  class CozmoGameHostImpl : public IBaseStationEventListener
  {
  public:
    enum PlaybackMode {
      LIVE_SESSION_NO_RECORD = 0,
      RECORD_SESSION,
      PLAYBACK_SESSION
    };
    
    using AdvertisingUiDevice = CozmoGameHost::AdvertisingUiDevice;
    using AdvertisingRobot    = CozmoGameHost::AdvertisingRobot;
    
    CozmoGameHostImpl();
    virtual ~CozmoGameHostImpl();
    
    Result Init(const Json::Value& config);
    
    void ForceAddRobot(int              robotID,
                       const char*      robotIP,
                       bool             robotIsSimulated);

    bool ConnectToUiDevice(AdvertisingUiDevice whichDevice);
    bool ConnectToRobot(AdvertisingRobot whichRobot);
    
    // Tick with game heartbeat:
    void Update(const float currentTime_sec);
    
  private:
    
    // Process raised events from CozmoEngine.
    // Req'd by IBaseStationEventListener
    virtual void OnEventRaised( const IBaseStationEventInterface* event ) override;
    
    enum RunState {
      STOPPED = 0
      ,WAITING_FOR_UI_DEVICES
      ,WAITING_FOR_ROBOTS
      ,ENGINE_RUNNING
    };
    
    RunState                     _runState;
    int                          _desiredNumUiDevices = 1;
    int                          _desiredNumRobots = 1;
    
    CozmoEngineHost              _cozmoEngine;
    
    // UI Comms Stuff
    Comms::AdvertisementService  _uiAdvertisementService;
    MultiClientComms             _uiComms;
    UiMessageHandler             _uiMsgHandler;
    u32                          _hostUiDeviceID;

    std::vector<AdvertisingUiDevice> _connectedUiDevices;
    
  }; // class CozmoGameHostImpl
  
  
  CozmoGameHostImpl::CozmoGameHostImpl()
  : _runState(STOPPED)
  , _uiAdvertisementService("UIAdvertisementService")
  , _hostUiDeviceID(1)
  {
    // Register for all events of interest here:
    BSE_RobotConnect::Register( this );
    BSE_DeviceDetectedVisionMarker::Register( this );
   
    BSE_RobotAvailable::Register( this );
    BSE_UiDeviceAvailable::Register( this );
    BSE_ConnectToRobot::Register( this );
    BSE_ConnectToUiDevice::Register( this );
    
    PRINT_NAMED_INFO("CozmoEngineHostImpl.Constructor",
                     "Starting UIAdvertisementService, reg port %d, ad port %d\n",
                     UI_ADVERTISEMENT_REGISTRATION_PORT, UI_ADVERTISING_PORT);
    
    _uiAdvertisementService.StartService(UI_ADVERTISEMENT_REGISTRATION_PORT,
                                         UI_ADVERTISING_PORT);
    
  }

  CozmoGameHostImpl::~CozmoGameHostImpl()
  {
    // Unregister for all events
    BSE_RobotConnect::Unregister( this );
    BSE_DeviceDetectedVisionMarker::Unregister( this );
    BSE_RobotAvailable::Unregister( this );
    BSE_ConnectToRobot::Unregister( this );
    BSE_ConnectToUiDevice::Unregister( this );
  }
  
  Result CozmoGameHostImpl::Init(const Json::Value& config)
  {
    /*
    // Force add any advertising UI device on this same device:
    Comms::AdvertisementRegistrationMsg localUiRegMsg;
    localUiRegMsg.id = 1;
    localUiRegMsg.port = UI_ADVERTISEMENT_REGISTRATION_PORT;
    localUiRegMsg.protocol = Comms::UDP;
    localUiRegMsg.enableAdvertisement = 1;
    _uiAdvertisementService.ProcessRegistrationMsg(localUiRegMsg);
    */
    
    _cozmoEngine.Init(config);
    
    if(!config.isMember(AnkiUtil::kP_ADVERTISING_HOST_IP) ||
       !config.isMember(AnkiUtil::kP_UI_ADVERTISING_PORT)) {
     
      PRINT_NAMED_ERROR("CozmoGameHostImpl.Init", "Missing advertising hosdt / UI advertising port in Json config file.\n");
      return RESULT_FAIL;
    }
      
    Result lastResult = _uiComms.Init(config[AnkiUtil::kP_ADVERTISING_HOST_IP].asCString(),
                                      config[AnkiUtil::kP_UI_ADVERTISING_PORT].asInt());

    if(lastResult != RESULT_OK) {
      PRINT_NAMED_ERROR("CozmoGameHostImpl.Init", "Failed to initialize host uiComms.\n");
      return lastResult;
    }
    
    _uiMsgHandler.Init(&_uiComms, &_cozmoEngine);
    
    if(!config.isMember(AnkiUtil::kP_NUM_ROBOTS_TO_WAIT_FOR)) {
      PRINT_NAMED_WARNING("CozmoGameHostImpl.Init", "No NumRobotsToWaitFor defined in Json config, defaulting to 1.\n");
      _desiredNumRobots = 1;
    } else {
      _desiredNumRobots    = config[AnkiUtil::kP_NUM_ROBOTS_TO_WAIT_FOR].asInt();
    }
    
    if(!config.isMember(AnkiUtil::kP_NUM_UI_DEVICES_TO_WAIT_FOR)) {
      PRINT_NAMED_WARNING("CozmoGameHostImpl.Init", "No NumUiDevicesToWaitFor defined in Json config, defaulting to 1.\n");
      _desiredNumUiDevices = 1;
    } else {
      _desiredNumUiDevices = config[AnkiUtil::kP_NUM_UI_DEVICES_TO_WAIT_FOR].asInt();
    }
    
    _runState = WAITING_FOR_UI_DEVICES;

      return RESULT_OK;
  }
  
  void CozmoGameHostImpl::ForceAddRobot(int              robotID,
                                        const char*      robotIP,
                                        bool             robotIsSimulated)
  {
    _cozmoEngine.ForceAddRobot(robotID, robotIP, robotIsSimulated);
  }
  
  
  bool CozmoGameHostImpl::ConnectToUiDevice(AdvertisingUiDevice whichDevice)
  {
    const bool success = _uiComms.ConnectToDeviceByID(whichDevice);
    if(success) {
      _connectedUiDevices.push_back(whichDevice);
    }
    
    return success;
  }

  bool CozmoGameHostImpl::ConnectToRobot(AdvertisingRobot whichRobot)
  {
    return _cozmoEngine.ConnectToRobot(whichRobot);
  }
  
  void CozmoGameHostImpl::Update(const float currentTime_sec) {
    /*
    // Connect to any robots we see:
    std::vector<CozmoEngine::AdvertisingRobot> advertisingRobots;
    _cozmoEngine.GetAdvertisingRobots(advertisingRobots);
    for(auto robot : advertisingRobots) {
      _cozmoEngine.ConnectToRobot(robot);
    }
    
    // Connect to any UI devices we see:
    std::vector<CozmoEngine::AdvertisingUiDevice> advertisingUiDevices;
    _cozmoEngine.GetAdvertisingUiDevices(advertisingUiDevices);
    for(auto device : advertisingUiDevices) {
      _cozmoEngine.ConnectToUiDevice(device);
    }
     */
    
    // Update ui comms
    if(_uiComms.IsInitialized()) {
      _uiComms.Update();
    }
    
    // Handle UI messages
    _uiMsgHandler.ProcessMessages();
    
    switch(_runState)
    {
      case STOPPED:
        // Nothing to do
        break;
        
      case WAITING_FOR_UI_DEVICES:
      {
        _uiAdvertisementService.Update();

        // TODO: Do we want to do this all the time in case UI devices want to join later?
        // Notify the UI that there are advertising devices
        std::vector<int> advertisingUiDevices;
        _uiComms.GetAdvertisingDeviceIDs(advertisingUiDevices);
        for(auto & device : advertisingUiDevices) {
          if(device == _hostUiDeviceID) {
            // Force connection to first (local) UI device
            if(true == ConnectToUiDevice(device)) {
              PRINT_NAMED_INFO("CozmoGameHostImpl.Update", "Automatically connected to local UI device %d!\n", device);
            }
          } else {
            BSE_UiDeviceAvailable::RaiseEvent(device);
          }
          /*
          MessageG2U_UiDeviceAvailable msg;
          msg.deviceID = device;
          _uiMsgHandler.SendMessage(1, msg);
           */
        }

        if(_uiComms.GetNumConnectedDevices() >= _desiredNumUiDevices) {
          PRINT_NAMED_INFO("CozmoGameHostImpl.Update",
                           "Enough UI devices connected (%d), will wait for %d robots.\n",
                           _desiredNumUiDevices, _desiredNumRobots);
          _cozmoEngine.ListenForRobotConnections(true);
          _runState = WAITING_FOR_ROBOTS;
        }
        break;
      }
    
      case WAITING_FOR_ROBOTS:
      {
        Result status = _cozmoEngine.Update(SEC_TO_NANOS(currentTime_sec));
        if (status != RESULT_OK) {
          PRINT_NAMED_WARNING("CozmoGameHostImpl.Update","Bad engine update: status = %d\n", status);
        }
        
        // Tell the engine to keep listening for robots until it reports that
        // it has connections to enough
        if(_cozmoEngine.GetNumRobots() >= _desiredNumRobots) {
          PRINT_NAMED_INFO("CozmoGameHostImpl.Update",
                           "Enough robots connected (%d), will run engine.\n",
                           _desiredNumRobots);
          // TODO: We could keep listening for others to join mid-game...
          _cozmoEngine.ListenForRobotConnections(false);
          _runState = ENGINE_RUNNING;
        }
        break;
      }
        
      case ENGINE_RUNNING:
      {
        Result status = _cozmoEngine.Update(SEC_TO_NANOS(currentTime_sec));
        if (status != RESULT_OK) {
          PRINT_NAMED_WARNING("CozmoGameHostImpl.Update","Bad engine update: status = %d\n", status);
        }
        break;
      }
        
      default:
        PRINT_NAMED_ERROR("CozmoHostImpl.Update", "Reached unknown RunState %d.\n", _runState);
        
    }
    
    /*
     std::vector<BasestationMain::ObservedObjectBoundingBox> boundingQuads;
     if(true == bs.GetCurrentVisionMarkers(1, boundingQuads) ) {
     // TODO: stuff?
     }
     */
  }
  
  // Process raised events from CozmoEngine
  void CozmoGameHostImpl::OnEventRaised( const IBaseStationEventInterface* event )
  {
    switch( event->GetEventType() )
    {
      case BSETYPE_RobotAvailable:
      {
        MessageG2U_RobotAvailable msg;
        msg.robotID = reinterpret_cast<const BSE_RobotAvailable*>(event)->robotID_;
        
        // TODO: Notify UI that robot is availale and let it issue message to connect
        //_uiMsgHandler.SendMessage(_hostUiDeviceID, msg);

        // For now, just connect any robot we see
        BSE_ConnectToRobot::RaiseEvent(msg.robotID);
        
        break;
      }
        
      case BSETYPE_UiDeviceAvailable:
      {
        MessageG2U_UiDeviceAvailable msg;
        msg.deviceID = reinterpret_cast<const BSE_UiDeviceAvailable*>(event)->deviceID_;
        
        // TODO: Notify UI that a UI device is availale and let it issue message to connect
        //_uiMsgHandler.SendMessage(_hostUiDeviceID, msg);
        
        // For now, just connect to any UI device we see:
        BSE_ConnectToUiDevice::RaiseEvent(msg.deviceID);
        
        break;
      }
        
      case BSETYPE_ConnectToRobot:
      {
        const RobotID_t robotID = reinterpret_cast<const BSE_ConnectToRobot*>(event)->robotID_;
        const bool success = ConnectToRobot(robotID);
        if(success) {
          PRINT_NAMED_INFO("CozmoGameHost.OnEventRaised", "Connected to robot %d!\n", robotID);
        } else {
          PRINT_NAMED_ERROR("CozmoGameHost.OnEventRaised", "Failed to connected to robot %d!\n", robotID);
        }
      }
        
      case BSETYPE_ConnectToUiDevice:
      {
        const u32 deviceID = reinterpret_cast<const BSE_ConnectToUiDevice*>(event)->deviceID_;
        const bool success = ConnectToUiDevice(deviceID);
        if(success) {
          PRINT_NAMED_INFO("CozmoGameHost.OnEventRaised", "Connected to UI device %d!\n", deviceID);
        } else {
          PRINT_NAMED_ERROR("CozmoGameHost.OnEventRaised", "Failed to connected to UI device %d!\n", deviceID);
        }
      }
        
        /*
      case BSETYPE_DeviceDetectedVisionMarker:
      {
        // Send a message out to UI that the device found a vision marker
        const BSE_DeviceDetectedVisionMarker *vmEvent = reinterpret_cast<const BSE_DeviceDetectedVisionMarker*>(event);
        assert(vmEvent != nullptr);
        
        MessageG2U_ObjectVisionMarker msg;
        msg.objectID = vmEvent->markerType_;
        
        break;
      }
       */
      case BSETYPE_RobotObservedObject:
      {
        // Send a message out to UI that the device found a vision marker
        const BSE_RobotObservedObject *obsObjEvent = reinterpret_cast<const BSE_RobotObservedObject*>(event);
        assert(obsObjEvent != nullptr);
        
        MessageG2U_ObjectVisionMarker msg;
        msg.robotID   = obsObjEvent->robotID_;
        msg.objectID  = obsObjEvent->objectID_;
        msg.topLeft_x = obsObjEvent->x_upperLeft_;
        msg.topLeft_y = obsObjEvent->y_upperLeft_;
        msg.width     = obsObjEvent->width_;
        msg.height    = obsObjEvent->height_;
        
        // TODO: Look up which UI device to notify based on the robotID that saw the object
        _uiMsgHandler.SendMessage(1, msg);
        
        break;
      }
        
      default:
        printf("CozmoGame: Received unknown event %d\n", event->GetEventType());
        break;
    }
  }
  
  
#pragma mark - CozmoGameHost Wrappers
  
  CozmoGameHost::CozmoGameHost()
  {
    _impl = new CozmoGameHostImpl();
  }
  
  CozmoGameHost::~CozmoGameHost()
  {
    delete _impl;
  }
  
  bool CozmoGameHost::Init(const Json::Value &config)
  {
    return _impl->Init(config) == RESULT_OK;
  }
  
  void CozmoGameHost::ForceAddRobot(int              robotID,
                                    const char*      robotIP,
                                    bool             robotIsSimulated)
  {
    _impl->ForceAddRobot(robotID, robotIP, robotIsSimulated);
  }
  
  bool CozmoGameHost::ConnectToRobot(AdvertisingRobot whichRobot)
  {
    return _impl->ConnectToRobot(whichRobot);
  }
  
  bool CozmoGameHost::ConnectToUiDevice(AdvertisingUiDevice whichDevice)
  {
    return _impl->ConnectToUiDevice(whichDevice);
  }
  
  void CozmoGameHost::Update(const float currentTime_sec)
  {
    _impl->Update(currentTime_sec);
  }
  
} // namespace Cozmo
} // namespace Anki


