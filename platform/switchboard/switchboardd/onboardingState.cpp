/**
 * File: switchboardd/onboardingState.cpp
 *
 * Author: paluri
 * Created: 7/19/2018
 *
 * Description: Read onboarding state from JSON file
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "fileutils.h"
#include "util/source/3rd/jsoncpp/json/json.h"
#include "onboardingState.h"

namespace Anki {
namespace Switchboard {

const std::string OnboardingState::kJsonPath = "/data/data/com.anki.victor/persistent/onboarding/onboardingState.json";

std::string OnboardingState::ReadState() {
  std::vector<uint8_t> bytes;
  bool success = Anki::ReadFileIntoVector(kJsonPath, bytes);

  if(!success) {
    return "error";  
  }

  std::string contents = std::string(bytes.begin(), bytes.end());

  Json::Value onboardingState;
  Json::Reader read;
  const bool jsonSuccess = read.parse(contents, onboardingState);

  if(!(jsonSuccess && onboardingState.isMember("onboardingStage"))) {
    return "error";  
  }

  std::string onboardingStage = onboardingState["onboardingStage"].asString();

  return onboardingStage;
}

bool OnboardingState::HasStartedOnboarding() {
  std::string state = ReadState();

  return (state != "error") && (state != "NotStarted");
}

} // Switchboard
} // Anki