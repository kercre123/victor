/**
 * File: habitatDetectorComponent.cpp
 *
 * Author: Arjun Menon
 * Created: 05/24/2018
 *
 * Description: Passive logic component to determine if we are in a habitat
 * after we have been moved. Uses a combination of sensors to reach "certainty"
 * with some heuristics.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/components/habitatDetectorComponent.h"

#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "coretech/common/engine/utils/timer.h"
#include "engine/components/battery/batteryComponent.h"
#include "engine/components/habitatDetectorComponent.h"
#include "engine/components/sensors/cliffSensorComponent.h"
#include "engine/components/sensors/proxSensorComponent.h"
#include "engine/components/movementComponent.h"
#include "engine/components/visionComponent.h"
#include "engine/cozmoContext.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/utils/cozmoFeatureGate.h"

#include "engine/robot.h"
#include "engine/ankiEventUtil.h"

#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/types/featureGateTypes.h"

#include "util/bitFlags/bitFlags.h"
#include "util/fileUtils/fileUtils.h"
#include "util/console/consoleInterface.h"

#include "webServerProcess/src/webService.h"

#include <array>
#include <vector>
#include <sstream>
#include <iomanip>

namespace Anki {

namespace Vector {


// when the robot is positioned such that its front two cliff
// sensors see the white line of the habitat, then this is
// the max expected of values for the sensor seeing the wall
const int HabitatDetectorComponent::kConfirmationConfigProxMaxReading = 70;

// "private" constants
namespace 
{
  
  // highest expected value to come out of any cliff sensor
  // when it is observing the grey-matte surface of a habitat
  const int kCliffGreyMaxHabitat = 300;
  
  // minimum expected value to come out of the cliff sensor
  // when observing the grey surface of the habitat
  // note: this value is arbitrarily chosen to prevent the
  // white detection threshold from getting too low (<300)
  const int kCliffGreyMinHabitat = 100;
  
  // number of consecutive readings while the front two cliff
  // sensors are positioned on the white line to average over
  // to make a determination if we're looking at a habitat wall
  const int kMinProxReadingsRequired = 7;
  
  // maximum number of ticks to wait for valid readings from the
  // prox sensor while the robot is positioned on the white-line
  // of the habitat (in order to detect the wall, if present)
  const int kMaxTicksToWaitForProx = 15;
  
  // length of largest diagonal in habitat
  const f32 kMinTravelDistanceWithoutSeeingWhite_mm = 260;
  
  // offset from the nominal/minimum grey value, for white detection
  const u16 kWhiteThresholdOffsetFromGrey = 200;
  
  // period to send updates to the robot process with white thresholds
  const f32 kWhiteThresholdUpdatePeriod_s = 0.5f;
  
  // number of grey value readings required to switch over
  // from using nominal grey to the measured mininum grey
  const int kMinNumGreyCliffReadings = 20;
  
  // default value to assume as the grey detection
  // note: this value is supplanted by the measured min
  // as soon as there are enough readings, for the
  // purposes of setting the white-detect threshold
  const u16 kNominalGreyValue = 200;
  
  // the minimum required change in the estimated white
  // threshold for either cliff sensor that requires sending
  // a message down to the robot to update it
  // note:
  // fluctuation in the cliff sensor reading is ~10 units
  // when looking at the same surface over time
  const int kMinWhiteThreshChangeToMessage = 10;
  
  const std::string kWebVizHabitatModuleName = "habitat";
  const f32 kSendWebVizDataPeriod_sec = 0.5f;
  Json::Value _toSendJson = Json::objectValue;
}
    

HabitatDetectorComponent::HabitatDetectorComponent()
: IDependencyManagedComponent<RobotComponentID>(this, RobotComponentID::HabitatDetector)
, _habitatBelief(HabitatBeliefState::Unknown)
, _habitatLineRelPose(HabitatLineRelPose::Invalid)
, _detectedWhiteFromCliffs(false)
, _nextSendWebVizDataTime_sec(0.0f)
, _proxReadingBuffer()
, _numTicksWaitingForProx(0)
{
}
  
HabitatDetectorComponent::~HabitatDetectorComponent()
{
}

void HabitatDetectorComponent::InitDependent(Robot* robot, const RobotCompMap& dependentComponents)
{
  _robot = robot;
  
  if(robot->HasExternalInterface()) {
    using namespace ExternalInterface;
    auto helper = MakeAnkiEventUtil(*(robot->GetExternalInterface()), *this, _signalHandles);
    helper.SubscribeEngineToGame<MessageEngineToGameTag::RobotStopped>();
    
    // webviz subscription to externally control the robot's belief state
    if (ANKI_DEV_CHEATS) {
      const auto* context = _robot->GetContext();
      if (context != nullptr) {
        auto* webService = context->GetWebService();
        if (webService != nullptr) {
          auto onData = [this](const Json::Value& data, const std::function<void(const Json::Value&)>& sendToClient) {
            ForceSetHabitatBeliefState(HabitatBeliefStateFromString(data["forceHabitatState"].asCString()), "WebViz"); 
          };
          _signalHandles.emplace_back(webService->OnWebVizData(kWebVizHabitatModuleName).ScopedSubscribe(onData));
        }
      }
    }
  } else {
    PRINT_NAMED_WARNING("HabitatDetectorComponent.InitDependent.MissingExternalIterface","");
  }

  _toSendJson["reason"] = "~";
  _toSendJson["whiteThresholds"] = "~";
}

template<>
void HabitatDetectorComponent::HandleMessage(const ExternalInterface::RobotStopped& msg)
{
  if(msg.reason == StopReason::CLIFF) {
    SetBelief(HabitatBeliefState::NotInHabitat, "CliffDetected");
  }
}

void HabitatDetectorComponent::SetBelief(HabitatBeliefState state, std::string devReasonStr)
{
  PRINT_NAMED_INFO("HabitatDetectorComponent.SetBelief","%s <- %s", EnumToString(state), devReasonStr.c_str());
  _habitatBelief = state;
  _habitatLineRelPose = HabitatLineRelPose::Invalid;
  _toSendJson["reason"] = devReasonStr;
}


bool HabitatDetectorComponent::UpdateProxObservations()
{
  auto proxData = _robot->GetProxSensorComponent().GetLatestProxData();
  // ignore the normal range spec, since we need to be very close to the wall
  // in order to be observing it
  const bool reliable = !proxData.isLiftInFOV &&
                        !proxData.isTooPitched &&
                        proxData.isValidSignalQuality;
  
  if(reliable && _proxReadingBuffer.size() < kMinProxReadingsRequired) {
    _proxReadingBuffer.push_back(proxData.distance_mm);
  }
  _numTicksWaitingForProx++;
  
  // whether enough readings are collected
  return _proxReadingBuffer.size() >= kMinProxReadingsRequired ||
         _numTicksWaitingForProx > kMaxTicksToWaitForProx;
}
  
void HabitatDetectorComponent::ForceSetHabitatBeliefState(HabitatBeliefState belief, const std::string& sourceStr)
{
  // prepend ForceSet to calls to this method so we can track the caller
  SetBelief(belief, "ForceSet(" + sourceStr + ")");
}

void HabitatDetectorComponent::UpdateDependent(const DependencyManagedEntity<RobotComponentID>& dependentComps)
{

  const auto* featureGate = _robot->GetContext()->GetFeatureGate();
  const bool inPRDemo = featureGate->IsFeatureEnabled(Anki::Vector::FeatureType::PRDemo);
  if(inPRDemo) {
    if( _habitatBelief != HabitatBeliefState::NotInHabitat ) {
      SetBelief(HabitatBeliefState::NotInHabitat, "FeatureGate");
    }
    
    return;
  }
  
  auto& cliffSensor = dependentComps.GetComponent<CliffSensorComponent>();

  const bool isPickedUp = _robot->GetOffTreadsState() == OffTreadsState::InAir;

  if(cliffSensor.IsCliffDetected() && isPickedUp) {
    _habitatBelief = HabitatBeliefState::Unknown;
    _detectedWhiteFromCliffs = false;
    _proxReadingBuffer.clear();
    _numTicksWaitingForProx = 0;

    // reset the Cliff-Grey measurement variables when the robot is moved and needs
    // to re-determine whether it is inside the habitat or not
    _numGreyCliffReadingsSincePutdown = 0;
    _minGreyCliffFL = std::numeric_limits<u16>::max();
    _minGreyCliffFR = std::numeric_limits<u16>::max();

    // initialized to zero to force a message to sent down with nominal thresholds
    // upon the next call to UpdateWhiteDetectThreshold() which subsequently calls
    // SendWhiteDetectThresholdsIfNeeded() 
    // which will only send if the last sent values differ by a minimum amount, and 
    // if enough time has passed since the last message message
    _lastSentWhiteThreshFL = 0;
    _lastSentWhiteThreshFR = 0;
    _timeForWhiteThresholdUpdate_s = 0.0;
  }
  
  // habitat confirmation (or disconfirmation) can occur if we are in the unknown state
  if(!_robot->GetBatteryComponent().IsOnChargerPlatform() && 
      _habitatBelief == HabitatBeliefState::Unknown && 
      !isPickedUp) {
    // determine if we have driven too far without detecting white from cliffs
    f32 distance = ComputeDistanceBetween(_robot->GetPose(), _robot->GetWorldOrigin());
    if(!_detectedWhiteFromCliffs && distance > kMinTravelDistanceWithoutSeeingWhite_mm) {
      SetBelief(HabitatBeliefState::NotInHabitat, "OdometryTooHigh");
    }
    
    // compute the minimum grey observed while the robot is moving
    // additionally send updated white-thresholds based on observed grey so far
    const auto& cliffValues = cliffSensor.GetCliffDataRaw();
    UpdateWhiteDetectThreshold(cliffValues);
  
    u8 numGrey = 0;
    u8 numWhite = 0;
    std::array<HabitatCliffReadingType,kNumCliffSensors> cliffCats;
    for(int i=0; i<kNumCliffSensors; ++i) {
      if(cliffSensor.IsWhiteDetected(static_cast<CliffSensor>(i))) {
        // determined from robot state message
        cliffCats[i] = White;
        numWhite++;
        _detectedWhiteFromCliffs = true;
      } else if(cliffValues[i] <= kCliffGreyMaxHabitat) {
        // determine from raw readings
        cliffCats[i] = Grey;
        numGrey++;
      } else {
        cliffCats[i] = None;
      }
    }
    u8 numNone = kNumCliffSensors - (numGrey + numWhite);
    
    if(numWhite >= 3 || numNone >= 3) {
      // impossible configurations when not on the charger platform
      std::stringstream reason;
      reason << "InvalidCliffConfig" << " [";
      for(int i=0; i<kNumCliffSensors; ++i) {
        reason << cliffValues[i] << "(" << (int)cliffCats[i] << "), ";
      }
      reason << "]";
      SetBelief(HabitatBeliefState::NotInHabitat, reason.str());
    } else {
      // note: we take specific actions based on the line-pose determined here
      // inside the behavior ConfirmHabitat
      if(numGrey >= 2 && numWhite == 0) {
        // accounts for:
        // + 4G   (optimal)
        // + 3G1N (one cliff near the white line, but not over)
        // + 2G2N (two cliff near the white line, but not over)
        _habitatLineRelPose = HabitatLineRelPose::AllGrey;
      } else if(numWhite==2) {
        // + two white cliffs are triggering
        // + at least 1 grey observed (2-none are impossible)
        if(     cliffCats[0] == White && cliffCats[1] == White) { _habitatLineRelPose = HabitatLineRelPose::WhiteFLFR; }
        else if(cliffCats[1] == White && cliffCats[3] == White) { _habitatLineRelPose = HabitatLineRelPose::WhiteFRBR; }
        else if(cliffCats[0] == White && cliffCats[2] == White) { _habitatLineRelPose = HabitatLineRelPose::WhiteFLBL; }
        else if(cliffCats[2] == White && cliffCats[3] == White) { _habitatLineRelPose = HabitatLineRelPose::WhiteBLBR; }
      } else if(numWhite == 1 && numGrey >= 2) {
        // + at least one cliff sensor is strongly white
        // + must include at least 2 grey (impossible for 3-none and 2-none)
        if(     cliffCats[0] == White) { _habitatLineRelPose = HabitatLineRelPose::WhiteFL; }
        else if(cliffCats[1] == White) { _habitatLineRelPose = HabitatLineRelPose::WhiteFR; }
        else if(cliffCats[2] == White) { _habitatLineRelPose = HabitatLineRelPose::WhiteBL; }
        else if(cliffCats[3] == White) { _habitatLineRelPose = HabitatLineRelPose::WhiteBR; }
      } else {
        _habitatLineRelPose = HabitatLineRelPose::Invalid;
      }
      
      if(_habitatLineRelPose == HabitatLineRelPose::WhiteFLFR) {
        const bool proxDataCollectionEnded = UpdateProxObservations();
        if(proxDataCollectionEnded) {
          if(_proxReadingBuffer.size() >= kMinProxReadingsRequired) {
            u32 sum = 0;
            for(auto& reading : _proxReadingBuffer) {
              sum += reading;
            }
            float distance_mm = float(sum)/_proxReadingBuffer.size();

            // debug info for webviz
            std::stringstream reason;
            reason << "ProxReadings = [";
            for(auto& reading : _proxReadingBuffer) {
              reason << reading << ",";
            }
            reason << "] = " << std::setprecision(2) << distance_mm;

            if(distance_mm <= kConfirmationConfigProxMaxReading) {
              SetBelief(HabitatBeliefState::InHabitat, reason.str());
            } else {
              SetBelief(HabitatBeliefState::NotInHabitat, reason.str());
            }
          } else {
            // we timed out while waiting for
            // enough prox sensor readings
            SetBelief(HabitatBeliefState::NotInHabitat, "NotEnoughProxReadings");
          }
        }
      } else {
        // not positioned on the white line => clear the buffer for the next time we are aligned
        _proxReadingBuffer.clear();
      }
    }
  } // end(if Unknown)
  
  // Send info to web viz occasionally
  const float now_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  if (now_sec > _nextSendWebVizDataTime_sec) {
    SendDataToWebViz();
    _nextSendWebVizDataTime_sec = now_sec + kSendWebVizDataPeriod_sec;
  }
}
  
void HabitatDetectorComponent::UpdateWhiteDetectThreshold(const CliffSensorComponent::CliffSensorDataArray& cliffValues)
{
  // while the robot is:
  // - moving
  // - on treads
  // then sample the cliff sensor and compute the running minimum for FL and FR
  if(_robot->GetOffTreadsState()==OffTreadsState::OnTreads &&
     _robot->GetMoveComponent().AreWheelsMoving()) {
    auto cliffFL = cliffValues[(int)CliffSensor::CLIFF_FL] ;
    if(cliffFL>= kCliffGreyMinHabitat && cliffFL<= kCliffGreyMaxHabitat && cliffFL < _minGreyCliffFL) {
      _minGreyCliffFL = cliffFL;
    }
    auto cliffFR = cliffValues[(int)CliffSensor::CLIFF_FR] ;
    if(cliffFR>= kCliffGreyMinHabitat && cliffFR<= kCliffGreyMaxHabitat && cliffFR < _minGreyCliffFR) {
      _minGreyCliffFR = cliffFR;
    }
    _numGreyCliffReadingsSincePutdown++;
  }
  
  SendWhiteDetectThresholdsIfNeeded();
}
  
void HabitatDetectorComponent::SendWhiteDetectThresholdsIfNeeded()
{
  // if enough readings to compute a reasonable minimum have been collected,
  // periodically send the new white-detect thresholds down to the robot
  const bool enoughReadingsForMin = _numGreyCliffReadingsSincePutdown > kMinNumGreyCliffReadings;
  const u16 whiteThreshFL = (enoughReadingsForMin ? _minGreyCliffFL : kNominalGreyValue) + kWhiteThresholdOffsetFromGrey;
  const u16 whiteThreshFR = (enoughReadingsForMin ? _minGreyCliffFR : kNominalGreyValue) + kWhiteThresholdOffsetFromGrey;
  const bool whiteThreshChanged = (ABS((int)whiteThreshFL-(int)_lastSentWhiteThreshFL) >= kMinWhiteThreshChangeToMessage) ||
  (ABS((int)whiteThreshFR-(int)_lastSentWhiteThreshFR) >= kMinWhiteThreshChangeToMessage);
  
  const f32 now = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  if(whiteThreshChanged && now >= _timeForWhiteThresholdUpdate_s) {
    // note: we only care about the white thresholds for the front two sensors
    // the back sensors can be supplied with the nominal values, without
    // affecting how stop-on-white or cliff-align actions work
    _robot->SendRobotMessage<SetWhiteDetectThresholds>(
      CliffSensorComponent::CliffSensorDataArray{{
        u16(whiteThreshFL),
        u16(whiteThreshFR),
        u16(kNominalGreyValue + kWhiteThresholdOffsetFromGrey),
        u16(kNominalGreyValue + kWhiteThresholdOffsetFromGrey)
    }});
    
    _timeForWhiteThresholdUpdate_s = now + kWhiteThresholdUpdatePeriod_s;
    _lastSentWhiteThreshFL = whiteThreshFL;
    _lastSentWhiteThreshFR = whiteThreshFR;
    
    // update the json blob with debug info
    std::stringstream ss;
    ss << "[" << (int)whiteThreshFL << "," << (int)whiteThreshFR << "]";
    ss << " " << (int)_numGreyCliffReadingsSincePutdown << " readings so far";
    _toSendJson["whiteThresholds"] = ss.str();
  }
}
  
void HabitatDetectorComponent::SendDataToWebViz() const
{
  if (!ANKI_DEV_CHEATS) {
    return;
  }
  
  const auto* context = _robot->GetContext();
  if (context != nullptr) {
    auto* webService = context->GetWebService();
    if (webService!= nullptr && webService->IsWebVizClientSubscribed(kWebVizHabitatModuleName)) {
      _toSendJson["habitatState"] = EnumToString(_habitatBelief);
      _toSendJson["stopOnWhiteEnabled"] = _robot->GetCliffSensorComponent().IsStopOnWhiteEnabled() ? "yes" : "no";
      webService->SendToWebViz(kWebVizHabitatModuleName, _toSendJson);
    }
  }
}


} // Cozmo namespace
} // Anki namespace
