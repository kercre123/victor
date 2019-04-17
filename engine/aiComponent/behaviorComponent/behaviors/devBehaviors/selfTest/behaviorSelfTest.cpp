 /**
 * File: behaviorSelfTest.cpp
 *
 * Author: Al Chaussee
 * Created: 11/15/2018
 *
 * Description: Runs individual steps (behaviors) of the self test and manages switching between them and dealing
 *              with failures
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/selfTest/behaviorSelfTest.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/selfTest/iBehaviorSelfTest.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/cubes/cubeCommsComponent.h"
#include "engine/components/backpackLights/engineBackpackLightComponent.h"
#include "engine/components/settingsManager.h"
#include "engine/components/visionComponent.h"
#include "engine/cozmoContext.h"
#include "engine/drivingAnimationHandler.h"
#include "engine/robot.h"
#include "engine/robotManager.h"

#include "clad/types/behaviorComponent/behaviorClasses.h"
#include "clad/types/behaviorComponent/behaviorIDs.h"

#include "util/console/consoleSystem.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/DAS.h"

#include <cstddef>

namespace Anki {
namespace Vector {

BehaviorSelfTest::BehaviorSelfTest(const Json::Value& config)
: ICozmoBehavior( config)
{
  SubscribeToTags({GameToEngineTag::WifiConnectResponse});
}

void BehaviorSelfTest::InitBehavior()
{
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  const BehaviorContainer& BC = GetBEI().GetBehaviorContainer();

  ICozmoBehaviorPtr putOnChargerBehavior = BC.FindBehaviorByID(BehaviorID::SelfTestPutOnCharger);
  DEV_ASSERT(putOnChargerBehavior != nullptr &&
             putOnChargerBehavior->GetClass() == BehaviorClass::SelfTestPutOnCharger,
             "BehaviorSelfTest.ImproperClassRetrievedForName.SelfTestPutOnCharger");
  _selfTestBehaviors.push_back(std::static_pointer_cast<IBehaviorSelfTest>(putOnChargerBehavior));

  ICozmoBehaviorPtr buttonBehavior = BC.FindBehaviorByID(BehaviorID::SelfTestButton);
  DEV_ASSERT(buttonBehavior != nullptr &&
             buttonBehavior->GetClass() == BehaviorClass::SelfTestButton,
             "BehaviorSelfTest.ImproperClassRetrievedForName.SelfTestButton");
  _selfTestBehaviors.push_back(std::static_pointer_cast<IBehaviorSelfTest>(buttonBehavior));

  ICozmoBehaviorPtr initChecksBehavior = BC.FindBehaviorByID(BehaviorID::SelfTestInitChecks);
  DEV_ASSERT(initChecksBehavior != nullptr &&
             initChecksBehavior->GetClass() == BehaviorClass::SelfTestInitChecks,
             "BehaviorSelfTest.ImproperClassRetrievedForName.SelfTestInitChecks");
  _selfTestBehaviors.push_back(std::static_pointer_cast<IBehaviorSelfTest>(initChecksBehavior));

  ICozmoBehaviorPtr motorCalibrationBehavior = BC.FindBehaviorByID(BehaviorID::SelfTestMotorCalibration);
  DEV_ASSERT(motorCalibrationBehavior != nullptr &&
             motorCalibrationBehavior->GetClass() == BehaviorClass::SelfTestMotorCalibration,
             "BehaviorSelfTest.ImproperClassRetrievedForName.SelfTestMotorCalibration");
  _selfTestBehaviors.push_back(std::static_pointer_cast<IBehaviorSelfTest>(motorCalibrationBehavior));

  ICozmoBehaviorPtr driftCheckBehavior = BC.FindBehaviorByID(BehaviorID::SelfTestDriftCheck);
  DEV_ASSERT(driftCheckBehavior != nullptr &&
             driftCheckBehavior->GetClass() == BehaviorClass::SelfTestDriftCheck,
             "BehaviorSelfTest.ImproperClassRetrievedForName.SelfTestDriftCheck");
  _selfTestBehaviors.push_back(std::static_pointer_cast<IBehaviorSelfTest>(driftCheckBehavior));

  ICozmoBehaviorPtr soundCheckBehavior = BC.FindBehaviorByID(BehaviorID::SelfTestSoundCheck);
  DEV_ASSERT(soundCheckBehavior != nullptr &&
             soundCheckBehavior->GetClass() == BehaviorClass::SelfTestSoundCheck,
             "BehaviorSelfTest.ImproperClassRetrievedForName.SelfTestSoundCheck");
  _selfTestBehaviors.push_back(std::static_pointer_cast<IBehaviorSelfTest>(soundCheckBehavior));

  ICozmoBehaviorPtr driveFowardsBehavior = BC.FindBehaviorByID(BehaviorID::SelfTestDriveForwards);
  DEV_ASSERT(driveFowardsBehavior != nullptr &&
             driveFowardsBehavior->GetClass() == BehaviorClass::SelfTestDriveForwards,
             "BehaviorSelfTest.ImproperClassRetrievedForName.SelfTestDriveForwards");
  _selfTestBehaviors.push_back(std::static_pointer_cast<IBehaviorSelfTest>(driveFowardsBehavior));

  ICozmoBehaviorPtr lookAtCharger = BC.FindBehaviorByID(BehaviorID::SelfTestLookAtCharger);
  DEV_ASSERT(lookAtCharger != nullptr &&
             lookAtCharger->GetClass() == BehaviorClass::SelfTestLookAtCharger,
             "BehaviorSelfTest.ImproperClassRetrievedForName.SelfTestLookAtCharger");
  _selfTestBehaviors.push_back(std::static_pointer_cast<IBehaviorSelfTest>(lookAtCharger));

  ICozmoBehaviorPtr dockWithCharger = BC.FindBehaviorByID(BehaviorID::SelfTestDockWithCharger);
  DEV_ASSERT(dockWithCharger != nullptr &&
             dockWithCharger->GetClass() == BehaviorClass::SelfTestDockWithCharger,
             "BehaviorSelfTest.ImproperClassRetrievedForName.SelfTestDockWithCharger");
  _selfTestBehaviors.push_back(std::static_pointer_cast<IBehaviorSelfTest>(dockWithCharger));

  ICozmoBehaviorPtr pickupBehavior = BC.FindBehaviorByID(BehaviorID::SelfTestPickup);
  DEV_ASSERT(pickupBehavior != nullptr &&
             pickupBehavior->GetClass() == BehaviorClass::SelfTestPickup,
             "BehaviorSelfTest.ImproperClassRetrievedForName.SelfTestPickup");
  _selfTestBehaviors.push_back(std::static_pointer_cast<IBehaviorSelfTest>(pickupBehavior));

  ICozmoBehaviorPtr putOnChargerBehavior2 = BC.FindBehaviorByID(BehaviorID::SelfTestPutOnCharger2);
  DEV_ASSERT(putOnChargerBehavior2 != nullptr &&
             putOnChargerBehavior2->GetClass() == BehaviorClass::SelfTestPutOnCharger,
             "BehaviorSelfTest.ImproperClassRetrievedForName.SelfTestPutOnCharger2");
  _selfTestBehaviors.push_back(std::static_pointer_cast<IBehaviorSelfTest>(putOnChargerBehavior2));

  ICozmoBehaviorPtr touchBehavior = BC.FindBehaviorByID(BehaviorID::SelfTestTouch);
  DEV_ASSERT(touchBehavior != nullptr &&
             touchBehavior->GetClass() == BehaviorClass::SelfTestTouch,
             "BehaviorSelfTest.ImproperClassRetrievedForName.SelfTestTouch");
  _selfTestBehaviors.push_back(std::static_pointer_cast<IBehaviorSelfTest>(touchBehavior));

  ICozmoBehaviorPtr screenAndBackpackBehavior = BC.FindBehaviorByID(BehaviorID::SelfTestScreenAndBackpack);
  DEV_ASSERT(screenAndBackpackBehavior != nullptr &&
             screenAndBackpackBehavior->GetClass() == BehaviorClass::SelfTestScreenAndBackpack,
             "BehaviorSelfTest.ImproperClassRetrievedForName.SelfTestScreenAndBackpack");
  _selfTestBehaviors.push_back(std::static_pointer_cast<IBehaviorSelfTest>(screenAndBackpackBehavior));

  _currentSelfTestBehaviorIter = _selfTestBehaviors.begin();
  _currentBehavior = *_currentSelfTestBehaviorIter;

  if(robot.HasExternalInterface())
  {
    auto helper = MakeAnkiEventUtil(*robot.GetExternalInterface(), *this, _signalHandles);
    helper.SubscribeEngineToGame<ExternalInterface::MessageEngineToGameTag::SelfTestBehaviorFailed>();
  }
}

void BehaviorSelfTest::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  for(auto& entry: _selfTestBehaviors)
  {
    delegates.insert(entry.get());
  }
}


void BehaviorSelfTest::OnBehaviorActivated()
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  // Clear backpack lights
  robot.GetBackpackLightComponent().ClearAllBackpackLightConfigs();

  // Set master volume for speaker check
  // Not going through SettingsManager in order to
  // be able to easily restore the previous volume setting
  if(robot.HasExternalInterface())
  {
    robot.GetExternalInterface()->BroadcastToEngine<ExternalInterface::SetRobotVolume>(SelfTestConfig::kSoundVolume);
  }

  // Push empty driving animations so they don't play
  const DrivingAnimationHandler::DrivingAnimations anims;
  robot.GetDrivingAnimationHandler().PushDrivingAnimations(anims, GetDebugLabel());

  // Request scan/connection to specific SSID
  SwitchboardInterface::WifiConnectRequest msg;
  memset(&msg.ssid, 0, msg.ssid.size());
  memset(&msg.pwd, 0, msg.pwd.size());

  // Copy from string into fixed sized array, taking care to only copy what we need or what we have space for
  // msg.ssid.size()-1 to account for null terminator in fixed sized array
  memcpy(&msg.ssid, SelfTestConfig::kWifiSSID.data(), std::min(msg.ssid.size()-1, SelfTestConfig::kWifiSSID.size()));
  memcpy(&msg.pwd, SelfTestConfig::kWifiPwd.data(), std::min(msg.pwd.size()-1, SelfTestConfig::kWifiPwd.size()));

  msg.disconnectAfterConnection = true;
  robot.Broadcast(ExternalInterface::MessageEngineToGame(std::move(msg)));

  _radioScanState = RadioScanState::WaitingForWifiResult;

  // Delegate to the first behavior
  (void)_currentBehavior->WantsToBeActivated();
  DelegateNow(_currentBehavior.get());

  // DAS msg for self test start
  DASMSG(behavior_self_test_start, "behavior.self_test_start", "Self test has started");
  DASMSG_SEND();
}

void BehaviorSelfTest::OnBehaviorDeactivated()
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  // Clear backpack lights
  robot.GetBackpackLightComponent().ClearAllBackpackLightConfigs();

  // Remove the driving animations we pushed
  robot.GetDrivingAnimationHandler().RemoveDrivingAnimations(GetDebugLabel());

  // Disconnect from the cube if we had connected to it for the radio check
  robot.GetCubeCommsComponent().RequestDisconnectFromCube(0);

  // Reapply current settings, this is to put robot volume back to user set level
  SettingsManager& settings = robot.GetComponent<SettingsManager>();
  settings.ApplyAllCurrentSettings();

  Reset();
}

void BehaviorSelfTest::Reset()
{
  // Reset all self test behaviors
  for(auto& behavior : _selfTestBehaviors)
  {
    behavior->Reset();
  }

  CancelDelegates(false);

  // Set current behavior to first playpen behavior
  _currentSelfTestBehaviorIter = _selfTestBehaviors.begin();
  _currentBehavior = *_currentSelfTestBehaviorIter;

  _waitForButtonToEndTest = false;

  _buttonPressed = false;

  _radioScanState = RadioScanState::None;
}

void BehaviorSelfTest::BehaviorUpdate()
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  // If we had to fall back to cube connection and it passed
  // then immediately disconnect from the cube
  if(_radioScanState == RadioScanState::Passed &&
     robot.GetCubeCommsComponent().IsConnectedToCube())
  {
    robot.GetCubeCommsComponent().RequestDisconnectFromCube(0);
  }

  // Check for button press if we are waiting for one in order to end the test
  if(_waitForButtonToEndTest)
  {
    const bool buttonPressed = robot.IsPowerButtonPressed();

    if(_buttonPressed && !buttonPressed)
    {
      CancelSelf();

      // This should clear the face and put us back to whatever behavior was previously running
      robot.SendRobotMessage<RobotInterface::SelfTestEnd>();
      robot.Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::SelfTestEnd()));

      _waitForButtonToEndTest = false;
    }

    _buttonPressed = buttonPressed;
  }

  if(_currentBehavior != nullptr)
  {
    // Check if the current behavior has failed
    const SelfTestResultCode result = _currentBehavior->GetResult();
    if(result != SelfTestResultCode::UNKNOWN &&
       result != SelfTestResultCode::SUCCESS)
    {
      PRINT_NAMED_WARNING("BehaviorSelfTest.Update.BehaviorFailed",
                          "Behavior %s failed with %s",
                          _currentBehavior->GetDebugLabel().c_str(),
                          EnumToString(_currentBehavior->GetResult()));

      _currentBehavior = nullptr;

      HandleResult(result);
      return;
    }
  }

  // If the current behavior has completed
  if(_currentBehavior != nullptr &&
     _currentBehavior->GetResult() == SelfTestResultCode::SUCCESS)
  {
    // Move to the next behavior
    _currentSelfTestBehaviorIter++;

    // If we still have behaviors to run
    if(_currentSelfTestBehaviorIter != _selfTestBehaviors.end())
    {
      // Update current behavior
      _currentBehavior = *_currentSelfTestBehaviorIter;
      DEV_ASSERT(_currentBehavior != nullptr, "BehaviorSelfTest.BehaviorUpdate.NullCurrentBehavior");

      if(!_currentBehavior->WantsToBeActivated())
      {
        PRINT_NAMED_ERROR("BehaviorSelfTest.ChooseNextBehaviorInternal.BehaviorNotRunnable",
                          "Current behavior %s is not runnable",
                          _currentBehavior->GetDebugLabel().c_str());

        HandleResult(SelfTestResultCode::BEHAVIOR_NOT_RUNNABLE);
        return;
      }

      DelegateNow(_currentBehavior.get());
    }
    // All self test behaviors have run so success!
    else
    {
      PRINT_NAMED_INFO("BehaviorSelfTest.Complete", "All behaviors have been run");
      SelfTestResultCode res = DoFinalChecks();
      HandleResult(res);
    }
  }
}


void BehaviorSelfTest::HandleResult(SelfTestResultCode result)
{
  PRINT_NAMED_INFO("BehaviorSelfTest.HandleResult.OrigResult",
                   "%s", EnumToString(result));

  const auto& allResults = IBehaviorSelfTest::GetAllSelfTestResults();

  // If this is a success but we are ignoring failures a behavior may have actually
  // failed so check all results
  if(result == SelfTestResultCode::SUCCESS && SelfTestConfig::kIgnoreFailures)
  {
    for(const auto& resultPair : allResults)
    {
      for(const auto& res : resultPair.second)
      {
        if(res != SelfTestResultCode::SUCCESS)
        {
          result = res;
          PRINT_NAMED_INFO("BehaviorSelfTest.HandleResultInternal.ChangingResult",
                           "%s actually failed with %s, changing result",
                           resultPair.first.c_str(), EnumToString(res));
          break;
        }
      }
    }
  }

  IBehaviorSelfTest::ResetAllSelfTestResults();

  PRINT_NAMED_INFO("BehaviorSelfTest.HandleResultInternal.Result",
                   "Playpen completed with %s",
                   EnumToString(result));

  // Display result on screen
  DisplayResult(result);

  // DAS msg for end of self test
  const bool testPassed = (result == SelfTestResultCode::SUCCESS);
  DASMSG(behavior_self_test_end, "behavior.self_test_end", "Self test has completed");
  DASMSG_SET(s1, testPassed ? "success" : "failure", "Test passed or failed");
  DASMSG_SET(s2, testPassed ? "" : EnumToString(result), "Error code if test failed");
  DASMSG_SEND();

  // Reset playpen
  Reset();

  // Not done yet, need to wait for a button press in order to the test to end
  _waitForButtonToEndTest = true;
}

void BehaviorSelfTest::DisplayResult(SelfTestResultCode result)
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  if(result == SelfTestResultCode::SUCCESS)
  {
    static const BackpackLightAnimation::BackpackAnimation passLights = {
      .onColors               = {{NamedColors::GREEN,NamedColors::GREEN,NamedColors::GREEN}},
      .offColors              = {{NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK}},
      .onPeriod_ms            = {{1000,1000,1000}},
      .offPeriod_ms           = {{100,100,100}},
      .transitionOnPeriod_ms  = {{450,450,450}},
      .transitionOffPeriod_ms = {{450,450,450}},
      .offset                 = {{0,0,0}}
    };

    robot.GetBackpackLightComponent().SetBackpackAnimation(passLights);

    IBehaviorSelfTest::DrawTextOnScreen(robot,
                                        {"OK",
                                         "Press button",
                                         "to finish test"},
                                        NamedColors::BLACK,
                                        NamedColors::GREEN);
  }
  else
  {
    // I would like to play this animation here but it will cause the face to go blank
    // so the user would not be able to see the result text
    //PlayAnimationAction* action = new PlayAnimationAction("playpenFailAnim");
    //robot.GetActionList().AddConcurrentAction(action);

    static const BackpackLightAnimation::BackpackAnimation failLights = {
      .onColors               = {{NamedColors::RED,NamedColors::RED,NamedColors::RED}},
      .offColors              = {{NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK}},
      .onPeriod_ms            = {{500,500,500}},
      .offPeriod_ms           = {{500,500,500}},
      .transitionOnPeriod_ms  = {{0,0,0}},
      .transitionOffPeriod_ms = {{0,0,0}},
      .offset                 = {{0,0,0}}
    };

    robot.GetBackpackLightComponent().SetBackpackAnimation(failLights);

    IBehaviorSelfTest::DrawTextOnScreen(robot,
                                        {std::to_string((u32)result), "",
                                         "Press button",
                                         "to end test"},
                                        NamedColors::BLACK,
                                        NamedColors::RED);
  }
}

template<>
void BehaviorSelfTest::HandleMessage(const ExternalInterface::SelfTestBehaviorFailed& msg)
{
  PRINT_NAMED_WARNING("BehaviorSelfTest.HandleMessage.SelfTestBehaviorFailed",
                      "Some behavior failed with %s%s",
                      EnumToString(msg.result),
                      (SelfTestConfig::kIgnoreFailures ? ", ignoring" : ""));

  if(!SelfTestConfig::kIgnoreFailures)
  {
    if(_currentBehavior != nullptr)
    {
      _currentBehavior->OnDeactivated();
    }

    HandleResult(msg.result);
  }
}

void BehaviorSelfTest::StartCubeConnectionCheck()
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  auto cubeConnectionCallback = [this](bool success) {
      _radioScanState = ((success || SelfTestConfig::kSkipActiveObjectCheck) ?
                         RadioScanState::Passed :
                         RadioScanState::Failed);
    };

  robot.GetCubeCommsComponent().RequestConnectToCube(cubeConnectionCallback);

  _radioScanState = RadioScanState::WaitingForCubeResult;
}

void BehaviorSelfTest::HandleWhileActivated(const GameToEngineEvent& event)
{
  const auto& tag = event.GetData().GetTag();

  // The only GameToEngine message we expect is WifiConnectResponse
  if(tag != GameToEngineTag::WifiConnectResponse)
  {
    return;
  }

  const auto& payload = event.GetData().Get_WifiConnectResponse();

  // Fall back to a cube connection check should wifi fail
  // for normal users this is expected as the wifi checks for a hardcoded network
  if(payload.status_code != 0)
  {
    PRINT_NAMED_INFO("BehaviorSelfTest.HandleWifiConnectResponse",
                     "Connection request failed with %u, falling back to cube check",
                     payload.status_code);

    StartCubeConnectionCheck();
  }
  else
  {
    PRINT_NAMED_INFO("BehaviorSelfTest.HandleWifiConnectResponse.Passed", "");

    _radioScanState = RadioScanState::Passed;
  }
}

SelfTestResultCode BehaviorSelfTest::DoFinalChecks()
{
  return (_radioScanState == RadioScanState::Passed ?
          SelfTestResultCode::SUCCESS :
          SelfTestResultCode::RADIO_CHECK_FAILED);
}

}
}
