/**
 * File: batteryComponent.cpp
 *
 * Author: Matt Michini
 * Created: 2/27/2018
 *
 * Description: Component for monitoring battery state-of-charge, time on charger,
 *              and related information.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/components/battery/batteryComponent.h"

#include "engine/aiComponent/aiComponent.h"

#include "engine/blockWorld/blockWorld.h"
#include "engine/blockWorld/blockWorldFilter.h"
#include "engine/charger.h"
#include "engine/components/battery/batteryStats.h"
#include "engine/components/cubes/cubeBatteryComponent.h"
#include "engine/components/movementComponent.h"
#include "engine/robot.h"

#include "anki/cozmo/shared/cozmoConfig.h"

#include "coretech/common/engine/utils/timer.h"

#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/types/robotStatusAndActions.h"

#include "osState/osState.h"
#include "proto/external_interface/shared.pb.h"

#include "util/console/consoleInterface.h"
#include "util/filters/lowPassFilterSimple.h"
#include "util/logging/DAS.h"

#include <unistd.h>

#define LOG_CHANNEL "Battery"

namespace Anki {
namespace Vector {

namespace {
  // How often the filtered voltage reading is updated
  // (i.e. Rate of RobotState messages)
  const float kBatteryVoltsUpdatePeriod_sec =  STATE_MESSAGE_FREQUENCY * ROBOT_TIME_STEP_MS / 1000.f;
  const float kCalmModeBatteryVoltsUpdatePeriod_sec =  STATE_MESSAGE_FREQUENCY_CALM * ROBOT_TIME_STEP_MS / 1000.f;

  // Time constant of the low-pass filter for battery voltage
  const float kBatteryVoltsFilterTimeConstant_sec = 20.0f;

  const float kInvalidPitchAngle = std::numeric_limits<float>::max();


  #define CONSOLE_GROUP "BatteryComponent"

  // Console var for faking low battery
  CONSOLE_VAR(bool, kFakeLowBattery, CONSOLE_GROUP, false);
  const float kFakeLowBatteryVoltage = 3.5f;

  // If non-zero, a low battery status is faked after this many seconds off charger
  CONSOLE_VAR(uint32_t, kFakeLowBatteryAfterOffChargerTimeout_sec, CONSOLE_GROUP, 0);
  float _fakeLowBatteryTime_sec = 0.f;

  // Console var for faking disconnected battery
  CONSOLE_VAR(bool, kFakeDisconnectedBattery, CONSOLE_GROUP, false);

  // The app needs an estimate of time-to-charge in a few cases (mostly onboarding/FTUE). When low
  // battery, the robot must sit on the charger for kRequiredChargeTime_s seconds. If the robot is
  // removed, the timer pauses. When being re-placed on the charger, the remaining time is increased
  // my an amount proportional to the time off charger (const is kExtraChargingTimePerDischargePeriod_s),
  // clamped at a max of kRequiredChargeTime_s.
  CONSOLE_VAR_RANGED(float, kRequiredChargeTime_s, CONSOLE_GROUP, 5*60.0f, 10.0f, 9999.0f ); // must be set before low battery and then not changed
  const float kExtraChargingTimePerDischargePeriod_s = 1.0f; // if off the charger for 1 min, must charge an additional 1*X mins

#if ANKI_DEV_CHEATS
  // General periodic battery logging (for battery life testing)
  CONSOLE_VAR(bool, kPeriodicDebugDASLogging, CONSOLE_GROUP, false);
#endif  

  #undef CONSOLE_GROUP
}

BatteryComponent::BatteryComponent()
  : IDependencyManagedComponent<RobotComponentID>(this, RobotComponentID::Battery)
  , _chargerFilter(std::make_unique<BlockWorldFilter>())
  , _batteryStatsAccumulator(std::make_unique<BatteryStats>())
{
  // setup battery voltage low pass filter (samples come in at kBatteryVoltsUpdatePeriod_sec)
  _batteryVoltsFilter = std::make_unique<Util::LowPassFilterSimple>(kBatteryVoltsUpdatePeriod_sec,
                                                                    kBatteryVoltsFilterTimeConstant_sec);

  // setup block world filter to find chargers:
  _chargerFilter->AddAllowedType(ObjectType::Charger_Basic);

  _lastOnChargerContactsPitchAngle.performRescaling(false);
  _lastOnChargerContactsPitchAngle = kInvalidPitchAngle;
}

void BatteryComponent::Init(Vector::Robot* robot)
{
  _robot = robot;
}

void BatteryComponent::NotifyOfRobotState(const RobotState& msg)
{
  _lastMsgTimestamp = msg.timestamp;
  const float now_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

  // Update raw voltage
  _batteryVoltsRaw = msg.batteryVoltage;
  _chargerVoltsRaw = msg.chargerVoltage;

  // Check if faking low battery
  static bool wasFakeLowBattery = false;
  if (kFakeLowBattery) {
    _batteryVoltsRaw  = kFakeLowBatteryVoltage;
    _batteryVoltsFilter->Reset();
  } else if (wasFakeLowBattery) {
    _batteryVoltsFilter->Reset();
  }
  wasFakeLowBattery = kFakeLowBattery;

  SetTemperature(msg.battTemp_C);

  // Check if battery is _really_ overheating, enough for a shutdown to be coming.
  // This is actually handled in vic-robot, but is recorded here for viz purposes.
  #define IS_STATUS_FLAG_SET(x) ((msg.status & (uint32_t)RobotStatusFlag::x) != 0)
  _battOverheated = IS_STATUS_FLAG_SET(IS_BATTERY_OVERHEATED);

  // Only update filtered value if the battery isn't disconnected
  _wasBattDisconnected = _battDisconnected;
  _battDisconnected = IS_STATUS_FLAG_SET(IS_BATTERY_DISCONNECTED)
                      || (_batteryVoltsRaw < 3);  // Just in case syscon doesn't report IS_BATTERY_DISCONNECTED for some reason.
                                                  // Anything under 3V doesn't make sense.

  // Fake disconnected battery but only when on charger
  if (kFakeDisconnectedBattery && IsOnChargerContacts()) {
    _battDisconnected = true;
  }

  // If in calm mode, RobotState messages are expected to come in at a slower rate
  // and we therefore need to adjust the sampling rate of the filter.
  static bool prevSysconCalmMode = false;
  const bool currSysconCalmMode = IS_STATUS_FLAG_SET(CALM_POWER_MODE);
  if (currSysconCalmMode && !prevSysconCalmMode) {
    _batteryVoltsFilter->SetSamplePeriod(kCalmModeBatteryVoltsUpdatePeriod_sec);
  } else if (!currSysconCalmMode && prevSysconCalmMode) {
    _batteryVoltsFilter->SetSamplePeriod(kBatteryVoltsUpdatePeriod_sec);
  }
  prevSysconCalmMode = currSysconCalmMode;

  // If processes start while the battery is disconnected (because it's been on the charger for > 30min),
  // we make sure to set the battery voltage to a less wrong _batteryVoltsRaw.
  // Otherwise, the filtered value is only updated when the battery is connected.
  if (!_battDisconnected || NEAR_ZERO(_batteryVoltsFilt)) {
    if (_resetVoltageFilterWhenBatteryConnected) {
      DASMSG(battery_voltage_reset, "battery.voltage_reset", "Indicates that the battery voltage was reset following a change in onCharger state");  
      DASMSG_SET(i2, now_sec - _lastOnChargerContactsChange_sec, "Time since placed on charger (sec)");
      DASMSG_SET(i3, GetBatteryVoltsRaw_mV(), "New battery voltage (mV)");
      DASMSG_SET(i4, GetBatteryTemperature_C(), "Current temperature (C)");
      DASMSG_SEND();
      _batteryVoltsFilter->Reset();
      _resetVoltageFilterWhenBatteryConnected = false;
    }
    _batteryVoltsFilt = _batteryVoltsFilter->AddSample(_batteryVoltsRaw);
  }

  const bool wasCharging = IsCharging();
  const auto oldBatteryLevel = _batteryLevel;

  // Update isCharging / isOnChargerContacts / isOnChargerPlatform
  SetOnChargeContacts(IS_STATUS_FLAG_SET(IS_ON_CHARGER));
  SetIsCharging(IS_STATUS_FLAG_SET(IS_CHARGING));
  UpdateOnChargerPlatform();

  // Check for change in disconnected state
  if (_battDisconnected != _wasBattDisconnected) {
    _lastDisconnectedChange_sec = now_sec;

    // The battery becomes disconnected for one of two reasons:
    //
    // - Once when, on being put on the charger, the battery is too hot and needs to cool down.
    //   By disconnecting here, the battery will have a chance to cool down (given time).
    //   The IS_CHARGING bit is set to true by syscon even though it is not actually charging.
    //   After the battery cools down, it will be reconnected and charging will resume.
    //   During this cooldown period, the IS_CHARGING bit is kept true to hide the fact
    //   that it is disconnected from users. This ensures the robot is left alone, on 
    //   the charger, so that it may naturally cool down and eventually resume charging.
    //
    // - When the battery has been charging for 5 minutes past 4.2V, or 25 minutes of 
    //   battery-connected charging has occured, whichver comes first. In this case, the 
    //   battery is disconnected to prolong its life by not overcharging. 
    //   The IS_CHARGING bit is set to false here, because no more charging takes place.
    //   (This logic and relevant constants are defined in robot/syscon/src/analog.cpp)
    if (IsCharging()) {
      // DAS message for when battery is disconnected for cooldown
      DASMSG(battery_cooldown, "battery.cooldown", "Indicates that the battery was disconnected/reconnected in order to cool down the battery");
      DASMSG_SET(i1, _battDisconnected, "Whether we have started or stopped cooldown (1 if we have started, 0 if we have stopped)");
      DASMSG_SET(i2, now_sec - _lastOnChargerContactsChange_sec, "Time since placed on charger (sec)");
      DASMSG_SET(i3, GetBatteryVolts_mV(), "Current filtered battery voltage (mV)");
      DASMSG_SET(i4, GetBatteryTemperature_C(), "Current temperature (C)");
      DASMSG_SEND();
    }
  }

  // Check if charging has completed
  bool isFullyCharged = false;
  if (_battDisconnected && !IsCharging()) {
    isFullyCharged = true;
  }

  // Check if low battery
  bool isLowBattery = IS_STATUS_FLAG_SET(IS_BATTERY_LOW);

  // Fake low battery?
  if (!IsOnChargerContacts()) {
    if (kFakeLowBattery) {
      isLowBattery = true;
    } else if ((kFakeLowBatteryAfterOffChargerTimeout_sec != 0) &&
               (now_sec > _fakeLowBatteryTime_sec)) {
      isLowBattery = true;
    }
  } else if (kFakeLowBatteryAfterOffChargerTimeout_sec != 0) {
    // Reset countdown-to-low battery timer when on charger
    _fakeLowBatteryTime_sec = now_sec + kFakeLowBatteryAfterOffChargerTimeout_sec;
  }

  // Update battery charge level
  BatteryLevel level = BatteryLevel::Nominal;
  if (isFullyCharged) {
    // NOTE: Battery can only be full while on charger
    level = BatteryLevel::Full;
  } else if (isLowBattery) {
    level = BatteryLevel::Low;
  }
  

  if (level != _batteryLevel) {
    LOG_INFO("BatteryComponent.BatteryLevelChanged",
                     "New battery level %s (previously %s for %f seconds)",
                     BatteryLevelToString(level),
                     BatteryLevelToString(_batteryLevel),
                     now_sec - _lastBatteryLevelChange_sec);

    DASMSG(battery_level_changed, "battery.battery_level_changed", "The battery level has changed");
    DASMSG_SET(s1, BatteryLevelToString(level), "New battery level");
    DASMSG_SET(s2, BatteryLevelToString(_batteryLevel), "Previous battery level");
    DASMSG_SET(s3, now_sec - _lastOnChargerContactsChange_sec, "Time since last on charger (sec)");
    DASMSG_SET(i1, IsCharging(), "Is the battery currently charging? 1 if charging, 0 if not");
    DASMSG_SET(i2, now_sec - _lastBatteryLevelChange_sec, "Time spent at previous battery level (sec)");
    DASMSG_SET(i3, GetBatteryVolts_mV(), "Current filtered battery voltage (mV)");
    DASMSG_SET(i4, _battDisconnected, "Battery is (1) disconnected or (0) connected");
    DASMSG_SEND();

    _lastBatteryLevelChange_sec = now_sec;
    _prevBatteryLevel = _batteryLevel;
    _batteryLevel = level;
  }


  static RobotInterface::BatteryStatus prevStatus;
  RobotInterface::BatteryStatus curStatus;
  curStatus.isLow                 = IsBatteryLow();
  curStatus.isCharging            = IsCharging();
  curStatus.onChargerContacts     = IsOnChargerContacts();
  curStatus.isBatteryFull         = IsBatteryFull();
  curStatus.isBatteryDisconnected = IsBatteryDisconnectedFromCharger();

  if(curStatus != prevStatus)
  {
    prevStatus = curStatus;
    _robot->SendMessage(RobotInterface::EngineToRobot(std::move(curStatus)));
  }

  const bool wasLowBattery = (oldBatteryLevel == BatteryLevel::Low);
  UpdateSuggestedChargerTime(wasLowBattery, wasCharging);

  _batteryStatsAccumulator->Update(_battTemperature_C, _batteryVoltsFilt);

  // Report encoder stats only while off charger
  // (Encoders should normally be off while on charger)
  if (!IsOnChargerContacts()) {
    bool encodersDisabled = IS_STATUS_FLAG_SET(ENCODERS_DISABLED);
    _batteryStatsAccumulator->UpdateEncoderStats(encodersDisabled, currSysconCalmMode);
  }

#if ANKI_DEV_CHEATS
  // General periodic debug battery logging
  static int s_nextReportTime_sec = now_sec;
  static const int kReportPeriod_sec = 5;
  if (kPeriodicDebugDASLogging) {
    if (now_sec > s_nextReportTime_sec) {
      s_nextReportTime_sec += kReportPeriod_sec;
      DASMSG(battery_periodic_log, "battery.periodic_log", "For battery life debug logging");
      DASMSG_SET(i1, GetBatteryVoltsRaw_mV(), "Raw voltage (mV)");
      DASMSG_SET(i2, GetBatteryVolts_mV(), "Filtered voltage (mV)");
      DASMSG_SET(i3, GetBatteryTemperature_C(), "Temperature (C)");
      DASMSG_SET(i4, IsOnChargerContacts(), "On charge contacts");
      DASMSG_SET(s1, IsBatteryDisconnectedFromCharger() ? "1" : "0", "Battery disconnected state");
      DASMSG_SET(s2, IsCharging() ? "1" : "0", "Battery charging state");
      DASMSG_SET(s3, currSysconCalmMode ? "1" : "0", "Calm mode enabled");
      DASMSG_SET(s4, std::to_string( OSState::getInstance()->GetTemperature_C() ), "CPU temperature (C)");
      DASMSG_SEND();
    }
  } else {
    s_nextReportTime_sec = now_sec;
  }
#endif  
}


float BatteryComponent::GetTimeAtLevelSec(BatteryLevel level) const
{
  float timeSinceLevelChange_sec = 0.f;
  if (_batteryLevel == level) {
    const float now_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    timeSinceLevelChange_sec = now_sec - _lastBatteryLevelChange_sec;
  }
  return timeSinceLevelChange_sec;
}

void BatteryComponent::SetOnChargeContacts(const bool onChargeContacts)
{

  // If we are being set on a charger, we can update the instance of the charger in the current world to
  // match the robot. If we don't have an instance, we can add an instance now
  if (onChargeContacts)
  {
    _robot->GetBlockWorld().SetRobotOnChargerContacts();
    
    // Update the last OnChargeContacts pitch angle
    if (!_robot->GetMoveComponent().IsMoving()) {
      _lastOnChargerContactsPitchAngle = _robot->GetPitchAngle();
    }
  }

  // Log events and send message if state changed
  if (onChargeContacts != _isOnChargerContacts) {
    _isOnChargerContacts = onChargeContacts;

    // The voltage usually steps up or down by a few hundred millivolts when we start/stop charging, so reset the low
    // pass filter here to more closely track the actual battery voltage, but only if the battery isn't disconnected
    // otherwise the measured voltage doesn't reflect the actual battery voltage. We also delay the update by at least
    // one RobotState message delay to allow the voltage value to settle, but it will take longer if the battery is
    // disconnected (because it's too hot) since we don't want to reset the filter to a measurement taken while disconnected.
    _resetVoltageFilterWhenBatteryConnected = true;

    const float now_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

    // Report time since low battery started if battery was low
    u32 timeSinceLowBattStarted_sec = IsBatteryLow() ? static_cast<u32>(now_sec - _lastBatteryLevelChange_sec) : 0;

    LOG_INFO(onChargeContacts ? "robot.on_charger" : "robot.off_charger", "");
    // Broadcast to game
    using namespace ExternalInterface;
    _robot->Broadcast(MessageEngineToGame(ChargerEvent(onChargeContacts)));
    // Broadcast to DAS
    DASMSG(battery_on_charger_changed, "battery.on_charger_changed", "The robot onChargerContacts state has changed");
    DASMSG_SET(i1, onChargeContacts, 
               "New on charger contacts state (1: on contacts, 0: off contacts)");
    DASMSG_SET(i2, now_sec - _lastOnChargerContactsChange_sec, 
               "Time since previous change (sec)");
    DASMSG_SET(i3, onChargeContacts ? GetBatteryVolts_mV() : GetBatteryVoltsRaw_mV(), 
               "If on charger, last filtered battery voltage. "
               "If off charger, current raw battery voltage (mV)");
    DASMSG_SET(i4, timeSinceLowBattStarted_sec, 
               "Time since low battery (sec)");
    DASMSG_SET(s1, (onChargeContacts ? _battDisconnected : _wasBattDisconnected) ? "disconnected" : "connected", 
               "If on charger, current battery disconnected state. "
               "If off charger, battery disconnected state when it was on charger.");
    DASMSG_SET(s2, EnumToString(_batteryLevel), "Battery level just prior to on charger change");
    DASMSG_SEND();
    _lastOnChargerContactsChange_sec = now_sec;
  }

}


void BatteryComponent::SetIsCharging(const bool isCharging)
{
  if (isCharging != _isCharging) {
    _isCharging = isCharging;

    DASMSG(battery_is_charging_changed, "battery.is_charging_changed", "The robot isCharging state has changed");
    DASMSG_SET(i1, IsCharging(), "Is charging (1) or not (0)");
    DASMSG_SET(i2, _battTemperature_C,  "Battery temperature (C)");
    DASMSG_SET(i3, GetBatteryVolts_mV(), "Current filtered battery voltage (mV)");
    DASMSG_SET(i4, _battDisconnected, "Battery is (1) disconnected or (0) connected");
    DASMSG_SET(s1, std::to_string((int)(1000.f * GetChargerVoltsRaw())), "Charger voltage (mV)");
    DASMSG_SEND();
  }
}

void BatteryComponent::SetTemperature(const u8 temp_C)
{
  _battTemperature_C = temp_C;

  // Print DAS if temperature crosses 50C
  // Mostly for dev to see if conditionHighTemperature is making him too narcoleptic
  const u8 kHotBatteryTemp_degC = 50;
  const u8 kNoLongerHotTemp_degC = 45; // Still hot, but using hysteresis to prevent spamming
  static bool exceededHotThreshold = false;

  auto dasFunc = [kHotBatteryTemp_degC,kNoLongerHotTemp_degC](const bool exceeded) {
    exceededHotThreshold = exceeded;
    DASMSG(battery_temp_crossed_threshold, "battery.temp_crossed_threshold", "Indicates battery temperature exceeded a specified temperature");
    DASMSG_SET(i1, exceeded, "Higher than threshold (1) or lower (0)");
    DASMSG_SET(i2, exceeded ? kHotBatteryTemp_degC : kNoLongerHotTemp_degC, "Temperature crossed (C)");
    DASMSG_SEND();
  };

  if (!exceededHotThreshold && _battTemperature_C >= kHotBatteryTemp_degC) {
    dasFunc(true);
  } else if (exceededHotThreshold && _battTemperature_C <= kNoLongerHotTemp_degC) {
    dasFunc(false);
  }
}


void BatteryComponent::UpdateOnChargerPlatform()
{
  bool onPlatform = _isOnChargerPlatform;

  if (IsOnChargerContacts()) {
    // If we're on the charger _contacts_, we are definitely on the charger _platform_
    onPlatform = true;
  } else if (onPlatform) {
    // Not on the charger contacts, but we still think we're on the charger platform.
    // Make a reasonable conjecture about our current OnChargerPlatform state.

    // Not on charger platform if we're off treads
    if (_robot->GetOffTreadsState() != OffTreadsState::OnTreads) {
      onPlatform = false;
    }

    // Check intersection between robot and charger bounding quads
    auto* charger = _robot->GetBlockWorld().FindLocatedObjectClosestTo(_robot->GetPose(),
                                                                       *_chargerFilter);
    if (charger == nullptr ||
        !charger->GetBoundingQuadXY().Intersects(_robot->GetBoundingQuadXY())) {
      onPlatform = false;
    }

    // Check to see if the robot's pitch angle indicates that it has
    // been moved from the charger platform onto the ground.
    const bool lastPitchAngleValid = (_lastOnChargerContactsPitchAngle.ToFloat() != kInvalidPitchAngle);
    const bool robotMoving = _robot->GetMoveComponent().IsMoving();
    if (lastPitchAngleValid && !robotMoving) {
      const auto& expectedPitchAngleOffPlatform = _lastOnChargerContactsPitchAngle + kChargerSlopeAngle_rad;
      if (_robot->GetPitchAngle() > expectedPitchAngleOffPlatform - DEG_TO_RAD(2.f)) {
        // Pitch angle change indicates that we've probably moved from the
        // charger platform onto the table. Update the robot's pose accordingly.
        onPlatform = false;
        if (charger != nullptr) {
          _robot->SetCharger(charger->GetID());
          _robot->SetPosePostRollOffCharger();
        }
      }
    }
  }

  // Has OnChargerPlatform state changed?
  if (onPlatform != _isOnChargerPlatform) {
    _isOnChargerPlatform = onPlatform;

    using namespace ExternalInterface;
    _robot->Broadcast(MessageEngineToGame(RobotOnChargerPlatformEvent(_isOnChargerPlatform)));

    LOG_INFO("BatteryComponent.UpdateOnChargerPlatform.OnChargerPlatformChange",
                     "robot is now %s the charger platform",
                     _isOnChargerPlatform ? "ON" : "OFF");

    // Reset last on charger pitch angle if we're no longer on the platform
    if (!onPlatform) {
      _lastOnChargerContactsPitchAngle = kInvalidPitchAngle;
    }
  }
}

external_interface::GatewayWrapper BatteryComponent::GetBatteryState(const external_interface::BatteryStateRequest& request)
{
  auto* cubeBatteryMsg = _robot->GetCubeBatteryComponent().GetCubeBatteryMsg().release();
  auto* response = new external_interface::BatteryStateResponse {
    nullptr,
    (external_interface::BatteryLevel) GetBatteryLevel(),
    GetBatteryVolts(),
    IsCharging(),
    IsOnChargerPlatform(),
    GetSuggestedChargerTime(),
    cubeBatteryMsg
  };
  external_interface::GatewayWrapper wrapper;
  wrapper.set_allocated_battery_state_response(response);
  return wrapper;
}

void BatteryComponent::UpdateSuggestedChargerTime(bool wasLowBattery, bool wasCharging)
{
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  const bool isLowBattery = (_batteryLevel == BatteryLevel::Low);

  if( isLowBattery && !wasLowBattery && (_timeRemovedFromCharger_s == 0.0f) ) {
    // Just became low battery. reset end time so the countdown starts below
    _suggestedChargeEndTime_s = 0.0f;
  }

  bool countdownStarted = (_suggestedChargeEndTime_s != 0.0f);

  if( countdownStarted && (currTime_s >= _suggestedChargeEndTime_s) ) {
    // countdown finished
    if( isLowBattery ) {
      // this is ok since the logic below will just kick in again, but it means we should change the time
      // params
      PRINT_NAMED_WARNING( "BatteryComponent.UpdateSuggestedChargerTime.NotCharged",
                           "Charge parameters did not fully charge the robot!" );
    }
    _suggestedChargeEndTime_s = 0.0f;
    _timeRemovedFromCharger_s = 0.0f;
    countdownStarted = false;
  }

  if( IsCharging() ) {
    // currently charging. Don't check for starting to charge (IsCharging() && !wasCharging) here in case
    // the countdown just finished, above, and the times were reset to 0, but wasCharging is still true

    if( isLowBattery && !countdownStarted ) {
      // was never on charger before. start the low battery countdown! it will continue even after the robot
      // is no longer low battery
      _suggestedChargeEndTime_s = currTime_s + kRequiredChargeTime_s;
    } else if( countdownStarted && !wasCharging ){
      // The robot was just placed on the charger, and not for the first time, so there should be a
      // _timeRemovedFromCharger_s. Note that it may not be low battery at this point, but there's
      // still a countdown since countdownStarted
      ANKI_VERIFY( _timeRemovedFromCharger_s != 0.0f, "BatteryComponent.UpdateSuggestedChargerTime.UnexpectedOffChargerTime",
                   "Off charger time was 0, expecting positive" );
      const float elapsedOffChargerTime_s = currTime_s - _timeRemovedFromCharger_s;
      _suggestedChargeEndTime_s += elapsedOffChargerTime_s * (1.0f + kExtraChargingTimePerDischargePeriod_s);
      _suggestedChargeEndTime_s = Anki::Util::Min( _suggestedChargeEndTime_s, currTime_s + kRequiredChargeTime_s);
    }
  } else if( !IsCharging() && wasCharging ) {
    _timeRemovedFromCharger_s = currTime_s;
  }
}

float BatteryComponent::GetSuggestedChargerTime() const
{
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  if( _suggestedChargeEndTime_s == 0.0f ) {
    if( IsBatteryLow() ) {
      // charging hasn't started yet
      return kRequiredChargeTime_s;
    } else {
      return 0.0f;
    }
  } else {
    if( !IsCharging()) {
      // currently off the charger. keep timer fixed
      const float fixedTime = (_suggestedChargeEndTime_s - _timeRemovedFromCharger_s);
      return Anki::Util::Clamp(fixedTime, 0.0f, kRequiredChargeTime_s);
    } else {
      return Anki::Util::Max( _suggestedChargeEndTime_s - currTime_s, 0.0f );
    }
  }
}


float BatteryComponent::GetOnChargerDurationSec() const
{
  if (IsOnChargerContacts()) {
    const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    return currTime_s - _lastOnChargerContactsChange_sec;
  }
  return 0.f;
}

  
float BatteryComponent::GetBatteryDisconnectedDurationSec() const
{
  if (IsBatteryDisconnectedFromCharger()) {
    const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    return currTime_s - _lastDisconnectedChange_sec;
  }
  return 0.f;
}

} // Vector namespace
} // Anki namespace
