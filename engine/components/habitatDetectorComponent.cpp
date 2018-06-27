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

#include "engine/components/sensors/cliffSensorComponent.h"
#include "engine/components/sensors/proxSensorComponent.h"
#include "engine/components/batteryComponent.h"
#include "engine/components/visionComponent.h"
#include "engine/components/habitatDetectorComponent.h"
#include "engine/externalInterface/externalInterface.h"
#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "coretech/common/engine/utils/timer.h"

#include "engine/robot.h"
#include "engine/ankiEventUtil.h"

#include "clad/externalInterface/messageEngineToGame.h"

#include "util/bitFlags/bitFlags.h"
#include "util/fileUtils/fileUtils.h"
#include "util/console/consoleInterface.h"

#include <array>
#include <vector>
#include <sstream>

namespace Anki {

namespace Cozmo {
  
namespace {
  
  // highest expected value to come out of any cliff sensor
  // when it is observing the grey-matte surface of a habitat
  const int kCliffGreyMaxHabitat = 200;
  
  // when the robot is positioned such that its front two cliff
  // sensors see the white line of the habitat, then these two
  // constants are the min and max range of values for the sensor
  const int kConfirmationConfigProxMinReading = 17;
  const int kConfirmationConfigProxMaxReading = 28;
  
  const f32 kMinTravelDistanceWithoutSeeingWhite_mm = 260; // length of largest diagonal in habitat
}
    

HabitatDetectorComponent::HabitatDetectorComponent()
: IDependencyManagedComponent<RobotComponentID>(this, RobotComponentID::HabitatDetector)
, _habitatBelief(HabitatBeliefState::Unknown)
, _habitatLineRelPose(HabitatLineRelPose::Invalid)
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
    helper.SubscribeEngineToGame<MessageEngineToGameTag::RobotOffTreadsStateChanged>();
    helper.SubscribeEngineToGame<MessageEngineToGameTag::RobotStopped>();
  } else {
    PRINT_NAMED_WARNING("HabitatDetectorComponent.InitDependent.MissingExternalIterface","");
  }
}

template<>
void HabitatDetectorComponent::HandleMessage(const ExternalInterface::RobotOffTreadsStateChanged& msg)
{
  if(msg.treadsState!=OffTreadsState::OnTreads) {
    _habitatBelief = HabitatBeliefState::Unknown;
    _detectedWhiteFromCliffs = false;
  }
}

template<>
void HabitatDetectorComponent::HandleMessage(const ExternalInterface::RobotStopped& msg)
{
  if(msg.reason == StopReason::CLIFF) {
    _habitatBelief = HabitatBeliefState::NotInHabitat;
    _habitatLineRelPose = HabitatLineRelPose::Invalid;
  }
}

bool HabitatDetectorComponent::IsProxObservingHabitatWall() const
{
  auto proxData = _robot->GetProxSensorComponent().GetLatestProxData();
  // ignore the normal range spec, since we need to be very close to the wall
  // in order to be observing it
  const bool reliable = !proxData.isLiftInFOV &&
                        !proxData.isTooPitched &&
                        proxData.isValidSignalQuality;
  return reliable &&
         proxData.distance_mm <= kConfirmationConfigProxMaxReading &&
         proxData.distance_mm >= kConfirmationConfigProxMinReading;
}

void HabitatDetectorComponent::UpdateDependent(const DependencyManagedEntity<RobotComponentID>& dependentComps)
{
  auto& cliffSensor = dependentComps.GetComponent<CliffSensorComponent>();
  
  if(_robot->GetBatteryComponent().IsOnChargerPlatform()) {
    return;
  }
  
  // habitat confirmation (or disconfirmation) can occur if we are in the unknown state
  if(_habitatBelief == HabitatBeliefState::Unknown) {
    // determine if we have driven too far without detecting white from cliffs
    f32 distance = ComputeDistanceBetween(_robot->GetPose(), _robot->GetWorldOrigin());
    if(!_detectedWhiteFromCliffs && distance > kMinTravelDistanceWithoutSeeingWhite_mm) {
      _habitatBelief = HabitatBeliefState::NotInHabitat;
      _habitatLineRelPose = HabitatLineRelPose::Invalid;
      PRINT_NAMED_INFO("HabitatDetectorComponent.UpdateDependent.NotInHabitat.FromOdometry","");
    }
  
    u8 numGrey = 0;
    u8 numWhite = 0;
    std::array<HabitatCliffReadingType,kNumCliffSensors> cliffCats;
    const auto& cliffValues = cliffSensor.GetCliffDataRaw();
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
      _habitatBelief = HabitatBeliefState::NotInHabitat;
      _habitatLineRelPose = HabitatLineRelPose::Invalid;
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
        if(IsProxObservingHabitatWall()) {
          _habitatBelief = HabitatBeliefState::InHabitat;
          PRINT_NAMED_INFO("HabitatDetectorComponent.UpdateDependent.InHabitat","");
        } else {
          _habitatBelief = HabitatBeliefState::NotInHabitat;
          PRINT_NAMED_INFO("HabitatDetectorComponent.UpdateDependent.NotInHabitat","");
        }
      }
    }
  } // end(if Unknown)
}


} // Cozmo namespace
} // Anki namespace
