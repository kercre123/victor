/**
 * File: testRobotAudio.cpp
 *
 * Author: damjan stulic
 * Created: 6/8/16
 *
 * Description: Unit tests for Robot Audio
 *
 * Copyright: Anki, Inc. 2016
 *
 * --gtest_filter=RobotAudio*
 **/


#include "gtest/gtest.h"

#include "anki/cozmo/basestation/animation/animation.h"
#include "anki/cozmo/basestation/audio/robotAudioAnimationOnRobot.h"
#include "anki/cozmo/basestation/audio/robotAudioClient.h"
#include "anki/cozmo/basestation/cannedAnimationContainer.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/robotManager.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/utils/parsingConstants/parsingConstants.h"
#include "anki/cozmo/shared/cozmoConfig_common.h"
#include "util/logging/logging.h"
#include "helpers/audio/robotAudioTestClient.h"
//#include <assert.h>


using namespace Anki;
using namespace Anki::Cozmo;
using namespace Anki::Cozmo::Audio;

extern Anki::Cozmo::CozmoContext* cozmoContext;
const char* ROBOT_ADVERTISING_HOST_IP = "127.0.0.1";
const char* VIZ_HOST_IP = "127.0.0.1";

TEST(RobotAudio, CreateRobotAudio)
{
  ASSERT_TRUE(cozmoContext->GetDataPlatform() != nullptr);
  /*
  Json::Value config;
  cozmoContext->GetDataPlatform()->readAsJson(
    Anki::Util::Data::Scope::Resources,
    "config/basestation/config/configuration.json",
    config);
  config[AnkiUtil::kP_ADVERTISING_HOST_IP] = ROBOT_ADVERTISING_HOST_IP;
  config[AnkiUtil::kP_VIZ_HOST_IP] = VIZ_HOST_IP;
  config[AnkiUtil::kP_ROBOT_ADVERTISING_PORT] = ROBOT_ADVERTISING_PORT;
  config[AnkiUtil::kP_UI_ADVERTISING_PORT] = UI_ADVERTISING_PORT;
  config[AnkiUtil::kP_SDK_ON_DEVICE_TCP_PORT] = SDK_ON_DEVICE_TCP_PORT;

  RobotManager robotManager(cozmoContext);
  robotManager.Init(config);
  */
  const std::string animationFile = cozmoContext->GetDataPlatform()->pathToResource(Util::Data::Scope::Resources,
    "config/basestation/animations/ANIMATION_TEST.json");
  Json::Value animDefs;
  const bool success = cozmoContext->GetDataPlatform()->readAsJson(animationFile, animDefs);
  ASSERT_TRUE(success);
  std::string animationId;
  CannedAnimationContainer container;
  if (success && !animDefs.empty()) {
    container.DefineFromJson(animDefs, animationId);
  }
  auto names = container.GetAnimationNames();
  for (auto name : names) {
    PRINT_NAMED_INFO("RobotAudio.CreateRobotAudio", "%s", name.c_str());
  }
  ASSERT_GE(names.size(), 1);
  Robot robot(1, cozmoContext);
  PRINT_NAMED_INFO("RobotAudio.CreateRobotAudio", "robot created");
  Animation* animation = container.GetAnimation("ANIMATION_TEST");
  animation->AddKeyFrameToBack(RobotAudioKeyFrame(Audio::GameEvent::GenericEvent::Vo_Coz_Wakeup_Play,100));
  RobotAudioTestClient audioClient(&robot);
  RobotAudioAnimationOnRobot raaor(animation, &audioClient);
  raaor.Update(0,33);
}

