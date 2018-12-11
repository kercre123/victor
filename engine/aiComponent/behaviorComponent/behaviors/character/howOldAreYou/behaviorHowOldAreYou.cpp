/**
 * File: BehaviorHowOldAreYou.cpp
 *
 * Author: Andrew Stout
 * Created: 2018-11-13
 *
 * Description: Responds to How Old Are You character voice intent.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/character/howOldAreYou/behaviorHowOldAreYou.h"

#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/components/robotStatsTracker.h"
#include "engine/components/textToSpeech/textToSpeechCoordinator.h"


#define LOG_CHANNEL "BehaviorHowOldAreYou"

namespace Anki {
namespace Vector {

namespace {
  const float kMonthsThresh_d = 30.436875; // days at which we start counting in months instead
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorHowOldAreYou::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorHowOldAreYou::DynamicVariables::DynamicVariables():
    robotAge_h(-1),
    robotAge_presentable(std::make_pair(-1, "invalid"))
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorHowOldAreYou::BehaviorHowOldAreYou(const Json::Value& config)
 : ICozmoBehavior(config)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorHowOldAreYou::~BehaviorHowOldAreYou()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorHowOldAreYou::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorHowOldAreYou::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorHowOldAreYou::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert( _iConfig.countingAnimationBehavior.get() );
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorHowOldAreYou::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorHowOldAreYou::InitBehavior()
{
  const BehaviorContainer& container = GetBEI().GetBehaviorContainer();
  container.FindBehaviorByIDAndDowncast<BehaviorCountingAnimation>( BEHAVIOR_ID(HowOldAreYouCounting),
                                                                    BEHAVIOR_CLASS(CountingAnimation),
                                                                    _iConfig.countingAnimationBehavior );
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorHowOldAreYou::OnBehaviorActivated() 
{
  // reset dynamic variables
  _dVars = DynamicVariables();
  
  // the behavior is active now, time to do something!

  // get age
  _dVars.robotAge_h = GetRobotAge();
  // round age
  _dVars.robotAge_presentable = PresentableAgeFromHours(_dVars.robotAge_h);
  LOG_INFO("BehaviorHowOldAreYou.OnBehaviorActivated.PresentableAge",
      "Robot presentable age: %d %s", _dVars.robotAge_presentable.first, _dVars.robotAge_presentable.second.c_str());

  const std::string textToSay = std::to_string(_dVars.robotAge_presentable.first) + " " +  _dVars.robotAge_presentable.second;

  // delegate to counting animation
  _iConfig.countingAnimationBehavior->SetCountTarget(_dVars.robotAge_presentable.first, textToSay);
  if ( _iConfig.countingAnimationBehavior->WantsToBeActivated() ) {
    DelegateIfInControl( _iConfig.countingAnimationBehavior.get() );
  }

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float BehaviorHowOldAreYou::GetRobotAge()
{
  const auto& rst = GetBehaviorComp<RobotStatsTracker>();
  const float robotAge_h = rst.GetNumHoursAlive();
  return robotAge_h;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::pair<int, std::string> BehaviorHowOldAreYou::PresentableAgeFromHours(const float age_h)
{
  // std::chrono could have been useful here, but the days/weeks/months types don't come in until C++20,
  // and it's ultimately more trouble to make use of std::chrono than to DIY for something this small.

  // translate hours to days
  const float age_d = age_h/24.0; // exactly 24 hours in a day.
  // if we're less than kMonthsThresh_d days, use days
  if (age_d < kMonthsThresh_d) {
    // note: current implementation (at Design's request) is to return the floor:
    // i.e., round everything down until we get to a whole day: 47 hours -> 1 day, 49 hours -> 2 days
    const int age_dint = static_cast<int>( std::floor(age_d) );
    return std::make_pair( age_dint , age_dint == 1 ? "day" : "days" );
  }
  // else, translate to months
  const float age_m = age_d/30.436875; // 365.2425 days in a year, divided by 12 = 30.436875 days in a month
  // note: current implementation (at Design's request) is to return the floor:
  // i.e., round everything down until we get to a whole month: 59 days -> 1 month, 62 days -> 2 months
  const int age_mint = static_cast<int>( std::floor(age_m) );
  return std::make_pair(age_mint, age_mint == 1 ? "month" : "months" );
}


} // namespace Vector
} // namespace Anki


