/**
 * File:        behaviorDevEventSequenceCapture.cpp
 *
 * Author:      Humphrey Hu
 * Created:     2018-05-14
 *
 * Description: Dev behavior to trigger a user event and capture data before and after it.
 * 
 * Parameters:  All params are read through devEventSequenceCapture.json.
 * 
 * Usage:       After pressing the touch sensor or button, the robot will play a sound and wait for a specified setup
 *              time, during which the user should prepare to generate the event. After the setup time, the robot will
 *              play a sound and save images for the pre-event capture time. After this time, the robot will play a 
 *              sound to notify the user to generate the event, and will continue saving images for the post-event 
 *              capture time. Finally the robot will play a sound to signal the end of the sequence.
 *              
 *              The screen will mirror the image and display the number of sequences stored for the current event 
 *              class. Sequences carry over across restarts and will not be overwritten. The class can be changed by 
 *              moving the robot lift up and down.
 * 
 * Storage:     Sequences are stored as numbered folders in <cache>/images/<class>/. The numbers begin with 0 and are
 *              unpadded. Sequence images are saved in their respective folders along with their JSON sidecars and a
 *              sequenceInfo.json that contains the sequence start, event, and end timestamps.
 * 
 *
 * Copyright:   Anki, Inc. 2018
 *
 **/


#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"
#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/behaviorDevEventSequenceCapture.h"
#include "engine/audio/engineRobotAudioClient.h"
#include "engine/components/bodyLightComponent.h"
#include "engine/components/movementComponent.h"
#include "engine/components/visionComponent.h"
#include "engine/components/sensors/touchSensorComponent.h"
#include "engine/cozmoContext.h"
#include "engine/externalInterface/externalInterface.h"
#include "util/fileUtils/fileUtils.h"
#include "util/random/randomGenerator.h"

#include <chrono>
#include <fstream>

namespace Anki {
namespace Cozmo {

namespace {

// constexpr const float kLightBlinkPeriod_s = 0.5f;

static const BackpackLights kLightsWaiting = {
  .onColors               = {{NamedColors::GREEN,NamedColors::GREEN,NamedColors::GREEN}},
  .offColors              = {{NamedColors::GREEN,NamedColors::GREEN,NamedColors::GREEN}},
  .onPeriod_ms            = {{0,0,0}},
  .offPeriod_ms           = {{0,0,0}},
  .transitionOnPeriod_ms  = {{0,0,0}},
  .transitionOffPeriod_ms = {{0,0,0}},
  .offset                 = {{0,0,0}}
};

static const BackpackLights kLightsSetup = {
  .onColors               = {{NamedColors::BLUE,NamedColors::BLUE,NamedColors::BLUE}},
  .offColors              = {{NamedColors::BLUE,NamedColors::BLUE,NamedColors::BLUE}},
  .onPeriod_ms            = {{0,0,0}},
  .offPeriod_ms           = {{0,0,0}},
  .transitionOnPeriod_ms  = {{0,0,0}},
  .transitionOffPeriod_ms = {{0,0,0}},
  .offset                 = {{0,0,0}}
};

static const BackpackLights kLightsPreCap = {
  .onColors               = {{NamedColors::RED,NamedColors::RED,NamedColors::RED}},
  .offColors              = {{NamedColors::RED,NamedColors::RED,NamedColors::RED}},
  .onPeriod_ms            = {{0,0,0}},
  .offPeriod_ms           = {{0,0,0}},
  .transitionOnPeriod_ms  = {{0,0,0}},
  .transitionOffPeriod_ms = {{0,0,0}},
  .offset                 = {{0,0,0}}
};
  
static const BackpackLights kLightsPostCap = {
  .onColors               = {{NamedColors::RED,NamedColors::RED,NamedColors::RED}},
  .offColors              = {{NamedColors::RED,NamedColors::RED,NamedColors::RED}},
  .onPeriod_ms            = {{0,0,0}},
  .offPeriod_ms           = {{0,0,0}},
  .transitionOnPeriod_ms  = {{0,0,0}},
  .transitionOffPeriod_ms = {{0,0,0}},
  .offset                 = {{0,0,0}}
};

const char* const kSavePathKey = "save_path";
const char* const kImageSaveQualityKey = "quality";
const char* const kImageScaleKey = "image_scale";
const char* const kImageResizeMethodKey = "resize_method";
const char* const kUseCapacitiveTouchKey = "use_capacitive_touch";
const char* const kClassNamesKey = "class_names";
const char* const kSequenceSetupTimeKey = "sequence_setup_time";
const char* const kPreEventCaptureTimeKey = "pre_event_capture_time";
const char* const kPostEventCaptureTimeKey = "post_event_capture_time";
const char* const kEnableRandomHeadTiltKey = "enable_random_head_tilt";
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDevEventSequenceCapture::InstanceConfig::InstanceConfig()
{
  useCapTouch = false;
  imageSaveSize = Vision::ImageCache::Size::Full;
}

BehaviorDevEventSequenceCapture::DynamicVariables::DynamicVariables()
{
  seqState = SequenceState::Waiting;
  currentSeqNumber = 0;

  wasTouched = false;
  wasLiftUp = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDevEventSequenceCapture::BehaviorDevEventSequenceCapture(const Json::Value& config)
  : ICozmoBehavior(config)
{
  _iConfig.imageSavePath = JsonTools::ParseString(config, kSavePathKey, "BehaviorDevEventSequenceCapture");
  _iConfig.imageSaveQuality = JsonTools::ParseInt8(config, kImageSaveQualityKey, "BehaviorDevEventSequenceCapture");
  _iConfig.useCapTouch = JsonTools::ParseBool(config, kUseCapacitiveTouchKey, "BehaviorDevEventSequenceCapture");

  std::string scaleStr = JsonTools::ParseString(config, kImageScaleKey, "BehaviorDevEventSequenceCapture");
  std::string methodStr = JsonTools::ParseString(config, kImageResizeMethodKey, "BehaviorDevEventSequenceCapture");
  _iConfig.imageSaveSize = Vision::ImageCache::StringToSize(scaleStr, methodStr);

  _iConfig.sequenceSetupTime = JsonTools::ParseFloat(config, kSequenceSetupTimeKey, "BehaviorDevEventSequenceCapture");
  _iConfig.preEventCaptureTime = JsonTools::ParseFloat(config, kPreEventCaptureTimeKey, "BehaviorDevEventSequenceCapture");
  _iConfig.postEventCaptureTime = JsonTools::ParseFloat(config, kPostEventCaptureTimeKey, "BehaviorDevEventSequenceCapture");

  _iConfig.enableRandomHeadTilt = JsonTools::ParseBool(config, kEnableRandomHeadTiltKey, "BehaviorDevEventSequenceCapture");

  if(config.isMember(kClassNamesKey))
  {
    auto const& classNames = config[kClassNamesKey];
    if(classNames.isArray())
    {
      for(Json::ArrayIndex index=0; index < classNames.size(); ++index)
      {
        _iConfig.classNames.push_back(classNames[index].asString());
      }
    }
    else if(classNames.isString())
    {
      _iConfig.classNames.push_back(classNames.asString());
    }
    else 
    {
      PRINT_NAMED_WARNING("BehaviorDevEventSequenceCapture.Constructor.InvalidClassNames", "");
    }
  }
  _dVars.currentClassIter = _iConfig.classNames.begin();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDevEventSequenceCapture::~BehaviorDevEventSequenceCapture()
{
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevEventSequenceCapture::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kSavePathKey,
    kImageSaveQualityKey,
    kImageScaleKey,
    kImageResizeMethodKey,
    kUseCapacitiveTouchKey,
    kClassNamesKey,
    kSequenceSetupTimeKey,
    kPreEventCaptureTimeKey,
    kPostEventCaptureTimeKey
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevEventSequenceCapture::OnBehaviorActivated()
{
  _dVars.seqState = SequenceState::Waiting;
  _dVars.waitStartTime_s = -1.0f;
  _dVars.wasTouched = false;

  auto& visionComponent = GetBEI().GetComponentWrapper(BEIComponentID::Vision).GetValue<VisionComponent>();
  visionComponent.EnableDrawImagesToScreen(true);
  
  auto& robotInfo = GetBEI().GetRobotInfo();
  // wait for the lift to relax 
  robotInfo.GetMoveComponent().EnableLiftPower(false);

  GetBEI().GetBodyLightComponent().SetBackpackLights( kLightsWaiting );
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevEventSequenceCapture::OnBehaviorDeactivated()
{
  auto& robotInfo = GetBEI().GetRobotInfo();
  // wait for the lift to relax 
  robotInfo.GetMoveComponent().EnableLiftPower(true);

  auto& visionComponent = GetBEI().GetComponentWrapper(BEIComponentID::Vision).GetValue<VisionComponent>();
  visionComponent.EnableDrawImagesToScreen(false);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevEventSequenceCapture::SwitchToNextClass()
{
  if(!_iConfig.classNames.empty())
  {
    ++_dVars.currentClassIter; 
    if(_dVars.currentClassIter == _iConfig.classNames.end())
    {
      _dVars.currentClassIter = _iConfig.classNames.begin();
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string BehaviorDevEventSequenceCapture::GetRelClassSavePath() const
{
  if(_dVars.currentClassIter == _iConfig.classNames.end())
  {
    return _iConfig.imageSavePath;
  }
  
  return Util::FileUtils::FullFilePath({_iConfig.imageSavePath, *_dVars.currentClassIter});
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string BehaviorDevEventSequenceCapture::GetRelSequenceSavePath() const
{
  return Util::FileUtils::FullFilePath({GetRelClassSavePath(), std::to_string(_dVars.currentSeqNumber)});
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string BehaviorDevEventSequenceCapture::GetAbsInfoSavePath() const
{
  return Util::FileUtils::FullFilePath({GetAbsBaseSavePath(), GetRelSequenceSavePath(), "sequenceInfo.json"});
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string BehaviorDevEventSequenceCapture::GetAbsBaseSavePath() const
{
  const std::string cachePath = GetBEI().GetRobotInfo().GetContext()->GetDataPlatform()->pathToResource(Util::Data::Scope::Cache, "camera");
  return Util::FileUtils::FullFilePath({cachePath, "images"});
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int32_t BehaviorDevEventSequenceCapture::GetNumCurrentSequences() const
{
  std::vector<std::string> currentSequences;
  std::string absClassSavePath = Util::FileUtils::FullFilePath({GetAbsBaseSavePath(), GetRelClassSavePath()});
  Util::FileUtils::ListAllDirectories(absClassSavePath, currentSequences);
  return (int32_t) currentSequences.size();
}

TimeStamp_t BehaviorDevEventSequenceCapture::GetTimestamp() const
{
  // NOTE We're clocking ourselves using the image timestamps to make sure sequence info is on the same clock
  // This should be fixed when a proper wall time implementation is added
  return GetBEI().GetRobotInfo().GetLastImageTimeStamp();
}

float BehaviorDevEventSequenceCapture::GetTimestampSec() const
{
  return static_cast<float>(GetTimestamp()) / 1000.0f;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevEventSequenceCapture::BehaviorUpdate()
{
  if(!IsActivated())
  {
    return;
  }

  const bool isTouched = (_iConfig.useCapTouch ? 
                        GetBEI().GetTouchSensorComponent().GetIsPressed() :
                        GetBEI().GetRobotInfo().IsPowerButtonPressed());
  const bool isLiftUp = (GetBEI().GetRobotInfo().GetLiftHeight() > LIFT_HEIGHT_HIGHDOCK);

  const float currTime_s = GetTimestampSec();
  float waitTime_s = currTime_s - _dVars.waitStartTime_s;
  auto& visionComponent = GetBEI().GetComponentWrapper(BEIComponentID::Vision).GetValue<VisionComponent>();
  int32_t numCurrentSeqs = GetNumCurrentSequences();

  // Display the class name and sequence number
  // TODO Continue sequences from before so we don't overwrite upon restart?
  std::function<void(Vision::ImageRGB&)> drawClassName = [this, numCurrentSeqs](Vision::ImageRGB& img)
  {
    img.DrawText({1,14}, *_dVars.currentClassIter + ":" + std::to_string(numCurrentSeqs), NamedColors::RED, 0.6f, true);
  };
  visionComponent.AddDrawScreenModifier(drawClassName);

  // For audio files
  using GE = AudioMetaData::GameEvent::GenericEvent;
  using GO = AudioMetaData::GameObjectType;

  switch( _dVars.seqState )
  {
    case SequenceState::Waiting:
    {
      // Switch classes when lift is lowered
      if(_dVars.wasLiftUp && !isLiftUp)
      {
        SwitchToNextClass();
      }
      
      // Check for starting sequence upon button release
      if( _dVars.wasTouched && !isTouched )
      {
        _dVars.seqState = SequenceState::Setup;
        _dVars.currentSeqNumber = numCurrentSeqs;
        _dVars.waitStartTime_s = currTime_s;

        if( _iConfig.enableRandomHeadTilt )
        {
          double headAngle = GetBEI().GetRobotInfo().GetRNG().RandDblInRange( MIN_HEAD_ANGLE, MAX_HEAD_ANGLE );
          IActionRunner* tiltAction = new MoveHeadToAngleAction( headAngle );
          PRINT_CH_DEBUG( "Behavior", "BehaviorDevEventSequenceCapture.TiltHead",
                          "Tilting head to %f", RAD_TO_DEG(headAngle) );
          DelegateIfInControl( tiltAction );
        }

        GetBEI().GetRobotAudioClient().PostEvent(GE::Play__Robot_Vic_Sfx__Lift_High_Down_Long_Effort,
                                                 GO::Behavior);
        GetBEI().GetBodyLightComponent().SetBackpackLights( kLightsSetup );
        PRINT_CH_DEBUG("Behaviors", "BehaviorDevEventSequenceCapture.startSequence", 
                       "starting sequence %d", _dVars.currentSeqNumber);
      }
      break;
    }
    case SequenceState::Setup:
    {
      // Wait for setup time to finish, then start streaming
      if( waitTime_s > _iConfig.sequenceSetupTime )
      {
        _dVars.seqState = SequenceState::PreEventCapture;
        _dVars.waitStartTime_s = currTime_s;
        _dVars.seqStartTimeStamp = GetTimestamp();
        visionComponent.SetSaveImageParameters(ImageSendMode::Stream,
                                               GetRelSequenceSavePath(),
                                               _iConfig.imageSaveQuality,
                                               _iConfig.imageSaveSize);
        GetBEI().GetRobotAudioClient().PostEvent(GE::Play__Robot_Vic_Sfx__Timer_Countdown,
                                                 GO::Behavior);
        GetBEI().GetBodyLightComponent().SetBackpackLights( kLightsPreCap );
        PRINT_CH_DEBUG("Behaviors", "BehaviorDevEventSequenceCapture.setupSequence", 
                       "set up sequence %d", _dVars.currentSeqNumber);
      }
      break;
    }
    case SequenceState::PreEventCapture:
    {
      // Wait for PreEventCapture time to finish, then trigger
      if( waitTime_s > _iConfig.preEventCaptureTime )
      {
        _dVars.seqState = SequenceState::PostEventCapture;
        _dVars.waitStartTime_s = currTime_s;
        _dVars.seqEventTimeStamp = GetTimestamp();
        GetBEI().GetRobotAudioClient().PostEvent(GE::Play__Robot_Vic_Sfx__Timer_Beep,
                                                 GO::Behavior);
        GetBEI().GetBodyLightComponent().SetBackpackLights( kLightsPostCap );
        PRINT_CH_DEBUG("Behaviors", "BehaviorDevEventSequenceCapture.preCapSequence", 
                       "pre captured sequence %d", _dVars.currentSeqNumber);
      }
      break;
    }
    case SequenceState::PostEventCapture:
    {
      // Wait for PostEventCapture time to finish, then wrap up
      if( waitTime_s > _iConfig.postEventCaptureTime )
      {
        _dVars.seqState = SequenceState::Waiting;
        _dVars.seqEndTimeStamp = GetTimestamp();
        visionComponent.SetSaveImageParameters(ImageSendMode::Off,
                                               GetRelSequenceSavePath(),
                                               _iConfig.imageSaveQuality,
                                               _iConfig.imageSaveSize);
        GetBEI().GetRobotAudioClient().PostEvent(GE::Play__Robot_Vic_Sfx__Timer_Cancel,
                                                 GO::Behavior);
        GetBEI().GetBodyLightComponent().SetBackpackLights( kLightsWaiting );
        PRINT_CH_DEBUG("Behaviors", "BehaviorDevEventSequenceCapture.endSequence", 
                       "finished sequence %d", _dVars.currentSeqNumber);
        
        // Save JSON file in directory listing sequence timings
        Json::Value seqInfo;
        seqInfo["startTime"] = _dVars.seqStartTimeStamp;
        seqInfo["eventTime"] = _dVars.seqEventTimeStamp;
        seqInfo["endTime"] = _dVars.seqEndTimeStamp;
        std::ofstream seqFile(GetAbsInfoSavePath());
        Json::StyledWriter writer;
        seqFile << writer.write(seqInfo);
        seqFile.close();
      }
      break;
    }
    default:
    {
      PRINT_NAMED_ERROR("DevEventSequenceCapture.SequenceStateError", "Unrecognized sequence state!");
      _dVars.seqState = SequenceState::Waiting;
    }
  }

  _dVars.wasTouched = isTouched;
  _dVars.wasLiftUp = isLiftUp;
}

} // namespace Anki
} // namespace Cozmo