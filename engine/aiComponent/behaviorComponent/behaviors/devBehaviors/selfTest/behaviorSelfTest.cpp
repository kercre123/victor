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
#include "engine/components/bodyLightComponent.h"
#include "engine/components/cubes/cubeCommsComponent.h"
#include "engine/components/nvStorageComponent.h"
#include "engine/components/visionComponent.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"
#include "engine/robotManager.h"

#include "clad/types/behaviorComponent/behaviorTypes.h"

#include "util/console/consoleSystem.h"
#include "util/fileUtils/fileUtils.h"

//#include "anki/cozmo/shared/factory/emrHelper.h"

#include <cstddef>

namespace Anki {
namespace Cozmo {

BehaviorSelfTest::BehaviorSelfTest(const Json::Value& config)
: ICozmoBehavior( config)
{

}

void BehaviorSelfTest::InitBehavior()
{
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  const BehaviorContainer& BC = GetBEI().GetBehaviorContainer();

  // ICozmoBehaviorPtr waitToStartBehavior = BC.FindBehaviorByID(BehaviorID::SelfTestWaitToStart);
  // DEV_ASSERT(waitToStartBehavior != nullptr &&
  //            waitToStartBehavior->GetClass() == BehaviorClass::SelfTestWaitToStart,
  //            "BehaviorSelfTest.ImproperClassRetrievedForName.SelfTestWaitToStart");
  // _selfTestBehaviors.push_back(std::static_pointer_cast<IBehaviorSelfTest>(waitToStartBehavior));

  ICozmoBehaviorPtr putOnChargerBehavior = BC.FindBehaviorByID(BehaviorID::SelfTestPutOnCharger);
  DEV_ASSERT(putOnChargerBehavior != nullptr &&
             putOnChargerBehavior->GetClass() == BehaviorClass::SelfTestPutOnCharger,
             "BehaviorSelfTest.ImproperClassRetrievedForName.SelfTestPutOnCharger");
  _selfTestBehaviors.push_back(std::static_pointer_cast<IBehaviorSelfTest>(putOnChargerBehavior));

  ICozmoBehaviorPtr touchBehavior = BC.FindBehaviorByID(BehaviorID::SelfTestTouch);
  DEV_ASSERT(touchBehavior != nullptr &&
             touchBehavior->GetClass() == BehaviorClass::SelfTestTouch,
             "BehaviorSelfTest.ImproperClassRetrievedForName.SelfTestTouch");
  _selfTestBehaviors.push_back(std::static_pointer_cast<IBehaviorSelfTest>(touchBehavior));

  ICozmoBehaviorPtr buttonBehavior = BC.FindBehaviorByID(BehaviorID::SelfTestButton);
  DEV_ASSERT(buttonBehavior != nullptr &&
             buttonBehavior->GetClass() == BehaviorClass::SelfTestButton,
             "BehaviorSelfTest.ImproperClassRetrievedForName.SelfTestButton");
  _selfTestBehaviors.push_back(std::static_pointer_cast<IBehaviorSelfTest>(buttonBehavior));

  ICozmoBehaviorPtr screenAndBackpackBehavior = BC.FindBehaviorByID(BehaviorID::SelfTestScreenAndBackpack);
  DEV_ASSERT(screenAndBackpackBehavior != nullptr &&
             screenAndBackpackBehavior->GetClass() == BehaviorClass::SelfTestScreenAndBackpack,
             "BehaviorSelfTest.ImproperClassRetrievedForName.SelfTestScreenAndBackpack");
  _selfTestBehaviors.push_back(std::static_pointer_cast<IBehaviorSelfTest>(screenAndBackpackBehavior));

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
  //  _startTest = false;
  PRINT_NAMED_WARNING("","STARTING SELF TEST");

  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  robot.GetBodyLightComponent().ClearAllBackpackLightConfigs();

  // if(robot.HasExternalInterface())
  // {
  //   robot.GetExternalInterface()->BroadcastToEngine<ExternalInterface::SetRobotVolume>(SelfTestConfig::kSoundVolume);
  // }

  // // // Start the factory log
  // // std::stringstream serialNumString;
  // // serialNumString << std::hex << robot.GetHeadSerialNumber();
  // // _factoryTestLogger.StartLog( serialNumString.str(), true, robot.GetContextDataPlatform());
  // // PRINT_NAMED_INFO("BehaviorSelfTest.WillLogToDevice",
  // //                  "Log name: %s",
  // //                  _factoryTestLogger.GetLogName().c_str());

  // // Set blind docking mode
  // NativeAnkiUtilConsoleSetValueWithString("PickupDockingMethod", "0");

  // // Disable driving animations
  // NativeAnkiUtilConsoleSetValueWithString("EnableDrivingAnimations", "false");

  // // Disable image streaming
  // if(robot.IsPhysical())
  // {
  //   NativeAnkiUtilConsoleSetValueWithString("ImageCompressQuality", "0");
  // }

  // // Make sure only Marker Detection mode is enabled; we don't need anything else running
  // robot.GetVisionComponent().EnableMode(VisionMode::Idle, true); // first, turn everything off
  // robot.GetVisionComponent().EnableMode(VisionMode::DetectingMarkers, true);

  // // Set and disable auto exposure
  // robot.GetVisionComponent().SetAndDisableAutoExposure(SelfTestConfig::kExposure_ms, SelfTestConfig::kGain);

  // // Set and disable WhiteBalance
  // robot.GetVisionComponent().SetAndDisableWhiteBalance(SelfTestConfig::kGain, SelfTestConfig::kGain, SelfTestConfig::kGain);

  // // Disable block pool from auto connecting
  // if(robot.HasExternalInterface())
  // {
  //   robot.GetExternalInterface()->BroadcastToEngine<ExternalInterface::BlockPoolEnabledMessage>(0, false);
  // }

  // // Toggle cube discovery off and on
  // robot.GetCubeCommsComponent().EnableDiscovery(false, SelfTestConfig::kActiveObjectDiscoveryTime_s);
  // robot.GetCubeCommsComponent().EnableDiscovery(true, SelfTestConfig::kActiveObjectDiscoveryTime_s);

  // // Request a WiFi scan
  // if(!(Factory::GetEMR()->fields.playpenTestDisableMask & PlaypenTestMask::WifiScanError)) {
  //   PRINT_NAMED_INFO("BehaviorSelfTest.OnBehaviorActivated.SendingWifiScanRequest", "");
  //   robot.Broadcast(ExternalInterface::MessageEngineToGame(SwitchboardInterface::WifiScanRequest()));
  // }

  // // Disable filtered touch sensor value tracking
  // robot.EnableTrackTouchSensorFilt(false);

  // _imuTemp.tempStart_c = robot.GetImuTemperature();
  // _imuTemp.duration_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();

  // Delegate to the first behavior
  _currentBehavior->WantsToBeActivated();
  DelegateNow(_currentBehavior.get());
}

void BehaviorSelfTest::OnBehaviorDeactivated()
{
  PRINT_NAMED_WARNING("","SELFTEST DEACTIVATED");
  Reset();
}

void BehaviorSelfTest::Reset()
{
  // Reset all self test behaviors
  for(auto& behavior : _selfTestBehaviors)
  {
    behavior->Reset();
  }

  // _behaviorStartTimes.clear();

  // Set current behavior to first playpen behavior
  _currentSelfTestBehaviorIter = _selfTestBehaviors.begin();
  _currentBehavior = *_currentSelfTestBehaviorIter;

  // Clear imu temp struct
  // _imuTemp = IMUTempDuration();

  _restartOnButtonPress = false;
  _waitForButtonToEndTest = false;

  _buttonPressed = false;
}

void BehaviorSelfTest::BehaviorUpdate()
{
  if(_waitForButtonToEndTest)
  {
    // DEPRECATED - Grabbing robot to support current cozmo code, but this should
    // be removed
    Robot& robot = GetBEI().GetRobotInfo()._robot;

    const bool buttonPressed = robot.IsPowerButtonPressed();

    if(_buttonPressed && !buttonPressed)
    {
      if(_restartOnButtonPress)
      {
        // If we are restarting the test then skip the first behavior
        // as it just waits for the face menu screen option to be selected
        //_currentSelfTestBehaviorIter = _selfTestBehaviors.begin();
        //_currentSelfTestBehaviorIter++;

        //_currentBehavior = *_currentSelfTestBehaviorIter;

        // Fake being re-activated to startup selftest again
        OnBehaviorActivated();
      }
      else
      {
        CancelSelf();

        // This should clear the face and put us back
        robot.SendRobotMessage<RobotInterface::SelfTestEnd>();
        robot.Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::SelfTestEnd()));
      }

      _restartOnButtonPress = false;
      _waitForButtonToEndTest = false;
     }

    _buttonPressed = buttonPressed;
  }

  if(_currentBehavior != nullptr)
  {
    // Check if the current behavior has failed
    const FactoryTestResultCode result = _currentBehavior->GetResult();
    if(result != FactoryTestResultCode::UNKNOWN &&
       result != FactoryTestResultCode::SUCCESS)
    {
      //_currentBehavior->OnDeactivated();

      PRINT_NAMED_WARNING("BehaviorSelfTest.Update.BehaviorFailed",
                          "Behavior %s failed with %s",
                          _currentBehavior->GetDebugLabel().c_str(),
                          EnumToString(_currentBehavior->GetResult()));

      _currentBehavior = nullptr;

      HandleResult(result);
      //return RESULT_FAIL;
    }
  }

  // If the current behavior has completed
  if(_currentBehavior != nullptr &&
     _currentBehavior->GetResult() == FactoryTestResultCode::SUCCESS)
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

        HandleResult(FactoryTestResultCode::BEHAVIOR_NOT_RUNNABLE);
      }

      //_behaviorStartTimes.push_back(BaseStationTimer::getInstance()->GetCurrentTimeStamp());

      DelegateNow(_currentBehavior.get());
    }
    // All playpen behaviors have run so success!
    else
    {
      PRINT_NAMED_INFO("BehaviorSelfTest.Complete", "All behaviors have been run");
      HandleResult(FactoryTestResultCode::SUCCESS);
    }
  }
}


void BehaviorSelfTest::HandleResult(FactoryTestResultCode result)
{
  PRINT_NAMED_INFO("BehaviorSelfTest.HandleResult.OrigResult",
                   "%s", EnumToString(result));

  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  //Robot& robot = GetBEI().GetRobotInfo()._robot;

  // Update imu temp struct with the final temperature and append it to log
  //  _imuTemp.tempEnd_c = robot.GetImuTemperature();
  //_imuTemp.duration_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp() - _imuTemp.duration_ms;
  // if(!_factoryTestLogger.Append(_imuTemp))
  // {
  //   PRINT_NAMED_WARNING("BehaviorSelfTest.HandleResultInternal.WriteToLogFailed.ImuTemp", "");
  //   result = FactoryTestResultCode::WRITE_TO_LOG_FAILED;
  // }

  const auto& allResults = IBehaviorSelfTest::GetAllSelfTestResults();
  // // Write all playpen behavior results to log if we are ignoring failures
  // if(SelfTestConfig::kIgnoreFailures &&
  //    !_factoryTestLogger.Append(allResults))
  // {
  //   PRINT_NAMED_WARNING("BehaviorSelfTest.HandleResultInternal.WriteToLogFailed.AllResults", "");
  //   result = FactoryTestResultCode::WRITE_TO_LOG_FAILED;
  // }

  // If this is a success but we are ignoring failures a behavior may have actually
  // failed so check all results
  if(result == FactoryTestResultCode::SUCCESS && SelfTestConfig::kIgnoreFailures)
  {
    for(const auto& resultPair : allResults)
    {
      for(const auto& res : resultPair.second)
      {
        if(res != FactoryTestResultCode::SUCCESS)
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

  // Only check EMR PLAYPEN_READY_FLAG on success so that we can run robots through
  // playpen to check sensors and stuff before even if they have not passed previous fixtures
  // if(result == FactoryTestResultCode::SUCCESS &&
  //    (Factory::GetEMR() == nullptr ||
  //     !Factory::GetEMR()->fields.PLAYPEN_READY_FLAG))
  // {
  //   PRINT_NAMED_WARNING("BehaviorSelfTest.HandleResultInternal.NotReadyForPlaypen",
  //                       "Either couldn't read EMR or robot not ready for playpen");
  //   result = FactoryTestResultCode::ROBOT_FAILED_PREPLAYPEN_TESTS;
  // }

  //FactoryTestResultEntry resultEntry;

  // _behaviorStartTimes.resize(resultEntry.timestamps.size());
  // _behaviorStartTimes[_behaviorStartTimes.size() - 1] = BaseStationTimer::getInstance()->GetCurrentTimeStamp();

  // resultEntry.result = result;
  // resultEntry.engineSHA1 = 0; // TODO: Populate
  // resultEntry.utcTime = time(0);
  // resultEntry.stationID = 0; // TODO: Populate from fixture
  // std::copy(_behaviorStartTimes.begin(),
  //           _behaviorStartTimes.begin() + resultEntry.timestamps.size(),
  //           resultEntry.timestamps.begin());

  // u8 buf[resultEntry.Size()];
  // size_t numBytes = resultEntry.Pack(buf, sizeof(buf));
  // if(SelfTestConfig::kWriteToStorage &&
  //    !robot.GetNVStorageComponent().Write(NVStorage::NVEntryTag::NVEntry_PlaypenTestResults,
  //                                         buf,
  //                                         numBytes))
  // {
  //   PRINT_NAMED_WARNING("BehaviorSelfTest.HandleResultInternal.WriteTestResultToRobotFailed",
  //                       "Writing test results to robot failed");
  //   resultEntry.result = FactoryTestResultCode::TEST_RESULT_WRITE_FAILED;
  //   result = FactoryTestResultCode::TEST_RESULT_WRITE_FAILED;
  // }

  // if(!_factoryTestLogger.Append(resultEntry))
  // {
  //   PRINT_NAMED_WARNING("BehaviorSelfTest.HandleResultInternal.WriteToLogFailed",
  //                       "Failed to write result entry to log");
  //   result = FactoryTestResultCode::WRITE_TO_LOG_FAILED;
  // }

  // if((result == FactoryTestResultCode::SUCCESS) && SelfTestConfig::kWriteToStorage)
  // {
  //   time_t nowTime = time(0);
  //   struct tm* tmStruct = gmtime(&nowTime);

  //   // TODO: System time will be incorrect need to have a fixture set it
  //   BirthCertificate bc;
  //   bc.year   = static_cast<u8>(tmStruct->tm_year % 100);
  //   bc.month  = static_cast<u8>(tmStruct->tm_mon + 1); // Months start at zero
  //   bc.day    = static_cast<u8>(tmStruct->tm_mday);
  //   bc.hour   = static_cast<u8>(tmStruct->tm_hour);
  //   bc.minute = static_cast<u8>(tmStruct->tm_min);
  //   bc.second = static_cast<u8>(tmStruct->tm_sec);

  //   u8 buf[bc.Size()];
  //   size_t numBytes = bc.Pack(buf, sizeof(buf));

  //   if(!robot.GetNVStorageComponent().Write(NVStorage::NVEntryTag::NVEntry_BirthCertificate,
  //                                           buf,
  //                                           numBytes))
  //   {
  //     PRINT_NAMED_ERROR("BehaviorSelfTest.HandleResultInternal.BCWriteFailed", "");
  //     resultEntry.result = FactoryTestResultCode::BIRTH_CERTIFICATE_WRITE_FAILED;
  //     result = FactoryTestResultCode::BIRTH_CERTIFICATE_WRITE_FAILED;
  //   }


  //   static_assert(sizeof(Factory::EMR::Fields::playpen) >= sizeof(BirthCertificate),
  //                 "Not enough space for BirthCertificate in EMR::Playpen");
  //   Factory::WriteEMR(offsetof(Factory::EMR::Fields, playpen)/sizeof(uint32_t), buf, sizeof(buf));
  // }

  // const u32 kPlaypenPassedFlag = ((result == FactoryTestResultCode::SUCCESS) ? 1 : 0);
  // Factory::WriteEMR(offsetof(Factory::EMR::Fields, PLAYPEN_PASSED_FLAG)/sizeof(uint32_t), kPlaypenPassedFlag);

  // robot.Broadcast(ExternalInterface::MessageEngineToGame(FactoryTestResultEntry(resultEntry)));

  PRINT_NAMED_INFO("BehaviorSelfTest.HandleResultInternal.Result",
                   "Playpen completed with %s",
                   EnumToString(result));

  // Copy engine logs if the test failed or we are ignoring failures
  // if((result != FactoryTestResultCode::SUCCESS || SelfTestConfig::kIgnoreFailures) &&
  //    !_factoryTestLogger.CopyEngineLog(robot.GetContextDataPlatform()))
  // {
  //   PRINT_NAMED_WARNING("BehaviorSelfTest.HandleResultInternal.CopyEngineLogFailed", "");
  // }
  // _factoryTestLogger.CloseLog();

  DisplayResult(result);

  // Reset playpen
  Reset();

  _restartOnButtonPress = false;//(result != FactoryTestResultCode::SUCCESS);
  _waitForButtonToEndTest = true;

  // Handled by button press logic in BehaviorUpdate
  // Fake being re-activated to startup playpen again
  //OnBehaviorActivated();

  // If data directory is too large, delete it
  // if(Util::FileUtils::GetDirectorySize(SelfTestConfig::kDataDirPath) > SelfTestConfig::kMaxDataDirSize_bytes)
  // {
  //   PRINT_NAMED_WARNING("BehaviorSelfTest.HandleResultInternal.DeletingDataDir",
  //                       "%s is larger than %zd, deleting",
  //                       SelfTestConfig::kDataDirPath.c_str(),
  //                       SelfTestConfig::kMaxDataDirSize_bytes);

  //   Util::FileUtils::RemoveDirectory(SelfTestConfig::kDataDirPath);
  // }

  // Just-in-case sync
  //sync();

  // TODO(Al): Turn off Victor at end of playpen?
}

void BehaviorSelfTest::DisplayResult(FactoryTestResultCode result)
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  if(result == FactoryTestResultCode::SUCCESS)
  {
    static const BackpackLights passLights = {
      .onColors               = {{NamedColors::GREEN,NamedColors::GREEN,NamedColors::GREEN}},
      .offColors              = {{NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK}},
      .onPeriod_ms            = {{1000,1000,1000}},
      .offPeriod_ms           = {{100,100,100}},
      .transitionOnPeriod_ms  = {{450,450,450}},
      .transitionOffPeriod_ms = {{450,450,450}},
      .offset                 = {{0,0,0}}
    };

    robot.GetBodyLightComponent().SetBackpackLights(passLights);

    IBehaviorSelfTest::DrawTextOnScreen(robot, {"OK", "Press button", "to finish test"},
                                        NamedColors::BLACK, NamedColors::GREEN);
    // robot.SendMessage(RobotInterface::EngineToRobot(RobotInterface::DrawTextOnScreen(true,
    //                                                                                  RobotInterface::ColorRGB(0,0,0),
    //                                                                                  RobotInterface::ColorRGB(0,255,0),
    //                                                                                  "OK")));
  }
  else
  {
    //PlayAnimationAction* action = new PlayAnimationAction("playpenFailAnim");
    //robot.GetActionList().AddConcurrentAction(action);

    static const BackpackLights failLights = {
      .onColors               = {{NamedColors::RED,NamedColors::RED,NamedColors::RED}},
      .offColors              = {{NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK}},
      .onPeriod_ms            = {{500,500,500}},
      .offPeriod_ms           = {{500,500,500}},
      .transitionOnPeriod_ms  = {{0,0,0}},
      .transitionOffPeriod_ms = {{0,0,0}},
      .offset                 = {{0,0,0}}
    };

    robot.GetBodyLightComponent().SetBackpackLights(failLights);

    IBehaviorSelfTest::DrawTextOnScreen(robot, {std::to_string((u32)result), "Press button", "to end test"},
                                        NamedColors::BLACK, NamedColors::RED);

    // // Draw result to screen
    // robot.SendMessage(RobotInterface::EngineToRobot(RobotInterface::DrawTextOnScreen(true,
    //                                                                                  RobotInterface::ColorRGB(0,0,0),
    //                                                                                  RobotInterface::ColorRGB(255,0,0),
    //                                                                                  std::to_string((u32)result))));
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

// template<>
// void BehaviorSelfTest::HandleMessage(const SwitchboardInterface::EnterPairing& msg)
// {
//   // Only have playpen handle this message if this is the factory test and
//   // we have not been packed out
//   if(FACTORY_TEST)
//   {
//     BEGIN_DONT_RUN_AFTER_PACKOUT

//     PRINT_NAMED_WARNING("BehaviorSelfTest.HandleMessage.EnterPairing",
//                         "Pairing mode triggered. Exiting playpen test.");

//     // Reset playpen
//     Reset();

//     // Fake being re-activated to startup playpen again
//     OnBehaviorActivated();

//     END_DONT_RUN_AFTER_PACKOUT
//   }
// }

// void BehaviorSelfTest::AlwaysHandleInScope(const RobotToEngineEvent& event)
// {
//   const auto& tag = event.GetData().GetTag();
//   PRINT_NAMED_WARNING("","%hhu", tag);
//   if(tag != RobotInterface::RobotToEngineTag::startSelfTest)
//   {
//     return;
//   }
//   PRINT_NAMED_WARNING("","START TEST MESSAGE");
//   // Robot& robot = GetBEI().GetRobotInfo()._robot;

//   //const auto& payload = event.GetData().Get_startSelfTest();
//   _startTest = true;
// }

}
}
