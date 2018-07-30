/**
 * File: behaviorOnboardingDetectHabitat.cpp
 *
 * Author: ross
 * Created: 2018-07-26
 *
 * Description: passively detects being in the habitat during the first stage of onboarding
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/onboarding/behaviorOnboardingDetectHabitat.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/components/habitatDetectorComponent.h"
#include "engine/components/movementComponent.h"
#include "engine/components/sensors/cliffSensorComponent.h"
#include "engine/components/sensors/proxSensorComponent.h"
#include "proto/external_interface/onboardingSteps.pb.h"
#include "util/console/consoleInterface.h"

namespace Anki {
namespace Cozmo {
  
namespace{
  const char* const kBehaviorIfDetectedKey = "behaviorIfDetected";
  
  const u16 kMinProxDist_mm = 15;
  // the max is larger than it needs to be to account for oblique angles, which shouldn't happen, but perhaps
  // the robot was bumped. This can be reduced if we see false positives
  const u16 kMaxProxDist_mm = 45;
  const u16 kMinDrivingDist_mm = 70;

  CONSOLE_VAR(bool, kOnboardingHabitatLog, "Onboarding", false);
  CONSOLE_VAR(bool, kResetOnboardingHabitatDetection, "Onboarding", false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorOnboardingDetectHabitat::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorOnboardingDetectHabitat::DynamicVariables::DynamicVariables()
{
  state = DetectionState::Indeterminate;
  lastOnboardingStep = 0;
  drivingComeHere = false;
  activatedOnce = false;
  bodyOdomAtInitComeHere_mm = 0.0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorOnboardingDetectHabitat::BehaviorOnboardingDetectHabitat(const Json::Value& config)
 : ICozmoBehavior(config)
{
  _iConfig.behaviorIfDetectedStr = JsonTools::ParseString(config, kBehaviorIfDetectedKey, GetDebugLabel());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorOnboardingDetectHabitat::WantsToBeActivatedBehavior() const
{
  // The part of BehaviorUpdate() that runs when the behavior is _not_ active is checking for 
  // whether the robot is in the habitat. When it determines that the robot is in the habitat,
  // it flags that _dVars.state = DetectionState::InHabitat, so that WantsToBeActivatedBehavior
  // becomes true. When activated, it immediately delegates to behaviorIfDetected, and exits when
  // that behavior finishes

  const bool inHabitat = (_dVars.state == DetectionState::InHabitat);
  if( inHabitat && !_dVars.activatedOnce ) {
    const bool behaviorWantsToRun = (_iConfig.behaviorIfDetected != nullptr) && _iConfig.behaviorIfDetected->WantsToBeActivated();
    ANKI_VERIFY( behaviorWantsToRun,
                 "BehaviorOnboardingDetectHabitat.WantsToBeActivatedBehavior.DWTA",
                 "Behavior '%s' doesn't want to activate",
                 _iConfig.behaviorIfDetectedStr.c_str() );
    return behaviorWantsToRun;
  } else {
    return false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingDetectHabitat::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingDetectHabitat::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  if( _iConfig.behaviorIfDetected != nullptr ) {
    delegates.insert( _iConfig.behaviorIfDetected.get() );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingDetectHabitat::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kBehaviorIfDetectedKey,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingDetectHabitat::InitBehavior()
{
  _iConfig.behaviorIfDetected = FindBehavior( _iConfig.behaviorIfDetectedStr );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingDetectHabitat::OnBehaviorActivated() 
{
  // reset dynamic variables
  _dVars = DynamicVariables();
  _dVars.activatedOnce = true; // only run once so we don't continually pop modals when on some user's unique tablecloth
  
  DelegateIfInControl( _iConfig.behaviorIfDetected.get() ); // and then the behavior ends when this one does
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingDetectHabitat::OnBehaviorDeactivated()
{
  // For testing, if this is true before putting the robot back on the charger, it can be run again
  if( kResetOnboardingHabitatDetection ) {
    _dVars.activatedOnce = false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingDetectHabitat::BehaviorUpdate() 
{
  if( IsActivated() ) {
    return;
  }
  
  // when not active, run habitat detection:
  
  // Habitat detection assumes the following about the onboarding flow:
  // (1) The robot starts on the charger.
  // (2) The robot drives off the charger, straight, and just barely clears it. At this point it is usually in the
  //     center of the habitat
  // (3) It may turn left or right here while waiting for a voice command
  // (4) The "come here" voice command runs, so the robot will turn toward mic direction and drive (animated)
  //     a fixed distance. This distance will drive the robot to faceplant against the habitat wall, even along the
  //     habitat diagonal
  // As a consequence, this behavior passively monitors the front cliff sensor values for "white" after the robot
  // drives off the charger and starts driving the "come here" animation. If a front sensor reading looks white
  // during the "come here" stage, and not before, and at that point the prox reading is within a nominal range, and
  // the rear sensors have never read white, and the robot hasn't been picked up since leaving the charger, then the
  // behavior wants to be activated.
  // If the user places the robot in the habitat but not on the charger, this won't work. Since the app directs the
  // user to start on the charger, this is probably good enough for now. If needed, we could localize to the charger
  // marker. This also doesn't cover the case where the robot crashes or is power cycled and then resumes off the
  // charger in a later onboarding stage.
  
  const DetectionState oldState = _dVars.state;
  
  
  const bool pickedUp = GetBEI().GetRobotInfo().IsPickedUp();
  if( pickedUp ) {
    // we'll never know where the robot is again. It needs to go on the charger to start checking anything else
    _dVars.state = DetectionState::Indeterminate;
  } else {
    const bool onChargerContacts = GetBEI().GetRobotInfo().IsOnChargerContacts();
    const bool onChargerPlatform = GetBEI().GetRobotInfo().IsOnChargerPlatform();
    if( onChargerContacts ) {
      // it's localized to the charger again
      _dVars.state = DetectionState::OnChargerContacts;
    }
  
    if( !onChargerPlatform && (_dVars.state == DetectionState::OnChargerContacts) ) {
      // robot drove off charger. set to Undetermined so we start checking sensor values.
      // (don't check sensor values before this to avoid looking at the charger stripe)
      _dVars.state = DetectionState::Undetermined;
    }
    
    if( _dVars.state == DetectionState::Undetermined ) {
      const auto& cliffSensorComp = GetBEI().GetCliffSensorComponent();
      const bool whiteFL = cliffSensorComp.IsWhiteDetected( CliffSensor::CLIFF_FL );
      const bool whiteFR = cliffSensorComp.IsWhiteDetected( CliffSensor::CLIFF_FR );
      const bool whiteBL = cliffSensorComp.IsWhiteDetected( CliffSensor::CLIFF_BL );
      const bool whiteBR = cliffSensorComp.IsWhiteDetected( CliffSensor::CLIFF_BR );

      const bool drivenEnough = (GetDistanceDriven() > kMinDrivingDist_mm);
      
      if( whiteFL && whiteFR && whiteBL && whiteBR ) {
        // all are showing white -- even the glossy vector logo won't look all white
        _dVars.state = DetectionState::NotInHabitat;
      } else if( _dVars.drivingComeHere && drivenEnough ) {
        // wait until the robot has driven a small amount before checking cliff sensor values. The glossy "vector"
        // logo in the middle of the habitat looks white to the cliff sensors (VIC-4961). This is unfortunate, because
        // otherwise we could mark NotInHabitat if _any_ sensor reads white before driving the "come here" animation
        
        // check front sensor values don't read white until in the "come here" stage, and the back
        // sensors never read white.
        if( whiteBL || whiteBR ) {
          // a back cliff sensor sees white. this shouldn't happen in the habitat, and probably means
          // the surface has white on it ==> not the habitat
          _dVars.state = DetectionState::NotInHabitat;
        } else if( whiteFL || whiteFR ) {
          // a front sensor sees white
          u16 distance_mm = 0;
          const bool hasReading = GetProxReading( distance_mm );
          if( hasReading
              && (distance_mm >= kMinProxDist_mm)
              && (distance_mm <= kMaxProxDist_mm) )
          {
            // prox reading matches expected position. mark as being in the habitat
            _dVars.state = DetectionState::InHabitat;
          } else if( hasReading ) {
            // prox reading mismatch. mark as not in the habitat
            _dVars.state = DetectionState::NotInHabitat;
          }
        }
      }
    }
  }
  
  if( oldState != _dVars.state ) {
    // force the state on the normal habitat detector only if we determine that it's in habitat or
    // not in habitat. Let the component decide for itself when the state is undetermined
    if( _dVars.state == DetectionState::InHabitat ) {
      auto& habitatDetector = GetBEI().GetHabitatDetectorComponent();
      habitatDetector.ForceSetHabitatBeliefState( HabitatBeliefState::InHabitat );
    } else if( _dVars.state == DetectionState::NotInHabitat ) {
      auto& habitatDetector = GetBEI().GetHabitatDetectorComponent();
      habitatDetector.ForceSetHabitatBeliefState( HabitatBeliefState::NotInHabitat );
    }
  }
  
  if( kOnboardingHabitatLog ) {
    const bool pickedUp = GetBEI().GetRobotInfo().IsPickedUp();
    const bool onChargerContacts = GetBEI().GetRobotInfo().IsOnChargerContacts();
    const bool onChargerPlatform = GetBEI().GetRobotInfo().IsOnChargerPlatform();
    const auto& cliffSensorComp = GetBEI().GetCliffSensorComponent();
    const bool whiteFL = cliffSensorComp.IsWhiteDetected( CliffSensor::CLIFF_FL );
    const bool whiteFR = cliffSensorComp.IsWhiteDetected( CliffSensor::CLIFF_FR );
    const bool whiteBL = cliffSensorComp.IsWhiteDetected( CliffSensor::CLIFF_BL );
    const bool whiteBR = cliffSensorComp.IsWhiteDetected( CliffSensor::CLIFF_BR );
    u16 distance_mm = 0;
    const bool hasReading = GetProxReading( distance_mm );
    const char* const fmtStr = "oldState = %d, state = %d, pickedUp = %d, onContact = %d, onChargerPlatform = %d, white = {%d, %d, %d %d}, step = %d, prox = %u (%s), distDriven = %d";
    PRINT_CH_INFO( "Behaviors", "BehaviorOnboardingDetectHabitat.BehaviorUpdate.Info", fmtStr,
                   (int)oldState, (int)_dVars.state, pickedUp, onChargerContacts, onChargerPlatform,
                   whiteFL, whiteFR, whiteBL, whiteBR,
                   (int)_dVars.lastOnboardingStep, distance_mm,
                   (hasReading ? "valid" : "invalid"), GetDistanceDriven() );
  }
  
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorOnboardingDetectHabitat::GetProxReading( u16& distance_mm ) const
{
  distance_mm = 0;
  auto& proxSensor = GetBEI().GetComponentWrapper(BEIComponentID::ProxSensor).GetComponent<ProxSensorComponent>();
  const bool isSensorReadingValid = proxSensor.GetLatestDistance_mm(distance_mm);
  return isSensorReadingValid;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
u16 BehaviorOnboardingDetectHabitat::GetDistanceDriven() const
{
  const double distance_mm = GetBEI().GetRobotInfo().GetMoveComponent().GetBodyOdometer_mm()
                               - _dVars.bodyOdomAtInitComeHere_mm;
  return static_cast<u16>(distance_mm);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingDetectHabitat::SetOnboardingStep( int step )
{
  const bool isComeHere = (step == external_interface::STEP_FIRST_VOICE_COMMAND);
  if( isComeHere && !_dVars.drivingComeHere ) {
    _dVars.bodyOdomAtInitComeHere_mm = GetBEI().GetRobotInfo().GetMoveComponent().GetBodyOdometer_mm();
  }
  _dVars.drivingComeHere = isComeHere;
  _dVars.lastOnboardingStep = step; // only save this for logging purposes
}

}
}
