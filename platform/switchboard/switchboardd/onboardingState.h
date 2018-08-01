/**
 * File: switchboardd/onboardingState.h
 *
 * Author: paluri
 * Created: 7/19/2018
 *
 * Description: Read onboarding state from JSON file
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#pragma once

namespace Anki {
namespace Switchboard {

class OnboardingState {
public:
  static std::string ReadState();
  static bool HasStartedOnboarding();
private:
  static const std::string kJsonPath;
};

} // Switchboard
} // Anki